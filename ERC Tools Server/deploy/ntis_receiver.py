#!/usr/bin/env python3

import hashlib
import gzip
import json
import logging
import os
import re
import signal
import sqlite3
import tempfile
import threading
import time
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit


BIND_HOST = os.environ.get("NTIS_BIND_HOST", "127.0.0.1")
BIND_PORT = int(os.environ.get("NTIS_BIND_PORT", "18080"))
DATA_DIR = Path(os.environ.get("NTIS_DATA_DIR", "/var/lib/erc-tools/ntis"))
MAX_BODY_BYTES = int(os.environ.get("NTIS_MAX_BODY_BYTES", str(512 * 1024 * 1024)))
FEEDS = {
    "/ntis/event": "event",
    "/ntis/network-model": "network-model",
}
SNAPSHOT_PATH = DATA_DIR / "current" / "events.json"
FULL_REFRESH_IDLE_SECONDS = 10.0
ROAD_PATTERN = re.compile(r"\b(?:M\d+[A-Z]?|A\d+(?:\(M\))?|A\d+[A-Z]?|B\d+)\b", re.IGNORECASE)


def local_name(tag):
    return tag.rsplit("}", 1)[-1]


def descendant_text(element, name, default=""):
    for child in element.iter():
        if local_name(child.tag) == name and child.text:
            value = child.text.strip()
            if value:
                return value
    return default


def descendant_values(element, name):
    values = []
    for child in element.iter():
        if local_name(child.tag) == name and child.text:
            value = child.text.strip()
            if value:
                values.append(value)
    return values


def xml_attribute(element, name, default=""):
    for key, value in element.attrib.items():
        if local_name(key) == name:
            return value
    return default


def parse_iso_time(value):
    if not value:
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None


def integer_value(value, default=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def float_value(value, default=0.0):
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def general_public_comments(record):
    comments = []
    for container in record.iter():
        if local_name(container.tag) != "generalPublicComment":
            continue
        for child in container.iter():
            if local_name(child.tag) == "value" and child.text:
                value = " ".join(child.text.split())
                if value and value not in comments:
                    comments.append(value)
    return comments


def lane_states(record):
    lanes = []
    for lane in record.iter():
        if local_name(lane.tag) != "individualLane":
            continue
        name = descendant_text(lane, "laneIdentifier")
        status = descendant_text(lane, "laneStatus")
        if not name or "shoulder" in name.lower():
            continue
        lanes.append({"laneName": name, "laneStatus": status or "unknown"})
    return lanes


def normalise_reason(value):
    aliases = {
        "rtc": "Road traffic collision",
        "collision": "Road traffic collision",
        "broken down vehicle": "Broken down vehicle",
        "vehicle breakdown": "Broken down vehicle",
        "queue": "Congestion",
    }
    stripped = " ".join((value or "").split())
    return aliases.get(stripped.lower(), stripped)


def fallback_reason(record_type, record):
    type_fields = {
        "Accident": "accidentType",
        "AbnormalTraffic": "abnormalTrafficType",
        "AuthorityOperation": "authorityOperationType",
        "VehicleObstruction": "vehicleObstructionType",
        "RoadOrCarriagewayOrLaneManagement": "roadOrCarriagewayOrLaneManagementType",
        "MaintenanceWorks": "roadMaintenanceType",
        "PublicEvent": "publicEventType",
    }
    raw = descendant_text(record, type_fields.get(record_type, ""))
    if raw:
        raw = re.sub(r"(?<!^)([A-Z])", r" \1", raw).replace("_", " ").strip()
        return normalise_reason(raw.capitalize())
    defaults = {
        "Accident": "Road traffic collision",
        "AbnormalTraffic": "Congestion",
        "AuthorityOperation": "Authority operation",
        "VehicleObstruction": "Vehicle obstruction",
        "RoadOrCarriagewayOrLaneManagement": "Road management",
        "MaintenanceWorks": "Roadworks",
        "PublicEvent": "Public event",
    }
    return defaults.get(record_type, "Road incident")


def select_location_and_reason(comments, record_type, record):
    location_index = -1
    for index, comment in enumerate(comments):
        if ROAD_PATTERN.search(comment):
            location_index = index
            break
    location = comments[location_index] if location_index >= 0 else ""

    ignored = re.compile(
        r"^(?:there (?:is|are|will be)|lanes? |from \d|the event is expected|"
        r"normal traffic conditions|currently active|pending|cleared)$",
        re.IGNORECASE,
    )
    reason = ""
    candidates = comments[location_index + 1:] if location_index >= 0 else comments
    for candidate in candidates:
        lower = candidate.lower()
        if ignored.search(candidate) or lower.startswith((
            "there is ", "there are ", "there will be ", "lane ", "lanes ",
            "from ", "the event is expected", "normal traffic conditions",
        )) or lower in {"currently active", "pending", "cleared"}:
            continue
        reason = candidate
        break
    return location, normalise_reason(reason) or fallback_reason(record_type, record)


def extract_coordinates(record):
    for location in record.iter():
        if local_name(location.tag) != "locationForDisplay":
            continue
        latitude = float_value(descendant_text(location, "latitude"), None)
        longitude = float_value(descendant_text(location, "longitude"), None)
        if latitude is not None and longitude is not None:
            return latitude, longitude
    return None, None


def record_is_current(record, publication_time):
    if descendant_text(record, "cancel").lower() == "true":
        return False
    if descendant_text(record, "end").lower() == "true":
        return False

    status = descendant_text(record, "validityStatus").lower()
    if status == "active":
        return True
    if status != "definedbyvaliditytimespec":
        return False

    now = parse_iso_time(publication_time) or utc_now()
    start = parse_iso_time(descendant_text(record, "overallStartTime"))
    end = parse_iso_time(descendant_text(record, "overallEndTime"))
    return (start is None or start <= now) and (end is None or now <= end)


def map_severity(value):
    return {
        "highest": "Severe",
        "high": "Severe",
        "medium": "Moderate",
        "low": "Minor",
        "lowest": "Minor",
    }.get((value or "").lower(), "Unknown")


def build_description(record, comments, location, reason, lanes, lanes_closed, lanes_total):
    lines = []
    if location:
        lines.append("Location : " + location)
    if reason:
        lines.append("Reason : " + reason)

    for comment in comments:
        lower = comment.lower()
        if comment in {location, reason} or normalise_reason(comment) == reason:
            continue
        if lower in {"currently active", "pending", "cleared"}:
            lines.append("Status : " + comment)
        elif lower.startswith("the event is expected to clear"):
            lines.append("Time To Clear : " + comment)
        elif lower.startswith("normal traffic conditions"):
            lines.append("Return To Normal : " + comment)

    delay_seconds = float_value(descendant_text(record, "delayTimeValue"))
    if delay_seconds > 0:
        delay_minutes = max(1, round(delay_seconds / 60.0))
        lines.append(f"Delay : {delay_minutes} minutes")
    if lanes_closed > 0 and lanes_total > 0:
        lines.append(f"Lanes Closed : {lanes_closed} of {lanes_total}")
    elif lanes_closed > 0:
        lines.append(f"Lanes Closed : {lanes_closed}")
    return "\r\n".join(lines)


def normalise_record(record, situation_id, publication_time):
    record_id = xml_attribute(record, "id")
    version = integer_value(xml_attribute(record, "version"))
    version_time = descendant_text(record, "situationRecordVersionTime")
    if not record_id:
        return None

    current = record_is_current(record, publication_time)
    if not current:
        return {
            "recordId": record_id,
            "version": version,
            "versionTime": version_time,
            "current": False,
        }

    record_type = xml_attribute(record, "type") or "RoadIncident"
    comments = general_public_comments(record)
    location, reason = select_location_and_reason(comments, record_type, record)
    road_match = ROAD_PATTERN.search(location)
    road = road_match.group(0).upper() if road_match else ""
    if road and re.search(rf"\b{re.escape(road)}\s+SPUR\b", location, re.IGNORECASE):
        road += " SPUR"

    latitude, longitude = extract_coordinates(record)
    lanes = lane_states(record)
    lanes_closed = sum(1 for lane in lanes if lane["laneStatus"].lower() in {
        "closed", "blocked", "notusable", "not usable", "restricted",
    })
    lanes_total = len(lanes)
    if lanes_total == 0:
        lanes_total = integer_value(descendant_text(record, "originalNumberOfLanes"))
        operational = integer_value(descendant_text(record, "numberOfOperationalLanes"), lanes_total)
        lanes_closed = max(0, lanes_total - operational)

    planned = record_type in {"MaintenanceWorks", "PublicEvent"}
    event_type = "Roadworks" if record_type == "MaintenanceWorks" else (
        "Public event" if record_type == "PublicEvent" else reason
    )
    alert = {
        "id": "ntis:" + record_id,
        "ntisRecordId": record_id,
        "ntisSituationId": situation_id,
        "title": reason,
        "reason": reason,
        "description": build_description(
            record, comments, location, reason, lanes, lanes_closed, lanes_total),
        "road": road,
        "region": descendant_text(record, "allocatedRoc") or "England",
        "severity": map_severity(descendant_text(record, "severity")),
        "eventType": event_type,
        "ntisRecordType": record_type,
        "updated": version_time or descendant_text(record, "situationRecordCreationTime"),
        "planned": planned,
        "lanesClosed": lanes_closed,
        "lanesTotal": lanes_total,
        "lanes": lanes,
    }
    if latitude is not None and longitude is not None:
        alert["latitude"] = latitude
        alert["longitude"] = longitude
    return {
        "recordId": record_id,
        "version": version,
        "versionTime": version_time,
        "current": True,
        "alert": alert,
    }


def parse_event_publication(path):
    with open(path, "rb") as probe:
        compressed = probe.read(2) == b"\x1f\x8b"
    opener = gzip.open if compressed else open
    with opener(path, "rb") as payload:
        root = ET.parse(payload).getroot()
    feed_type = descendant_text(root, "feedType")
    publication_time = descendant_text(root, "publicationTime")
    records = []
    for situation in root.iter():
        if local_name(situation.tag) != "situation":
            continue
        situation_id = xml_attribute(situation, "id")
        for record in list(situation):
            if local_name(record.tag) != "situationRecord":
                continue
            normalised = normalise_record(record, situation_id, publication_time)
            if normalised is not None:
                records.append(normalised)
    return feed_type, publication_time, records


def utc_now():
    return datetime.now(timezone.utc)


def initialise_database():
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(DATA_DIR / "messages.sqlite3") as db:
        db.execute("PRAGMA journal_mode=WAL")
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                feed TEXT NOT NULL,
                received_at TEXT NOT NULL,
                path TEXT NOT NULL,
                metadata_path TEXT NOT NULL,
                content_type TEXT NOT NULL,
                content_encoding TEXT NOT NULL,
                size_bytes INTEGER NOT NULL,
                sha256 TEXT NOT NULL,
                processed_at TEXT,
                UNIQUE(feed, sha256)
            )
            """
        )
        db.execute(
            "CREATE INDEX IF NOT EXISTS messages_unprocessed "
            "ON messages(feed, processed_at, id)"
        )
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS event_state (
                record_id TEXT PRIMARY KEY,
                version INTEGER NOT NULL,
                version_time TEXT NOT NULL,
                situation_id TEXT NOT NULL,
                refresh_generation INTEGER NOT NULL,
                alert_json TEXT NOT NULL
            )
            """
        )
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS processor_metadata (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """
        )


def metadata_get(db, key, default=""):
    row = db.execute(
        "SELECT value FROM processor_metadata WHERE key = ?", (key,)
    ).fetchone()
    return row[0] if row else default


def metadata_set(db, key, value):
    db.execute(
        """
        INSERT INTO processor_metadata(key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value
        """,
        (key, str(value)),
    )


class NtisEventProcessor:
    def __init__(self):
        self._wake = threading.Event()
        self._stop = threading.Event()
        self._thread = threading.Thread(
            target=self._run, name="ntis-event-processor", daemon=True
        )

    def start(self):
        self._thread.start()

    def notify(self):
        self._wake.set()

    def stop(self):
        self._stop.set()
        self._wake.set()
        self._thread.join(timeout=10)

    def _write_snapshot(self, db):
        rows = db.execute(
            "SELECT alert_json FROM event_state ORDER BY record_id"
        ).fetchall()
        alerts = [json.loads(row[0]) for row in rows]
        alerts.sort(key=lambda item: (
            item.get("planned", False),
            item.get("road", ""),
            item.get("title", ""),
            item.get("id", ""),
        ))
        generation = integer_value(metadata_get(db, "snapshot_generation")) + 1
        metadata_set(db, "snapshot_generation", generation)
        document = {
            "schemaVersion": 1,
            "source": "National Highways NTIS Event Data",
            "generation": generation,
            "updatedAt": utc_now().isoformat(),
            "refreshInProgress": False,
            "alerts": alerts,
        }
        SNAPSHOT_PATH.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary_name = tempfile.mkstemp(
            prefix=".events-", suffix=".json", dir=SNAPSHOT_PATH.parent
        )
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as output:
                json.dump(document, output, ensure_ascii=False, separators=(",", ":"))
                output.write("\n")
                output.flush()
                os.fsync(output.fileno())
            os.replace(temporary_name, SNAPSHOT_PATH)
        finally:
            if os.path.exists(temporary_name):
                os.unlink(temporary_name)
        logging.info(
            "Published NTIS event snapshot generation %d with %d current record(s)",
            generation,
            len(alerts),
        )

    def _finalise_refresh_if_idle(self, db, force=False):
        if metadata_get(db, "refresh_in_progress", "0") != "1":
            return False
        last_received = float(metadata_get(db, "last_refresh_received_epoch", "0") or 0)
        if not force and time.time() - last_received < FULL_REFRESH_IDLE_SECONDS:
            return False
        refresh_generation = integer_value(metadata_get(db, "refresh_generation"))
        removed = db.execute(
            "DELETE FROM event_state WHERE refresh_generation <> ?",
            (refresh_generation,),
        ).rowcount
        metadata_set(db, "refresh_in_progress", "0")
        metadata_set(db, "last_feed_type", "")
        logging.info(
            "Finalised NTIS full refresh generation %d; removed %d stale record(s)",
            refresh_generation,
            removed,
        )
        return True

    def _apply_publication(self, db, message_id, path, received_at):
        feed_type, publication_time, records = parse_event_publication(path)
        is_full_refresh = "full refresh" in feed_type.lower()
        last_feed_type = metadata_get(db, "last_feed_type")
        last_refresh_epoch = float(
            metadata_get(db, "last_refresh_received_epoch", "0") or 0
        )
        received = parse_iso_time(received_at)
        received_epoch = received.timestamp() if received else time.time()
        refresh_generation = integer_value(metadata_get(db, "refresh_generation"))

        if is_full_refresh:
            new_refresh = (
                "full refresh" not in last_feed_type.lower()
                or received_epoch - last_refresh_epoch > FULL_REFRESH_IDLE_SECONDS
            )
            if new_refresh:
                if metadata_get(db, "refresh_in_progress", "0") == "1":
                    self._finalise_refresh_if_idle(db, force=True)
                refresh_generation += 1
                metadata_set(db, "refresh_generation", refresh_generation)
                metadata_set(db, "refresh_in_progress", "1")
                logging.info(
                    "Started NTIS full refresh generation %d", refresh_generation
                )
            metadata_set(db, "last_refresh_received_epoch", received_epoch)
        elif metadata_get(db, "refresh_in_progress", "0") == "1":
            self._finalise_refresh_if_idle(db, force=True)

        changed = False
        for record in records:
            record_id = record["recordId"]
            version = record["version"]
            if not record["current"]:
                removed = db.execute(
                    "DELETE FROM event_state WHERE record_id = ? AND version <= ?",
                    (record_id, version),
                ).rowcount
                changed = changed or removed > 0
                continue
            alert = record["alert"]
            previous = db.execute(
                "SELECT version, alert_json, refresh_generation FROM event_state WHERE record_id = ?",
                (record_id,),
            ).fetchone()
            alert_json = json.dumps(alert, ensure_ascii=False, separators=(",", ":"))
            if previous and previous[0] > version:
                if is_full_refresh and previous[2] != refresh_generation:
                    db.execute(
                        "UPDATE event_state SET refresh_generation = ? WHERE record_id = ?",
                        (refresh_generation, record_id),
                    )
                continue
            row_generation = refresh_generation
            db.execute(
                """
                INSERT INTO event_state(
                    record_id, version, version_time, situation_id,
                    refresh_generation, alert_json
                ) VALUES (?, ?, ?, ?, ?, ?)
                ON CONFLICT(record_id) DO UPDATE SET
                    version = excluded.version,
                    version_time = excluded.version_time,
                    situation_id = excluded.situation_id,
                    refresh_generation = excluded.refresh_generation,
                    alert_json = excluded.alert_json
                """,
                (
                    record_id,
                    version,
                    record["versionTime"],
                    alert.get("ntisSituationId", ""),
                    row_generation,
                    alert_json,
                ),
            )
            changed = changed or not previous or previous[1] != alert_json

        metadata_set(db, "last_feed_type", feed_type)
        db.execute(
            "UPDATE messages SET processed_at = ? WHERE id = ?",
            (utc_now().isoformat(), message_id),
        )
        return changed

    def _process_pending(self):
        any_changed = False
        finalised = False
        with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
            db.execute("PRAGMA journal_mode=WAL")
            rows = db.execute(
                """
                SELECT id, path, received_at
                FROM messages
                WHERE feed = 'event' AND processed_at IS NULL
                ORDER BY id
                LIMIT 50
                """
            ).fetchall()
            for message_id, path, received_at in rows:
                try:
                    any_changed = self._apply_publication(
                        db, message_id, path, received_at
                    ) or any_changed
                    db.commit()
                except Exception as error:
                    logging.exception(
                        "Failed to process NTIS event publication %d", message_id
                    )
                    db.execute(
                        "UPDATE messages SET processed_at = ? WHERE id = ?",
                        ("ERROR: " + str(error), message_id),
                    )
                    db.commit()
            if not rows:
                finalised = self._finalise_refresh_if_idle(db)
            refresh_active = metadata_get(db, "refresh_in_progress", "0") == "1"
            if (any_changed or finalised) and not refresh_active:
                self._write_snapshot(db)
                db.commit()
            return len(rows)

    def _run(self):
        while not self._stop.is_set():
            try:
                processed = self._process_pending()
                if processed:
                    continue
            except Exception:
                logging.exception("NTIS event processor failed")
            self._wake.wait(timeout=1.0)
            self._wake.clear()


class ReceiverServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, server_address, handler_class, event_processor):
        super().__init__(server_address, handler_class)
        self.event_processor = event_processor


class ReceiverHandler(BaseHTTPRequestHandler):
    server_version = "ERC-NTIS-Receiver/1.0"
    protocol_version = "HTTP/1.1"

    def log_message(self, message, *args):
        logging.info("%s - %s", self.client_address[0], message % args)

    def send_text(self, status, body, content_type="text/plain; charset=utf-8"):
        encoded = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(encoded)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self):
        parsed = urlsplit(self.path)
        path = parsed.path
        if path == "/health":
            snapshot_exists = SNAPSHOT_PATH.exists()
            self.send_text(
                200,
                json.dumps({"status": "ok", "eventsReady": snapshot_exists}) + "\n",
                "application/json",
            )
            return
        if path == "/internal/events":
            if not SNAPSHOT_PATH.exists():
                self.send_text(
                    503,
                    '{"error":"NTIS event snapshot is not ready"}\n',
                    "application/json",
                )
                return
            try:
                document = json.loads(SNAPSHOT_PATH.read_text(encoding="utf-8"))
                options = parse_qs(parsed.query)
                if options.get("unplannedOnly", ["0"])[0].lower() in {
                    "1", "true", "yes",
                }:
                    document["alerts"] = [
                        alert for alert in document.get("alerts", [])
                        if not alert.get("planned", False)
                    ]
                self.send_text(
                    200,
                    json.dumps(document, ensure_ascii=False, separators=(",", ":")) + "\n",
                    "application/json; charset=utf-8",
                )
            except Exception:
                logging.exception("Failed to serve the current NTIS event snapshot")
                self.send_text(
                    500,
                    '{"error":"NTIS event snapshot could not be read"}\n',
                    "application/json",
                )
            return
        feed = FEEDS.get(path)
        if feed is None:
            self.send_text(404, "Not found\n")
            return
        self.send_text(200, f"ERC Tools NTIS {feed} receiver ready\n")

    def do_POST(self):
        path = self.path.split("?", 1)[0]
        feed = FEEDS.get(path)
        if feed is None:
            self.send_text(404, "Not found\n")
            return

        length_text = self.headers.get("Content-Length")
        try:
            content_length = int(length_text or "")
        except ValueError:
            self.send_text(411, "A valid Content-Length header is required\n")
            return
        if content_length < 0 or content_length > MAX_BODY_BYTES:
            self.send_text(413, "Publication is too large\n")
            return

        received_at = utc_now()
        target_dir = DATA_DIR / "incoming" / feed / received_at.strftime("%Y/%m/%d")
        target_dir.mkdir(parents=True, exist_ok=True)
        digest = hashlib.sha256()
        remaining = content_length

        fd, temporary_name = tempfile.mkstemp(prefix=".incoming-", dir=target_dir)
        try:
            with os.fdopen(fd, "wb") as output:
                while remaining:
                    chunk = self.rfile.read(min(1024 * 1024, remaining))
                    if not chunk:
                        raise ConnectionError("Publication ended before Content-Length bytes arrived")
                    output.write(chunk)
                    digest.update(chunk)
                    remaining -= len(chunk)
                output.flush()
                os.fsync(output.fileno())

            sha256 = digest.hexdigest()
            stem = received_at.strftime("%Y%m%dT%H%M%S.%fZ") + "-" + sha256[:16]
            payload_path = target_dir / f"{stem}.payload"
            metadata_path = target_dir / f"{stem}.json"
            content_type = self.headers.get("Content-Type", "application/octet-stream")
            content_encoding = self.headers.get("Content-Encoding", "")
            metadata = {
                "feed": feed,
                "receivedAt": received_at.isoformat(),
                "sizeBytes": content_length,
                "sha256": sha256,
                "contentType": content_type,
                "contentEncoding": content_encoding,
                "sourceAddress": self.headers.get("X-Forwarded-For", self.client_address[0]),
                "userAgent": self.headers.get("User-Agent", ""),
            }

            duplicate = False
            with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
                try:
                    os.replace(temporary_name, payload_path)
                    temporary_name = ""
                    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
                    db.execute(
                        """
                        INSERT INTO messages(
                            feed, received_at, path, metadata_path, content_type,
                            content_encoding, size_bytes, sha256
                        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                        """,
                        (
                            feed,
                            metadata["receivedAt"],
                            str(payload_path),
                            str(metadata_path),
                            content_type,
                            content_encoding,
                            content_length,
                            sha256,
                        ),
                    )
                except sqlite3.IntegrityError:
                    duplicate = True
                    payload_path.unlink(missing_ok=True)
                    metadata_path.unlink(missing_ok=True)

            logging.info(
                "Accepted %s publication: %d bytes, sha256=%s%s",
                feed,
                content_length,
                sha256,
                " (duplicate)" if duplicate else "",
            )
            if not duplicate and feed == "event":
                self.server.event_processor.notify()
            self.send_text(200, "OK\n")
        except Exception:
            logging.exception("Failed to store %s publication", feed)
            self.send_text(500, "Publication could not be stored\n")
        finally:
            if temporary_name:
                try:
                    os.unlink(temporary_name)
                except FileNotFoundError:
                    pass


def main():
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )
    initialise_database()
    event_processor = NtisEventProcessor()
    event_processor.start()
    event_processor.notify()
    server = ReceiverServer(
        (BIND_HOST, BIND_PORT), ReceiverHandler, event_processor
    )

    def stop_server(_signum, _frame):
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, stop_server)
    signal.signal(signal.SIGINT, stop_server)
    logging.info("NTIS receiver listening on %s:%d", BIND_HOST, BIND_PORT)
    server.serve_forever(poll_interval=0.5)
    server.server_close()
    event_processor.stop()


if __name__ == "__main__":
    main()
