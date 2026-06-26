// =================================================================================
// FILE: binary_protocol.h
// Compact binary collaboration transport for ERC Tools.
// =================================================================================

#pragma once

#include "net/auth_session.h"
#include "core/models.h"

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
    std::vector<PrivateMessage> privateMessages;
    std::vector<IncidentExclusion> incidentExclusions;
    uint32_t version = 0;
};

struct BinarySourceBlob
{
    std::wstring name;
    std::wstring url;
    bool ok = false;
    std::string body;
    std::wstring error;
};

struct BinarySourceBundleResult : BinaryCallResult
{
    bool fromCache = false;
    bool changed = true;
    uint32_t ageMs = 0;
    uint32_t generation = 0;
    std::vector<BinarySourceBlob> blobs;
};

bool BinaryLogin(
    const std::wstring& serverBaseUrl,
    const std::wstring& username,
    const std::wstring& password,
    const std::wstring& position,
    const std::wstring& pod,
    BinaryLoginResult& resultOut);

bool BinaryLogout(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& reason, BinaryCallResult& resultOut);
bool BinaryPollCollaboration(const std::wstring& serverBaseUrl, const ClientSession& session, uint32_t knownVersion, BinaryPollResult& resultOut);
bool BinarySendChat(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& text, BinaryCallResult& resultOut);
bool BinaryClearChat(const std::wstring& serverBaseUrl, const ClientSession& session, BinaryCallResult& resultOut);
bool BinaryDeleteChatMessage(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& messageId, BinaryCallResult& resultOut);
bool BinaryKickUser(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& username, BinaryCallResult& resultOut);
bool BinaryMuteUser(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& username, uint32_t minutes, BinaryCallResult& resultOut);
bool BinarySendPrivateMessage(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& recipientUsername, const std::wstring& text, BinaryCallResult& resultOut);
bool BinaryAddIncidentExclusion(const std::wstring& serverBaseUrl, const ClientSession& session, const IncidentExclusion& exclusion, BinaryCallResult& resultOut);
bool BinaryRemoveIncidentExclusion(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& exclusionKey, BinaryCallResult& resultOut);
bool BinaryCreateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, MapNote& serverNoteOut, BinaryCallResult& resultOut);
bool BinaryUpdateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, BinaryCallResult& resultOut);
bool BinaryDeleteNote(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& noteId, BinaryCallResult& resultOut);
bool BinaryGetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, json& settingsOut, BinaryCallResult& resultOut);
bool BinarySetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, const json& settings, BinaryCallResult& resultOut);
bool BinaryFetchSourceBundle(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& sourceType,
    uint32_t requestedIntervalMs,
    const json& options,
    BinarySourceBundleResult& resultOut);
bool BinaryWaitSourceBundle(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& sourceType,
    uint32_t knownGeneration,
    uint32_t waitTimeoutMs,
    const json& options,
    BinarySourceBundleResult& resultOut);
bool BinaryCreateAccount(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& username,
    const std::wstring& displayName,
    const std::wstring& password,
    const std::wstring& position,
    bool active,
    BinaryCallResult& resultOut);
