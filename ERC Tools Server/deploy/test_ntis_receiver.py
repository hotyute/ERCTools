import gc
import json
import shutil
import sqlite3
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

import ntis_receiver


class StaticResolver:
    def __init__(self, road="M1", description="M1 between junctions J2 and J3"):
        self.road = road
        self.description = description

    def resolve_cached(self, latitude, longitude, source_road, resolution_db):
        return {
            "road": self.road,
            "description": self.description,
            "distanceMetres": 10.0,
        }


class MissingResolver:
    def resolve_cached(self, latitude, longitude, source_road, resolution_db):
        return None


def make_record(record_type="Accident", probability="certain", comments=None,
                validity="active", road="M1", source="", delay=0,
                overall_end=""):
    comments = comments if comments is not None else [
        f"The {road} northbound between junctions J2 and J3",
        "Road traffic collision",
        "Currently Active",
    ]
    values = "".join(f"<value>{value}</value>" for value in comments)
    return ET.fromstring(f"""
        <situationRecord xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            id="record-1" version="3" xsi:type="{record_type}">
          <situationRecordCreationTime>2026-07-03T12:00:00Z</situationRecordCreationTime>
          <situationRecordVersionTime>2026-07-03T12:05:00Z</situationRecordVersionTime>
          <probabilityOfOccurrence>{probability}</probabilityOfOccurrence>
          <severity>medium</severity>
          <source><sourceName><values><value>{source}</value></values></sourceName>
            <sourceExtension><sourceSituation>
              <sourceSituationCreationTime>2026-07-03T12:04:00Z</sourceSituationCreationTime>
            </sourceSituation></sourceExtension>
          </source>
          <validity><validityStatus>{validity}</validityStatus>
            <validityTimeSpecification>
              <overallStartTime>2026-07-03T12:00:00Z</overallStartTime>
              {f'<overallEndTime>{overall_end}</overallEndTime>' if overall_end else ''}
            </validityTimeSpecification>
          </validity>
          <impact><delays><delayTimeValue>{delay}</delayTimeValue></delays></impact>
          <generalPublicComment><comment><values>{values}</values></comment></generalPublicComment>
          <groupOfLocations><locationForDisplay>
            <latitude>52.0</latitude><longitude>-1.0</longitude>
          </locationForDisplay>
          <locationContainedInGroup>
            <predefinedLocationReference id="link-1" />
            <locationByReferenceExtension><directionalityReference>
              <startNode id="node-1" /><startChainage>0</startChainage>
            </directionalityReference></locationByReferenceExtension>
          </locationContainedInGroup></groupOfLocations>
        </situationRecord>
    """)


class TrafficEnglandCompatibilityTests(unittest.TestCase):
    def normalise(self, record, resolver=None):
        return ntis_receiver.normalise_record(
            record,
            "situation-1",
            "2026-07-03T12:10:00Z",
            "real",
            resolver or StaticResolver(),
            None,
        )["alert"]

    def test_confirmed_current_incident_is_public(self):
        alert = self.normalise(make_record())
        self.assertEqual("INCIDENT", alert["trafficEnglandEventType"])
        self.assertTrue(alert["trafficEnglandEligible"])
        self.assertTrue(alert["trafficEnglandUnplanned"])
        self.assertTrue(alert["trafficEnglandVisible"])

    def test_abnormal_traffic_is_congestion(self):
        alert = self.normalise(make_record(record_type="AbnormalTraffic"))
        self.assertEqual("CONGESTION", alert["trafficEnglandEventType"])
        self.assertTrue(alert["trafficEnglandVisible"])

    def test_public_record_does_not_depend_on_auxiliary_network_lookup(self):
        alert = self.normalise(make_record(), MissingResolver())
        self.assertFalse(alert["networkResolved"])
        self.assertTrue(alert["trafficEnglandVisible"])

    def test_other_road_management_matches_former_public_label(self):
        alert = self.normalise(make_record(
            record_type="RoadOrCarriagewayOrLaneManagement",
            comments=[
                "The M5 southbound between junctions J14 and J15",
                "Other Road Management",
                "Currently Active",
            ],
        ))
        self.assertEqual("Road Management", alert["title"])
        self.assertIn("Reason : Road Management", alert["description"])
        self.assertNotIn("Other Road Management", alert["description"])

    def test_plain_other_road_management_matches_former_public_label(self):
        alert = self.normalise(make_record(
            record_type="RoadOrCarriagewayOrLaneManagement",
            comments=[
                "The M1 southbound between junctions J14 and J13",
                "Other",
                "Currently Active",
            ],
        ))
        self.assertEqual("Road Management", alert["title"])
        self.assertIn("Reason : Road Management", alert["description"])
        self.assertNotIn("Reason : Other", alert["description"])

    def test_plain_other_vehicle_obstruction_matches_public_bucket(self):
        alert = self.normalise(make_record(
            record_type="VehicleObstruction",
            comments=[
                "The M1 southbound between junctions J15 and J14",
                "Other",
                "Currently Active",
            ],
        ))
        self.assertEqual("Vehicle obstruction", alert["title"])
        self.assertIn("Reason : Vehicle obstruction", alert["description"])
        self.assertNotIn("Reason : Other", alert["description"])

    def test_other_authority_operation_matches_police_incident_label(self):
        alert = self.normalise(make_record(
            record_type="AuthorityOperation",
            comments=[
                "The M18 southbound between junctions J7 and J6",
                "Other",
                "Currently Active",
            ],
        ))
        self.assertEqual("Police incident", alert["title"])
        self.assertIn("Reason : Police incident", alert["description"])
        self.assertNotIn("Reason : Other", alert["description"])

    def test_fused_congestion_has_stable_rolling_identity(self):
        alert = self.normalise(make_record(
            record_type="AbnormalTraffic",
            source="Fused Traffic Data",
            delay=131,
        ))
        self.assertTrue(alert["ephemeral"])
        self.assertEqual(
            "fused-congestion:link-1:startNode=node-1,startChainage=0",
            alert["ephemeralKey"],
        )
        self.assertIn("Delay : 10 minutes", alert["description"])
        self.assertEqual("Moderate", alert["severity"])

    def test_active_record_past_its_overall_end_is_not_current(self):
        result = ntis_receiver.normalise_record(
            make_record(overall_end="2026-07-03T12:09:00Z"),
            "situation-1",
            "2026-07-03T12:10:00Z",
            "real",
            StaticResolver(),
            None,
        )
        self.assertFalse(result["current"])

    def test_current_alert_preserves_validity_window(self):
        alert = self.normalise(
            make_record(overall_end="2026-07-03T12:30:00Z")
        )
        self.assertEqual("active", alert["validityStatus"])
        self.assertEqual("2026-07-03T12:30:00Z", alert["overallEndTime"])
        self.assertTrue(ntis_receiver.cached_alert_is_current(
            alert,
            ntis_receiver.parse_iso_time("2026-07-03T12:29:00Z"),
        ))
        self.assertFalse(ntis_receiver.cached_alert_is_current(
            alert,
            ntis_receiver.parse_iso_time("2026-07-03T12:31:00Z"),
        ))

    def test_public_clearance_band_uses_uk_time_and_upper_bound(self):
        self.assertEqual(
            "2026-08-15T13:30:00+00:00",
            ntis_receiver.public_clearance_end_time(
                "The event is expected to clear between 14:15 and 14:30 "
                "on 15 August 2026"
            ),
        )

    def test_public_clearance_band_rolls_midnight_into_next_day(self):
        self.assertEqual(
            "2026-08-17T23:00:00+00:00",
            ntis_receiver.public_clearance_end_time(
                "The event is expected to clear between 23:45 and 00:00 "
                "on 17 August 2026"
            ),
        )

    def test_active_unplanned_record_past_public_clearance_is_not_current(self):
        result = ntis_receiver.normalise_record(
            make_record(comments=[
                "The M1 northbound between junctions J2 and J3",
                "Road traffic collision",
                "The event is expected to clear between 11:45 and 12:00 "
                "on 3 July 2026",
                "Currently Active",
            ]),
            "situation-1",
            "2026-07-03T12:10:00Z",
            "real",
            StaticResolver(),
            None,
        )
        self.assertFalse(result["current"])

    def test_cached_unplanned_record_lazily_uses_public_clearance(self):
        alert = {
            "validityStatus": "active",
            "overallStartTime": "2026-07-03T10:00:00Z",
            "overallEndTime": "",
            "trafficEnglandEventType": "INCIDENT",
            "description": (
                "Time To Clear : The event is expected to clear between "
                "11:45 and 12:00 on 3 July 2026"
            ),
        }
        self.assertFalse(ntis_receiver.cached_alert_is_current(
            alert,
            ntis_receiver.parse_iso_time("2026-07-03T12:10:00Z"),
        ))

    def test_public_clearance_fallback_does_not_expire_roadworks(self):
        result = ntis_receiver.normalise_record(
            make_record(record_type="MaintenanceWorks", comments=[
                "The M1 northbound between junctions J2 and J3",
                "Roadworks",
                "The event is expected to clear between 11:45 and 12:00 "
                "on 3 July 2026",
                "Currently Active",
            ]),
            "situation-1",
            "2026-07-03T12:10:00Z",
            "real",
            StaticResolver(),
            None,
        )
        self.assertTrue(result["current"])

    def test_legacy_cached_alert_without_validity_metadata_remains_current(self):
        self.assertTrue(ntis_receiver.cached_alert_is_current({"id": "legacy"}))

    def test_unconfirmed_incident_is_not_public(self):
        alert = self.normalise(make_record(probability="probable"))
        self.assertFalse(alert["confirmed"])
        self.assertFalse(alert["trafficEnglandEligible"])
        self.assertFalse(alert["trafficEnglandVisible"])

    def test_roadworks_are_not_in_default_unplanned_public_set(self):
        alert = self.normalise(make_record(record_type="MaintenanceWorks"))
        self.assertEqual("ROADWORKS", alert["trafficEnglandEventType"])
        self.assertTrue(alert["trafficEnglandEligible"])
        self.assertFalse(alert["trafficEnglandUnplanned"])
        self.assertTrue(alert["trafficEnglandVisible"])

    def test_record_without_public_presentation_is_not_public(self):
        alert = self.normalise(make_record(comments=[]))
        self.assertFalse(alert["hasPublicPresentation"])
        self.assertFalse(alert["trafficEnglandEligible"])

    def test_source_unresolved_record_remains_optional_after_network_resolution(self):
        alert = self.normalise(
            make_record(comments=["Eastern region", "Congestion"]),
            StaticResolver("A14", "A14 eastbound near junction J2"),
        )
        self.assertTrue(alert["unresolved"])
        self.assertTrue(alert["networkResolved"])
        self.assertTrue(alert["trafficEnglandEligible"])
        self.assertFalse(alert["trafficEnglandVisible"])

    def test_network_model_upgrades_a1_to_a1m(self):
        alert = self.normalise(make_record(road="A1"), StaticResolver("A1(M)"))
        self.assertEqual("A1(M)", alert["road"])
        self.assertIn("The A1(M) northbound", alert["description"])

    def test_public_snapshot_matches_former_unplanned_query(self):
        document = {
            "updatedAt": "2026-07-03T12:10:00+00:00",
            "currentRecordCount": 4,
            "alerts": [
                {
                    "id": "incident",
                    "trafficEnglandVisible": True,
                    "trafficEnglandUnplanned": True,
                },
                {
                    "id": "roadworks",
                    "trafficEnglandVisible": True,
                    "trafficEnglandUnplanned": False,
                },
                {
                    "id": "raw-hidden",
                    "trafficEnglandVisible": False,
                    "trafficEnglandUnplanned": True,
                },
                {"id": "raw-unconfirmed"},
            ],
        }
        now = ntis_receiver.parse_iso_time("2026-07-03T12:11:00Z")

        unplanned = ntis_receiver.public_snapshot(document, True, now)
        self.assertEqual(["incident"], [item["id"] for item in unplanned["alerts"]])
        self.assertEqual(1, unplanned["trafficEnglandPublicCount"])

        all_public = ntis_receiver.public_snapshot(document, False, now)
        self.assertEqual(
            ["incident", "roadworks"],
            [item["id"] for item in all_public["alerts"]],
        )
        self.assertEqual(2, all_public["trafficEnglandPublicCount"])

    def test_quiet_change_driven_source_retains_current_cache(self):
        document = {
            "updatedAt": "2026-07-03T12:00:00+00:00",
            "currentRecordCount": 1,
            "alerts": [{
                "id": "retained",
                "trafficEnglandVisible": True,
                "trafficEnglandUnplanned": True,
            }],
        }
        result = ntis_receiver.public_snapshot(
            document,
            True,
            ntis_receiver.parse_iso_time("2026-07-03T12:11:00Z"),
        )
        self.assertFalse(result["sourceStale"])
        self.assertTrue(result["sourceQuiet"])
        self.assertEqual(["retained"], [item["id"] for item in result["alerts"]])
        self.assertEqual(1, result["currentRecordCount"])

    def test_source_receive_time_governs_freshness_not_snapshot_build_time(self):
        result = ntis_receiver.public_snapshot({
            "updatedAt": "2026-07-03T12:10:30+00:00",
            "sourceUpdatedAt": "2026-07-03T12:00:00+00:00",
            "alerts": [{
                "id": "stale-source",
                "trafficEnglandVisible": True,
                "trafficEnglandUnplanned": True,
            }],
        }, True, ntis_receiver.parse_iso_time("2026-07-03T12:11:00Z"))
        self.assertFalse(result["sourceStale"])
        self.assertTrue(result["sourceQuiet"])
        self.assertEqual("2026-07-03T12:00:00+00:00", result["sourceUpdatedAt"])
        self.assertEqual(["stale-source"], [item["id"] for item in result["alerts"]])

    def test_legacy_full_refresh_latch_does_not_block_a_fresh_source(self):
        result = ntis_receiver.public_snapshot({
            "updatedAt": "2026-07-03T12:10:00+00:00",
            "fullRefreshRequired": True,
            "alerts": [{
                "id": "untrusted",
                "trafficEnglandVisible": True,
                "trafficEnglandUnplanned": True,
            }],
        }, True, ntis_receiver.parse_iso_time("2026-07-03T12:11:00Z"))
        self.assertFalse(result["sourceStale"])
        self.assertEqual(["untrusted"], [item["id"] for item in result["alerts"]])


class TrafficEnglandMigrationTests(unittest.TestCase):
    def test_full_refresh_preserves_only_concurrent_updates_newer_than_refresh(self):
        old_data_dir = ntis_receiver.DATA_DIR
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        try:
            ntis_receiver.DATA_DIR = root
            ntis_receiver.initialise_database()
            processor = ntis_receiver.NtisEventProcessor(StaticResolver())
            with ntis_receiver.open_database() as db:
                ntis_receiver.metadata_set(db, "refresh_in_progress", "1")
                ntis_receiver.metadata_set(db, "refresh_generation", "2")
                ntis_receiver.metadata_set(
                    db, "refresh_publication_time", "2026-07-03T12:00:00Z"
                )
                for record_id, version_time in (
                        ("older-absent", "2026-07-03T11:59:00Z"),
                        ("newer-concurrent", "2026-07-03T12:01:00Z")):
                    db.execute(
                        "INSERT INTO event_state("
                        "record_id, version, version_time, situation_id, "
                        "refresh_generation, received_at, ephemeral_key, alert_json"
                        ") VALUES (?, 1, ?, 's', 1, '', '', '{}')",
                        (record_id, version_time),
                    )
                self.assertTrue(processor._finalise_refresh_if_idle(db, force=True))
                rows = db.execute(
                    "SELECT record_id, refresh_generation FROM event_state"
                ).fetchall()
            self.assertEqual([("newer-concurrent", 2)], rows)
        finally:
            ntis_receiver.DATA_DIR = old_data_dir
            gc.collect()

    def test_database_context_closes_connection(self):
        old_data_dir = ntis_receiver.DATA_DIR
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        try:
            ntis_receiver.DATA_DIR = root
            root.mkdir(parents=True, exist_ok=True)
            with ntis_receiver.open_database() as db:
                db.execute("SELECT 1").fetchone()
            with self.assertRaises(sqlite3.ProgrammingError):
                db.execute("SELECT 1")
        finally:
            ntis_receiver.DATA_DIR = old_data_dir

    def test_startup_uses_last_source_receive_time_without_a_permanent_latch(self):
        old_data_dir = ntis_receiver.DATA_DIR
        old_snapshot_path = ntis_receiver.SNAPSHOT_PATH
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        try:
            ntis_receiver.DATA_DIR = root
            ntis_receiver.SNAPSHOT_PATH = root / "current" / "events.json"
            ntis_receiver.initialise_database()
            with ntis_receiver.open_database() as db:
                db.execute(
                    """
                    INSERT INTO messages(
                        feed, received_at, path, metadata_path, content_type,
                        content_encoding, size_bytes, sha256, processed_at
                    ) VALUES('event', '2000-01-01T00:00:00+00:00', 'payload',
                             'meta', 'xml', '', 1, 'startup-hash', 'done')
                    """
                )
                db.execute(
                    "DELETE FROM processor_metadata WHERE key = 'last_event_received_at'"
                )
            ntis_receiver.write_json_document(ntis_receiver.SNAPSHOT_PATH, {
                "updatedAt": "2000-01-02T00:00:00+00:00",
                "alerts": [{"id": "legacy"}],
            })

            ntis_receiver.initialise_database()
            document = json.loads(
                ntis_receiver.SNAPSHOT_PATH.read_text(encoding="utf-8")
            )
            self.assertEqual(
                "2000-01-01T00:00:00+00:00", document["sourceUpdatedAt"]
            )
            self.assertNotIn("fullRefreshRequired", document)
            self.assertTrue(ntis_receiver.snapshot_is_stale(document))
            with ntis_receiver.open_database() as db:
                self.assertIsNone(db.execute(
                    "SELECT value FROM processor_metadata "
                    "WHERE key = 'full_refresh_required'"
                ).fetchone())
        finally:
            ntis_receiver.DATA_DIR = old_data_dir
            ntis_receiver.SNAPSHOT_PATH = old_snapshot_path

    def test_fused_congestion_freshness_is_clamped_and_persisted(self):
        old_data_dir = ntis_receiver.DATA_DIR
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        try:
            ntis_receiver.DATA_DIR = root
            ntis_receiver.initialise_database()
            processor = ntis_receiver.NtisEventProcessor(StaticResolver())

            self.assertEqual(120, processor.set_fused_congestion_max_age_seconds(30))
            self.assertEqual(120, processor.fused_congestion_max_age_seconds())
            processor = ntis_receiver.NtisEventProcessor(StaticResolver())
            self.assertEqual(120, processor.fused_congestion_max_age_seconds())

            self.assertEqual(600, processor.set_fused_congestion_max_age_seconds(900))
            processor = ntis_receiver.NtisEventProcessor(StaticResolver())
            self.assertEqual(600, processor.fused_congestion_max_age_seconds())
        finally:
            ntis_receiver.DATA_DIR = old_data_dir
            gc.collect()

    def test_legacy_cache_is_replayed_once_and_can_resume(self):
        old_data_dir = ntis_receiver.DATA_DIR
        old_snapshot_path = ntis_receiver.SNAPSHOT_PATH
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        try:
            ntis_receiver.DATA_DIR = root
            ntis_receiver.SNAPSHOT_PATH = root / "current" / "events.json"
            self.assertFalse(ntis_receiver.initialise_database())

            database_path = root / "messages.sqlite3"
            payload_path = root / "legacy.xml"
            payload_path.write_text("<legacy/>", encoding="utf-8")
            with sqlite3.connect(database_path) as db:
                db.execute(
                    "DELETE FROM processor_metadata "
                    "WHERE key = 'traffic_england_schema_version'"
                )
                db.execute(
                    """
                    INSERT INTO messages(
                        feed, received_at, path, metadata_path, content_type,
                        content_encoding, size_bytes, sha256, processed_at
                    ) VALUES('event', 'now', ?, 'meta', 'xml', '', 9, 'hash', 'done')
                    """,
                    (str(payload_path),),
                )
                db.execute(
                    "INSERT INTO event_state(" 
                    "record_id, version, version_time, situation_id, "
                    "refresh_generation, received_at, ephemeral_key, alert_json" 
                    ") VALUES('old', 1, 'now', 's', 1, 'now', '', '{}')"
                )

            ntis_receiver.SNAPSHOT_PATH.parent.mkdir(parents=True)
            ntis_receiver.SNAPSHOT_PATH.write_text("{}", encoding="utf-8")
            self.assertTrue(ntis_receiver.initialise_database())
            self.assertFalse(ntis_receiver.SNAPSHOT_PATH.exists())
            with sqlite3.connect(database_path) as db:
                self.assertEqual(0, db.execute(
                    "SELECT COUNT(*) FROM event_state"
                ).fetchone()[0])
                self.assertIsNone(db.execute(
                    "SELECT processed_at FROM messages WHERE feed = 'event'"
                ).fetchone()[0])
                self.assertEqual("1", db.execute(
                    "SELECT value FROM processor_metadata "
                    "WHERE key = 'traffic_england_rebuild_pending'"
                ).fetchone()[0])

            # A restart during the replay must retain its progress rather than
            # deleting state and resetting every message a second time.
            with sqlite3.connect(database_path) as db:
                db.execute("UPDATE messages SET processed_at = 'partial'")
            self.assertTrue(ntis_receiver.initialise_database())
            with sqlite3.connect(database_path) as db:
                self.assertEqual("partial", db.execute(
                    "SELECT processed_at FROM messages WHERE feed = 'event'"
                ).fetchone()[0])
        finally:
            ntis_receiver.DATA_DIR = old_data_dir
            ntis_receiver.SNAPSHOT_PATH = old_snapshot_path
            gc.collect()


if __name__ == "__main__":
    unittest.main()
