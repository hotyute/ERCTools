// =================================================================================
// FILE: binary_protocol.cpp
// =================================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>

#include "net/binary_protocol.h"
#include "core/util.h"
#include "../../../shared/payload_codec.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

namespace
{
constexpr uint16_t kProtocolVersion = 1;
constexpr uint32_t kMaxBinaryPayload = 32u * 1024u * 1024u;

constexpr uint16_t kOpLogin = 1;
constexpr uint16_t kOpLogout = 2;
constexpr uint16_t kOpPoll = 3;
constexpr uint16_t kOpSendChat = 4;
constexpr uint16_t kOpClearChat = 5;
constexpr uint16_t kOpCreateNote = 6;
constexpr uint16_t kOpUpdateNote = 7;
constexpr uint16_t kOpDeleteNote = 8;
constexpr uint16_t kOpGetSettings = 9;
constexpr uint16_t kOpSetSettings = 10;
constexpr uint16_t kOpCreateAccount = 11;
constexpr uint16_t kOpDeleteChatMessage = 12;
constexpr uint16_t kOpKickUser = 13;
constexpr uint16_t kOpMuteUser = 14;
constexpr uint16_t kOpSendPrivateMessage = 15;
constexpr uint16_t kOpAddIncidentExclusion = 16;
constexpr uint16_t kOpRemoveIncidentExclusion = 17;
constexpr uint16_t kOpFetchSourceBundle = 18;
constexpr uint16_t kOpWaitSourceBundle = 19;
constexpr uint16_t kOpUnifiedSync = 20;

static void WriteU16(std::vector<BYTE>& out, uint16_t value)
{
    out.push_back(static_cast<BYTE>(value & 0xFF));
    out.push_back(static_cast<BYTE>((value >> 8) & 0xFF));
}

static void WriteU32(std::vector<BYTE>& out, uint32_t value)
{
    out.push_back(static_cast<BYTE>(value & 0xFF));
    out.push_back(static_cast<BYTE>((value >> 8) & 0xFF));
    out.push_back(static_cast<BYTE>((value >> 16) & 0xFF));
    out.push_back(static_cast<BYTE>((value >> 24) & 0xFF));
}

static uint16_t ReadU16Raw(const BYTE* p)
{
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

static uint32_t ReadU32Raw(const BYTE* p)
{
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1]) << 8) |
        (static_cast<uint32_t>(p[2]) << 16) |
        (static_cast<uint32_t>(p[3]) << 24);
}

class BinaryWriter
{
public:
    void U16(uint16_t value) { WriteU16(m_data, value); }
    void U32(uint32_t value) { WriteU32(m_data, value); }

    void F64(double value)
    {
        uint64_t raw = 0;
        static_assert(sizeof(raw) == sizeof(value));
        std::memcpy(&raw, &value, sizeof(value));
        for (int i = 0; i < 8; ++i)
            m_data.push_back(static_cast<BYTE>((raw >> (i * 8)) & 0xFF));
    }

    void Text(const std::wstring& value)
    {
        std::string utf8 = WideToUtf8(value);
        if (utf8.size() > kMaxBinaryPayload)
            utf8.resize(kMaxBinaryPayload);
        U32(static_cast<uint32_t>(utf8.size()));
        m_data.insert(m_data.end(), utf8.begin(), utf8.end());
    }

    void JsonText(const json& value)
    {
        std::string text = value.dump();
        if (text.size() > kMaxBinaryPayload)
            text.resize(kMaxBinaryPayload);
        U32(static_cast<uint32_t>(text.size()));
        m_data.insert(m_data.end(), text.begin(), text.end());
    }

    void Bytes(const std::string& value)
    {
        const size_t length = std::min<size_t>(value.size(), kMaxBinaryPayload);
        U32(static_cast<uint32_t>(length));
        m_data.insert(m_data.end(), value.begin(), value.begin() + length);
    }

    const std::vector<BYTE>& Data() const { return m_data; }

private:
    std::vector<BYTE> m_data;
};

class BinaryReader
{
public:
    explicit BinaryReader(const std::vector<BYTE>& data) : m_data(data) {}

    bool U16(uint16_t& value)
    {
        if (Remaining() < 2)
            return false;
        value = ReadU16Raw(m_data.data() + m_pos);
        m_pos += 2;
        return true;
    }

    bool U32(uint32_t& value)
    {
        if (Remaining() < 4)
            return false;
        value = ReadU32Raw(m_data.data() + m_pos);
        m_pos += 4;
        return true;
    }

    bool F64(double& value)
    {
        if (Remaining() < 8)
            return false;
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i)
            raw |= (static_cast<uint64_t>(m_data[m_pos + i]) << (i * 8));
        std::memcpy(&value, &raw, sizeof(value));
        m_pos += 8;
        return true;
    }

    bool Text(std::wstring& value)
    {
        uint32_t len = 0;
        if (!U32(len) || len > kMaxBinaryPayload || Remaining() < len)
            return false;
        value = Utf8ToWide(std::string(reinterpret_cast<const char*>(m_data.data() + m_pos), len));
        m_pos += len;
        return true;
    }

    bool Json(json& value)
    {
        uint32_t len = 0;
        if (!U32(len) || len > kMaxBinaryPayload || Remaining() < len)
            return false;
        std::string text(reinterpret_cast<const char*>(m_data.data() + m_pos), len);
        m_pos += len;
        value = json::parse(text.empty() ? "{}" : text);
        return value.is_object();
    }

    bool Bytes(std::string& value)
    {
        uint32_t len = 0;
        if (!U32(len) || len > kMaxBinaryPayload || Remaining() < len)
            return false;
        value.assign(reinterpret_cast<const char*>(m_data.data() + m_pos), len);
        m_pos += len;
        return true;
    }

private:
    size_t Remaining() const { return m_pos <= m_data.size() ? m_data.size() - m_pos : 0; }

    const std::vector<BYTE>& m_data;
    size_t m_pos = 0;
};

struct ServerEndpoint
{
    std::wstring host;
    INTERNET_PORT port = 0;
};

static bool EnsureWinsock(std::wstring& errorOut)
{
    static std::once_flag once;
    static int result = SOCKET_ERROR;
    std::call_once(once, []() {
        WSADATA wsa{};
        result = WSAStartup(MAKEWORD(2, 2), &wsa);
        });

    if (result != 0) {
        errorOut = L"WSAStartup failed.";
        return false;
    }
    return true;
}

static bool ParseServerEndpoint(const std::wstring& serverBaseUrl, ServerEndpoint& endpointOut, std::wstring& errorOut)
{
    std::wstring url = NormalizeUrl(serverBaseUrl);
    if (url.empty()) {
        errorOut = L"No collaboration server configured.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[2048]{};
    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse collaboration server: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP) {
        errorOut = L"Binary collaboration transport requires http:// server access.";
        return false;
    }

    endpointOut.host.assign(host, parts.dwHostNameLength);
    endpointOut.port = parts.nPort ? parts.nPort : INTERNET_DEFAULT_HTTP_PORT;
    return !endpointOut.host.empty();
}

static bool SendAll(SOCKET s, const BYTE* data, size_t size)
{
    while (size > 0) {
        int chunk = static_cast<int>(std::min<size_t>(size, 64 * 1024));
        int sent = send(s, reinterpret_cast<const char*>(data), chunk, 0);
        if (sent <= 0)
            return false;
        data += sent;
        size -= static_cast<size_t>(sent);
    }
    return true;
}

static bool RecvExact(SOCKET s, BYTE* data, size_t size)
{
    while (size > 0) {
        int chunk = static_cast<int>(std::min<size_t>(size, 64 * 1024));
        int got = recv(s, reinterpret_cast<char*>(data), chunk, 0);
        if (got <= 0)
            return false;
        data += got;
        size -= static_cast<size_t>(got);
    }
    return true;
}

static std::vector<BYTE> BuildFrame(uint16_t opcode, const std::vector<BYTE>& payload)
{
    std::vector<BYTE> frame;
    frame.reserve(12 + payload.size());
    frame.push_back('E');
    frame.push_back('R');
    frame.push_back('C');
    frame.push_back('B');
    WriteU16(frame, kProtocolVersion);
    WriteU16(frame, opcode);
    WriteU32(frame, static_cast<uint32_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

static bool ExchangeFrame(
    const std::wstring& serverBaseUrl,
    uint16_t opcode,
    const std::vector<BYTE>& payload,
    bool& protocolAvailableOut,
    uint16_t& statusOut,
    std::vector<BYTE>& responsePayloadOut,
    std::wstring& errorOut)
{
    static std::mutex persistentSyncMutex;
    static SOCKET persistentSyncSocket = INVALID_SOCKET;
    static std::wstring persistentSyncServer;
    std::unique_lock<std::mutex> persistentLock(persistentSyncMutex, std::defer_lock);
    const bool persistent = opcode == kOpUnifiedSync;
    if (persistent)
        persistentLock.lock();

    protocolAvailableOut = false;
    statusOut = 0;
    responsePayloadOut.clear();
    errorOut.clear();

    if (payload.size() > kMaxBinaryPayload) {
        errorOut = L"Binary payload is too large.";
        return false;
    }

    if (!EnsureWinsock(errorOut))
        return false;

    ServerEndpoint endpoint;
    if (!ParseServerEndpoint(serverBaseUrl, endpoint, errorOut))
        return false;

    SOCKET sock = INVALID_SOCKET;
    if (persistent && persistentSyncSocket != INVALID_SOCKET && persistentSyncServer == serverBaseUrl) {
        sock = persistentSyncSocket;
    }
    else {
        if (persistentSyncSocket != INVALID_SOCKET && persistent) {
            shutdown(persistentSyncSocket, SD_BOTH);
            closesocket(persistentSyncSocket);
            persistentSyncSocket = INVALID_SOCKET;
        }

        addrinfoW hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        std::wstring port = std::to_wstring(endpoint.port);
        addrinfoW* results = nullptr;
        if (GetAddrInfoW(endpoint.host.c_str(), port.c_str(), &hints, &results) != 0 || !results) {
            errorOut = L"Could not resolve collaboration server.";
            return false;
        }

        for (addrinfoW* it = results; it; it = it->ai_next) {
            sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (sock == INVALID_SOCKET)
                continue;
            DWORD timeout = (opcode == kOpFetchSourceBundle || opcode == kOpWaitSourceBundle || opcode == kOpUnifiedSync)
                ? 90000
                : (opcode == kOpPoll ? 30000 : 2500);
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
            BOOL noDelay = TRUE;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
            if (connect(sock, it->ai_addr, static_cast<int>(it->ai_addrlen)) == 0)
                break;
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        FreeAddrInfoW(results);
        if (persistent && sock != INVALID_SOCKET) {
            persistentSyncSocket = sock;
            persistentSyncServer = serverBaseUrl;
        }
    }

    const auto closeTransport = [&]() {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        if (persistent) {
            persistentSyncSocket = INVALID_SOCKET;
            persistentSyncServer.clear();
        }
    };

    if (sock == INVALID_SOCKET) {
        errorOut = L"Could not connect to collaboration server binary stream.";
        return false;
    }

    std::vector<BYTE> frame = BuildFrame(opcode, payload);
    bool ok = SendAll(sock, frame.data(), frame.size());
    BYTE header[14]{};
    if (ok)
        ok = RecvExact(sock, header, sizeof(header));

    if (!ok) {
        closeTransport();
        errorOut = L"Binary collaboration transport did not receive a complete response.";
        return false;
    }

    if (std::memcmp(header, "ERCR", 4) != 0) {
        closeTransport();
        errorOut = L"Server does not support the binary collaboration protocol yet.";
        return false;
    }

    protocolAvailableOut = true;
    uint16_t version = ReadU16Raw(header + 4);
    uint16_t responseOpcode = ReadU16Raw(header + 6);
    statusOut = ReadU16Raw(header + 8);
    uint32_t payloadLen = ReadU32Raw(header + 10);
    if (version != kProtocolVersion || responseOpcode != opcode || payloadLen > kMaxBinaryPayload) {
        closeTransport();
        errorOut = L"Binary collaboration protocol response was invalid.";
        return false;
    }

    responsePayloadOut.resize(payloadLen);
    if (payloadLen > 0 && !RecvExact(sock, responsePayloadOut.data(), payloadLen)) {
        closeTransport();
        errorOut = L"Binary collaboration response ended early.";
        return false;
    }

    if (!persistent)
        closeTransport();
    return true;
}

static void DecodeStatusPayload(const std::vector<BYTE>& payload, BinaryCallResult& result)
{
    BinaryReader reader(payload);
    reader.Text(result.code);
    reader.Text(result.error);
}

static bool FinishCall(uint16_t opcode, const BinaryWriter& request, BinaryCallResult& resultOut, std::vector<BYTE>& payloadOut, const std::wstring& serverBaseUrl)
{
    payloadOut.clear();
    resultOut = {};
    uint16_t status = 0;
    if (!ExchangeFrame(serverBaseUrl, opcode, request.Data(), resultOut.protocolAvailable, status, payloadOut, resultOut.error))
        return false;

    resultOut.status = status;
    if (status != 0) {
        DecodeStatusPayload(payloadOut, resultOut);
        if (resultOut.error.empty())
            resultOut.error = L"Binary collaboration request failed.";
        return false;
    }

    resultOut.ok = true;
    return true;
}

static void WriteSessionToken(BinaryWriter& writer, const ClientSession& session)
{
    writer.Text(session.token);
}

static bool ReadNote(BinaryReader& reader, MapNote& note)
{
    return reader.Text(note.id) &&
        reader.Text(note.author) &&
        reader.Text(note.text) &&
        reader.Text(note.timestamp) &&
        reader.F64(note.latitude) &&
        reader.F64(note.longitude);
}

static bool ReadChat(BinaryReader& reader, ChatMessage& message)
{
    return reader.Text(message.id) &&
        reader.Text(message.author) &&
        reader.Text(message.username) &&
        reader.Text(message.position) &&
        reader.Text(message.text) &&
        reader.Text(message.timestamp);
}

static bool ReadOnlineUser(BinaryReader& reader, OnlineUser& user)
{
    return reader.Text(user.id) &&
        reader.Text(user.displayName) &&
        reader.Text(user.username) &&
        reader.Text(user.position) &&
        reader.Text(user.pod) &&
        reader.Text(user.lastSeen);
}

static bool ReadPrivateMessage(BinaryReader& reader, PrivateMessage& message)
{
    return reader.Text(message.id) &&
        reader.Text(message.senderUsername) &&
        reader.Text(message.senderDisplayName) &&
        reader.Text(message.senderPosition) &&
        reader.Text(message.recipientUsername) &&
        reader.Text(message.recipientDisplayName) &&
        reader.Text(message.recipientPosition) &&
        reader.Text(message.text) &&
        reader.Text(message.timestamp);
}

static bool ReadIncidentExclusion(BinaryReader& reader, IncidentExclusion& exclusion)
{
    return reader.Text(exclusion.key) &&
        reader.Text(exclusion.sourceId) &&
        reader.Text(exclusion.source) &&
        reader.Text(exclusion.road) &&
        reader.Text(exclusion.summary) &&
        reader.Text(exclusion.addedBy) &&
        reader.Text(exclusion.addedAt);
}
}

bool BinaryLogin(
    const std::wstring& serverBaseUrl,
    const std::wstring& username,
    const std::wstring& password,
    const std::wstring& position,
    const std::wstring& pod,
    BinaryLoginResult& resultOut)
{
    resultOut = {};
    BinaryWriter request;
    request.Text(username);
    request.Text(password);
    request.Text(position);
    request.Text(pod);
    request.Text(kClientVersion);
    request.Text(kClientPlatform);

    BinaryCallResult call;
    std::vector<BYTE> payload;
    if (!FinishCall(kOpLogin, request, call, payload, serverBaseUrl)) {
        resultOut.protocolAvailable = call.protocolAvailable;
        resultOut.status = call.status;
        resultOut.code = call.code;
        resultOut.error = call.error;
        return false;
    }

    BinaryReader reader(payload);
    ClientSession session;
    session.authenticated = true;
    if (!reader.Text(session.token) ||
        !reader.Text(session.username) ||
        !reader.Text(session.displayName) ||
        !reader.Text(session.position) ||
        !reader.Text(session.pod))
    {
        resultOut.protocolAvailable = true;
        resultOut.error = L"Binary login response was incomplete.";
        return false;
    }

    resultOut.ok = true;
    resultOut.protocolAvailable = true;
    resultOut.session = std::move(session);
    return true;
}

bool BinaryLogout(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& reason, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(reason);
    std::vector<BYTE> payload;
    return FinishCall(kOpLogout, request, resultOut, payload, serverBaseUrl);
}

bool BinaryPollCollaboration(const std::wstring& serverBaseUrl, const ClientSession& session, uint32_t knownVersion, BinaryPollResult& resultOut)
{
    static std::atomic<uint32_t> lastPollPingMs{ 0 };
    resultOut = {};
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.U32(knownVersion);
    request.U32(lastPollPingMs.load(std::memory_order_acquire));

    BinaryCallResult call;
    std::vector<BYTE> payload;
    const auto pollStarted = std::chrono::steady_clock::now();
    if (!FinishCall(kOpPoll, request, call, payload, serverBaseUrl)) {
        static_cast<BinaryCallResult&>(resultOut) = call;
        return false;
    }

    BinaryReader reader(payload);
    uint32_t count = 0;
    if (!reader.U32(count) || count > 10000) {
        resultOut.error = L"Binary chat payload was invalid.";
        resultOut.protocolAvailable = true;
        return false;
    }
    resultOut.chat.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        ChatMessage message;
        if (!ReadChat(reader, message)) {
            resultOut.error = L"Binary chat record was incomplete.";
            resultOut.protocolAvailable = true;
            return false;
        }
        resultOut.chat.push_back(std::move(message));
    }

    if (!reader.U32(count) || count > 10000) {
        resultOut.error = L"Binary notes payload was invalid.";
        resultOut.protocolAvailable = true;
        return false;
    }
    resultOut.notes.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        MapNote note;
        if (!ReadNote(reader, note)) {
            resultOut.error = L"Binary note record was incomplete.";
            resultOut.protocolAvailable = true;
            return false;
        }
        resultOut.notes.push_back(std::move(note));
    }

    if (!reader.U32(count) || count > 10000) {
        resultOut.error = L"Binary users payload was invalid.";
        resultOut.protocolAvailable = true;
        return false;
    }
    resultOut.users.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        OnlineUser user;
        if (!ReadOnlineUser(reader, user)) {
            resultOut.error = L"Binary user record was incomplete.";
            resultOut.protocolAvailable = true;
            return false;
        }
        resultOut.users.push_back(std::move(user));
    }

    if (!reader.U32(count) || count > 10000) {
        resultOut.error = L"Binary private-message payload was invalid.";
        resultOut.protocolAvailable = true;
        return false;
    }
    resultOut.privateMessages.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        PrivateMessage message;
        if (!ReadPrivateMessage(reader, message)) {
            resultOut.error = L"Binary private-message record was incomplete.";
            resultOut.protocolAvailable = true;
            return false;
        }
        resultOut.privateMessages.push_back(std::move(message));
    }

    uint32_t version = 0;
    if (reader.U32(version))
        resultOut.version = version;

    // Exclusions were appended after the original poll payload so new clients can
    // still talk to an older server during a rolling update.
    if (reader.U32(count)) {
        if (count > 10000) {
            resultOut.error = L"Binary incident-exclusion payload was invalid.";
            resultOut.protocolAvailable = true;
            return false;
        }
        resultOut.incidentExclusions.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            IncidentExclusion exclusion;
            if (!ReadIncidentExclusion(reader, exclusion)) {
                resultOut.error = L"Binary incident-exclusion record was incomplete.";
                resultOut.protocolAvailable = true;
                return false;
            }
            resultOut.incidentExclusions.push_back(std::move(exclusion));
        }
    }

    uint32_t serverWaitMs = 0;
    if (reader.U32(serverWaitMs)) {
        const uint64_t elapsedMs = static_cast<uint64_t>(std::max<int64_t>(1,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pollStarted).count()));
        const uint64_t measuredMs = elapsedMs > serverWaitMs ? elapsedMs - serverWaitMs : 1;
        lastPollPingMs.store(
            static_cast<uint32_t>(std::min<uint64_t>(measuredMs, 60000u)),
            std::memory_order_release);
    }

    static_cast<BinaryCallResult&>(resultOut) = call;
    return true;
}

bool BinarySendChat(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& text, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(text);
    std::vector<BYTE> payload;
    return FinishCall(kOpSendChat, request, resultOut, payload, serverBaseUrl);
}

bool BinaryClearChat(const std::wstring& serverBaseUrl, const ClientSession& session, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    std::vector<BYTE> payload;
    return FinishCall(kOpClearChat, request, resultOut, payload, serverBaseUrl);
}

bool BinaryDeleteChatMessage(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& messageId, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(messageId);
    std::vector<BYTE> payload;
    return FinishCall(kOpDeleteChatMessage, request, resultOut, payload, serverBaseUrl);
}

bool BinaryKickUser(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& username, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(username);
    std::vector<BYTE> payload;
    return FinishCall(kOpKickUser, request, resultOut, payload, serverBaseUrl);
}

bool BinaryMuteUser(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& username, uint32_t minutes, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(username);
    request.U32(minutes);
    std::vector<BYTE> payload;
    return FinishCall(kOpMuteUser, request, resultOut, payload, serverBaseUrl);
}

bool BinarySendPrivateMessage(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& recipientUsername, const std::wstring& text, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(recipientUsername);
    request.Text(text);
    std::vector<BYTE> payload;
    return FinishCall(kOpSendPrivateMessage, request, resultOut, payload, serverBaseUrl);
}

bool BinaryAddIncidentExclusion(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const IncidentExclusion& exclusion,
    BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(exclusion.key);
    request.Text(exclusion.sourceId);
    request.Text(exclusion.source);
    request.Text(exclusion.road);
    request.Text(exclusion.summary);
    std::vector<BYTE> payload;
    return FinishCall(kOpAddIncidentExclusion, request, resultOut, payload, serverBaseUrl);
}

bool BinaryRemoveIncidentExclusion(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& exclusionKey,
    BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(exclusionKey);
    std::vector<BYTE> payload;
    return FinishCall(kOpRemoveIncidentExclusion, request, resultOut, payload, serverBaseUrl);
}

bool BinaryCreateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, MapNote& serverNoteOut, BinaryCallResult& resultOut)
{
    serverNoteOut = {};
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(note.text);
    request.F64(note.latitude);
    request.F64(note.longitude);

    std::vector<BYTE> payload;
    if (!FinishCall(kOpCreateNote, request, resultOut, payload, serverBaseUrl))
        return false;

    BinaryReader reader(payload);
    if (!ReadNote(reader, serverNoteOut)) {
        resultOut.ok = false;
        resultOut.protocolAvailable = true;
        resultOut.error = L"Binary create-note response was incomplete.";
        return false;
    }
    return true;
}

bool BinaryUpdateNote(const std::wstring& serverBaseUrl, const ClientSession& session, const MapNote& note, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(note.id);
    request.Text(note.text);
    request.F64(note.latitude);
    request.F64(note.longitude);
    std::vector<BYTE> payload;
    return FinishCall(kOpUpdateNote, request, resultOut, payload, serverBaseUrl);
}

bool BinaryDeleteNote(const std::wstring& serverBaseUrl, const ClientSession& session, const std::wstring& noteId, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(noteId);
    std::vector<BYTE> payload;
    return FinishCall(kOpDeleteNote, request, resultOut, payload, serverBaseUrl);
}

bool BinaryGetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, json& settingsOut, BinaryCallResult& resultOut)
{
    settingsOut = json::object();
    BinaryWriter request;
    WriteSessionToken(request, session);

    std::vector<BYTE> payload;
    if (!FinishCall(kOpGetSettings, request, resultOut, payload, serverBaseUrl))
        return false;

    try {
        BinaryReader reader(payload);
        if (!reader.Json(settingsOut))
            throw std::runtime_error("settings payload is not an object");
        return true;
    }
    catch (const std::exception& e) {
        resultOut.ok = false;
        resultOut.protocolAvailable = true;
        resultOut.error = L"Binary settings response parse failed: " + Utf8ToWide(e.what());
        return false;
    }
}

bool BinarySetGlobalSettings(const std::wstring& serverBaseUrl, const ClientSession& session, const json& settings, BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.JsonText(settings);
    std::vector<BYTE> payload;
    return FinishCall(kOpSetSettings, request, resultOut, payload, serverBaseUrl);
}

static bool ReadSourceBlobs(BinaryReader& reader, uint32_t blobCount, BinarySourceBundleResult& resultOut)
{
    resultOut.blobs.reserve(blobCount);
    for (uint32_t i = 0; i < blobCount; ++i) {
        BinarySourceBlob blob;
        uint32_t ok = 0;
        if (!reader.Text(blob.name) ||
            !reader.Text(blob.url) ||
            !reader.U32(ok) ||
            !reader.Bytes(blob.body) ||
            !reader.Text(blob.error))
        {
            resultOut.ok = false;
            resultOut.error = L"Binary source bundle response contained an incomplete source body.";
            return false;
        }
        blob.ok = ok != 0;
        resultOut.blobs.push_back(std::move(blob));
    }
    return true;
}

static bool ReadCompressedSourceBlobs(
    BinaryReader& reader,
    uint32_t blobCount,
    BinarySourceBundleResult& resultOut);

struct CachedSourcePayload
{
    uint32_t generation = 0;
    std::unordered_map<std::wstring, std::string> bodies;
};

static std::mutex g_sourcePayloadCacheMutex;
static std::unordered_map<std::wstring, CachedSourcePayload> g_sourcePayloadCache;

static std::wstring SourcePayloadCacheKey(const std::wstring& serverBaseUrl, const std::wstring& sourceType)
{
    return serverBaseUrl + L"|" + sourceType;
}

static void CacheFullSourcePayload(
    const std::wstring& serverBaseUrl,
    const std::wstring& sourceType,
    const BinarySourceBundleResult& source)
{
    CachedSourcePayload cached;
    cached.generation = source.generation;
    for (const auto& blob : source.blobs) {
        if (blob.ok)
            cached.bodies[blob.name] = blob.body;
    }
    std::lock_guard<std::mutex> lock(g_sourcePayloadCacheMutex);
    g_sourcePayloadCache[SourcePayloadCacheKey(serverBaseUrl, sourceType)] = std::move(cached);
}

static uint32_t ValidatedKnownGeneration(
    const std::wstring& serverBaseUrl,
    const std::wstring& sourceType,
    uint32_t requestedGeneration)
{
    if (requestedGeneration == 0)
        return 0;
    std::lock_guard<std::mutex> lock(g_sourcePayloadCacheMutex);
    const auto found = g_sourcePayloadCache.find(SourcePayloadCacheKey(serverBaseUrl, sourceType));
    return found != g_sourcePayloadCache.end() && found->second.generation == requestedGeneration
        ? requestedGeneration
        : 0;
}

static bool MaterializeSourceDelta(
    const std::wstring& serverBaseUrl,
    const std::wstring& sourceType,
    BinarySourceBundleResult& source,
    std::wstring& errorOut)
{
    if (!source.delta) {
        CacheFullSourcePayload(serverBaseUrl, sourceType, source);
        return true;
    }

    std::lock_guard<std::mutex> lock(g_sourcePayloadCacheMutex);
    const auto cacheIt = g_sourcePayloadCache.find(SourcePayloadCacheKey(serverBaseUrl, sourceType));
    if (cacheIt == g_sourcePayloadCache.end() || cacheIt->second.generation != source.baseGeneration) {
        g_sourcePayloadCache.erase(SourcePayloadCacheKey(serverBaseUrl, sourceType));
        errorOut = L"Source delta did not match the local generation; a full snapshot is required.";
        return false;
    }

    try {
        for (BinarySourceBlob& blob : source.blobs) {
            const auto bodyIt = cacheIt->second.bodies.find(blob.name);
            if (!blob.ok || bodyIt == cacheIt->second.bodies.end()) {
                errorOut = L"Source delta did not have a matching local payload.";
                return false;
            }

            const json delta = json::parse(blob.body);
            const json oldRoot = json::parse(bodyIt->second);
            const std::string collection = delta.value("collection", "");
            if (collection.empty() || !oldRoot.contains(collection) || !oldRoot[collection].is_array()) {
                errorOut = L"Source delta collection was invalid.";
                return false;
            }

            std::unordered_map<std::string, json> items;
            for (const auto& item : oldRoot[collection]) {
                if (item.is_object() && item.contains("id") && item["id"].is_string())
                    items[item["id"].get<std::string>()] = item;
            }
            for (const auto& id : delta.value("removed", json::array())) {
                if (id.is_string())
                    items.erase(id.get<std::string>());
            }
            for (const auto& item : delta.value("upserts", json::array())) {
                if (item.is_object() && item.contains("id") && item["id"].is_string())
                    items[item["id"].get<std::string>()] = item;
            }

            json rebuilt = delta.value("root", json::object());
            rebuilt[collection] = json::array();
            for (const auto& id : delta.value("order", json::array())) {
                if (!id.is_string())
                    continue;
                const auto item = items.find(id.get<std::string>());
                if (item != items.end())
                    rebuilt[collection].push_back(item->second);
            }

            blob.body = rebuilt.dump();
            bodyIt->second = blob.body;
        }
        cacheIt->second.generation = source.generation;
        source.delta = false;
        return true;
    }
    catch (const std::exception& e) {
        errorOut = L"Source delta could not be applied: " + Utf8ToWide(e.what());
        return false;
    }
}

bool BinaryFetchSourceBundle(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& sourceType,
    uint32_t requestedIntervalMs,
    const json& options,
    BinarySourceBundleResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(sourceType);
    request.U32(requestedIntervalMs);
    request.JsonText(options);

    BinaryCallResult call;
    std::vector<BYTE> payload;
    if (!FinishCall(kOpFetchSourceBundle, request, call, payload, serverBaseUrl)) {
        resultOut = {};
        resultOut.ok = call.ok;
        resultOut.protocolAvailable = call.protocolAvailable;
        resultOut.status = call.status;
        resultOut.code = call.code;
        resultOut.error = call.error;
        return false;
    }

    resultOut = {};
    resultOut.ok = call.ok;
    resultOut.protocolAvailable = call.protocolAvailable;
    resultOut.status = call.status;
    resultOut.code = call.code;
    resultOut.error = call.error;

    BinaryReader reader(payload);
    uint32_t fromCache = 0;
    uint32_t blobCount = 0;
    if (!reader.U32(resultOut.ageMs) || !reader.U32(fromCache) || !reader.U32(blobCount)) {
        resultOut.ok = false;
        resultOut.error = L"Binary source bundle response was incomplete.";
        return false;
    }
    resultOut.fromCache = fromCache != 0;
    if (!ReadSourceBlobs(reader, blobCount, resultOut))
        return false;
    reader.U32(resultOut.generation);
    CacheFullSourcePayload(serverBaseUrl, sourceType, resultOut);

    return true;
}

bool BinaryWaitSourceBundle(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& sourceType,
    uint32_t knownGeneration,
    uint32_t waitTimeoutMs,
    const json& options,
    BinarySourceBundleResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(sourceType);
    request.U32(knownGeneration);
    request.U32(waitTimeoutMs);
    request.JsonText(options);

    BinaryCallResult call;
    std::vector<BYTE> payload;
    if (!FinishCall(kOpWaitSourceBundle, request, call, payload, serverBaseUrl)) {
        resultOut = {};
        resultOut.ok = call.ok;
        resultOut.protocolAvailable = call.protocolAvailable;
        resultOut.status = call.status;
        resultOut.code = call.code;
        resultOut.error = call.error;
        return false;
    }

    resultOut = {};
    resultOut.ok = call.ok;
    resultOut.protocolAvailable = call.protocolAvailable;
    resultOut.status = call.status;
    resultOut.code = call.code;
    resultOut.error = call.error;

    BinaryReader reader(payload);
    uint32_t changed = 0;
    uint32_t fromCache = 0;
    uint32_t blobCount = 0;
    if (!reader.U32(resultOut.generation) ||
        !reader.U32(changed) ||
        !reader.U32(resultOut.ageMs) ||
        !reader.U32(fromCache) ||
        !reader.U32(blobCount))
    {
        resultOut.ok = false;
        resultOut.error = L"Binary source wait response was incomplete.";
        return false;
    }

    resultOut.changed = changed != 0;
    resultOut.fromCache = fromCache != 0;
    if (!ReadSourceBlobs(reader, blobCount, resultOut))
        return false;
    return true;
}

static bool ReadCompressedSourceBlobs(
    BinaryReader& reader,
    uint32_t blobCount,
    BinarySourceBundleResult& resultOut)
{
    resultOut.blobs.reserve(blobCount);
    for (uint32_t i = 0; i < blobCount; ++i) {
        BinarySourceBlob blob;
        uint32_t ok = 0;
        uint32_t method = 0;
        uint32_t originalSize = 0;
        std::string encodedText;
        if (!reader.Text(blob.name) || !reader.Text(blob.url) || !reader.U32(ok) ||
            !reader.U32(method) || !reader.U32(originalSize) ||
            !reader.Bytes(encodedText) || !reader.Text(blob.error))
        {
            resultOut.error = L"Compressed source update was incomplete.";
            return false;
        }

        std::vector<unsigned char> encoded(encodedText.begin(), encodedText.end());
        if (!erc::payload::Decode(method, originalSize, encoded, blob.body)) {
            resultOut.error = L"Compressed source update could not be decoded.";
            return false;
        }
        blob.ok = ok != 0;
        resultOut.blobs.push_back(std::move(blob));
    }
    return true;
}

bool BinaryUnifiedSync(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    uint32_t knownCollaborationVersion,
    const std::vector<BinarySourceSubscription>& subscriptions,
    BinaryUnifiedSyncResult& resultOut)
{
    static std::atomic<uint32_t> lastPingMs{ 0 };
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.U32(knownCollaborationVersion);
    request.U32(lastPingMs.load(std::memory_order_acquire));
    request.U32(static_cast<uint32_t>(subscriptions.size()));
    for (const auto& subscription : subscriptions) {
        request.Text(subscription.sourceType);
        request.U32(ValidatedKnownGeneration(serverBaseUrl, subscription.sourceType, subscription.knownGeneration));
        request.JsonText(subscription.options);
    }

    BinaryCallResult call;
    std::vector<BYTE> payload;
    const auto started = std::chrono::steady_clock::now();
    if (!FinishCall(kOpUnifiedSync, request, call, payload, serverBaseUrl)) {
        resultOut = {};
        static_cast<BinaryCallResult&>(resultOut) = call;
        return false;
    }

    resultOut = {};
    static_cast<BinaryCallResult&>(resultOut) = call;
    BinaryReader reader(payload);
    uint32_t collaborationChanged = 0;
    if (!reader.U32(resultOut.collaborationVersion) || !reader.U32(collaborationChanged)) {
        resultOut.ok = false;
        resultOut.error = L"Unified synchronization response was incomplete.";
        return false;
    }
    resultOut.collaborationChanged = collaborationChanged != 0;

    if (resultOut.collaborationChanged) {
        uint32_t count = 0;
        if (!reader.U32(count))
            return false;
        resultOut.chat.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            ChatMessage item;
            if (!ReadChat(reader, item))
                return false;
            resultOut.chat.push_back(std::move(item));
        }
        if (!reader.U32(count))
            return false;
        resultOut.notes.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            MapNote item;
            if (!ReadNote(reader, item))
                return false;
            resultOut.notes.push_back(std::move(item));
        }
        if (!reader.U32(count))
            return false;
        resultOut.users.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            OnlineUser item;
            if (!ReadOnlineUser(reader, item))
                return false;
            resultOut.users.push_back(std::move(item));
        }
        if (!reader.U32(count))
            return false;
        resultOut.privateMessages.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            PrivateMessage item;
            if (!ReadPrivateMessage(reader, item))
                return false;
            resultOut.privateMessages.push_back(std::move(item));
        }
        if (!reader.U32(count))
            return false;
        resultOut.incidentExclusions.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            IncidentExclusion item;
            if (!ReadIncidentExclusion(reader, item))
                return false;
            resultOut.incidentExclusions.push_back(std::move(item));
        }
    }

    uint32_t sourceCount = 0;
    if (!reader.U32(sourceCount) || sourceCount > 16) {
        resultOut.ok = false;
        resultOut.error = L"Unified synchronization source list was invalid.";
        return false;
    }
    resultOut.sources.reserve(sourceCount);
    for (uint32_t i = 0; i < sourceCount; ++i) {
        std::wstring sourceType;
        BinarySourceBundleResult source;
        uint32_t blobCount = 0;
        uint32_t delta = 0;
        if (!reader.Text(sourceType) || !reader.U32(source.generation) ||
            !reader.U32(source.ageMs) || !reader.U32(delta) ||
            !reader.U32(source.baseGeneration) || !reader.U32(blobCount) ||
            !ReadCompressedSourceBlobs(reader, blobCount, source))
        {
            resultOut.ok = false;
            if (resultOut.error.empty())
                resultOut.error = source.error.empty() ? L"Unified source update was incomplete." : source.error;
            return false;
        }
        source.ok = true;
        source.protocolAvailable = true;
        source.changed = true;
        source.fromCache = true;
        source.delta = delta != 0;
        if (!MaterializeSourceDelta(serverBaseUrl, sourceType, source, resultOut.error)) {
            resultOut.ok = false;
            return false;
        }
        resultOut.sources.emplace_back(std::move(sourceType), std::move(source));
    }
    if (!reader.U32(resultOut.waitMs)) {
        resultOut.ok = false;
        resultOut.error = L"Unified synchronization timing data was missing.";
        return false;
    }

    const uint32_t elapsedMs = static_cast<uint32_t>(std::min<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count(),
        60000));
    if (resultOut.waitMs < elapsedMs)
        lastPingMs.store(std::max<uint32_t>(1u, elapsedMs - resultOut.waitMs), std::memory_order_release);
    return true;
}

bool BinaryCreateAccount(
    const std::wstring& serverBaseUrl,
    const ClientSession& session,
    const std::wstring& username,
    const std::wstring& displayName,
    const std::wstring& password,
    const std::wstring& position,
    bool active,
    BinaryCallResult& resultOut)
{
    BinaryWriter request;
    WriteSessionToken(request, session);
    request.Text(username);
    request.Text(displayName);
    request.Text(password);
    request.Text(position);
    request.U32(active ? 1u : 0u);
    std::vector<BYTE> payload;
    return FinishCall(kOpCreateAccount, request, resultOut, payload, serverBaseUrl);
}
