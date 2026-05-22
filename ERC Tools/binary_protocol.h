// =================================================================================
// FILE: binary_protocol.h
// Compact binary collaboration transport for ERC Tools.
// =================================================================================

#pragma once

#include "auth_session.h"
#include "models.h"

struct BinaryLoginResult
{
    bool ok = false;
    bool protocolAvailable = false;
    DWORD status = 0;
    std::wstring code;
    std::wstring error;
    ClientSession session;
};

struct BinaryCallResult
{
    bool ok = false;
    bool protocolAvailable = false;
    DWORD status = 0;
    std::wstring code;
    std::wstring error;
};

struct BinaryPollResult : BinaryCallResult
{
    std::vector<ChatMessage> chat;
    std::vector<MapNote> notes;
    std::vector<OnlineUser> users;
};

bool BinaryLogin(
    const std::wstring& serverBaseUrl,
    const std::wstring& username,
    const std::wstring& password,
    const std::wstring& position,
    const std::wstring& pod,
    BinaryLoginResult& resultOut);

bool BinaryLogout(const std::wstring& serverBaseUrl, const ClientSession& session, BinaryCallResult& resultOut);
bool BinaryPollCollaboration(const std::wstring& serverBaseUrl, const ClientSession& session, BinaryPollResult& resultOut);
bool BinarySendChat(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& text, BinaryCallResult& resultOut);
bool BinaryClearChat(const std::wstring& serverBaseUrl, const ClientSession& session, BinaryCallResult& resultOut);
bool BinaryCreateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, MapNote& serverNoteOut, BinaryCallResult& resultOut);
bool BinaryUpdateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, BinaryCallResult& resultOut);
bool BinaryDeleteNote(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& noteId, BinaryCallResult& resultOut);
bool BinaryGetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, json& settingsOut, BinaryCallResult& resultOut);
bool BinarySetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, const json& settings, BinaryCallResult& resultOut);
