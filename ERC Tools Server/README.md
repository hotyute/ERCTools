# ERC Tools Server

Multithreaded HTTP server for ERC Tools authentication, collaboration data, and dynamic client updates.

## Setup

1. Install the MySQL ODBC 8 Unicode driver on the server.
2. Run `schema.sql` against MySQL.
3. Copy `server_config.example.json` to `server_config.json` and set the MySQL connection string.
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
