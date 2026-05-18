// =================================================================================
// FILE: client_update.h
// =================================================================================

#pragma once
#include "auth_session.h"

struct ClientUpdateFile
{
    std::wstring id;
    std::wstring url;
    std::wstring target;
    std::wstring applyMode;
    std::wstring sha256;
    std::filesystem::path stagedPath;
};

struct ClientUpdateResult
{
    bool ok = false;
    bool updateAvailable = false;
    bool restartRequired = false;
    bool hotApplied = false;
    std::wstring version;
    std::wstring error;
    std::filesystem::path stagingDir;
    std::vector<ClientUpdateFile> files;
};

bool CheckAndStageClientUpdate(const std::wstring& serverBaseUrl, const ClientSession& session, ClientUpdateResult& resultOut);
bool ApplyHotClientUpdate(const ClientUpdateResult& update, std::wstring& errorOut);
bool LaunchRestartClientUpdate(const ClientUpdateResult& update, std::wstring& errorOut);
