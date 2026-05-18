# ERC Tools Server

Multithreaded HTTP server for ERC Tools authentication, collaboration data, and dynamic client updates.

## Setup

1. Install the MySQL ODBC 8 Unicode driver on the server.
2. Run `schema.sql` against MySQL.
3. Copy `server_config.example.json` to `server_config.json` and set the MySQL connection string.
4. Generate password material:

```powershell
.\ERC Tools Server.exe --hash-password "temporary-password"
```

5. Insert a user with the generated `password_salt`, `password_hash`, and `password_iterations`.
6. Copy `updates\manifest.example.json` to `updates\manifest.json` when you are ready to publish updates.

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
