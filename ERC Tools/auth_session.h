// =================================================================================
// FILE: auth_session.h
// =================================================================================

#pragma once
#include "common.h"

inline constexpr const wchar_t* kClientVersion = L"1.3.0";
inline constexpr const wchar_t* kClientPlatform = L"win-x64";

struct ClientSession
{
    bool authenticated = false;
    std::wstring token;
    std::wstring displayName;
    std::wstring username;
    std::wstring position;
    std::wstring pod;
};

inline std::wstring BearerAuthHeader(const ClientSession& session)
{
    if (!session.authenticated || session.token.empty())
        return L"";
    return L"Authorization: Bearer " + session.token + L"\r\n";
}
