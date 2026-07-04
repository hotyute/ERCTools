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
from collections import deque
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from math import ceil, cos, hypot, pi
from pathlib import Path
from urllib.parse import parse_qs, urlencode, urlsplit
from urllib.request import urlopen


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
TRAFFIC_ENGLAND_SCHEMA_VERSION = "2026-07-04-public-reasons-v4"
TRAFFIC_ENGLAND_REBUILD_BATCH_SIZE = 500
DEFAULT_FUSED_CONGESTION_MAX_AGE_SECONDS = int(
    os.environ.get("NTIS_FUSED_CONGESTION_MAX_AGE_SECONDS", "600")
)
MIN_FUSED_CONGESTION_MAX_AGE_SECONDS = 120
MAX_FUSED_CONGESTION_MAX_AGE_SECONDS = 600
NETWORK_MODEL_LINK_URL = os.environ.get(
    "NTIS_NETWORK_MODEL_LINK_URL",
    "https://services-eu1.arcgis.com/mZXeBXkkZpekxjXT/arcgis/rest/services/"
    "Network_Model_Public_view2/FeatureServer/1/query",
)
NETWORK_MODEL_SEARCH_METRES = int(os.environ.get("NTIS_NETWORK_MODEL_SEARCH_METRES", "2500"))
NETWORK_MODEL_CACHE_DAYS = int(os.environ.get("NTIS_NETWORK_MODEL_CACHE_DAYS", "7"))
ROAD_PATTERN = re.compile(
    r"(?<![A-Z0-9])(?:A\d+(?:\s*\(M\)|[A-Z])?|M\d+[A-Z]?|B\d+[A-Z]?)(?![A-Z0-9])",
    re.IGNORECASE,
)


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


def nested_values(element, container_name):
    for container in element.iter():
        if local_name(container.tag) == container_name:
            return descendant_values(container, "value")
    return []


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


def normalise_road_name(value):
    match = ROAD_PATTERN.search(value or "")
    if not match:
        return ""
    road = re.sub(r"\s+", "", match.group(0)).upper()
    return road


def extract_road_name(value):
    road = normalise_road_name(value)
    if road and re.search(rf"(?<![A-Z0-9]){re.escape(road)}\s+SPUR(?![A-Z0-9])", value or "", re.IGNORECASE):
        road += " SPUR"
    return road


def road_base(value):
    return normalise_road_name(value).replace("(M)", "")


def resolved_road_name(source_road, model_road):
    source = normalise_road_name(source_road)
    model = normalise_road_name(model_road)
    if not model or (source and road_base(model) != road_base(source)):
        return source
    # Network Model geometry may meet an adjacent all-purpose section at a
    # junction. Never let that ambiguity downgrade an explicit motorway name.
    if "(M)" in source and "(M)" not in model:
        return source
    return model


def replace_first_road_name(value, road):
    if not value or not road:
        return value
    return ROAD_PATTERN.sub(road, value, count=1)


def traffic_england_event_type(record_type):
    if record_type == "AbnormalTraffic":
        return "CONGESTION"
    if record_type == "MaintenanceWorks":
        return "ROADWORKS"
    if record_type == "WeatherRelatedRoadConditions":
        return "WEATHER"
    if record_type == "PublicEvent":
        return "MAJOR_ORGANISED_EVENTS"
    if record_type == "AbnormalLoad":
        return "ABNORMAL_LOADS"
    return "INCIDENT"


def refresh_traffic_england_flags(alert):
    event_type = alert.get("trafficEnglandEventType", "")
    confirmed = bool(alert.get("confirmed", False))
    eligible = (
        bool(alert.get("current", True))
        and (confirmed or event_type == "ROADWORKS")
        and not bool(alert.get("completed", False))
        and bool(alert.get("hasPublicPresentation", False))
        and str(alert.get("informationStatus", "real")).lower() == "real"
    )
    alert["trafficEnglandEligible"] = eligible
    alert["trafficEnglandUnplanned"] = (
        eligible and confirmed and event_type in {"CONGESTION", "INCIDENT"}
    )
    # The public Alerts page only exposed records already carrying a public
    # road identity. Network-resolved regional records remain available via
    # the explicit "Show unresolved incidents" client option.
    alert["trafficEnglandVisible"] = (
        eligible
        and bool(alert.get("sourceRoad", ""))
        and bool(alert.get("networkResolved", False))
    )
    return alert


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
        # The former Traffic England public API shortened the NTIS
        # "Other Road Management" subtype to this public-facing label.
        "other road management": "Road Management",
        "other road or carriageway or lane management": "Road Management",
    }
    stripped = " ".join((value or "").split())
    return aliases.get(stripped.lower(), stripped)


def normalise_reason_for_record(value, record_type):
    reason = normalise_reason(value)
    # NTIS encodes its public "Police Incident" subtype as an
    # AuthorityOperation whose authorityOperationType is "other".
    if record_type == "AuthorityOperation" and reason.lower() == "other":
        return "Police incident"
    return reason


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
        return normalise_reason_for_record(raw.capitalize(), record_type)
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
        if normalise_road_name(comment):
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
    return (
        location,
        normalise_reason_for_record(reason, record_type)
        or fallback_reason(record_type, record),
    )


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

    now = parse_iso_time(publication_time) or utc_now()
    start = parse_iso_time(descendant_text(record, "overallStartTime"))
    end = parse_iso_time(descendant_text(record, "overallEndTime"))
    within_validity = (start is None or start <= now) and (end is None or now <= end)
    status = descendant_text(record, "validityStatus").lower()
    if status == "active":
        return within_validity
    if status == "definedbyvaliditytimespec":
        return within_validity
    return False


def map_severity(value, event_type="", delay_seconds=0.0):
    if event_type == "CONGESTION" and delay_seconds > 0:
        if delay_seconds >= 360:
            return "Severe"
        if delay_seconds >= 120:
            return "Moderate"
        return "Minor"
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
        # Traffic England presented measured congestion in five-minute bands,
        # with ten minutes as the smallest displayed non-zero delay.
        delay_minutes = max(10, int(ceil(delay_seconds / 300.0) * 5))
        lines.append(f"Delay : {delay_minutes} minutes")
    if lanes_closed > 0 and lanes_total > 0:
        lines.append(f"Lanes Closed : {lanes_closed} of {lanes_total}")
    elif lanes_closed > 0:
        lines.append(f"Lanes Closed : {lanes_closed}")
    return "\r\n".join(lines)


def event_source_name(record):
    values = nested_values(record, "sourceName")
    return values[0] if values else ""


def event_location_key(record, location, latitude, longitude):
    references = []
    for contained in record.iter():
        if local_name(contained.tag) != "locationContainedInGroup":
            continue
        reference = ""
        for child in contained.iter():
            if local_name(child.tag) == "predefinedLocationReference":
                reference = xml_attribute(child, "id")
                break
        if not reference:
            continue
        direction = []
        for field in ("startNode", "endNode"):
            for child in contained.iter():
                if local_name(child.tag) == field:
                    direction.append(field + "=" + xml_attribute(child, "id"))
                    break
        for field in ("startChainage", "endChainage"):
            value = descendant_text(contained, field)
            if value:
                direction.append(field + "=" + value)
        references.append(reference + ":" + ",".join(direction))
    if references:
        return "|".join(references)
    if latitude is not None and longitude is not None:
        return f"{location.lower()}|{latitude:.6f}|{longitude:.6f}"
    return location.lower()


def normalise_record(
        record, situation_id, publication_time, information_status="real",
        network_resolver=None, resolution_db=None):
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
    source_road = extract_road_name(location)
    road = source_road

    latitude, longitude = extract_coordinates(record)
    network_match = None
    if network_resolver is not None and latitude is not None and longitude is not None:
        network_match = network_resolver.resolve_cached(
            latitude, longitude, source_road, resolution_db
        )
        if network_match:
            road = resolved_road_name(source_road, network_match["road"])
            if road and source_road.endswith(" SPUR"):
                road += " SPUR"
            if source_road and road and source_road != road:
                location = replace_first_road_name(location, road)
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
    te_event_type = traffic_england_event_type(record_type)
    delay_seconds = float_value(descendant_text(record, "delayTimeValue"))
    source_name = event_source_name(record)
    source_situation_time = descendant_text(record, "sourceSituationCreationTime")
    ephemeral = (
        te_event_type == "CONGESTION"
        and source_name.lower() == "fused traffic data"
    )
    location_key = event_location_key(record, location, latitude, longitude)
    description = build_description(
        record, comments, location, reason, lanes, lanes_closed, lanes_total)
    if not location and network_match and network_match.get("description"):
        description = "Location : " + network_match["description"] + (
            "\r\n" + description if description else ""
        )
    alert = {
        "id": "ntis:" + record_id,
        "ntisRecordId": record_id,
        "ntisSituationId": situation_id,
        "title": reason,
        "reason": reason,
        "description": description,
        "road": road,
        "sourceRoad": source_road,
        "unresolved": not bool(source_road),
        "networkResolved": bool(network_match),
        "networkLocation": network_match.get("description", "") if network_match else "",
        "trafficEnglandEventType": te_event_type,
        "confirmed": descendant_text(record, "probabilityOfOccurrence").lower() == "certain",
        "completed": False,
        "current": True,
        "hasPublicPresentation": bool(comments),
        "informationStatus": information_status or "real",
        "sourceName": source_name,
        "sourceSituationCreationTime": source_situation_time,
        "ephemeral": ephemeral,
        "ephemeralKey": "fused-congestion:" + location_key if ephemeral else "",
        "region": descendant_text(record, "allocatedRoc") or "England",
        "severity": map_severity(
            descendant_text(record, "severity"), te_event_type, delay_seconds
        ),
        "eventType": te_event_type,
        "ntisRecordType": record_type,
        "updated": version_time or descendant_text(record, "situationRecordCreationTime"),
        "planned": planned,
        "delaySeconds": delay_seconds,
        "lanesClosed": lanes_closed,
        "lanesTotal": lanes_total,
        "lanes": lanes,
    }
    if latitude is not None and longitude is not None:
        alert["latitude"] = latitude
        alert["longitude"] = longitude
    refresh_traffic_england_flags(alert)
    return {
        "recordId": record_id,
        "version": version,
        "versionTime": version_time,
        "current": True,
        "alert": alert,
    }


def parse_event_publication(path, network_resolver=None, resolution_db=None):
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
        information_status = descendant_text(situation, "informationStatus") or "real"
        for record in list(situation):
            if local_name(record.tag) != "situationRecord":
                continue
            normalised = normalise_record(
                record, situation_id, publication_time, information_status,
                network_resolver, resolution_db
            )
            if normalised is not None:
                records.append(normalised)
    return feed_type, publication_time, records


def utc_now():
    return datetime.now(timezone.utc)


def initialise_database():
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    rebuild_required = False
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
                received_at TEXT NOT NULL DEFAULT '',
                ephemeral_key TEXT NOT NULL DEFAULT '',
                alert_json TEXT NOT NULL
            )
            """
        )
        columns = {
            row[1] for row in db.execute("PRAGMA table_info(event_state)")
        }
        if "received_at" not in columns:
            db.execute(
                "ALTER TABLE event_state ADD COLUMN received_at TEXT NOT NULL DEFAULT ''"
            )
        if "ephemeral_key" not in columns:
            db.execute(
                "ALTER TABLE event_state ADD COLUMN ephemeral_key TEXT NOT NULL DEFAULT ''"
            )
        db.execute(
            "CREATE INDEX IF NOT EXISTS event_state_ephemeral "
            "ON event_state(ephemeral_key, received_at)"
        )
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS processor_metadata (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """
        )
        db.execute(
            """
            CREATE TABLE IF NOT EXISTS network_resolution (
                cache_key TEXT PRIMARY KEY,
                latitude REAL NOT NULL,
                longitude REAL NOT NULL,
                source_road TEXT NOT NULL,
                road TEXT NOT NULL,
                description TEXT NOT NULL,
                distance_metres REAL NOT NULL,
                resolved_at TEXT NOT NULL
            )
            """
        )

        schema_row = db.execute(
            "SELECT value FROM processor_metadata WHERE key = ?",
            ("traffic_england_schema_version",),
        ).fetchone()
        schema_version = schema_row[0] if schema_row else ""
        pending_row = db.execute(
            "SELECT value FROM processor_metadata WHERE key = ?",
            ("traffic_england_rebuild_pending",),
        ).fetchone()
        rebuild_pending = pending_row and pending_row[0] == "1"
        event_message_count = db.execute(
            "SELECT COUNT(*) FROM messages WHERE feed = 'event'"
        ).fetchone()[0]

        if schema_version != TRAFFIC_ENGLAND_SCHEMA_VERSION and event_message_count:
            rebuild_required = True
            if not rebuild_pending:
                # The original DATEX II publications are retained specifically so
                # compatibility changes can be rebuilt without guessing fields that
                # were not present in older cached alert JSON.
                db.execute("DELETE FROM event_state")
                db.execute(
                    "UPDATE messages SET processed_at = NULL WHERE feed = 'event'"
                )
                db.execute(
                    "DELETE FROM processor_metadata WHERE key IN "
                    "('refresh_in_progress', 'last_feed_type', "
                    "'last_refresh_received_epoch', 'refresh_generation')"
                )
                db.execute(
                    "INSERT INTO processor_metadata(key, value) VALUES(?, '1') "
                    "ON CONFLICT(key) DO UPDATE SET value = excluded.value",
                    ("traffic_england_rebuild_pending",),
                )
                logging.warning(
                    "Rebuilding Traffic England compatibility state from %d stored NTIS publication(s)",
                    event_message_count,
                )
        elif schema_version != TRAFFIC_ENGLAND_SCHEMA_VERSION:
            db.execute(
                "INSERT INTO processor_metadata(key, value) VALUES(?, ?) "
                "ON CONFLICT(key) DO UPDATE SET value = excluded.value",
                ("traffic_england_schema_version", TRAFFIC_ENGLAND_SCHEMA_VERSION),
            )

    if rebuild_required:
        try:
            SNAPSHOT_PATH.unlink()
        except FileNotFoundError:
            pass
    return rebuild_required


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


def point_segment_distance_metres(latitude, longitude, start, end):
    scale_x = 111320.0 * cos(latitude * pi / 180.0)
    scale_y = 110540.0
    ax = (float(start[0]) - longitude) * scale_x
    ay = (float(start[1]) - latitude) * scale_y
    bx = (float(end[0]) - longitude) * scale_x
    by = (float(end[1]) - latitude) * scale_y
    dx = bx - ax
    dy = by - ay
    length_squared = dx * dx + dy * dy
    if length_squared <= 0.000001:
        return hypot(ax, ay)
    amount = max(0.0, min(1.0, -(ax * dx + ay * dy) / length_squared))
    return hypot(ax + amount * dx, ay + amount * dy)


def feature_distance_metres(latitude, longitude, geometry):
    closest = None
    for path in geometry.get("paths", []):
        for index in range(1, len(path)):
            distance = point_segment_distance_metres(
                latitude, longitude, path[index - 1], path[index]
            )
            if closest is None or distance < closest:
                closest = distance
    return closest


class NetworkModelResolver:
    def __init__(self):
        self._wake = threading.Event()
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._queued = set()
        self._queue = deque()
        self._retry_after = {}
        self._retry_items = {}
        self._on_change = None
        self._thread = threading.Thread(
            target=self._run, name="ntis-network-model-resolver", daemon=True
        )

    def set_change_callback(self, callback):
        self._on_change = callback

    def start(self):
        self._thread.start()

    def stop(self):
        self._stop.set()
        self._wake.set()
        self._thread.join(timeout=10)

    @staticmethod
    def _cache_key(latitude, longitude, source_road):
        return f"{latitude:.5f},{longitude:.5f}|{road_base(source_road)}"

    def resolve_cached(self, latitude, longitude, source_road="", db=None):
        key = self._cache_key(latitude, longitude, source_road)
        row = None
        try:
            if db is not None:
                row = db.execute(
                    """
                    SELECT road, description, distance_metres, resolved_at
                    FROM network_resolution WHERE cache_key = ?
                    """,
                    (key,),
                ).fetchone()
            else:
                with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=5) as cache_db:
                    row = cache_db.execute(
                        """
                        SELECT road, description, distance_metres, resolved_at
                        FROM network_resolution WHERE cache_key = ?
                        """,
                        (key,),
                    ).fetchone()
        except sqlite3.Error:
            logging.exception("Could not read the Network Model resolution cache")

        stale = True
        if row:
            resolved_at = parse_iso_time(row[3])
            stale = resolved_at is None or (
                utc_now() - resolved_at
            ).total_seconds() > NETWORK_MODEL_CACHE_DAYS * 86400
        if not row or stale:
            self._schedule(key, latitude, longitude, source_road)
        if not row or not row[0]:
            return None
        return {
            "road": row[0],
            "description": row[1],
            "distanceMetres": float(row[2]),
        }

    def invalidate(self):
        try:
            with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
                db.execute("UPDATE network_resolution SET resolved_at = ''")
        except sqlite3.Error:
            logging.exception("Could not invalidate the Network Model resolution cache")
        if self._on_change:
            self._on_change()

    def _schedule(self, key, latitude, longitude, source_road):
        with self._lock:
            if key in self._queued or time.monotonic() < self._retry_after.get(key, 0):
                return
            self._queued.add(key)
            self._queue.append((key, latitude, longitude, source_road))
        self._wake.set()

    def _take(self):
        with self._lock:
            if self._queue:
                return self._queue.popleft()
            now = time.monotonic()
            for key, item in list(self._retry_items.items()):
                if now < self._retry_after.get(key, 0):
                    continue
                self._retry_items.pop(key, None)
                self._retry_after.pop(key, None)
                self._queued.add(key)
                return item
            return None

    def _finish(self, key):
        with self._lock:
            self._queued.discard(key)

    def _retry_later(self, key, latitude, longitude, source_road):
        with self._lock:
            self._retry_after[key] = time.monotonic() + 60.0
            self._retry_items[key] = (key, latitude, longitude, source_road)

    def _clear_retry(self, key):
        with self._lock:
            self._retry_after.pop(key, None)
            self._retry_items.pop(key, None)

    def _query(self, latitude, longitude, source_road):
        params = {
            "f": "json",
            "where": "srn='Y' AND roadname IS NOT NULL",
            "geometry": f"{longitude:.7f},{latitude:.7f}",
            "geometryType": "esriGeometryPoint",
            "inSR": "4326",
            "outSR": "4326",
            "spatialRel": "esriSpatialRelIntersects",
            "distance": str(NETWORK_MODEL_SEARCH_METRES),
            "units": "esriSRUnit_Meter",
            "outFields": "roadname,linkref,linkdesc,operationalstate,srn",
            "returnGeometry": "true",
            "resultRecordCount": "2000",
        }
        with urlopen(NETWORK_MODEL_LINK_URL + "?" + urlencode(params), timeout=20) as response:
            document = json.load(response)
        if document.get("error"):
            raise RuntimeError(document["error"].get("message", "Network Model query failed"))

        wanted_base = road_base(source_road)
        candidates = []
        for feature in document.get("features", []):
            attributes = feature.get("attributes", {})
            road = normalise_road_name(attributes.get("roadname", ""))
            if not road:
                continue
            if wanted_base and road_base(road) != wanted_base:
                continue
            distance = feature_distance_metres(
                latitude, longitude, feature.get("geometry", {})
            )
            if distance is None or distance > NETWORK_MODEL_SEARCH_METRES:
                continue
            candidates.append((distance, road, attributes.get("linkdesc", "")))
        if not candidates:
            return "", "", -1.0
        distance, road, description = min(candidates, key=lambda candidate: candidate[0])
        return road, description, distance

    def _store(self, key, latitude, longitude, source_road, result):
        road, description, distance = result
        with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
            db.execute(
                """
                INSERT INTO network_resolution(
                    cache_key, latitude, longitude, source_road, road,
                    description, distance_metres, resolved_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(cache_key) DO UPDATE SET
                    latitude = excluded.latitude,
                    longitude = excluded.longitude,
                    source_road = excluded.source_road,
                    road = excluded.road,
                    description = excluded.description,
                    distance_metres = excluded.distance_metres,
                    resolved_at = excluded.resolved_at
                """,
                (
                    key, latitude, longitude, source_road, road, description,
                    distance, utc_now().isoformat(),
                ),
            )

    def _run(self):
        changed = False
        last_notification = time.monotonic()
        while not self._stop.is_set():
            item = self._take()
            if item is None:
                if changed and self._on_change:
                    self._on_change()
                    changed = False
                self._wake.wait(timeout=1.0)
                self._wake.clear()
                continue

            key, latitude, longitude, source_road = item
            try:
                result = self._query(latitude, longitude, source_road)
                self._store(key, latitude, longitude, source_road, result)
                self._clear_retry(key)
                changed = True
            except Exception:
                self._retry_later(key, latitude, longitude, source_road)
                logging.exception(
                    "Could not resolve %.6f, %.6f through the Network Model",
                    latitude,
                    longitude,
                )
            finally:
                self._finish(key)

            if changed and time.monotonic() - last_notification >= 1.0:
                if self._on_change:
                    self._on_change()
                changed = False
                last_notification = time.monotonic()


class NtisEventProcessor:
    def __init__(self, network_resolver):
        self._network_resolver = network_resolver
        self._settings_lock = threading.Lock()
        with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
            stored_age = integer_value(
                metadata_get(
                    db,
                    "fused_congestion_max_age_seconds",
                    DEFAULT_FUSED_CONGESTION_MAX_AGE_SECONDS,
                ),
                DEFAULT_FUSED_CONGESTION_MAX_AGE_SECONDS,
            )
        self._fused_congestion_max_age_seconds = max(
            MIN_FUSED_CONGESTION_MAX_AGE_SECONDS,
            min(MAX_FUSED_CONGESTION_MAX_AGE_SECONDS, stored_age),
        )
        self._wake = threading.Event()
        self._stop = threading.Event()
        self._network_refresh_requested = threading.Event()
        self._thread = threading.Thread(
            target=self._run, name="ntis-event-processor", daemon=True
        )

    def start(self):
        self._thread.start()

    def notify(self):
        self._wake.set()

    def request_network_refresh(self):
        self._network_refresh_requested.set()
        self.notify()

    def fused_congestion_max_age_seconds(self):
        with self._settings_lock:
            return self._fused_congestion_max_age_seconds

    def set_fused_congestion_max_age_seconds(self, value):
        value = max(
            MIN_FUSED_CONGESTION_MAX_AGE_SECONDS,
            min(MAX_FUSED_CONGESTION_MAX_AGE_SECONDS, integer_value(value, 600)),
        )
        with self._settings_lock:
            self._fused_congestion_max_age_seconds = value
        with sqlite3.connect(DATA_DIR / "messages.sqlite3", timeout=30) as db:
            metadata_set(db, "fused_congestion_max_age_seconds", value)
            db.commit()
        self.notify()
        logging.info("Fused congestion freshness changed to %d seconds", value)
        return value

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
        public_count = sum(
            1 for alert in alerts if alert.get("trafficEnglandVisible", False)
        )
        unplanned_public_count = sum(
            1 for alert in alerts
            if alert.get("trafficEnglandVisible", False)
            and alert.get("trafficEnglandUnplanned", False)
        )
        unresolved_count = sum(
            1 for alert in alerts
            if alert.get("trafficEnglandEligible", False)
            and alert.get("unresolved", False)
            and alert.get("networkResolved", False)
        )
        generation = integer_value(metadata_get(db, "snapshot_generation")) + 1
        metadata_set(db, "snapshot_generation", generation)
        document = {
            "schemaVersion": 1,
            "source": "National Highways NTIS Event Data",
            "generation": generation,
            "updatedAt": utc_now().isoformat(),
            "refreshInProgress": False,
            "trafficEnglandPublicCount": public_count,
            "trafficEnglandUnplannedPublicCount": unplanned_public_count,
            "trafficEnglandResolvedExtraCount": unresolved_count,
            "currentRecordCount": len(alerts),
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
            "Published NTIS event snapshot generation %d with %d Traffic England-compatible public incident(s), %d resolved extra(s), and %d current raw record(s)",
            generation,
            public_count,
            unresolved_count,
            len(alerts),
        )

    def _refresh_network_resolutions(self, db):
        changed = False
        rows = db.execute(
            "SELECT record_id, alert_json FROM event_state"
        ).fetchall()
        for record_id, alert_json in rows:
            alert = json.loads(alert_json)
            if "trafficEnglandEventType" not in alert:
                alert["trafficEnglandEventType"] = traffic_england_event_type(
                    alert.get("ntisRecordType", "RoadIncident")
                )
            # Missing source compatibility fields are treated conservatively. A
            # one-time raw-publication replay upgrades pre-existing databases.
            alert.setdefault("confirmed", False)
            alert.setdefault("completed", False)
            alert.setdefault("current", True)
            alert.setdefault("hasPublicPresentation", False)
            alert.setdefault("informationStatus", "real")
            latitude = float_value(alert.get("latitude"), None)
            longitude = float_value(alert.get("longitude"), None)
            if latitude is None or longitude is None:
                refresh_traffic_england_flags(alert)
                updated_json = json.dumps(alert, ensure_ascii=False, separators=(",", ":"))
                if updated_json != alert_json:
                    db.execute(
                        "UPDATE event_state SET alert_json = ? WHERE record_id = ?",
                        (updated_json, record_id),
                    )
                    changed = True
                continue

            source_road = extract_road_name(
                alert.get("sourceRoad", alert.get("road", ""))
            )
            unresolved = bool(alert.get("unresolved", not source_road))
            network_match = self._network_resolver.resolve_cached(
                latitude, longitude, source_road, db
            )
            if not network_match:
                alert["sourceRoad"] = source_road
                alert["unresolved"] = unresolved
                alert["networkResolved"] = False
                refresh_traffic_england_flags(alert)
                updated_json = json.dumps(alert, ensure_ascii=False, separators=(",", ":"))
                if updated_json != alert_json:
                    db.execute(
                        "UPDATE event_state SET alert_json = ? WHERE record_id = ?",
                        (updated_json, record_id),
                    )
                    changed = True
                continue

            model_road = resolved_road_name(source_road, network_match["road"])
            if not model_road:
                continue
            if source_road.endswith(" SPUR"):
                model_road += " SPUR"

            previous_location = alert.get("networkLocation", "")
            model_location = network_match.get("description", "")
            alert["road"] = model_road
            alert["sourceRoad"] = source_road
            alert["unresolved"] = unresolved
            alert["networkResolved"] = True
            alert["networkLocation"] = model_location
            if source_road and model_road and source_road != model_road:
                alert["description"] = replace_first_road_name(
                    alert.get("description", ""), model_road
                )
            if unresolved and model_location and not previous_location:
                existing = alert.get("description", "")
                alert["description"] = "Location : " + model_location + (
                    "\r\n" + existing if existing else ""
                )
            refresh_traffic_england_flags(alert)
            updated_json = json.dumps(alert, ensure_ascii=False, separators=(",", ":"))
            if updated_json != alert_json:
                db.execute(
                    "UPDATE event_state SET alert_json = ? WHERE record_id = ?",
                    (updated_json, record_id),
                )
                changed = True
        return changed

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

    def _expire_ephemeral_records(self, db):
        cutoff = datetime.fromtimestamp(
            time.time() - self.fused_congestion_max_age_seconds(),
            timezone.utc,
        ).isoformat()
        return db.execute(
            "DELETE FROM event_state "
            "WHERE ephemeral_key <> '' AND received_at < ?",
            (cutoff,),
        ).rowcount

    def _apply_publication(self, db, message_id, path, received_at):
        feed_type, publication_time, records = parse_event_publication(
            path, self._network_resolver, db
        )
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
            alert["receivedAt"] = received_at
            ephemeral_key = alert.get("ephemeralKey", "")
            if ephemeral_key:
                # Fused congestion is a rolling sensor-derived snapshot. NTIS
                # assigns a new record id to each sample, so the network
                # location is the stable identity Traffic England used.
                removed = db.execute(
                    "DELETE FROM event_state "
                    "WHERE ephemeral_key = ? AND record_id <> ?",
                    (ephemeral_key, record_id),
                ).rowcount
                changed = changed or removed > 0
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
                    refresh_generation, received_at, ephemeral_key, alert_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(record_id) DO UPDATE SET
                    version = excluded.version,
                    version_time = excluded.version_time,
                    situation_id = excluded.situation_id,
                    refresh_generation = excluded.refresh_generation,
                    received_at = excluded.received_at,
                    ephemeral_key = excluded.ephemeral_key,
                    alert_json = excluded.alert_json
                """,
                (
                    record_id,
                    version,
                    record["versionTime"],
                    alert.get("ntisSituationId", ""),
                    row_generation,
                    received_at,
                    ephemeral_key,
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
            rebuild_pending = (
                metadata_get(db, "traffic_england_rebuild_pending", "0") == "1"
            )
            batch_size = (
                TRAFFIC_ENGLAND_REBUILD_BATCH_SIZE if rebuild_pending else 50
            )
            rows = db.execute(
                """
                SELECT id, path, received_at
                FROM messages
                WHERE feed = 'event' AND processed_at IS NULL
                ORDER BY id
                LIMIT ?
                """,
                (batch_size,),
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
            expired = self._expire_ephemeral_records(db)
            any_changed = any_changed or expired > 0
            if self._network_refresh_requested.is_set():
                self._network_refresh_requested.clear()
                if not rebuild_pending:
                    any_changed = self._refresh_network_resolutions(db) or any_changed
            if rebuild_pending and not rows:
                self._finalise_refresh_if_idle(db, force=True)
                self._refresh_network_resolutions(db)
                metadata_set(
                    db,
                    "traffic_england_schema_version",
                    TRAFFIC_ENGLAND_SCHEMA_VERSION,
                )
                metadata_set(db, "traffic_england_rebuild_pending", "0")
                self._write_snapshot(db)
                db.commit()
                logging.info(
                    "Traffic England compatibility rebuild completed at schema %s",
                    TRAFFIC_ENGLAND_SCHEMA_VERSION,
                )
                return 0
            refresh_active = metadata_get(db, "refresh_in_progress", "0") == "1"
            if (any_changed or finalised) and not refresh_active and not rebuild_pending:
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

    def __init__(self, server_address, handler_class, event_processor, network_resolver):
        super().__init__(server_address, handler_class)
        self.event_processor = event_processor
        self.network_resolver = network_resolver


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
        if path == "/internal/settings":
            age_seconds = self.server.event_processor.fused_congestion_max_age_seconds()
            self.send_text(
                200,
                json.dumps({
                    "fusedCongestionMaxAgeSeconds": age_seconds,
                    "fusedCongestionMaxAgeMinutes": age_seconds // 60,
                }) + "\n",
                "application/json; charset=utf-8",
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
                        if alert.get("trafficEnglandUnplanned", False)
                    ]
                    document["trafficEnglandPublicCount"] = document.get(
                        "trafficEnglandUnplannedPublicCount",
                        sum(
                            1 for alert in document["alerts"]
                            if alert.get("trafficEnglandVisible", False)
                        ),
                    )
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
        if path == "/internal/settings":
            length_text = self.headers.get("Content-Length")
            try:
                content_length = int(length_text or "")
            except ValueError:
                self.send_text(411, "A valid Content-Length header is required\n")
                return
            if content_length < 0 or content_length > 4096:
                self.send_text(413, "Settings payload is too large\n")
                return
            try:
                payload = json.loads(self.rfile.read(content_length).decode("utf-8"))
                age_seconds = self.server.event_processor.set_fused_congestion_max_age_seconds(
                    payload["fusedCongestionMaxAgeSeconds"]
                )
            except (KeyError, TypeError, ValueError, json.JSONDecodeError):
                self.send_text(400, '{"error":"Invalid settings payload"}\n', "application/json")
                return
            self.send_text(
                200,
                json.dumps({
                    "fusedCongestionMaxAgeSeconds": age_seconds,
                    "fusedCongestionMaxAgeMinutes": age_seconds // 60,
                }) + "\n",
                "application/json; charset=utf-8",
            )
            return
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
            elif not duplicate and feed == "network-model":
                self.server.network_resolver.invalidate()
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
    network_resolver = NetworkModelResolver()
    event_processor = NtisEventProcessor(network_resolver)
    network_resolver.set_change_callback(event_processor.request_network_refresh)
    network_resolver.start()
    event_processor.start()
    event_processor.request_network_refresh()
    server = ReceiverServer(
        (BIND_HOST, BIND_PORT), ReceiverHandler, event_processor, network_resolver
    )

    def stop_server(_signum, _frame):
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, stop_server)
    signal.signal(signal.SIGINT, stop_server)
    logging.info("NTIS receiver listening on %s:%d", BIND_HOST, BIND_PORT)
    server.serve_forever(poll_interval=0.5)
    server.server_close()
    event_processor.stop()
    network_resolver.stop()


if __name__ == "__main__":
    main()
