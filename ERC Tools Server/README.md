# ERC Tools Server

Multithreaded HTTP server for ERC Tools authentication, collaboration data, and dynamic client updates.

## Setup

1. Install the MySQL ODBC 8 Unicode driver on the server.
2. Run `schema.sql` against MySQL.
3. Copy `server_config.example.json` to `server_config.json`, set the MySQL connection string, and configure the traffic data source.
4. Start the server. The default port is `8081`.
5. Check the ODBC driver name and database connection:

```powershell
.\ERC Tools Account Creator.exe --list-odbc-drivers
.\ERC Tools Account Creator.exe --config server_config.json --test-connection
```

The `DRIVER={...}` value in `server_config.json` must exactly match one of the installed 64-bit ODBC driver names shown by `--list-odbc-drivers`. If the list does not include a MySQL Unicode driver, install the 64-bit MySQL ODBC connector first. phpMyAdmin can work even when the Windows ODBC driver is missing.

6. Create user accounts with the Account Creator:

```powershell
.\ERC Tools Account Creator.exe server_config.json
```

Or provide the fields directly:

```powershell
.\ERC Tools Account Creator.exe --config server_config.json --username sam --display-name "Sam" --password "temporary-password" --position Administrator --pod "Pod 1"
```

Omit `--password` to enter it through the hidden console prompt instead of putting it in command history. Use a dedicated MySQL user for ERC Tools rather than the MySQL root account once the connection is working.

When running under Wine on Linux, ODBC can require extra Wine-side driver setup. To create the first account through phpMyAdmin or the MySQL CLI without ODBC, print a ready-to-run SQL statement:

```powershell
.\ERC Tools Account Creator.exe --print-sql --username sam --display-name "Sam" --position Administrator --pod "Pod 1"
```

Run the printed SQL against the `erc_tools` database.

This only bypasses ODBC for account creation. `ERC Tools Server.exe` still needs a working ODBC driver inside the Wine prefix for login, chat, notes, and update authentication.

7. Copy `updates\manifest.example.json` to `updates\manifest.json` when you are ready to publish updates.

## National Highways NTIS Event Data

The preferred online road source is the National Highways NTIS Event Data push feed. The hardened receiver in `deploy/ntis_receiver.py` stores every DATEX II publication, merges overlapping full-refresh partitions by stable record id, applies incremental lifecycle updates, and exposes a compact current snapshot on localhost. The ERC Tools Server polls that local snapshot and advances the client-facing source generation only when its contents change.

The receiver recreates the former public Traffic England Alerts contract. **Unplanned Only** contains current, confirmed, non-completed `INCIDENT` and `CONGESTION` records with a general-public presentation and a resolved Strategic Road Network identity. **All Events** additionally includes the other Traffic England categories and preserves the former public-page exception for unconfirmed roadworks. Raw NTIS records remain in the internal snapshot with explicit eligibility flags, but do not leak into the default map, side panel, or notification checks.

Sensor-derived `Fused Traffic Data` congestion is a rolling publication: NTIS assigns a new event id to replacement samples instead of ending the previous id. The receiver therefore uses the DATEX II network-location references as the stable identity, retains only the newest sample for each location, and expires samples after 10 minutes by default. The server settings panel can adjust this evidence-based compatibility window from 2 to 10 minutes while running; the receiver persists the selected value. `NTIS_FUSED_CONGESTION_MAX_AGE_SECONDS` supplies the initial default for a fresh receiver database.

Events whose public text does not contain a road number are tagged as source-unresolved. The receiver resolves their coordinates asynchronously against the National Highways Network Model Link layer and caches the result for seven days. The same lookup refines ambiguous route names, including upgrading `A1` to `A1(M)` where the Network Model identifies the motorway section. A genuine Network Model push publication invalidates the cached matches. The client hides source-unresolved incidents by default; users can include successfully resolved records from **Roads > Incident Filters > Show unresolved incidents**.

```json
{
  "ntisEventSnapshotUrl": "http://127.0.0.1:18080/internal/events",
  "ntisPollIntervalSeconds": 2
}
```

NTIS is the sole bundled source for National Highways incidents. If the receiver is unavailable, the server reports that failure instead of switching providers. Traffic Scotland remains a separate additive source.

## Accounts and 401 Responses

The client login calls `POST /api/auth/login`. An HTTP `401 Unauthorized` means the server rejected the login or bearer session. Common causes are:

- No row exists in the `users` table for that username.
- The password is wrong.
- The selected Position is higher than the account's allowed position.
- The selected Pod already has an active online session.
- The account is disabled.
- For non-login endpoints, the bearer token is missing, invalid, or expired.

Use `ERC Tools Account Creator.exe` on the server side to create or update rows in `users`. It uses the same PBKDF2 password hashing as `ERC Tools Server.exe`.

## Update Protocol

The client checks:

```text
GET /api/updates/manifest?version=<client-version>&platform=win-x64&position=<role>&pod=<pod>
Authorization: Bearer <token>
```

The server reads `manifest.json` on every request, so changing the manifest or files is live without restarting the server. Each file supports:

- `applyMode: "hot-reload"`: downloaded and applied while ERC Tools keeps running. Targets are relative to the ERC Tools local app-data cache.
- `applyMode: "restart"`: downloaded to a staging folder. The client prompts, exits, copies staged files into the install directory, then restarts.

Set `restartRequired` to `true` when any executable or DLL needs replacement.
