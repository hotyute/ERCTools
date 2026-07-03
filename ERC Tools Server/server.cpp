// =================================================================================
// FILE: server.cpp
// Multithreaded ERC Tools collaboration/auth/update server.
// =================================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sqlext.h>
#include <bcrypt.h>
#include <winhttp.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cwctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "user32.lib")

using json = nlohmann::json;

namespace
{
struct ServerConfig
{
    int port = 8081;
    int workerThreads = 0;
    int sessionMinutes = 12 * 60;
    int onlineTimeoutSeconds = 30;
    int sourceFetchIntervalSeconds = 60;
    int ntisPollIntervalSeconds = 2;
    bool serveStaleSourceCache = true;
    std::wstring databaseConnectionString =
        L"DRIVER={Maria Unicode};SERVER=127.0.0.1;PORT=3306;DATABASE=erc_tools;UID=erc_tools;PWD=change-me;OPTION=3;";
    std::wstring ntisEventSnapshotUrl =
        L"http://127.0.0.1:18080/internal/events";
    std::filesystem::path updateRoot = L"updates";
    std::filesystem::path manifestPath = L"updates\\manifest.json";
    std::filesystem::path globalSettingsPath = L"global_settings.json";
};

struct UserRecord
{
    std::wstring id;
    std::wstring username;
    std::wstring displayName;
    std::wstring position;
    std::wstring pod;
};

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse
{
    int status = 200;
    std::string reason = "OK";
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::vector<unsigned char> binary;
};

static std::wstring Utf8ToWide(const std::string& s);
static std::string WideToUtf8(const std::wstring& s);

constexpr uint16_t kBinaryProtocolVersion = 1;
constexpr uint32_t kMaxBinaryPayload = 32u * 1024u * 1024u;

constexpr uint16_t kBinaryLogin = 1;
constexpr uint16_t kBinaryLogout = 2;
constexpr uint16_t kBinaryPoll = 3;
constexpr uint16_t kBinarySendChat = 4;
constexpr uint16_t kBinaryClearChat = 5;
constexpr uint16_t kBinaryCreateNote = 6;
constexpr uint16_t kBinaryUpdateNote = 7;
constexpr uint16_t kBinaryDeleteNote = 8;
constexpr uint16_t kBinaryGetSettings = 9;
constexpr uint16_t kBinarySetSettings = 10;
constexpr uint16_t kBinaryCreateAccount = 11;
constexpr uint16_t kBinaryDeleteChatMessage = 12;
constexpr uint16_t kBinaryKickUser = 13;
constexpr uint16_t kBinaryMuteUser = 14;
constexpr uint16_t kBinarySendPrivateMessage = 15;
constexpr uint16_t kBinaryAddIncidentExclusion = 16;
constexpr uint16_t kBinaryRemoveIncidentExclusion = 17;
constexpr uint16_t kBinaryFetchSourceBundle = 18;
constexpr uint16_t kBinaryWaitSourceBundle = 19;

struct BinaryResponse
{
    uint16_t opcode = 0;
    uint16_t status = 0;
    std::vector<unsigned char> payload;
};

static void WriteU16(std::vector<unsigned char>& out, uint16_t value)
{
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
}

static void WriteU32(std::vector<unsigned char>& out, uint32_t value)
{
    out.push_back(static_cast<unsigned char>(value & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xFF));
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xFF));
}

static uint16_t ReadU16Raw(const unsigned char* p)
{
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

static uint32_t ReadU32Raw(const unsigned char* p)
{
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1]) << 8) |
        (static_cast<uint32_t>(p[2]) << 16) |
        (static_cast<uint32_t>(p[3]) << 24);
}

class BinaryWriter
{
public:
    void U32(uint32_t value) { WriteU32(m_data, value); }

    void F64(double value)
    {
        uint64_t raw = 0;
        static_assert(sizeof(raw) == sizeof(value));
        std::memcpy(&raw, &value, sizeof(value));
        for (int i = 0; i < 8; ++i)
            m_data.push_back(static_cast<unsigned char>((raw >> (i * 8)) & 0xFF));
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

    const std::vector<unsigned char>& Data() const { return m_data; }

private:
    std::vector<unsigned char> m_data;
};

class BinaryReader
{
public:
    explicit BinaryReader(const std::vector<unsigned char>& data) : m_data(data) {}

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

    const std::vector<unsigned char>& m_data;
    size_t m_pos = 0;
};

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0)
        return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

static std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}

static std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return s;
}

static std::string Trim(std::string s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

static std::wstring TrimWide(std::wstring s)
{
    while (!s.empty() && std::iswspace(s.front()))
        s.erase(s.begin());
    while (!s.empty() && std::iswspace(s.back()))
        s.pop_back();
    return s;
}

static std::wstring CanonicalPosition(std::wstring position)
{
    std::wstring key = ToLower(TrimWide(position));
    if (key == L"administrator" || key == L"admin")
        return L"Administrator";
    if (key == L"supervisor" || key == L"sup")
        return L"Supervisor";
    if (key == L"manager" || key == L"mgr")
        return L"Manager";
    if (key == L"erc")
        return L"ERC";
    return TrimWide(position);
}

static int PositionRank(const std::wstring& position)
{
    std::wstring key = ToLower(TrimWide(position));
    if (key == L"administrator" || key == L"admin")
        return 4;
    if (key == L"supervisor" || key == L"sup")
        return 3;
    if (key == L"manager" || key == L"mgr")
        return 2;
    if (key == L"erc")
        return 1;
    return 0;
}

static std::wstring PickWide(const json& obj, std::initializer_list<const char*> keys)
{
    if (!obj.is_object())
        return L"";
    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end() && it->is_string())
            return Utf8ToWide(it->get<std::string>());
    }
    return L"";
}

static std::string HexFromBytes(const std::vector<unsigned char>& bytes)
{
    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (unsigned char b : bytes) {
        out.push_back(hex[(b >> 4) & 0x0F]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

static std::vector<unsigned char> BytesFromHex(const std::wstring& hex)
{
    auto valueOf = [](wchar_t ch) -> int {
        if (ch >= L'0' && ch <= L'9') return ch - L'0';
        ch = static_cast<wchar_t>(::towlower(ch));
        if (ch >= L'a' && ch <= L'f') return 10 + ch - L'a';
        return -1;
    };

    std::vector<unsigned char> out;
    if (hex.size() % 2 != 0)
        return out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int hi = valueOf(hex[i]);
        int lo = valueOf(hex[i + 1]);
        if (hi < 0 || lo < 0)
            return {};
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return out;
}

static bool RandomBytes(std::vector<unsigned char>& bytes)
{
    return BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
}

static bool Sha256Hex(const std::string& text, std::string& hashOut)
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0;
    DWORD hashSize = 0;
    DWORD cb = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return false;
    auto closeAlg = [&]() { if (alg) BCryptCloseAlgorithmProvider(alg, 0); };
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &cb, 0) < 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize), sizeof(hashSize), &cb, 0) < 0)
    {
        closeAlg();
        return false;
    }
    std::vector<unsigned char> object(objectSize);
    std::vector<unsigned char> digest(hashSize);
    if (BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) < 0) {
        closeAlg();
        return false;
    }
    bool ok = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(text.data())), static_cast<ULONG>(text.size()), 0) >= 0 &&
        BCryptFinishHash(hash, digest.data(), hashSize, 0) >= 0;
    BCryptDestroyHash(hash);
    closeAlg();
    if (ok)
        hashOut = HexFromBytes(digest);
    return ok;
}

static bool DerivePasswordHash(const std::wstring& password, const std::vector<unsigned char>& salt, int iterations, std::string& hashOut)
{
    if (iterations <= 0)
        iterations = 150000;

    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        return false;
    std::string utf8Password = WideToUtf8(password);
    std::vector<unsigned char> derived(32);
    NTSTATUS status = BCryptDeriveKeyPBKDF2(
        alg,
        reinterpret_cast<PUCHAR>(utf8Password.data()),
        static_cast<ULONG>(utf8Password.size()),
        const_cast<PUCHAR>(salt.data()),
        static_cast<ULONG>(salt.size()),
        static_cast<ULONGLONG>(iterations),
        derived.data(),
        static_cast<ULONG>(derived.size()),
        0);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (status < 0)
        return false;
    hashOut = HexFromBytes(derived);
    return true;
}

static bool ConstantTimeEquals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

static std::wstring SqlDiagnostic(SQLSMALLINT handleType, SQLHANDLE handle)
{
    std::wstring out;
    for (SQLSMALLINT i = 1;; ++i) {
        SQLWCHAR state[8]{};
        SQLWCHAR message[512]{};
        SQLINTEGER nativeError = 0;
        SQLSMALLINT length = 0;
        SQLRETURN ret = SQLGetDiagRecW(handleType, handle, i, state, &nativeError, message, _countof(message), &length);
        if (!SQL_SUCCEEDED(ret))
            break;
        if (!out.empty())
            out += L" | ";
        out += L"[";
        out += state;
        out += L"] ";
        out += message;
    }
    return out;
}

class BlockingSocketQueue
{
public:
    void Push(SOCKET s)
    {
        {
            std::lock_guard lock(m_mutex);
            m_queue.push(s);
        }
        m_cv.notify_one();
    }

    bool Pop(SOCKET& s)
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [&]() { return m_stopping || !m_queue.empty(); });
        if (m_queue.empty())
            return false;
        s = m_queue.front();
        m_queue.pop();
        return true;
    }

    void Stop()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stopping = true;
        }
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<SOCKET> m_queue;
    bool m_stopping = false;
};

class OdbcConnection
{
public:
    explicit OdbcConnection(const std::wstring& connectionString) : m_connectionString(connectionString) {}
    ~OdbcConnection() { Close(); }

    bool Open(std::wstring& errorOut)
    {
        if (m_connected)
            return true;

        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env) != SQL_SUCCESS)
            return Fail(errorOut, L"SQLAllocHandle ENV failed.");
        SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        if (SQLAllocHandle(SQL_HANDLE_DBC, m_env, &m_dbc) != SQL_SUCCESS)
            return Fail(errorOut, L"SQLAllocHandle DBC failed.");

        SQLWCHAR outConn[1024]{};
        SQLSMALLINT outLen = 0;
        SQLRETURN ret = SQLDriverConnectW(
            m_dbc,
            nullptr,
            reinterpret_cast<SQLWCHAR*>(m_connectionString.data()),
            SQL_NTS,
            outConn,
            _countof(outConn),
            &outLen,
            SQL_DRIVER_NOPROMPT);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_DBC, m_dbc);
            return Fail(errorOut, diagnostic.empty() ? L"Could not connect to MySQL through ODBC." : diagnostic);
        }

        m_connected = true;
        return true;
    }

    std::vector<std::vector<std::wstring>> Query(const std::wstring& sql, const std::vector<std::wstring>& params, std::wstring& errorOut)
    {
        std::vector<std::vector<std::wstring>> rows;
        SQLHSTMT stmt = nullptr;
        if (!Open(errorOut))
            return rows;
        if (SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &stmt) != SQL_SUCCESS) {
            errorOut = L"SQLAllocHandle STMT failed.";
            return rows;
        }
        auto closeStmt = [&]() { SQLFreeHandle(SQL_HANDLE_STMT, stmt); };

        if (!PrepareAndBind(stmt, sql, params, errorOut)) {
            closeStmt();
            return rows;
        }

        SQLRETURN ret = SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
            errorOut = diagnostic.empty() ? L"SQLExecute failed." : diagnostic;
            closeStmt();
            return rows;
        }

        SQLSMALLINT cols = 0;
        SQLNumResultCols(stmt, &cols);
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            std::vector<std::wstring> row;
            for (SQLUSMALLINT col = 1; col <= static_cast<SQLUSMALLINT>(cols); ++col) {
                wchar_t buffer[4096]{};
                SQLLEN indicator = 0;
                SQLRETURN getRet = SQLGetData(stmt, col, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator);
                if (!SQL_SUCCEEDED(getRet) || indicator == SQL_NULL_DATA)
                    row.emplace_back();
                else
                    row.emplace_back(buffer);
            }
            rows.push_back(std::move(row));
        }

        closeStmt();
        return rows;
    }

    bool Execute(const std::wstring& sql, const std::vector<std::wstring>& params, std::wstring& errorOut)
    {
        SQLHSTMT stmt = nullptr;
        if (!Open(errorOut))
            return false;
        if (SQLAllocHandle(SQL_HANDLE_STMT, m_dbc, &stmt) != SQL_SUCCESS) {
            errorOut = L"SQLAllocHandle STMT failed.";
            return false;
        }
        auto closeStmt = [&]() { SQLFreeHandle(SQL_HANDLE_STMT, stmt); };
        if (!PrepareAndBind(stmt, sql, params, errorOut)) {
            closeStmt();
            return false;
        }
        SQLRETURN ret = SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
            errorOut = diagnostic.empty() ? L"SQLExecute failed." : diagnostic;
            closeStmt();
            return false;
        }
        closeStmt();
        return true;
    }

private:
    bool PrepareAndBind(SQLHSTMT stmt, const std::wstring& sql, const std::vector<std::wstring>& params, std::wstring& errorOut)
    {
        SQLRETURN ret = SQLPrepareW(stmt, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql.c_str())), SQL_NTS);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
            errorOut = diagnostic.empty() ? L"SQLPrepare failed." : diagnostic;
            return false;
        }

        m_boundParams = params;
        m_paramLengths.assign(m_boundParams.size(), SQL_NTS);
        for (size_t i = 0; i < m_boundParams.size(); ++i) {
            ret = SQLBindParameter(
                stmt,
                static_cast<SQLUSMALLINT>(i + 1),
                SQL_PARAM_INPUT,
                SQL_C_WCHAR,
                SQL_WVARCHAR,
                static_cast<SQLULEN>(std::max<size_t>(m_boundParams[i].size(), 1)),
                0,
                const_cast<wchar_t*>(m_boundParams[i].c_str()),
                0,
                &m_paramLengths[i]);
            if (!SQL_SUCCEEDED(ret)) {
                std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
                errorOut = diagnostic.empty() ? L"SQLBindParameter failed." : diagnostic;
                return false;
            }
        }
        return true;
    }

    bool Fail(std::wstring& errorOut, const std::wstring& message)
    {
        errorOut = message;
        Close();
        return false;
    }

    void Close()
    {
        if (m_dbc) {
            if (m_connected)
                SQLDisconnect(m_dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, m_dbc);
            m_dbc = nullptr;
        }
        if (m_env) {
            SQLFreeHandle(SQL_HANDLE_ENV, m_env);
            m_env = nullptr;
        }
        m_connected = false;
    }

    std::wstring m_connectionString;
    SQLHENV m_env = nullptr;
    SQLHDBC m_dbc = nullptr;
    bool m_connected = false;
    std::vector<std::wstring> m_boundParams;
    std::vector<SQLLEN> m_paramLengths;
};

class Database
{
public:
    explicit Database(ServerConfig config) : m_config(std::move(config)) {}

    bool ValidateLogin(const std::wstring& username, const std::wstring& password, const std::wstring& position, const std::wstring& pod, UserRecord& userOut, std::wstring& errorOut, std::string& codeOut)
    {
        codeOut.clear();
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureSessionContextColumns(db, errorOut)) {
            errorOut = L"Database session schema check failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }
        if (!PurgeStaleSessions(db, errorOut)) {
            errorOut = L"Database stale session cleanup failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }

        auto rows = db.Query(
            L"SELECT id, username, display_name, position, password_salt, password_hash, password_iterations, active FROM users WHERE username = ? LIMIT 1",
            { username },
            errorOut);
        if (!errorOut.empty()) {
            errorOut = L"Database login lookup failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }
        if (rows.empty()) {
            errorOut = L"Invalid username or password.";
            codeOut = "invalid_credentials";
            return false;
        }

        const auto& row = rows.front();
        if (row.size() < 8 || row[7] == L"0") {
            errorOut = L"This account is disabled.";
            codeOut = "account_disabled";
            return false;
        }

        std::vector<unsigned char> salt = BytesFromHex(row[4]);
        int iterations = row[6].empty() ? 150000 : _wtoi(row[6].c_str());
        std::string actualHash;
        if (salt.empty() || !DerivePasswordHash(password, salt, iterations, actualHash) ||
            !ConstantTimeEquals(actualHash, WideToUtf8(ToLower(row[5]))))
        {
            errorOut = L"Invalid username or password.";
            codeOut = "invalid_credentials";
            return false;
        }

        std::wstring requestedPosition = CanonicalPosition(position);
        int requestedRank = PositionRank(requestedPosition);
        int accountRank = PositionRank(row[3]);
        if (requestedRank <= 0 || accountRank <= 0 || requestedRank > accountRank) {
            errorOut = L"This account cannot sign in as the selected position.";
            codeOut = "position_not_allowed";
            return false;
        }
        std::wstring sessionPod = TrimWide(pod);
        bool podInUse = false;
        if (!CheckPodInUse(db, sessionPod, podInUse, errorOut)) {
            errorOut = L"Database pod availability check failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }
        if (podInUse) {
            errorOut = L"Selected pod is already in use.";
            codeOut = "pod_in_use";
            return false;
        }

        userOut = UserRecord{
            row[0],
            row[1],
            row[2],
            requestedPosition,
            sessionPod
        };
        db.Execute(L"UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE id = ?", { userOut.id }, errorOut);
        errorOut.clear();
        codeOut.clear();
        return true;
    }

    bool CreateOrUpdateUser(
        const std::wstring& username,
        const std::wstring& displayName,
        const std::wstring& password,
        const std::wstring& position,
        bool active,
        std::wstring& errorOut,
        std::string& codeOut)
    {
        codeOut.clear();
        std::wstring cleanUsername = TrimWide(username);
        std::wstring cleanPassword = password;
        std::wstring cleanDisplayName = TrimWide(displayName);
        std::wstring cleanPosition = CanonicalPosition(position);
        if (cleanDisplayName.empty())
            cleanDisplayName = cleanUsername;

        if (cleanUsername.empty() || cleanPassword.empty() || cleanDisplayName.empty() || PositionRank(cleanPosition) <= 0) {
            errorOut = L"Username, display name, password and position are required.";
            codeOut = "missing_fields";
            return false;
        }

        std::vector<unsigned char> salt(16);
        if (!RandomBytes(salt)) {
            errorOut = L"Could not create password salt.";
            codeOut = "hash_failed";
            return false;
        }

        constexpr int iterations = 150000;
        std::string hashHex;
        if (!DerivePasswordHash(cleanPassword, salt, iterations, hashHex)) {
            errorOut = L"Could not hash password.";
            codeOut = "hash_failed";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        auto existing = db.Query(L"SELECT id FROM users WHERE username = ? LIMIT 1", { cleanUsername }, errorOut);
        if (!errorOut.empty()) {
            errorOut = L"Database account lookup failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }

        std::wstring activeText = active ? L"1" : L"0";
        std::wstring saltHex = Utf8ToWide(HexFromBytes(salt));
        std::wstring hashWide = Utf8ToWide(hashHex);
        std::wstring iterationsText = std::to_wstring(iterations);
        if (!existing.empty()) {
            bool ok = db.Execute(
                L"UPDATE users SET display_name = ?, position = ?, password_salt = ?, password_hash = ?, password_iterations = ?, active = ? WHERE username = ?",
                { cleanDisplayName, cleanPosition, saltHex, hashWide, iterationsText, activeText, cleanUsername },
                errorOut);
            if (!ok) {
                errorOut = L"Database account update failed: " + errorOut;
                codeOut = "database_error";
                return false;
            }
            return true;
        }

        bool ok = db.Execute(
            L"INSERT INTO users (username, display_name, position, pod, password_salt, password_hash, password_iterations, active) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            { cleanUsername, cleanDisplayName, cleanPosition, L"Pod 1", saltHex, hashWide, iterationsText, activeText },
            errorOut);
        if (!ok) {
            errorOut = L"Database account creation failed: " + errorOut;
            codeOut = "database_error";
            return false;
        }
        return true;
    }

    bool CreateSession(const UserRecord& user, std::wstring& tokenOut, std::wstring& errorOut)
    {
        std::vector<unsigned char> tokenBytes(32);
        if (!RandomBytes(tokenBytes)) {
            errorOut = L"Could not create secure session token.";
            return false;
        }

        std::string token = HexFromBytes(tokenBytes);
        std::string tokenHash;
        if (!Sha256Hex(token, tokenHash)) {
            errorOut = L"Could not hash session token.";
            return false;
        }

        std::lock_guard<std::mutex> sessionLock(m_sessionCreateMutex);
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (!PurgeStaleSessions(db, errorOut))
            return false;

        bool podInUse = false;
        if (!CheckPodInUse(db, user.pod, podInUse, errorOut))
            return false;
        if (podInUse) {
            errorOut = L"Selected pod is already in use.";
            return false;
        }

        bool ok = db.Execute(
            L"INSERT INTO user_sessions (token_hash, user_id, session_display_name, session_position, session_pod, expires_at, created_at, last_seen_at) "
            L"VALUES (?, ?, ?, ?, ?, DATE_ADD(CURRENT_TIMESTAMP, INTERVAL ? MINUTE), CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)",
            { Utf8ToWide(tokenHash), user.id, user.displayName, user.position, user.pod, std::to_wstring(m_config.sessionMinutes) },
            errorOut);
        if (!ok)
            return false;
        LogUserEvent(db, user, user, L"login", L"", errorOut);
        errorOut.clear();
        tokenOut = Utf8ToWide(token);
        return true;
    }

    bool Authenticate(const std::string& bearerToken, UserRecord& userOut, std::wstring& errorOut)
    {
        if (bearerToken.empty()) {
            errorOut = L"Missing bearer token.";
            return false;
        }
        std::string tokenHash;
        if (!Sha256Hex(bearerToken, tokenHash)) {
            errorOut = L"Could not hash bearer token.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureSessionContextColumns(db, errorOut)) {
            errorOut = L"Database session schema check failed: " + errorOut;
            return false;
        }
        if (!PurgeStaleSessions(db, errorOut)) {
            errorOut = L"Database stale session cleanup failed: " + errorOut;
            return false;
        }

        auto rows = db.Query(
            L"SELECT u.id, u.username, "
            L"COALESCE(NULLIF(s.session_display_name, ''), u.display_name), "
            L"COALESCE(NULLIF(s.session_position, ''), u.position), "
            L"COALESCE(NULLIF(s.session_pod, ''), u.pod) "
            L"FROM user_sessions s JOIN users u ON u.id = s.user_id "
            L"WHERE s.token_hash = ? AND s.expires_at > CURRENT_TIMESTAMP AND u.active = 1 LIMIT 1",
            { Utf8ToWide(tokenHash) },
            errorOut);
        if (!errorOut.empty()) {
            errorOut = L"Database session lookup failed: " + errorOut;
            return false;
        }
        if (rows.empty()) {
            errorOut = L"Session is invalid or expired.";
            return false;
        }

        const auto& row = rows.front();
        userOut = UserRecord{ row[0], row[1], row[2], row[3], row[4] };
        db.Execute(L"UPDATE user_sessions SET last_seen_at = CURRENT_TIMESTAMP WHERE token_hash = ?", { Utf8ToWide(tokenHash) }, errorOut);
        errorOut.clear();
        return true;
    }

    bool DeleteSession(const std::string& bearerToken, std::wstring& errorOut)
    {
        if (bearerToken.empty()) {
            errorOut = L"Missing bearer token.";
            return false;
        }

        std::string tokenHash;
        if (!Sha256Hex(bearerToken, tokenHash)) {
            errorOut = L"Could not hash bearer token.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;

        UserRecord user;
        auto rows = db.Query(
            L"SELECT u.id, u.username, COALESCE(NULLIF(s.session_display_name, ''), u.display_name), "
            L"COALESCE(NULLIF(s.session_position, ''), u.position), COALESCE(NULLIF(s.session_pod, ''), u.pod) "
            L"FROM user_sessions s JOIN users u ON u.id = s.user_id WHERE s.token_hash = ? LIMIT 1",
            { Utf8ToWide(tokenHash) },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (!rows.empty() && rows.front().size() >= 5) {
            const auto& row = rows.front();
            user = UserRecord{ row[0], row[1], row[2], row[3], row[4] };
            LogUserEvent(db, user, user, L"logout", L"", errorOut);
            errorOut.clear();
        }

        return db.Execute(
            L"DELETE FROM user_sessions WHERE token_hash = ?",
            { Utf8ToWide(tokenHash) },
            errorOut);
    }

    bool KickUser(const UserRecord& actor, const std::wstring& username, std::wstring& errorOut)
    {
        UserRecord target;
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (!FindUserByUsername(db, username, target, errorOut))
            return false;
        if (!CanManageTarget(actor, target, errorOut))
            return false;

        LogUserEvent(db, actor, target, L"kick", L"", errorOut);
        errorOut.clear();
        return db.Execute(L"DELETE FROM user_sessions WHERE user_id = ?", { target.id }, errorOut);
    }

    bool MuteUser(const UserRecord& actor, const std::wstring& username, uint32_t minutes, std::wstring& errorOut)
    {
        UserRecord target;
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (!FindUserByUsername(db, username, target, errorOut))
            return false;
        if (!CanManageTarget(actor, target, errorOut))
            return false;

        minutes = std::clamp<uint32_t>(minutes == 0 ? 15 : minutes, 1, 24 * 60);
        bool ok = db.Execute(
            L"UPDATE users SET muted_until = DATE_ADD(CURRENT_TIMESTAMP, INTERVAL ? MINUTE), muted_by = ?, muted_at = CURRENT_TIMESTAMP WHERE id = ?",
            { std::to_wstring(minutes), actor.id, target.id },
            errorOut);
        if (ok) {
            LogUserEvent(db, actor, target, L"mute", std::to_wstring(minutes) + L" minute(s)", errorOut);
            errorOut.clear();
        }
        return ok;
    }

    json UserLoginTimes(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return json::array();
        auto rows = db.Query(
            L"SELECT event_type, username, display_name, position, pod, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s'), actor_username, details "
            L"FROM user_session_audit ORDER BY created_at DESC LIMIT 200",
            {},
            errorOut);
        json items = json::array();
        for (const auto& row : rows) {
            if (row.size() < 8)
                continue;
            items.push_back({
                { "event", WideToUtf8(row[0]) },
                { "username", WideToUtf8(row[1]) },
                { "displayName", WideToUtf8(row[2]) },
                { "position", WideToUtf8(row[3]) },
                { "pod", WideToUtf8(row[4]) },
                { "timestamp", WideToUtf8(row[5]) },
                { "actor", WideToUtf8(row[6]) },
                { "details", WideToUtf8(row[7]) }
                });
        }
        return items;
    }

    bool ClearAdminLogCategory(const std::wstring& category, std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;

        const std::wstring clean = ToLower(TrimWide(category));
        if (clean == L"exclusions") {
            return db.Execute(
                L"DELETE FROM user_session_audit "
                L"WHERE event_type LIKE 'incident_exclusion_%' OR event_type LIKE 'exclusion_%'",
                {},
                errorOut);
        }
        if (clean == L"user_login_times") {
            return db.Execute(
                L"DELETE FROM user_session_audit WHERE event_type IN ('login', 'logout')",
                {},
                errorOut);
        }

        errorOut = L"Unknown administrator log category.";
        return false;
    }

    json IncidentExclusions(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return json::array();

        auto rows = db.Query(
            L"SELECT incident_key, source_id, source_name, road, summary, added_by_username, "
            L"DATE_FORMAT(added_at, '%Y-%m-%d %H:%i:%s') "
            L"FROM incident_exclusions ORDER BY added_at DESC",
            {},
            errorOut);
        json items = json::array();
        for (const auto& row : rows) {
            if (row.size() < 7)
                continue;
            items.push_back({
                { "key", WideToUtf8(row[0]) },
                { "sourceId", WideToUtf8(row[1]) },
                { "source", WideToUtf8(row[2]) },
                { "road", WideToUtf8(row[3]) },
                { "summary", WideToUtf8(row[4]) },
                { "addedBy", WideToUtf8(row[5]) },
                { "addedAt", WideToUtf8(row[6]) }
                });
        }
        return items;
    }

    bool AddIncidentExclusion(
        const UserRecord& actor,
        const std::wstring& key,
        const std::wstring& sourceId,
        const std::wstring& source,
        const std::wstring& road,
        const std::wstring& summary,
        std::wstring& errorOut)
    {
        const std::wstring cleanKey = TrimWide(key);
        if (cleanKey.empty()) {
            errorOut = L"Exclusion key is empty.";
            return false;
        }

        std::string hash;
        if (!Sha256Hex(WideToUtf8(cleanKey), hash)) {
            errorOut = L"Could not hash the exclusion key.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (!db.Execute(
            L"INSERT INTO incident_exclusions "
            L"(key_hash, incident_key, source_id, source_name, road, summary, added_by_user_id, added_by_username, added_at, updated_at) "
            L"VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP) "
            L"ON DUPLICATE KEY UPDATE source_id = VALUES(source_id), source_name = VALUES(source_name), "
            L"road = VALUES(road), summary = VALUES(summary), added_by_user_id = VALUES(added_by_user_id), "
            L"added_by_username = VALUES(added_by_username), updated_at = CURRENT_TIMESTAMP",
            {
                Utf8ToWide(hash),
                cleanKey,
                TrimWide(sourceId),
                TrimWide(source),
                TrimWide(road),
                TrimWide(summary),
                actor.id,
                actor.username
            },
            errorOut))
        {
            return false;
        }

        std::wstring auditError;
        const std::wstring auditTarget = TrimWide(road).empty() ? TrimWide(source) : TrimWide(road);
        db.Execute(
            L"INSERT INTO user_session_audit "
            L"(event_type, username, display_name, actor_user_id, actor_username, details, created_at) "
            L"VALUES ('exclusion_add', ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)",
            { auditTarget, TrimWide(summary), actor.id, actor.username, cleanKey.substr(0, 255) },
            auditError);
        return true;
    }

    bool RemoveIncidentExclusion(
        const UserRecord& actor,
        const std::wstring& key,
        std::wstring& errorOut)
    {
        const std::wstring cleanKey = TrimWide(key);
        std::string hash;
        if (cleanKey.empty() || !Sha256Hex(WideToUtf8(cleanKey), hash)) {
            errorOut = L"Exclusion key is invalid.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        auto rows = db.Query(
            L"SELECT source_name, road, summary FROM incident_exclusions WHERE key_hash = ? LIMIT 1",
            { Utf8ToWide(hash) },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (!db.Execute(
            L"DELETE FROM incident_exclusions WHERE key_hash = ?",
            { Utf8ToWide(hash) },
            errorOut))
        {
            return false;
        }

        const std::wstring source = rows.empty() || rows.front().empty() ? L"" : rows.front()[0];
        const std::wstring road = rows.empty() || rows.front().size() < 2 ? L"" : rows.front()[1];
        const std::wstring summary = rows.empty() || rows.front().size() < 3 ? L"" : rows.front()[2];
        const std::wstring auditTarget = TrimWide(road).empty() ? source : road;
        std::wstring auditError;
        db.Execute(
            L"INSERT INTO user_session_audit "
            L"(event_type, username, display_name, actor_user_id, actor_username, details, created_at) "
            L"VALUES ('exclusion_remove', ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)",
            { auditTarget, summary, actor.id, actor.username, cleanKey.substr(0, 255) },
            auditError);
        return true;
    }

    json OnlineUsers(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut)) {
            errorOut = L"Database collaboration schema check failed: " + errorOut;
            return json::array();
        }
        if (!PurgeStaleSessions(db, errorOut)) {
            errorOut = L"Database stale session cleanup failed: " + errorOut;
            return json::array();
        }

        const std::wstring sql = std::wstring(
            L"SELECT "
            L"u.id, "
            L"COALESCE(NULLIF(s.session_display_name, ''), u.display_name), "
            L"u.username, "
            L"COALESCE(NULLIF(s.session_position, ''), u.position), "
            L"COALESCE(NULLIF(s.session_pod, ''), u.pod), "
            L"DATE_FORMAT(COALESCE(s.last_seen_at, s.created_at), '%Y-%m-%d %H:%i:%s') "
            L"FROM user_sessions s JOIN users u ON u.id = s.user_id "
            L"WHERE s.expires_at > CURRENT_TIMESTAMP "
            L"AND COALESCE(s.last_seen_at, s.created_at) >= DATE_SUB(CURRENT_TIMESTAMP, INTERVAL ") +
            OnlineTimeoutSecondsText() +
            std::wstring(L" SECOND) "
            L"AND u.active = 1 "
            L"ORDER BY COALESCE(NULLIF(s.session_pod, ''), u.pod), COALESCE(NULLIF(s.session_display_name, ''), u.display_name)");
        auto rows = db.Query(
            sql,
            {},
            errorOut);
        if (!errorOut.empty()) {
            errorOut = L"Database online user lookup failed: " + errorOut;
            return json::array();
        }

        json users = json::array();
        for (const auto& row : rows) {
            if (row.size() < 6)
                continue;
            users.push_back({
                { "id", WideToUtf8(row[0]) },
                { "displayName", WideToUtf8(row[1]) },
                { "username", WideToUtf8(row[2]) },
                { "position", WideToUtf8(row[3]) },
                { "pod", WideToUtf8(row[4]) },
                { "lastSeen", WideToUtf8(row[5]) }
                });
        }
        return users;
    }

    json ChatMessages(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut)) {
            errorOut = L"Database collaboration schema check failed: " + errorOut;
            return json::array();
        }
        auto rows = db.Query(
            L"SELECT c.id, c.author, COALESCE(u.username, ''), c.body, DATE_FORMAT(c.created_at, '%Y-%m-%d %H:%i:%s'), COALESCE(u.position, '') "
            L"FROM chat_messages c LEFT JOIN users u ON u.id = c.user_id "
            L"WHERE c.deleted_at IS NULL ORDER BY c.created_at DESC LIMIT 100",
            {},
            errorOut);
        json messages = json::array();
        for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
            messages.push_back({
                { "id", WideToUtf8((*it)[0]) },
                { "author", WideToUtf8((*it)[1]) },
                { "username", WideToUtf8((*it)[2]) },
                { "text", WideToUtf8((*it)[3]) },
                { "timestamp", WideToUtf8((*it)[4]) },
                { "position", WideToUtf8((*it)[5]) }
                });
        }
        return messages;
    }

    bool AddChatMessage(const UserRecord& user, const std::wstring& text, std::wstring& errorOut)
    {
        std::wstring author = user.displayName.empty() ? user.username : user.displayName;
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (IsUserMuted(db, user.id, errorOut)) {
            if (errorOut.empty())
                errorOut = L"You are currently muted from responder chat.";
            return false;
        }
        return db.Execute(
            L"INSERT INTO chat_messages (user_id, author, body, created_at) VALUES (?, ?, ?, CURRENT_TIMESTAMP)",
            { user.id, author, text },
            errorOut);
    }

    json PrivateMessages(const UserRecord& user, std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut)) {
            errorOut = L"Database collaboration schema check failed: " + errorOut;
            return json::array();
        }

        auto rows = db.Query(
            L"SELECT pm.id, "
            L"su.username, su.display_name, su.position, "
            L"ru.username, ru.display_name, ru.position, "
            L"pm.body, DATE_FORMAT(pm.created_at, '%Y-%m-%d %H:%i:%s') "
            L"FROM private_messages pm "
            L"JOIN users su ON su.id = pm.sender_user_id "
            L"JOIN users ru ON ru.id = pm.recipient_user_id "
            L"WHERE ((pm.sender_user_id = ? AND pm.deleted_by_sender_at IS NULL) "
            L"OR (pm.recipient_user_id = ? AND pm.deleted_by_recipient_at IS NULL)) "
            L"ORDER BY pm.created_at DESC LIMIT 200",
            { user.id, user.id },
            errorOut);
        json messages = json::array();
        for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
            if (it->size() < 9)
                continue;
            messages.push_back({
                { "id", WideToUtf8((*it)[0]) },
                { "senderUsername", WideToUtf8((*it)[1]) },
                { "senderDisplayName", WideToUtf8((*it)[2]) },
                { "senderPosition", WideToUtf8((*it)[3]) },
                { "recipientUsername", WideToUtf8((*it)[4]) },
                { "recipientDisplayName", WideToUtf8((*it)[5]) },
                { "recipientPosition", WideToUtf8((*it)[6]) },
                { "text", WideToUtf8((*it)[7]) },
                { "timestamp", WideToUtf8((*it)[8]) }
                });
        }
        return messages;
    }

    bool AddPrivateMessage(const UserRecord& sender, const std::wstring& recipientUsername, const std::wstring& text, std::wstring& errorOut)
    {
        std::wstring cleanText = TrimWide(text);
        if (cleanText.empty()) {
            errorOut = L"Private message text is required.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        if (IsUserMuted(db, sender.id, errorOut)) {
            if (errorOut.empty())
                errorOut = L"You are currently muted from messaging.";
            return false;
        }

        UserRecord recipient;
        if (!FindUserByUsername(db, recipientUsername, recipient, errorOut))
            return false;
        if (recipient.id == sender.id) {
            errorOut = L"You cannot send a private message to yourself.";
            return false;
        }

        return db.Execute(
            L"INSERT INTO private_messages (sender_user_id, recipient_user_id, body, created_at) VALUES (?, ?, ?, CURRENT_TIMESTAMP)",
            { sender.id, recipient.id, cleanText },
            errorOut);
    }

    bool ClearChatMessages(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;
        return db.Execute(L"UPDATE chat_messages SET deleted_at = CURRENT_TIMESTAMP WHERE deleted_at IS NULL", {}, errorOut);
    }

    bool DeleteChatMessage(const UserRecord& actor, const std::wstring& messageId, std::wstring& errorOut)
    {
        const int actorRank = PositionRank(actor.position);
        if (actorRank < 2) {
            errorOut = L"Only Managers, Supervisors and Administrators can delete responder chat messages.";
            return false;
        }

        std::wstring cleanId = TrimWide(messageId);
        if (cleanId.empty()) {
            errorOut = L"Chat message id is required.";
            return false;
        }

        OdbcConnection db(m_config.databaseConnectionString);
        if (!EnsureCollaborationSchema(db, errorOut))
            return false;

        auto rows = db.Query(
            L"SELECT c.id, COALESCE(u.id, ''), COALESCE(u.username, c.author), COALESCE(u.display_name, c.author), "
            L"COALESCE(u.position, ''), COALESCE(u.pod, '') "
            L"FROM chat_messages c LEFT JOIN users u ON u.id = c.user_id "
            L"WHERE c.id = ? AND c.deleted_at IS NULL LIMIT 1",
            { cleanId },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (rows.empty()) {
            errorOut = L"Responder chat message was not found.";
            return false;
        }

        UserRecord target;
        const auto& row = rows.front();
        if (row.size() >= 6) {
            target.id = row[1];
            target.username = row[2];
            target.displayName = row[3];
            target.position = row[4];
            target.pod = row[5];
        }
        const int targetRank = PositionRank(target.position);
        if (targetRank <= 0 || actorRank <= targetRank) {
            errorOut = L"You can only delete responder chat messages from users below your position.";
            return false;
        }

        bool ok = db.Execute(
            L"UPDATE chat_messages SET deleted_at = CURRENT_TIMESTAMP, deleted_by = ? WHERE id = ? AND deleted_at IS NULL",
            { actor.id, cleanId },
            errorOut);
        if (ok)
            LogUserEvent(db, actor, target, L"chat_delete", L"message " + cleanId, errorOut);
        return ok;
    }

    json Notes(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        auto rows = db.Query(
            L"SELECT id, author, body, latitude, longitude, DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') FROM map_notes WHERE deleted_at IS NULL ORDER BY updated_at ASC",
            {},
            errorOut);
        json notes = json::array();
        for (const auto& row : rows) {
            notes.push_back({
                { "id", WideToUtf8(row[0]) },
                { "author", WideToUtf8(row[1]) },
                { "text", WideToUtf8(row[2]) },
                { "lat", _wtof(row[3].c_str()) },
                { "lon", _wtof(row[4].c_str()) },
                { "timestamp", WideToUtf8(row[5]) }
                });
        }
        return notes;
    }

    bool AddNote(const UserRecord& user, const json& body, json& noteOut, std::wstring& errorOut)
    {
        std::wstring text = PickWide(body, { "text", "note", "body" });
        double lat = body.value("lat", body.value("latitude", 0.0));
        double lon = body.value("lon", body.value("longitude", 0.0));
        std::wstring id = L"note-" + std::to_wstring(GetTickCount64());
        std::wstring author = user.displayName.empty() ? user.username : user.displayName;
        OdbcConnection db(m_config.databaseConnectionString);
        if (!db.Execute(
            L"INSERT INTO map_notes (id, user_id, author, body, latitude, longitude, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)",
            { id, user.id, author, text, std::to_wstring(lat), std::to_wstring(lon) },
            errorOut))
        {
            return false;
        }

        noteOut = {
            { "id", WideToUtf8(id) },
            { "author", WideToUtf8(author) },
            { "text", WideToUtf8(text) },
            { "lat", lat },
            { "lon", lon }
        };
        return true;
    }

    bool UpdateNote(const UserRecord& user, const std::wstring& id, const json& body, std::wstring& errorOut)
    {
        std::wstring text = PickWide(body, { "text", "note", "body" });
        double lat = body.value("lat", body.value("latitude", 0.0));
        double lon = body.value("lon", body.value("longitude", 0.0));
        OdbcConnection db(m_config.databaseConnectionString);
        return db.Execute(
            L"UPDATE map_notes SET body = ?, latitude = ?, longitude = ?, updated_by = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND deleted_at IS NULL",
            { text, std::to_wstring(lat), std::to_wstring(lon), user.id, id },
            errorOut);
    }

    bool DeleteNote(const UserRecord& user, const std::wstring& id, std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        return db.Execute(
            L"UPDATE map_notes SET deleted_at = CURRENT_TIMESTAMP, updated_by = ? WHERE id = ? AND deleted_at IS NULL",
            { user.id, id },
            errorOut);
    }

private:
    bool ColumnExists(OdbcConnection& db, const std::wstring& table, const std::wstring& column, bool& existsOut, std::wstring& errorOut)
    {
        existsOut = false;
        auto rows = db.Query(
            L"SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ? AND COLUMN_NAME = ?",
            { table, column },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (!rows.empty() && !rows.front().empty())
            existsOut = _wtoi(rows.front().front().c_str()) > 0;
        return true;
    }

    bool EnsureSessionContextColumns(OdbcConnection& db, std::wstring& errorOut)
    {
        struct ColumnDef
        {
            const wchar_t* name;
            const wchar_t* ddl;
        };

        const ColumnDef columns[] = {
            { L"session_display_name", L"session_display_name VARCHAR(255) NULL DEFAULT NULL" },
            { L"session_position", L"session_position VARCHAR(32) NULL DEFAULT NULL" },
            { L"session_pod", L"session_pod VARCHAR(32) NULL DEFAULT NULL" }
        };

        for (const ColumnDef& column : columns) {
            bool exists = false;
            if (!ColumnExists(db, L"user_sessions", column.name, exists, errorOut))
                return false;
            if (exists)
                continue;

            std::wstring sql = L"ALTER TABLE user_sessions ADD COLUMN ";
            sql += column.ddl;
            if (!db.Execute(sql, {}, errorOut))
                return false;
        }
        return true;
    }

    bool EnsureColumn(OdbcConnection& db, const std::wstring& table, const std::wstring& name, const std::wstring& ddl, std::wstring& errorOut)
    {
        bool exists = false;
        if (!ColumnExists(db, table, name, exists, errorOut))
            return false;
        if (exists)
            return true;

        return db.Execute(L"ALTER TABLE " + table + L" ADD COLUMN " + ddl, {}, errorOut);
    }

    bool EnsureCollaborationSchema(OdbcConnection& db, std::wstring& errorOut)
    {
        if (!EnsureSessionContextColumns(db, errorOut))
            return false;

        if (!EnsureColumn(db, L"chat_messages", L"deleted_at", L"deleted_at TIMESTAMP NULL DEFAULT NULL", errorOut))
            return false;
        if (!EnsureColumn(db, L"chat_messages", L"deleted_by", L"deleted_by BIGINT UNSIGNED NULL DEFAULT NULL", errorOut))
            return false;
        if (!EnsureColumn(db, L"users", L"muted_until", L"muted_until TIMESTAMP NULL DEFAULT NULL", errorOut))
            return false;
        if (!EnsureColumn(db, L"users", L"muted_by", L"muted_by BIGINT UNSIGNED NULL DEFAULT NULL", errorOut))
            return false;
        if (!EnsureColumn(db, L"users", L"muted_at", L"muted_at TIMESTAMP NULL DEFAULT NULL", errorOut))
            return false;

        if (!db.Execute(
            L"CREATE TABLE IF NOT EXISTS user_session_audit ("
            L"id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            L"event_type VARCHAR(32) NOT NULL,"
            L"target_user_id BIGINT UNSIGNED NULL DEFAULT NULL,"
            L"username VARCHAR(128) NULL DEFAULT NULL,"
            L"display_name VARCHAR(255) NULL DEFAULT NULL,"
            L"position VARCHAR(32) NULL DEFAULT NULL,"
            L"pod VARCHAR(32) NULL DEFAULT NULL,"
            L"actor_user_id BIGINT UNSIGNED NULL DEFAULT NULL,"
            L"actor_username VARCHAR(128) NULL DEFAULT NULL,"
            L"details VARCHAR(255) NULL DEFAULT NULL,"
            L"created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            L"INDEX idx_user_session_audit_created (created_at),"
            L"INDEX idx_user_session_audit_user (target_user_id)"
            L")",
            {},
            errorOut))
        {
            return false;
        }

        if (!db.Execute(
            L"CREATE TABLE IF NOT EXISTS incident_exclusions ("
            L"key_hash CHAR(64) NOT NULL PRIMARY KEY,"
            L"incident_key VARCHAR(1024) NOT NULL,"
            L"source_id VARCHAR(255) NULL DEFAULT NULL,"
            L"source_name VARCHAR(64) NULL DEFAULT NULL,"
            L"road VARCHAR(64) NULL DEFAULT NULL,"
            L"summary VARCHAR(512) NULL DEFAULT NULL,"
            L"added_by_user_id BIGINT UNSIGNED NULL DEFAULT NULL,"
            L"added_by_username VARCHAR(128) NULL DEFAULT NULL,"
            L"added_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            L"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            L"INDEX idx_incident_exclusions_added (added_at),"
            L"INDEX idx_incident_exclusions_source (source_id)"
            L")",
            {},
            errorOut))
        {
            return false;
        }

        return db.Execute(
            L"CREATE TABLE IF NOT EXISTS private_messages ("
            L"id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            L"sender_user_id BIGINT UNSIGNED NOT NULL,"
            L"recipient_user_id BIGINT UNSIGNED NOT NULL,"
            L"body TEXT NOT NULL,"
            L"created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            L"deleted_by_sender_at TIMESTAMP NULL DEFAULT NULL,"
            L"deleted_by_recipient_at TIMESTAMP NULL DEFAULT NULL,"
            L"INDEX idx_private_messages_pair (sender_user_id, recipient_user_id, created_at),"
            L"INDEX idx_private_messages_recipient (recipient_user_id, created_at),"
            L"CONSTRAINT fk_private_messages_sender FOREIGN KEY (sender_user_id) REFERENCES users(id) ON DELETE CASCADE,"
            L"CONSTRAINT fk_private_messages_recipient FOREIGN KEY (recipient_user_id) REFERENCES users(id) ON DELETE CASCADE"
            L")",
            {},
            errorOut);
    }

    bool FindUserByUsername(OdbcConnection& db, const std::wstring& username, UserRecord& userOut, std::wstring& errorOut)
    {
        std::wstring cleanUsername = TrimWide(username);
        if (cleanUsername.empty()) {
            errorOut = L"Username is required.";
            return false;
        }

        auto rows = db.Query(
            L"SELECT id, username, display_name, position, pod FROM users WHERE username = ? AND active = 1 LIMIT 1",
            { cleanUsername },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (rows.empty() || rows.front().size() < 5) {
            errorOut = L"User was not found.";
            return false;
        }

        const auto& row = rows.front();
        userOut = UserRecord{ row[0], row[1], row[2], row[3], row[4] };
        return true;
    }

    static bool CanManageTarget(const UserRecord& actor, const UserRecord& target, std::wstring& errorOut)
    {
        const int actorRank = PositionRank(actor.position);
        const int targetRank = PositionRank(target.position);
        if (actorRank < 2) {
            errorOut = L"Only Managers, Supervisors and Administrators can manage users.";
            return false;
        }
        if (targetRank <= 0 || actorRank <= targetRank) {
            errorOut = L"You can only manage users below your current position.";
            return false;
        }
        return true;
    }

    bool IsUserMuted(OdbcConnection& db, const std::wstring& userId, std::wstring& errorOut)
    {
        auto rows = db.Query(
            L"SELECT COUNT(*) FROM users WHERE id = ? AND muted_until IS NOT NULL AND muted_until > CURRENT_TIMESTAMP",
            { userId },
            errorOut);
        if (!errorOut.empty())
            return false;
        return !rows.empty() && !rows.front().empty() && _wtoi(rows.front().front().c_str()) > 0;
    }

    void LogUserEvent(
        OdbcConnection& db,
        const UserRecord& actor,
        const UserRecord& target,
        const std::wstring& eventType,
        const std::wstring& details,
        std::wstring& errorOut)
    {
        db.Execute(
            L"INSERT INTO user_session_audit (event_type, target_user_id, username, display_name, position, pod, actor_user_id, actor_username, details, created_at) "
            L"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)",
            { eventType, target.id, target.username, target.displayName, target.position, target.pod, actor.id, actor.username, details },
            errorOut);
    }

    bool CheckPodInUse(OdbcConnection& db, const std::wstring& pod, bool& inUseOut, std::wstring& errorOut)
    {
        inUseOut = false;
        std::wstring sessionPod = TrimWide(pod);
        if (sessionPod.empty())
            return true;

        const std::wstring sql = std::wstring(
            L"SELECT COUNT(*) FROM user_sessions "
            L"WHERE session_pod = ? "
            L"AND expires_at > CURRENT_TIMESTAMP "
            L"AND COALESCE(last_seen_at, created_at) >= DATE_SUB(CURRENT_TIMESTAMP, INTERVAL ") +
            OnlineTimeoutSecondsText() +
            L" SECOND)";
        auto rows = db.Query(
            sql,
            { sessionPod },
            errorOut);
        if (!errorOut.empty())
            return false;
        if (!rows.empty() && !rows.front().empty())
            inUseOut = _wtoi(rows.front().front().c_str()) > 0;
        return true;
    }

    int OnlineTimeoutSeconds() const
    {
        return std::clamp(m_config.onlineTimeoutSeconds, 15, 3600);
    }

    std::wstring OnlineTimeoutSecondsText() const
    {
        return std::to_wstring(OnlineTimeoutSeconds());
    }

    bool PurgeStaleSessions(OdbcConnection& db, std::wstring& errorOut)
    {
        const std::wstring sql =
            std::wstring(L"DELETE FROM user_sessions "
                L"WHERE expires_at <= CURRENT_TIMESTAMP "
                L"OR COALESCE(last_seen_at, created_at) < DATE_SUB(CURRENT_TIMESTAMP, INTERVAL ") +
            OnlineTimeoutSecondsText() + L" SECOND)";
        return db.Execute(sql, {}, errorOut);
    }

    ServerConfig m_config;
    std::mutex m_sessionCreateMutex;
};

static std::string BearerToken(const HttpRequest& req)
{
    auto it = req.headers.find("authorization");
    if (it == req.headers.end())
        return {};
    std::string value = Trim(it->second);
    std::string lower = Lower(value);
    if (lower.rfind("bearer ", 0) != 0)
        return {};
    return Trim(value.substr(7));
}

static HttpResponse JsonResponse(int status, json body)
{
    HttpResponse response;
    response.status = status;
    response.reason = status == 200 ? "OK" : status == 201 ? "Created" : status == 401 ? "Unauthorized" : status == 404 ? "Not Found" : "Error";
    response.body = body.dump();
    return response;
}

static HttpResponse ErrorResponse(int status, const std::wstring& message, const char* code = nullptr)
{
    json body = { { "ok", false }, { "error", WideToUtf8(message) } };
    if (code && *code)
        body["code"] = code;
    return JsonResponse(status, body);
}

struct SourceBlob
{
    std::wstring name;
    std::wstring url;
    bool ok = false;
    std::string body;
    std::wstring error;
};

struct SourceBundle
{
    std::vector<SourceBlob> blobs;
    std::wstring status;
    std::chrono::steady_clock::time_point fetchedAt{};
};

struct SourceCacheEntry
{
    SourceBundle bundle;
    std::wstring sourceType;
    json options = json::object();
    uint32_t generation = 0;
};

static std::wstring WinErrorText(DWORD error)
{
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD length = FormatMessageW(flags, nullptr, error, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring message = length && buffer ? std::wstring(buffer, length) : L"error " + std::to_wstring(error);
    if (buffer)
        LocalFree(buffer);
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L'.' || std::iswspace(message.back())))
        message.pop_back();
    return message;
}

struct WinHttpHandle
{
    HINTERNET handle = nullptr;
    explicit WinHttpHandle(HINTERNET value = nullptr) : handle(value) {}
    ~WinHttpHandle()
    {
        if (handle)
            WinHttpCloseHandle(handle);
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    operator HINTERNET() const { return handle; }
};

static std::wstring NormalizeAbsoluteUrl(std::wstring url)
{
    url = TrimWide(std::move(url));
    if (url.empty())
        return {};
    std::wstring lower = ToLower(url);
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0)
        url = L"https://" + url;
    return url;
}

static bool ServerHttpGetText(
    const std::wstring& inputUrl,
    std::string& bodyOut,
    std::wstring& errorOut,
    DWORD timeoutMs = 15000)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeAbsoluteUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[2048]{};
    wchar_t path[8192]{};
    wchar_t extra[8192]{};
    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse source URL: " + WinErrorText(GetLastError());
        return false;
    }
    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// source URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring object(path, parts.dwUrlPathLength);
    object.append(extra, parts.dwExtraInfoLength);
    if (object.empty())
        object = L"/";

    WinHttpHandle session(WinHttpOpen(L"ERC Tools Server/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }
    WinHttpSetTimeouts(session, static_cast<int>(timeoutMs), static_cast<int>(timeoutMs), static_cast<int>(timeoutMs), static_cast<int>(timeoutMs));

    WinHttpHandle connect(WinHttpConnect(session, hostName.c_str(), parts.nPort, 0));
    if (!connect) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    WinHttpHandle request(WinHttpOpenRequest(connect, L"GET", object.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    WinHttpSetOption(request, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

    std::wstring headers =
        L"Accept: application/json, text/html, text/plain, */*\r\n"
        L"Accept-Encoding: identity\r\n"
        L"Cache-Control: no-cache\r\n";
    if (!WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr))
    {
        errorOut = L"Source request failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    if (status < 200 || status >= 300) {
        errorOut = L"Source returned HTTP " + std::to_wstring(status) + L".";
        return false;
    }

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }
        if (available == 0)
            break;

        if (bodyOut.size() + available > kMaxBinaryPayload) {
            errorOut = L"Source body is larger than the binary transport limit.";
            return false;
        }
        const size_t oldSize = bodyOut.size();
        bodyOut.resize(oldSize + available);
        DWORD read = 0;
        if (!WinHttpReadData(request, bodyOut.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }
        bodyOut.resize(oldSize + read);
    }

    return true;
}

static bool JsonBoolOption(const json& options, const char* key, bool fallback)
{
    auto it = options.find(key);
    if (it != options.end() && it->is_boolean())
        return it->get<bool>();
    return fallback;
}

static std::wstring JsonWideOption(const json& options, const char* key, const std::wstring& fallback = L"")
{
    auto it = options.find(key);
    if (it != options.end() && it->is_string())
        return Utf8ToWide(it->get<std::string>());
    return fallback;
}

static void AddFetchedBlob(
    std::vector<SourceBlob>& blobs,
    const std::wstring& name,
    const std::wstring& url,
    DWORD timeoutMs = 15000)
{
    SourceBlob blob;
    blob.name = name;
    blob.url = url;
    blob.ok = ServerHttpGetText(url, blob.body, blob.error, timeoutMs);
    blobs.push_back(std::move(blob));
}

static std::wstring ResolveRelativeSourceUrl(const std::wstring& baseUrl, const std::wstring& link)
{
    if (link.empty())
        return {};
    const std::wstring lower = ToLower(link);
    if (lower.rfind(L"http://", 0) == 0 || lower.rfind(L"https://", 0) == 0)
        return link;
    if (link.rfind(L"//", 0) == 0)
        return (ToLower(baseUrl).rfind(L"http://", 0) == 0 ? L"http:" : L"https:") + link;

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[2048]{};
    wchar_t path[8192]{};
    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    if (!WinHttpCrackUrl(baseUrl.c_str(), 0, 0, &parts))
        return link;

    std::wstring prefix = parts.nScheme == INTERNET_SCHEME_HTTP ? L"http://" : L"https://";
    prefix += std::wstring(host, parts.dwHostNameLength);
    if ((parts.nScheme == INTERNET_SCHEME_HTTP && parts.nPort != 80) ||
        (parts.nScheme == INTERNET_SCHEME_HTTPS && parts.nPort != 443))
    {
        prefix += L":" + std::to_wstring(parts.nPort);
    }
    if (!link.empty() && link.front() == L'/')
        return prefix + link;

    std::wstring basePath(path, parts.dwUrlPathLength);
    size_t slash = basePath.find_last_of(L'/');
    if (slash != std::wstring::npos)
        basePath.resize(slash + 1);
    else
        basePath = L"/";
    return prefix + basePath + link;
}

static std::vector<std::wstring> ExtractTrafficScotlandSids(const std::string& body)
{
    std::vector<std::wstring> sids;
    std::wstring html = Utf8ToWide(body);
    const std::wregex sidRegex(LR"erc(/more-details\?sid=([^&"'<>\s]+))erc", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), sidRegex), end; it != end; ++it) {
        std::wstring sid = (*it)[1].str();
        if (!sid.empty() && std::find(sids.begin(), sids.end(), sid) == sids.end())
            sids.push_back(std::move(sid));
    }
    return sids;
}

static std::vector<std::wstring> ExtractWeatherSystemDetailLinks(const std::string& body)
{
    std::vector<std::wstring> links;
    std::wstring html = Utf8ToWide(body);
    const std::wregex hrefRegex(LR"erc(href\s*=\s*(?:"([^"]+)"|'([^']+)'|([^>\s]+)))erc", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), hrefRegex), end; it != end; ++it) {
        std::wstring href = (*it)[1].matched ? (*it)[1].str() : ((*it)[2].matched ? (*it)[2].str() : (*it)[3].str());
        std::wstring lowerHref = ToLower(href);
        if (lowerHref.find(L".html") == std::wstring::npos ||
            lowerHref.find(L"main.html") != std::wstring::npos ||
            lowerHref.find(L"index") != std::wstring::npos)
        {
            continue;
        }
        if (std::find(links.begin(), links.end(), href) == links.end())
            links.push_back(std::move(href));
        if (links.size() >= 80)
            break;
    }
    return links;
}

static bool BuildSourceBundle(
    const std::wstring& sourceType,
    const json& options,
    const ServerConfig& config,
    SourceBundle& bundleOut,
    std::wstring& errorOut)
{
    bundleOut = {};
    bundleOut.fetchedAt = std::chrono::steady_clock::now();
    errorOut.clear();

    const std::wstring source = ToLower(TrimWide(sourceType));
    if (source == L"roads") {
        const bool unplannedOnly = JsonBoolOption(options, "unplannedOnly", true);
        if (!config.ntisEventSnapshotUrl.empty()) {
            std::wstring ntisUrl = config.ntisEventSnapshotUrl;
            ntisUrl += ntisUrl.find(L'?') == std::wstring::npos ? L"?" : L"&";
            ntisUrl += L"unplannedOnly=";
            ntisUrl += unplannedOnly ? L"1" : L"0";
            AddFetchedBlob(bundleOut.blobs, L"ntis_events", ntisUrl, 10000);
            if (!bundleOut.blobs.empty() && bundleOut.blobs.back().ok)
                bundleOut.status = L"National Highways NTIS Event Data.";
            else
                errorOut = L"National Highways NTIS event snapshot is unavailable.";
        }
        else {
            errorOut = L"National Highways NTIS event snapshot URL is not configured.";
        }

        if (JsonBoolOption(options, "trafficScotlandEnabled", false)) {
            const std::wstring scotlandUrl = NormalizeAbsoluteUrl(JsonWideOption(options, "trafficScotlandIncidentsUrl", L"https://www.traffic.gov.scot/traffic-information/incidents"));
            AddFetchedBlob(bundleOut.blobs, L"traffic_scotland_list", scotlandUrl);
            if (!bundleOut.blobs.empty() && bundleOut.blobs.back().ok) {
                std::vector<std::wstring> sids = ExtractTrafficScotlandSids(bundleOut.blobs.back().body);
                constexpr size_t maxScotlandDetails = 250;
                for (size_t i = 0; i < sids.size() && i < maxScotlandDetails; ++i) {
                    const std::wstring detailUrl = L"https://www.traffic.gov.scot/more-details?sid=" + sids[i] + L"&type=incidents";
                    AddFetchedBlob(bundleOut.blobs, L"traffic_scotland_detail:" + sids[i], detailUrl);
                }
            }
        }
    }
    else if (source == L"earthquakes") {
        AddFetchedBlob(bundleOut.blobs, L"earthquakes", L"https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=-82.94033&maxlatitude=82.9834&minlongitude=-180&maxlongitude=180&orderby=time&limit=20000");
    }
    else if (source == L"weather_systems") {
        const std::wstring mainUrl = L"https://www.tropicalstormrisk.com/tracker/dynamic/main.html";
        AddFetchedBlob(bundleOut.blobs, L"weather_systems_main", mainUrl);
        if (!bundleOut.blobs.empty() && bundleOut.blobs.back().ok) {
            std::vector<std::wstring> links = ExtractWeatherSystemDetailLinks(bundleOut.blobs.back().body);
            for (const std::wstring& link : links) {
                const std::wstring detailUrl = ResolveRelativeSourceUrl(mainUrl, link);
                if (!detailUrl.empty())
                    AddFetchedBlob(bundleOut.blobs, L"weather_system_detail:" + link, detailUrl);
            }
        }
    }
    else if (source == L"weather_warnings") {
        AddFetchedBlob(bundleOut.blobs, L"weather_warnings", NormalizeAbsoluteUrl(JsonWideOption(options, "url", L"https://weather.metoffice.gov.uk/warnings-and-advice/uk-warnings")));
    }
    else if (source == L"floods") {
        AddFetchedBlob(bundleOut.blobs, L"floods", NormalizeAbsoluteUrl(JsonWideOption(options, "url", L"https://environment.data.gov.uk/flood-monitoring/id/floods?_view=full")));
    }
    else {
        errorOut = L"Unknown source bundle type.";
        return false;
    }

    if (bundleOut.blobs.empty()) {
        if (errorOut.empty())
            errorOut = L"No source documents were fetched.";
        return false;
    }
    if (std::none_of(bundleOut.blobs.begin(), bundleOut.blobs.end(), [](const SourceBlob& blob) { return blob.ok; })) {
        if (errorOut.empty())
            errorOut = bundleOut.blobs.front().error.empty() ? L"Every source document failed to fetch." : bundleOut.blobs.front().error;
        return false;
    }
    return true;
}

static bool IsDatabaseErrorMessage(const std::wstring& message)
{
    return message.rfind(L"Database ", 0) == 0;
}

static bool IsSafeRelativePath(const std::filesystem::path& path)
{
    if (path.empty() || path.is_absolute())
        return false;
    for (const auto& part : path) {
        std::wstring text = part.wstring();
        if (text == L".." || text.find(L':') != std::wstring::npos)
            return false;
    }
    return true;
}

static int CompareVersionParts(const std::wstring& a, const std::wstring& b)
{
    auto parts = [](const std::wstring& v) {
        std::vector<int> out;
        std::wstring token;
        for (wchar_t ch : v) {
            if (ch == L'.') {
                out.push_back(token.empty() ? 0 : _wtoi(token.c_str()));
                token.clear();
            }
            else if (::iswdigit(ch)) {
                token.push_back(ch);
            }
        }
        out.push_back(token.empty() ? 0 : _wtoi(token.c_str()));
        return out;
    };
    auto av = parts(a);
    auto bv = parts(b);
    size_t n = std::max(av.size(), bv.size());
    av.resize(n);
    bv.resize(n);
    for (size_t i = 0; i < n; ++i) {
        if (av[i] != bv[i])
            return av[i] < bv[i] ? -1 : 1;
    }
    return 0;
}

static uint32_t ClampSourceFetchIntervalMs(uint64_t intervalMs)
{
    constexpr uint64_t minMs = 15ull * 1000ull;
    constexpr uint64_t maxMs = 60ull * 60ull * 1000ull;
    return static_cast<uint32_t>(std::clamp<uint64_t>(intervalMs, minMs, maxMs));
}

static uint32_t ClampNtisPollIntervalMs(uint64_t intervalMs)
{
    constexpr uint64_t minMs = 1000ull;
    constexpr uint64_t maxMs = 60ull * 1000ull;
    return static_cast<uint32_t>(std::clamp<uint64_t>(intervalMs, minMs, maxMs));
}

class ErcServer
{
public:
    explicit ErcServer(ServerConfig config) : m_config(std::move(config)), m_database(m_config)
    {
        m_sourceFetchIntervalMs.store(
            ClampSourceFetchIntervalMs(static_cast<uint64_t>(std::max(1, m_config.sourceFetchIntervalSeconds)) * 1000ull),
            std::memory_order_release);
        m_ntisPollIntervalMs.store(
            ClampNtisPollIntervalMs(static_cast<uint64_t>(std::max(1, m_config.ntisPollIntervalSeconds)) * 1000ull),
            std::memory_order_release);
        m_serveStaleSourceCache.store(m_config.serveStaleSourceCache, std::memory_order_release);
        m_sourceRefreshThread = std::thread([this]() { SourceRefreshLoop(); });
    }

    ~ErcServer()
    {
        m_sourceRefreshStop.store(true, std::memory_order_release);
        m_sourceRefreshCv.notify_all();
        if (m_sourceRefreshThread.joinable())
            m_sourceRefreshThread.join();
    }

    uint32_t SourceFetchIntervalMs() const
    {
        return m_sourceFetchIntervalMs.load(std::memory_order_acquire);
    }

    void SetSourceFetchIntervalMs(uint32_t intervalMs)
    {
        m_sourceFetchIntervalMs.store(ClampSourceFetchIntervalMs(intervalMs), std::memory_order_release);
        m_sourceRefreshCv.notify_all();
    }

    bool ServeStaleSourceCache() const
    {
        return m_serveStaleSourceCache.load(std::memory_order_acquire);
    }

    void SetServeStaleSourceCache(bool enabled)
    {
        m_serveStaleSourceCache.store(enabled, std::memory_order_release);
    }

    uint32_t NtisPollIntervalMs() const
    {
        return m_ntisPollIntervalMs.load(std::memory_order_acquire);
    }

    void SetNtisPollIntervalMs(uint32_t intervalMs)
    {
        m_ntisPollIntervalMs.store(ClampNtisPollIntervalMs(intervalMs), std::memory_order_release);
        m_sourceRefreshCv.notify_all();
    }

    bool HasNtisEventSnapshot() const
    {
        return !m_config.ntisEventSnapshotUrl.empty();
    }

    uint32_t SourceFetchIntervalMsFor(const std::wstring& sourceType) const
    {
        const uint32_t general = SourceFetchIntervalMs();
        if (ToLower(TrimWide(sourceType)) == L"roads" && HasNtisEventSnapshot())
            return NtisPollIntervalMs();
        return general;
    }

    size_t SourceCacheEntryCount()
    {
        std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
        return m_sourceCache.size();
    }

    void ClearSourceCache()
    {
        std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
        m_sourceCache.clear();
        m_sourceCacheChanged.notify_all();
    }

    BinaryResponse HandleBinary(uint16_t opcode, const std::vector<unsigned char>& payload)
    {
        try {
            BinaryReader reader(payload);
            if (opcode == kBinaryLogin)
                return HandleBinaryLogin(reader);

            std::wstring token;
            if (!reader.Text(token))
                return BinaryError(opcode, 400, "invalid_payload", L"Binary request is missing the session token.");

            if (opcode == kBinaryLogout) {
                std::wstring reason;
                reader.Text(reason);
                std::wstring error;
                if (!m_database.DeleteSession(WideToUtf8(token), error))
                    return BinaryError(opcode, error.rfind(L"Missing bearer token", 0) == 0 ? 401 : 500, IsDatabaseErrorMessage(error) ? "database_error" : "logout_failed", error);
                NotifyCollaborationChanged();
                if (!reason.empty())
                    std::wcout << L"Client session ended: " << reason << L"\n";
                return BinaryOk(opcode);
            }

            UserRecord user;
            std::wstring authError;
            if (!m_database.Authenticate(WideToUtf8(token), user, authError))
                return BinaryError(opcode, IsDatabaseErrorMessage(authError) ? 500 : 401, IsDatabaseErrorMessage(authError) ? "database_error" : "session_invalid", authError);

            switch (opcode) {
            case kBinaryPoll:
                return HandleBinaryPoll(opcode, reader, user);
            case kBinarySendChat:
                return HandleBinarySendChat(opcode, reader, user);
            case kBinaryClearChat:
                return HandleBinaryClearChat(opcode, user);
            case kBinaryDeleteChatMessage:
                return HandleBinaryDeleteChatMessage(opcode, reader, user);
            case kBinaryKickUser:
                return HandleBinaryKickUser(opcode, reader, user);
            case kBinaryMuteUser:
                return HandleBinaryMuteUser(opcode, reader, user);
            case kBinarySendPrivateMessage:
                return HandleBinarySendPrivateMessage(opcode, reader, user);
            case kBinaryAddIncidentExclusion:
                return HandleBinaryAddIncidentExclusion(opcode, reader, user);
            case kBinaryRemoveIncidentExclusion:
                return HandleBinaryRemoveIncidentExclusion(opcode, reader, user);
            case kBinaryCreateNote:
                return HandleBinaryCreateNote(opcode, reader, user);
            case kBinaryUpdateNote:
                return HandleBinaryUpdateNote(opcode, reader, user);
            case kBinaryDeleteNote:
                return HandleBinaryDeleteNote(opcode, reader, user);
            case kBinaryGetSettings:
                return HandleBinaryGetSettings(opcode);
            case kBinarySetSettings:
                return HandleBinarySetSettings(opcode, reader, user);
            case kBinaryFetchSourceBundle:
                return HandleBinaryFetchSourceBundle(opcode, reader, user);
            case kBinaryWaitSourceBundle:
                return HandleBinaryWaitSourceBundle(opcode, reader, user);
            case kBinaryCreateAccount:
                return HandleBinaryCreateAccount(opcode, reader, user);
            default:
                return BinaryError(opcode, 404, "unknown_opcode", L"Unknown ERC binary opcode.");
            }
        }
        catch (const std::exception& e) {
            return BinaryError(opcode, 500, "server_error", L"Binary server error: " + Utf8ToWide(e.what()));
        }
    }

    HttpResponse Handle(const HttpRequest& req)
    {
        try {
            if (req.method == "OPTIONS")
                return JsonResponse(200, { { "ok", true } });
            if (req.method == "GET" && req.path == "/health")
                return JsonResponse(200, { { "ok", true }, { "service", "erc-tools-server" } });
            if (req.method == "POST" && req.path == "/api/auth/login")
                return HandleLogin(req);
            if (req.method == "POST" && req.path == "/api/auth/logout")
                return HandleLogout(req);

            UserRecord user;
            std::wstring authError;
            if (!m_database.Authenticate(BearerToken(req), user, authError))
                return ErrorResponse(IsDatabaseErrorMessage(authError) ? 500 : 401, authError, IsDatabaseErrorMessage(authError) ? "database_error" : "session_invalid");

            if (req.method == "GET" && req.path == "/api/users/online")
                return HandleOnlineUsers();
            if (req.method == "POST" && req.path.rfind("/api/users/", 0) == 0 && req.path.size() > std::string("/api/users/").size())
                return HandleUserAction(req, user);
            if (req.method == "POST" && req.path == "/api/users")
                return HandleCreateAccount(req, user);
            if (req.method == "GET" && req.path == "/api/admin/logs")
                return HandleAdminLogs(req, user);
            if (req.method == "DELETE" && req.path.rfind("/api/admin/logs/", 0) == 0)
                return HandleClearAdminLogs(req, user);
            if (req.method == "GET" && req.path == "/api/incidents/exclusions")
                return HandleGetIncidentExclusions();
            if (req.method == "POST" && req.path == "/api/incidents/exclusions")
                return HandleAddIncidentExclusion(req, user);
            if (req.method == "POST" && req.path == "/api/incidents/exclusions/remove")
                return HandleRemoveIncidentExclusion(req, user);
            if (req.method == "GET" && req.path == "/api/chat")
                return HandleGetChat();
            if (req.method == "POST" && req.path == "/api/chat")
                return HandlePostChat(req, user);
            if (req.method == "DELETE" && req.path.rfind("/api/chat/", 0) == 0)
                return HandleDeleteChatMessage(req, user);
            if (req.method == "DELETE" && req.path == "/api/chat")
                return HandleClearChat(user);
            if (req.method == "GET" && req.path == "/api/private-messages")
                return HandleGetPrivateMessages(user);
            if (req.method == "POST" && req.path == "/api/private-messages")
                return HandlePostPrivateMessage(req, user);
            if (req.method == "GET" && req.path == "/api/notes")
                return HandleGetNotes();
            if (req.method == "POST" && req.path == "/api/notes")
                return HandlePostNote(req, user);
            if ((req.method == "PUT" || req.method == "PATCH") && req.path.rfind("/api/notes/", 0) == 0)
                return HandleUpdateNote(req, user);
            if (req.method == "DELETE" && req.path.rfind("/api/notes/", 0) == 0)
                return HandleDeleteNote(req, user);
            if (req.method == "GET" && req.path == "/api/settings/global")
                return HandleGetGlobalSettings();
            if ((req.method == "POST" || req.method == "PUT" || req.method == "PATCH") && req.path == "/api/settings/global")
                return HandleSetGlobalSettings(req, user);
            if (req.method == "GET" && req.path == "/api/updates/manifest")
                return HandleUpdateManifest(req);
            if (req.method == "GET" && req.path.rfind("/api/updates/files/", 0) == 0)
                return HandleUpdateFile(req);
            if (req.method == "POST" && req.path == "/api/updates/report")
                return JsonResponse(200, { { "ok", true } });

            return ErrorResponse(404, L"Endpoint not found.");
        }
        catch (const std::exception& e) {
            return ErrorResponse(500, L"Server error: " + Utf8ToWide(e.what()));
        }
    }

private:
    static BinaryResponse BinaryError(uint16_t opcode, uint16_t status, const char* code, const std::wstring& message)
    {
        BinaryWriter writer;
        writer.Text(code ? Utf8ToWide(code) : L"error");
        writer.Text(message);
        return BinaryResponse{ opcode, status, writer.Data() };
    }

    static BinaryResponse BinaryOk(uint16_t opcode)
    {
        return BinaryResponse{ opcode, 0, {} };
    }

    static BinaryResponse BinaryOk(uint16_t opcode, const BinaryWriter& writer)
    {
        return BinaryResponse{ opcode, 0, writer.Data() };
    }

    static std::wstring JsonTextField(const json& item, std::initializer_list<const char*> keys)
    {
        return PickWide(item, keys);
    }

    static double JsonDoubleField(const json& item, const char* primary, const char* fallback = nullptr)
    {
        if (item.is_object()) {
            auto it = item.find(primary);
            if (it != item.end() && it->is_number())
                return it->get<double>();
            if (fallback) {
                it = item.find(fallback);
                if (it != item.end() && it->is_number())
                    return it->get<double>();
            }
        }
        return 0.0;
    }

    static void WriteChatJson(BinaryWriter& writer, const json& item)
    {
        writer.Text(JsonTextField(item, { "id", "messageId" }));
        writer.Text(JsonTextField(item, { "author", "user", "name" }));
        writer.Text(JsonTextField(item, { "username", "login" }));
        writer.Text(JsonTextField(item, { "position", "role" }));
        writer.Text(JsonTextField(item, { "text", "message", "body" }));
        writer.Text(JsonTextField(item, { "timestamp", "time", "createdAt" }));
    }

    static void WriteNoteJson(BinaryWriter& writer, const json& item)
    {
        writer.Text(JsonTextField(item, { "id", "noteId" }));
        writer.Text(JsonTextField(item, { "author", "user", "name" }));
        writer.Text(JsonTextField(item, { "text", "note", "body" }));
        writer.Text(JsonTextField(item, { "timestamp", "time", "createdAt" }));
        writer.F64(JsonDoubleField(item, "lat", "latitude"));
        writer.F64(JsonDoubleField(item, "lon", "longitude"));
    }

    static void WriteOnlineUserJson(BinaryWriter& writer, const json& item)
    {
        writer.Text(JsonTextField(item, { "id", "userId" }));
        writer.Text(JsonTextField(item, { "displayName", "display_name", "name" }));
        writer.Text(JsonTextField(item, { "username", "user" }));
        writer.Text(JsonTextField(item, { "position", "role" }));
        writer.Text(JsonTextField(item, { "pod" }));
        writer.Text(JsonTextField(item, { "lastSeen", "last_seen", "timestamp" }));
    }

    static void WritePrivateMessageJson(BinaryWriter& writer, const json& item)
    {
        writer.Text(JsonTextField(item, { "id", "messageId" }));
        writer.Text(JsonTextField(item, { "senderUsername", "sender_username" }));
        writer.Text(JsonTextField(item, { "senderDisplayName", "sender_display_name" }));
        writer.Text(JsonTextField(item, { "senderPosition", "sender_position" }));
        writer.Text(JsonTextField(item, { "recipientUsername", "recipient_username" }));
        writer.Text(JsonTextField(item, { "recipientDisplayName", "recipient_display_name" }));
        writer.Text(JsonTextField(item, { "recipientPosition", "recipient_position" }));
        writer.Text(JsonTextField(item, { "text", "message", "body" }));
        writer.Text(JsonTextField(item, { "timestamp", "time", "createdAt" }));
    }

    static void WriteIncidentExclusionJson(BinaryWriter& writer, const json& item)
    {
        writer.Text(JsonTextField(item, { "key", "incidentKey" }));
        writer.Text(JsonTextField(item, { "sourceId", "source_id" }));
        writer.Text(JsonTextField(item, { "source", "sourceName" }));
        writer.Text(JsonTextField(item, { "road" }));
        writer.Text(JsonTextField(item, { "summary", "title" }));
        writer.Text(JsonTextField(item, { "addedBy", "added_by" }));
        writer.Text(JsonTextField(item, { "addedAt", "added_at" }));
    }

    uint32_t CollaborationVersion() const
    {
        return m_collaborationVersion.load(std::memory_order_acquire);
    }

    void NotifyCollaborationChanged()
    {
        m_collaborationVersion.fetch_add(1, std::memory_order_acq_rel);
        m_collaborationChanged.notify_all();
    }

    void WaitForCollaborationChange(uint32_t knownVersion)
    {
        if (knownVersion == 0 || knownVersion != CollaborationVersion())
            return;

        std::unique_lock<std::mutex> lock(m_collaborationMutex);
        m_collaborationChanged.wait_for(lock, std::chrono::seconds(25), [&]() {
            return knownVersion != CollaborationVersion();
            });
    }

    static std::wstring SourceCacheKey(const std::wstring& sourceType, const json& options)
    {
        return ToLower(TrimWide(sourceType)) + L"\n" + Utf8ToWide(options.dump());
    }

    static uint32_t SourceBundleAgeMs(const SourceBundle& bundle)
    {
        const uint64_t age = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - bundle.fetchedAt).count());
        return static_cast<uint32_t>(std::min<uint64_t>(age, std::numeric_limits<uint32_t>::max()));
    }

    static bool SourceBundlePayloadMatches(const SourceBundle& left, const SourceBundle& right)
    {
        if (left.status != right.status || left.blobs.size() != right.blobs.size())
            return false;
        for (size_t i = 0; i < left.blobs.size(); ++i) {
            const SourceBlob& a = left.blobs[i];
            const SourceBlob& b = right.blobs[i];
            if (a.name != b.name || a.url != b.url || a.ok != b.ok ||
                a.body != b.body || a.error != b.error)
            {
                return false;
            }
        }
        return true;
    }

    uint32_t NextSourceCacheGenerationLocked()
    {
        uint32_t generation = m_nextSourceCacheGeneration++;
        if (generation == 0)
            generation = m_nextSourceCacheGeneration++;
        if (m_nextSourceCacheGeneration == 0)
            m_nextSourceCacheGeneration = 1;
        return generation;
    }

    void StoreSourceCacheEntryLocked(
        const std::wstring& key,
        SourceBundle fresh,
        const std::wstring& sourceType,
        const json& options,
        uint32_t* generationOut = nullptr)
    {
        SourceCacheEntry entry;
        entry.bundle = std::move(fresh);
        entry.sourceType = sourceType;
        entry.options = options;
        entry.generation = NextSourceCacheGenerationLocked();
        if (generationOut)
            *generationOut = entry.generation;
        m_sourceCache[key] = std::move(entry);
    }

    void RefreshDueSourceCacheEntries()
    {
        struct RefreshTarget
        {
            std::wstring key;
            std::wstring sourceType;
            json options;
        };

        std::vector<RefreshTarget> targets;
        {
            std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
            for (const auto& [key, entry] : m_sourceCache) {
                const uint32_t ttlMs = SourceFetchIntervalMsFor(entry.sourceType);
                if (entry.sourceType.empty() ||
                    SourceBundleAgeMs(entry.bundle) < ttlMs ||
                    m_sourceCacheInFlight.find(key) != m_sourceCacheInFlight.end())
                {
                    continue;
                }
                m_sourceCacheInFlight.insert(key);
                targets.push_back({ key, entry.sourceType, entry.options });
            }
        }

        for (const RefreshTarget& target : targets) {
            SourceBundle fresh;
            std::wstring error;
            const bool fetched = BuildSourceBundle(target.sourceType, target.options, m_config, fresh, error);
            if (fetched)
                fresh.fetchedAt = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
                if (fetched) {
                    auto existing = m_sourceCache.find(target.key);
                    if (existing != m_sourceCache.end() &&
                        SourceBundlePayloadMatches(existing->second.bundle, fresh))
                    {
                        existing->second.bundle = std::move(fresh);
                    }
                    else {
                        StoreSourceCacheEntryLocked(target.key, std::move(fresh), target.sourceType, target.options);
                    }
                }
                m_sourceCacheInFlight.erase(target.key);
            }
            m_sourceCacheChanged.notify_all();
        }
    }

    void SourceRefreshLoop()
    {
        while (!m_sourceRefreshStop.load(std::memory_order_acquire)) {
            const uint32_t intervalMs = HasNtisEventSnapshot()
                ? std::min(SourceFetchIntervalMs(), NtisPollIntervalMs())
                : SourceFetchIntervalMs();
            const auto waitFor = std::chrono::milliseconds(intervalMs);
            std::unique_lock<std::mutex> lock(m_sourceRefreshMutex);
            m_sourceRefreshCv.wait_for(lock, waitFor, [&]() {
                return m_sourceRefreshStop.load(std::memory_order_acquire);
                });
            lock.unlock();
            if (m_sourceRefreshStop.load(std::memory_order_acquire))
                break;
            RefreshDueSourceCacheEntries();
        }
    }

    bool GetCachedSourceBundle(
        const std::wstring& sourceType,
        uint32_t requestedIntervalMs,
        const json& options,
        SourceBundle& bundleOut,
        bool& fromCacheOut,
        uint32_t& ageMsOut,
        uint32_t& generationOut,
        std::wstring& errorOut)
    {
        bundleOut = {};
        fromCacheOut = false;
        ageMsOut = 0;
        generationOut = 0;
        errorOut.clear();

        (void)requestedIntervalMs;
        const uint32_t ttlMs = SourceFetchIntervalMsFor(sourceType);
        const std::wstring key = SourceCacheKey(sourceType, options);
        {
            std::unique_lock<std::mutex> lock(m_sourceCacheMutex);
            auto cached = m_sourceCache.find(key);
            if (cached != m_sourceCache.end()) {
                const uint32_t cachedAge = SourceBundleAgeMs(cached->second.bundle);
                if (cachedAge <= ttlMs) {
                    bundleOut = cached->second.bundle;
                    fromCacheOut = true;
                    ageMsOut = cachedAge;
                    generationOut = cached->second.generation;
                    return true;
                }
            }

            if (m_sourceCacheInFlight.find(key) != m_sourceCacheInFlight.end()) {
                if (cached != m_sourceCache.end() && ServeStaleSourceCache()) {
                    bundleOut = cached->second.bundle;
                    fromCacheOut = true;
                    ageMsOut = SourceBundleAgeMs(cached->second.bundle);
                    generationOut = cached->second.generation;
                    if (bundleOut.status.empty())
                        bundleOut.status = L"Served cache while a refresh is already running.";
                    else
                        bundleOut.status += L" Served cache while a refresh is already running.";
                    return true;
                }

                const bool completed = m_sourceCacheChanged.wait_for(
                    lock,
                    std::chrono::seconds(60),
                    [&]() { return m_sourceCacheInFlight.find(key) == m_sourceCacheInFlight.end(); });
                cached = m_sourceCache.find(key);
                if (cached != m_sourceCache.end()) {
                    bundleOut = cached->second.bundle;
                    fromCacheOut = true;
                    ageMsOut = SourceBundleAgeMs(cached->second.bundle);
                    generationOut = cached->second.generation;
                    return true;
                }
                if (!completed) {
                    errorOut = L"Timed out waiting for server source cache refresh.";
                    return false;
                }
            }

            m_sourceCacheInFlight.insert(key);
        }

        SourceBundle fresh;
        const bool fetched = BuildSourceBundle(sourceType, options, m_config, fresh, errorOut);
        if (fetched)
            fresh.fetchedAt = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
            if (fetched) {
                StoreSourceCacheEntryLocked(key, std::move(fresh), sourceType, options, &generationOut);
                bundleOut = m_sourceCache[key].bundle;
            }
            m_sourceCacheInFlight.erase(key);
        }
        m_sourceCacheChanged.notify_all();

        if (fetched)
            return true;

        {
            std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
            auto cached = m_sourceCache.find(key);
            if (cached != m_sourceCache.end() && ServeStaleSourceCache()) {
                bundleOut = cached->second.bundle;
                fromCacheOut = true;
                ageMsOut = SourceBundleAgeMs(cached->second.bundle);
                generationOut = cached->second.generation;
                if (!errorOut.empty())
                    bundleOut.status += L" Served stale cache after fetch failure: " + errorOut;
                return true;
            }
        }

        if (errorOut.empty())
            errorOut = L"Could not fetch source bundle.";
        return false;
    }

    bool EnsureSourceCacheEntry(const std::wstring& sourceType, const json& options, std::wstring& errorOut)
    {
        errorOut.clear();
        const std::wstring key = SourceCacheKey(sourceType, options);
        {
            std::unique_lock<std::mutex> lock(m_sourceCacheMutex);
            if (m_sourceCache.find(key) != m_sourceCache.end())
                return true;

            if (m_sourceCacheInFlight.find(key) != m_sourceCacheInFlight.end()) {
                const bool completed = m_sourceCacheChanged.wait_for(
                    lock,
                    std::chrono::seconds(60),
                    [&]() {
                        return m_sourceCacheInFlight.find(key) == m_sourceCacheInFlight.end() ||
                            m_sourceCache.find(key) != m_sourceCache.end();
                    });
                if (m_sourceCache.find(key) != m_sourceCache.end())
                    return true;
                if (!completed) {
                    errorOut = L"Timed out waiting for server source cache seed.";
                    return false;
                }
            }

            m_sourceCacheInFlight.insert(key);
        }

        SourceBundle fresh;
        const bool fetched = BuildSourceBundle(sourceType, options, m_config, fresh, errorOut);
        if (fetched)
            fresh.fetchedAt = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(m_sourceCacheMutex);
            if (fetched)
                StoreSourceCacheEntryLocked(key, std::move(fresh), sourceType, options);
            m_sourceCacheInFlight.erase(key);
        }
        m_sourceCacheChanged.notify_all();

        if (!fetched && errorOut.empty())
            errorOut = L"Could not seed server source cache.";
        return fetched;
    }

    bool WaitForSourceBundleChange(
        const std::wstring& sourceType,
        uint32_t knownGeneration,
        uint32_t waitTimeoutMs,
        const json& options,
        SourceBundle& bundleOut,
        bool& changedOut,
        bool& fromCacheOut,
        uint32_t& ageMsOut,
        uint32_t& generationOut,
        std::wstring& errorOut)
    {
        bundleOut = {};
        changedOut = false;
        fromCacheOut = true;
        ageMsOut = 0;
        generationOut = knownGeneration;
        errorOut.clear();

        if (!EnsureSourceCacheEntry(sourceType, options, errorOut))
            return false;

        const std::wstring key = SourceCacheKey(sourceType, options);
        const uint32_t clampedWaitMs = std::clamp<uint32_t>(waitTimeoutMs, 1000u, 65000u);

        std::unique_lock<std::mutex> lock(m_sourceCacheMutex);
        auto cached = m_sourceCache.find(key);
        if (cached == m_sourceCache.end()) {
            generationOut = knownGeneration;
            return true;
        }

        if (knownGeneration == 0 || cached->second.generation != knownGeneration) {
            bundleOut = cached->second.bundle;
            changedOut = true;
            generationOut = cached->second.generation;
            ageMsOut = SourceBundleAgeMs(cached->second.bundle);
            return true;
        }

        m_sourceCacheChanged.wait_for(
            lock,
            std::chrono::milliseconds(clampedWaitMs),
            [&]() {
                auto latest = m_sourceCache.find(key);
                return latest == m_sourceCache.end() || latest->second.generation != knownGeneration;
            });

        cached = m_sourceCache.find(key);
        if (cached == m_sourceCache.end()) {
            generationOut = knownGeneration;
            return true;
        }

        generationOut = cached->second.generation;
        ageMsOut = SourceBundleAgeMs(cached->second.bundle);
        if (cached->second.generation != knownGeneration) {
            bundleOut = cached->second.bundle;
            changedOut = true;
        }
        return true;
    }

    BinaryResponse HandleBinaryFetchSourceBundle(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        (void)user;

        std::wstring sourceType;
        uint32_t requestedIntervalMs = 0;
        json options = json::object();
        if (!reader.Text(sourceType) ||
            !reader.U32(requestedIntervalMs) ||
            !reader.Json(options))
        {
            return BinaryError(opcode, 400, "invalid_payload", L"Binary source-bundle payload is incomplete.");
        }
        if (!options.is_object())
            options = json::object();

        SourceBundle bundle;
        bool fromCache = false;
        uint32_t ageMs = 0;
        uint32_t generation = 0;
        std::wstring error;
        if (!GetCachedSourceBundle(sourceType, requestedIntervalMs, options, bundle, fromCache, ageMs, generation, error))
            return BinaryError(opcode, 502, "source_fetch_failed", error);

        BinaryWriter writer;
        writer.U32(ageMs);
        writer.U32(fromCache ? 1u : 0u);
        writer.U32(static_cast<uint32_t>(bundle.blobs.size()));
        for (const SourceBlob& blob : bundle.blobs) {
            writer.Text(blob.name);
            writer.Text(blob.url);
            writer.U32(blob.ok ? 1u : 0u);
            writer.Bytes(blob.body);
            writer.Text(blob.error);
        }
        writer.U32(generation);
        return BinaryOk(opcode, writer);
    }

    BinaryResponse HandleBinaryWaitSourceBundle(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        (void)user;

        std::wstring sourceType;
        uint32_t knownGeneration = 0;
        uint32_t waitTimeoutMs = 0;
        json options = json::object();
        if (!reader.Text(sourceType) ||
            !reader.U32(knownGeneration) ||
            !reader.U32(waitTimeoutMs) ||
            !reader.Json(options))
        {
            return BinaryError(opcode, 400, "invalid_payload", L"Binary source-wait payload is incomplete.");
        }
        if (!options.is_object())
            options = json::object();

        SourceBundle bundle;
        bool changed = false;
        bool fromCache = true;
        uint32_t ageMs = 0;
        uint32_t generation = knownGeneration;
        std::wstring error;
        if (!WaitForSourceBundleChange(
            sourceType,
            knownGeneration,
            waitTimeoutMs,
            options,
            bundle,
            changed,
            fromCache,
            ageMs,
            generation,
            error))
        {
            return BinaryError(opcode, 502, "source_wait_failed", error);
        }

        BinaryWriter writer;
        writer.U32(generation);
        writer.U32(changed ? 1u : 0u);
        writer.U32(ageMs);
        writer.U32(fromCache ? 1u : 0u);
        writer.U32(changed ? static_cast<uint32_t>(bundle.blobs.size()) : 0u);
        if (changed) {
            for (const SourceBlob& blob : bundle.blobs) {
                writer.Text(blob.name);
                writer.Text(blob.url);
                writer.U32(blob.ok ? 1u : 0u);
                writer.Bytes(blob.body);
                writer.Text(blob.error);
            }
        }
        return BinaryOk(opcode, writer);
    }

    BinaryResponse HandleBinaryLogin(BinaryReader& reader)
    {
        std::wstring username;
        std::wstring password;
        std::wstring position;
        std::wstring pod;
        std::wstring clientVersion;
        std::wstring platform;
        if (!reader.Text(username) ||
            !reader.Text(password) ||
            !reader.Text(position) ||
            !reader.Text(pod) ||
            !reader.Text(clientVersion) ||
            !reader.Text(platform))
        {
            return BinaryError(kBinaryLogin, 400, "invalid_payload", L"Binary login payload is incomplete.");
        }

        if (username.empty() || password.empty() || position.empty() || pod.empty())
            return BinaryError(kBinaryLogin, 400, "missing_fields", L"Username, password, position and pod are required.");

        UserRecord user;
        std::wstring error;
        std::string code;
        if (!m_database.ValidateLogin(username, password, position, pod, user, error, code))
            return BinaryError(kBinaryLogin, IsDatabaseErrorMessage(error) ? 500 : 401, code.empty() ? "login_failed" : code.c_str(), error);

        std::wstring token;
        if (!m_database.CreateSession(user, token, error)) {
            if (error == L"Selected pod is already in use.")
                return BinaryError(kBinaryLogin, 401, "pod_in_use", error);
            return BinaryError(kBinaryLogin, IsDatabaseErrorMessage(error) ? 500 : 500, IsDatabaseErrorMessage(error) ? "database_error" : "session_create_failed", error);
        }
        NotifyCollaborationChanged();

        BinaryWriter writer;
        writer.Text(token);
        writer.Text(user.username);
        writer.Text(user.displayName);
        writer.Text(user.position);
        writer.Text(user.pod);
        return BinaryOk(kBinaryLogin, writer);
    }

    BinaryResponse HandleBinaryPoll(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        uint32_t knownVersion = 0;
        reader.U32(knownVersion);
        WaitForCollaborationChange(knownVersion);

        std::wstring error;
        json chat = m_database.ChatMessages(error);
        if (!error.empty())
            return BinaryError(opcode, 500, "database_error", L"Chat poll failed: " + error);

        json notes = m_database.Notes(error);
        if (!error.empty())
            return BinaryError(opcode, 500, "database_error", L"Notes poll failed: " + error);

        json users = m_database.OnlineUsers(error);
        if (!error.empty())
            return BinaryError(opcode, 500, "database_error", L"Online users poll failed: " + error);

        json privateMessages = m_database.PrivateMessages(user, error);
        if (!error.empty())
            return BinaryError(opcode, 500, "database_error", L"Private message poll failed: " + error);

        json incidentExclusions = m_database.IncidentExclusions(error);
        if (!error.empty())
            return BinaryError(opcode, 500, "database_error", L"Incident exclusions poll failed: " + error);

        BinaryWriter writer;
        writer.U32(chat.is_array() ? static_cast<uint32_t>(chat.size()) : 0);
        if (chat.is_array()) {
            for (const auto& item : chat)
                WriteChatJson(writer, item);
        }

        writer.U32(notes.is_array() ? static_cast<uint32_t>(notes.size()) : 0);
        if (notes.is_array()) {
            for (const auto& item : notes)
                WriteNoteJson(writer, item);
        }

        writer.U32(users.is_array() ? static_cast<uint32_t>(users.size()) : 0);
        if (users.is_array()) {
            for (const auto& item : users)
                WriteOnlineUserJson(writer, item);
        }

        writer.U32(privateMessages.is_array() ? static_cast<uint32_t>(privateMessages.size()) : 0);
        if (privateMessages.is_array()) {
            for (const auto& item : privateMessages)
                WritePrivateMessageJson(writer, item);
        }
        // Preserve the original version field position for rolling client/server updates.
        writer.U32(CollaborationVersion());
        writer.U32(incidentExclusions.is_array() ? static_cast<uint32_t>(incidentExclusions.size()) : 0);
        if (incidentExclusions.is_array()) {
            for (const auto& item : incidentExclusions)
                WriteIncidentExclusionJson(writer, item);
        }
        return BinaryOk(opcode, writer);
    }

    BinaryResponse HandleBinarySendChat(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring text;
        if (!reader.Text(text))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary chat payload is incomplete.");
        text = TrimWide(text);
        if (text.empty())
            return BinaryError(opcode, 400, "missing_text", L"Chat message is empty.");

        std::wstring error;
        if (!m_database.AddChatMessage(user, text, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryClearChat(uint16_t opcode, const UserRecord& user)
    {
        if (!CanEditGlobalSettings(user))
            return BinaryError(opcode, 403, "forbidden", L"Only Administrators and Supervisors can clear responder chat.");

        std::wstring error;
        if (!m_database.ClearChatMessages(error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryDeleteChatMessage(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring messageId;
        if (!reader.Text(messageId))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary delete-chat payload is incomplete.");

        std::wstring error;
        if (!m_database.DeleteChatMessage(user, messageId, error))
            return BinaryError(opcode, IsDatabaseErrorMessage(error) ? 500 : 403, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryKickUser(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring username;
        if (!reader.Text(username))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary kick payload is incomplete.");

        std::wstring error;
        if (!m_database.KickUser(user, username, error))
            return BinaryError(opcode, IsDatabaseErrorMessage(error) ? 500 : 403, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryMuteUser(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring username;
        uint32_t minutes = 15;
        if (!reader.Text(username) || !reader.U32(minutes))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary mute payload is incomplete.");

        std::wstring error;
        if (!m_database.MuteUser(user, username, minutes, error))
            return BinaryError(opcode, IsDatabaseErrorMessage(error) ? 500 : 403, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinarySendPrivateMessage(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring recipientUsername;
        std::wstring text;
        if (!reader.Text(recipientUsername) || !reader.Text(text))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary private-message payload is incomplete.");

        std::wstring error;
        if (!m_database.AddPrivateMessage(user, recipientUsername, text, error))
            return BinaryError(opcode, IsDatabaseErrorMessage(error) ? 500 : 400, IsDatabaseErrorMessage(error) ? "database_error" : "private_message_failed", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryAddIncidentExclusion(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring key;
        std::wstring sourceId;
        std::wstring source;
        std::wstring road;
        std::wstring summary;
        if (!reader.Text(key) ||
            !reader.Text(sourceId) ||
            !reader.Text(source) ||
            !reader.Text(road) ||
            !reader.Text(summary))
        {
            return BinaryError(opcode, 400, "invalid_payload", L"Binary incident-exclusion payload is incomplete.");
        }

        std::wstring error;
        if (!m_database.AddIncidentExclusion(user, key, sourceId, source, road, summary, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryRemoveIncidentExclusion(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring key;
        if (!reader.Text(key))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary remove-exclusion payload is incomplete.");

        std::wstring error;
        if (!m_database.RemoveIncidentExclusion(user, key, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryCreateAccount(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring username;
        std::wstring displayName;
        std::wstring password;
        std::wstring position;
        uint32_t active = 1;
        if (!reader.Text(username) ||
            !reader.Text(displayName) ||
            !reader.Text(password) ||
            !reader.Text(position) ||
            !reader.U32(active))
        {
            return BinaryError(opcode, 400, "invalid_payload", L"Binary account creation payload is incomplete.");
        }

        if (!CanEditGlobalSettings(user))
            return BinaryError(opcode, 403, "forbidden", L"Only Administrators and Supervisors can create accounts.");

        if (PositionRank(position) > PositionRank(user.position))
            return BinaryError(opcode, 403, "position_not_allowed", L"You cannot create an account above your current position.");

        std::wstring error;
        std::string code;
        if (!m_database.CreateOrUpdateUser(username, displayName, password, position, active != 0, error, code))
            return BinaryError(opcode, IsDatabaseErrorMessage(error) ? 500 : 400, code.empty() ? "account_create_failed" : code.c_str(), error);

        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryCreateNote(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring text;
        double lat = 0.0;
        double lon = 0.0;
        if (!reader.Text(text) || !reader.F64(lat) || !reader.F64(lon))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary note payload is incomplete.");

        json body = {
            { "text", WideToUtf8(text) },
            { "lat", lat },
            { "lon", lon }
        };

        json note;
        std::wstring error;
        if (!m_database.AddNote(user, body, note, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();

        BinaryWriter writer;
        WriteNoteJson(writer, note);
        return BinaryOk(opcode, writer);
    }

    BinaryResponse HandleBinaryUpdateNote(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring id;
        std::wstring text;
        double lat = 0.0;
        double lon = 0.0;
        if (!reader.Text(id) || !reader.Text(text) || !reader.F64(lat) || !reader.F64(lon))
            return BinaryError(opcode, 400, "invalid_payload", L"Binary note update payload is incomplete.");

        json body = {
            { "text", WideToUtf8(text) },
            { "lat", lat },
            { "lon", lon }
        };

        std::wstring error;
        if (!m_database.UpdateNote(user, id, body, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    BinaryResponse HandleBinaryDeleteNote(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        std::wstring id;
        if (!reader.Text(id) || id.empty())
            return BinaryError(opcode, 400, "invalid_payload", L"Binary note delete payload is incomplete.");

        std::wstring error;
        if (!m_database.DeleteNote(user, id, error))
            return BinaryError(opcode, 500, "database_error", error);
        NotifyCollaborationChanged();
        return BinaryOk(opcode);
    }

    bool LoadGlobalSettings(json& settingsOut, std::wstring& errorOut) const
    {
        std::ifstream in(m_config.globalSettingsPath, std::ios::binary);
        if (!in) {
            settingsOut = json::object();
            return true;
        }

        try {
            settingsOut = json::parse(in);
            if (!settingsOut.is_object())
                settingsOut = json::object();
            auto nested = settingsOut.find("settings");
            if (nested != settingsOut.end() && nested->is_object())
                settingsOut = *nested;
            return true;
        }
        catch (const std::exception& e) {
            errorOut = L"Global settings parse failed: " + Utf8ToWide(e.what());
            return false;
        }
    }

    bool SaveGlobalSettings(const json& settings, std::wstring& errorOut) const
    {
        try {
            std::filesystem::path parent = m_config.globalSettingsPath.parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent);
            std::ofstream out(m_config.globalSettingsPath, std::ios::binary | std::ios::trunc);
            if (!out) {
                errorOut = L"Could not write global settings.";
                return false;
            }
            out << settings.dump(2);
            return true;
        }
        catch (const std::exception& e) {
            errorOut = L"Could not save global settings: " + Utf8ToWide(e.what());
            return false;
        }
    }

    BinaryResponse HandleBinaryGetSettings(uint16_t opcode)
    {
        json settings;
        std::wstring error;
        if (!LoadGlobalSettings(settings, error))
            return BinaryError(opcode, 500, "settings_error", error);

        BinaryWriter writer;
        writer.JsonText(settings);
        return BinaryOk(opcode, writer);
    }

    BinaryResponse HandleBinarySetSettings(uint16_t opcode, BinaryReader& reader, const UserRecord& user)
    {
        if (!CanEditGlobalSettings(user))
            return BinaryError(opcode, 403, "forbidden", L"Only Administrators and Supervisors can update global settings.");

        json settings;
        try {
            if (!reader.Json(settings))
                return BinaryError(opcode, 400, "invalid_payload", L"Binary global settings payload is incomplete.");
        }
        catch (const std::exception& e) {
            return BinaryError(opcode, 400, "invalid_json", L"Invalid global settings JSON: " + Utf8ToWide(e.what()));
        }

        std::wstring error;
        if (!SaveGlobalSettings(settings, error))
            return BinaryError(opcode, 500, "settings_error", error);
        return BinaryOk(opcode);
    }

    HttpResponse HandleLogin(const HttpRequest& req)
    {
        json body;
        try {
            body = json::parse(req.body);
        }
        catch (...) {
            return ErrorResponse(400, L"Invalid login JSON.", "invalid_json");
        }

        std::wstring username = PickWide(body, { "username" });
        std::wstring password = PickWide(body, { "password" });
        std::wstring position = PickWide(body, { "position" });
        std::wstring pod = PickWide(body, { "pod" });
        if (username.empty() || password.empty() || position.empty() || pod.empty())
            return ErrorResponse(400, L"Username, password, position and pod are required.", "missing_fields");

        UserRecord user;
        std::wstring error;
        std::string code;
        if (!m_database.ValidateLogin(username, password, position, pod, user, error, code)) {
            printf("Error: %ls\n", error.c_str());
            return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 401, error, code.empty() ? nullptr : code.c_str());
        }

        std::wstring token;
        if (!m_database.CreateSession(user, token, error)) {
            if (error == L"Selected pod is already in use.")
                return ErrorResponse(401, error, "pod_in_use");
            return ErrorResponse(500, error, IsDatabaseErrorMessage(error) ? "database_error" : "session_create_failed");
        }
        NotifyCollaborationChanged();

        return JsonResponse(200, {
            { "ok", true },
            { "token", WideToUtf8(token) },
            { "user", {
                { "id", WideToUtf8(user.id) },
                { "username", WideToUtf8(user.username) },
                { "displayName", WideToUtf8(user.displayName) },
                { "position", WideToUtf8(user.position) },
                { "pod", WideToUtf8(user.pod) }
            } }
            });
    }

    HttpResponse HandleLogout(const HttpRequest& req)
    {
        std::wstring reason;
        if (!req.body.empty()) {
            try {
                json body = json::parse(req.body);
                reason = PickWide(body, { "reason" });
            }
            catch (...) {
            }
        }

        std::wstring error;
        if (!m_database.DeleteSession(BearerToken(req), error))
            return ErrorResponse(error.rfind(L"Missing bearer token", 0) == 0 ? 401 : 500, error, error.rfind(L"Missing bearer token", 0) == 0 ? "missing_token" : "database_error");
        NotifyCollaborationChanged();
        if (!reason.empty())
            std::wcout << L"Client session ended: " << reason << L"\n";
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleOnlineUsers()
    {
        std::wstring error;
        json users = m_database.OnlineUsers(error);
        if (!error.empty())
            return ErrorResponse(500, error, "database_error");
        return JsonResponse(200, { { "ok", true }, { "users", users } });
    }

    HttpResponse HandleCreateAccount(const HttpRequest& req, const UserRecord& user)
    {
        if (!CanEditGlobalSettings(user))
            return ErrorResponse(403, L"Only Administrators and Supervisors can create accounts.", "forbidden");

        json body;
        try {
            body = json::parse(req.body.empty() ? "{}" : req.body);
        }
        catch (const std::exception& e) {
            return ErrorResponse(400, L"Invalid account JSON: " + Utf8ToWide(e.what()), "invalid_json");
        }

        std::wstring username = PickWide(body, { "username" });
        std::wstring displayName = PickWide(body, { "displayName", "display_name", "name" });
        std::wstring password = PickWide(body, { "password" });
        std::wstring position = PickWide(body, { "position", "role" });
        bool active = true;
        auto activeIt = body.find("active");
        if (activeIt != body.end() && activeIt->is_boolean())
            active = activeIt->get<bool>();

        if (PositionRank(position) > PositionRank(user.position))
            return ErrorResponse(403, L"You cannot create an account above your current position.", "position_not_allowed");

        std::wstring error;
        std::string code;
        if (!m_database.CreateOrUpdateUser(username, displayName, password, position, active, error, code))
            return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 400, error, code.empty() ? "account_create_failed" : code.c_str());

        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleGetChat()
    {
        std::wstring error;
        json messages = m_database.ChatMessages(error);
        if (!error.empty())
            return ErrorResponse(500, error);
        return JsonResponse(200, { { "messages", messages } });
    }

    HttpResponse HandlePostChat(const HttpRequest& req, const UserRecord& user)
    {
        json body = json::parse(req.body.empty() ? "{}" : req.body);
        std::wstring text = PickWide(body, { "text", "message", "body" });
        if (text.empty())
            return ErrorResponse(400, L"Chat text is required.");
        std::wstring error;
        if (!m_database.AddChatMessage(user, text, error))
            return ErrorResponse(500, error);
        NotifyCollaborationChanged();
        return JsonResponse(201, { { "ok", true } });
    }

    HttpResponse HandleClearChat(const UserRecord& user)
    {
        if (!CanEditGlobalSettings(user))
            return ErrorResponse(403, L"Only Administrators and Supervisors can clear responder chat.", "forbidden");

        std::wstring error;
        if (!m_database.ClearChatMessages(error))
            return ErrorResponse(500, error, "database_error");
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleDeleteChatMessage(const HttpRequest& req, const UserRecord& user)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/chat/").size()));
        std::wstring error;
        if (!m_database.DeleteChatMessage(user, id, error))
            return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 403, error, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden");
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleGetPrivateMessages(const UserRecord& user)
    {
        std::wstring error;
        json messages = m_database.PrivateMessages(user, error);
        if (!error.empty())
            return ErrorResponse(500, error, "database_error");
        return JsonResponse(200, { { "ok", true }, { "messages", messages } });
    }

    HttpResponse HandlePostPrivateMessage(const HttpRequest& req, const UserRecord& user)
    {
        json body = json::parse(req.body.empty() ? "{}" : req.body);
        std::wstring recipient = PickWide(body, { "recipient", "recipientUsername", "username", "to" });
        std::wstring text = PickWide(body, { "text", "message", "body" });
        std::wstring error;
        if (!m_database.AddPrivateMessage(user, recipient, text, error))
            return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 400, error, IsDatabaseErrorMessage(error) ? "database_error" : "private_message_failed");
        NotifyCollaborationChanged();
        return JsonResponse(201, { { "ok", true } });
    }

    HttpResponse HandleUserAction(const HttpRequest& req, const UserRecord& user)
    {
        const std::string prefix = "/api/users/";
        std::string suffix = req.path.substr(prefix.size());
        const size_t slash = suffix.find('/');
        if (slash == std::string::npos)
            return ErrorResponse(404, L"User action endpoint not found.", "not_found");

        std::wstring username = Utf8ToWide(suffix.substr(0, slash));
        std::string action = suffix.substr(slash + 1);
        std::wstring error;
        if (action == "kick") {
            if (!m_database.KickUser(user, username, error))
                return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 403, error, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden");
            NotifyCollaborationChanged();
            return JsonResponse(200, { { "ok", true } });
        }
        if (action == "mute") {
            uint32_t minutes = 15;
            try {
                json body = json::parse(req.body.empty() ? "{}" : req.body);
                auto it = body.find("minutes");
                if (it != body.end() && it->is_number_unsigned())
                    minutes = it->get<uint32_t>();
            }
            catch (...) {
            }
            if (!m_database.MuteUser(user, username, minutes, error))
                return ErrorResponse(IsDatabaseErrorMessage(error) ? 500 : 403, error, IsDatabaseErrorMessage(error) ? "database_error" : "forbidden");
            NotifyCollaborationChanged();
            return JsonResponse(200, { { "ok", true } });
        }
        return ErrorResponse(404, L"User action endpoint not found.", "not_found");
    }

    HttpResponse HandleAdminLogs(const HttpRequest& req, const UserRecord& user)
    {
        if (PositionRank(user.position) < 4)
            return ErrorResponse(403, L"Only Administrators can view administrator logs.", "forbidden");

        std::wstring error;
        json rows = m_database.UserLoginTimes(error);
        if (!error.empty())
            return ErrorResponse(500, error, "database_error");
        return JsonResponse(200, { { "ok", true }, { "type", "user_login_times" }, { "items", rows } });
    }

    HttpResponse HandleClearAdminLogs(const HttpRequest& req, const UserRecord& user)
    {
        if (PositionRank(user.position) < 4)
            return ErrorResponse(403, L"Only Administrators can clear administrator logs.", "forbidden");

        const std::string prefix = "/api/admin/logs/";
        const std::wstring category = Utf8ToWide(req.path.substr(prefix.size()));
        std::wstring error;
        if (!m_database.ClearAdminLogCategory(category, error))
            return ErrorResponse(
                error == L"Unknown administrator log category." ? 400 : 500,
                error,
                error == L"Unknown administrator log category." ? "invalid_category" : "database_error");
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleGetIncidentExclusions()
    {
        std::wstring error;
        json items = m_database.IncidentExclusions(error);
        if (!error.empty())
            return ErrorResponse(500, error, "database_error");
        return JsonResponse(200, { { "ok", true }, { "items", items } });
    }

    HttpResponse HandleAddIncidentExclusion(const HttpRequest& req, const UserRecord& user)
    {
        json body;
        try {
            body = json::parse(req.body.empty() ? "{}" : req.body);
        }
        catch (const std::exception& e) {
            return ErrorResponse(400, L"Invalid incident exclusion JSON: " + Utf8ToWide(e.what()), "invalid_json");
        }

        std::wstring error;
        if (!m_database.AddIncidentExclusion(
            user,
            JsonTextField(body, { "key", "incidentKey" }),
            JsonTextField(body, { "sourceId", "source_id" }),
            JsonTextField(body, { "source", "sourceName" }),
            JsonTextField(body, { "road" }),
            JsonTextField(body, { "summary", "title" }),
            error))
        {
            return ErrorResponse(500, error, "database_error");
        }
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleRemoveIncidentExclusion(const HttpRequest& req, const UserRecord& user)
    {
        json body;
        try {
            body = json::parse(req.body.empty() ? "{}" : req.body);
        }
        catch (const std::exception& e) {
            return ErrorResponse(400, L"Invalid incident exclusion JSON: " + Utf8ToWide(e.what()), "invalid_json");
        }

        std::wstring error;
        if (!m_database.RemoveIncidentExclusion(
            user,
            JsonTextField(body, { "key", "incidentKey" }),
            error))
        {
            return ErrorResponse(500, error, "database_error");
        }
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleGetNotes()
    {
        std::wstring error;
        json notes = m_database.Notes(error);
        if (!error.empty())
            return ErrorResponse(500, error);
        return JsonResponse(200, { { "notes", notes } });
    }

    HttpResponse HandlePostNote(const HttpRequest& req, const UserRecord& user)
    {
        json body = json::parse(req.body.empty() ? "{}" : req.body);
        json note;
        std::wstring error;
        if (!m_database.AddNote(user, body, note, error))
            return ErrorResponse(500, error);
        NotifyCollaborationChanged();
        return JsonResponse(201, { { "ok", true }, { "note", note } });
    }

    HttpResponse HandleUpdateNote(const HttpRequest& req, const UserRecord& user)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/notes/").size()));
        json body = json::parse(req.body.empty() ? "{}" : req.body);
        std::wstring error;
        if (!m_database.UpdateNote(user, id, body, error))
            return ErrorResponse(500, error);
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleDeleteNote(const HttpRequest& req, const UserRecord& user)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/notes/").size()));
        std::wstring error;
        if (!m_database.DeleteNote(user, id, error))
            return ErrorResponse(500, error);
        NotifyCollaborationChanged();
        return JsonResponse(200, { { "ok", true } });
    }

    static bool CanEditGlobalSettings(const UserRecord& user)
    {
        std::wstring role = ToLower(TrimWide(user.position));
        return role == L"administrator" || role == L"admin" || role == L"supervisor" || role == L"sup";
    }

    HttpResponse HandleGetGlobalSettings()
    {
        std::ifstream in(m_config.globalSettingsPath, std::ios::binary);
        if (!in)
            return JsonResponse(200, { { "ok", true }, { "settings", json::object() } });

        try {
            json settings = json::parse(in);
            if (!settings.is_object())
                settings = json::object();
            auto nested = settings.find("settings");
            if (nested != settings.end() && nested->is_object())
                settings = *nested;
            return JsonResponse(200, { { "ok", true }, { "settings", settings } });
        }
        catch (const std::exception& e) {
            return ErrorResponse(500, L"Global settings parse failed: " + Utf8ToWide(e.what()));
        }
    }

    HttpResponse HandleSetGlobalSettings(const HttpRequest& req, const UserRecord& user)
    {
        if (!CanEditGlobalSettings(user))
            return ErrorResponse(403, L"Only Administrators and Supervisors can update global settings.");

        json body;
        try {
            body = json::parse(req.body.empty() ? "{}" : req.body);
        }
        catch (const std::exception& e) {
            return ErrorResponse(400, L"Invalid global settings JSON: " + Utf8ToWide(e.what()));
        }

        json settings = json::object();
        auto settingsIt = body.find("settings");
        if (settingsIt != body.end() && settingsIt->is_object())
            settings = *settingsIt;
        else if (body.is_object())
            settings = body;
        else
            return ErrorResponse(400, L"Global settings must be a JSON object.");

        try {
            std::filesystem::path parent = m_config.globalSettingsPath.parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent);
            std::ofstream out(m_config.globalSettingsPath, std::ios::binary | std::ios::trunc);
            if (!out)
                return ErrorResponse(500, L"Could not write global settings.");
            out << settings.dump(2);
        }
        catch (const std::exception& e) {
            return ErrorResponse(500, L"Could not save global settings: " + Utf8ToWide(e.what()));
        }

        return JsonResponse(200, { { "ok", true }, { "settings", settings } });
    }

    std::optional<json> ReadManifest(std::wstring& errorOut)
    {
        std::ifstream in(m_config.manifestPath, std::ios::binary);
        if (!in) {
            errorOut = L"Update manifest not found.";
            return std::nullopt;
        }
        try {
            return json::parse(in);
        }
        catch (const std::exception& e) {
            errorOut = L"Update manifest parse failed: " + Utf8ToWide(e.what());
            return std::nullopt;
        }
    }

    HttpResponse HandleUpdateManifest(const HttpRequest& req)
    {
        std::wstring error;
        auto manifestOpt = ReadManifest(error);
        if (!manifestOpt)
            return JsonResponse(200, { { "updateAvailable", false }, { "error", WideToUtf8(error) } });

        json manifest = *manifestOpt;
        std::wstring latest = PickWide(manifest, { "version", "latestVersion" });
        std::wstring current = QueryParam(req.query, "version");
        bool available = !latest.empty() && CompareVersionParts(current, latest) < 0;
        manifest["updateAvailable"] = available;
        manifest["currentVersion"] = WideToUtf8(current);
        manifest["protocol"] = "erc-tools-update-v1";
        return JsonResponse(200, manifest);
    }

    HttpResponse HandleUpdateFile(const HttpRequest& req)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/updates/files/").size()));
        std::wstring error;
        auto manifestOpt = ReadManifest(error);
        if (!manifestOpt)
            return ErrorResponse(404, error);

        const json& manifest = *manifestOpt;
        auto filesIt = manifest.find("files");
        if (filesIt == manifest.end() || !filesIt->is_array())
            return ErrorResponse(404, L"No update files are configured.");

        for (const json& file : *filesIt) {
            if (!file.is_object())
                continue;
            std::wstring fileId = PickWide(file, { "id", "fileId", "name" });
            if (fileId != id)
                continue;
            std::wstring pathText = PickWide(file, { "serverPath", "path", "target" });
            std::filesystem::path rel(pathText);
            if (!IsSafeRelativePath(rel))
                return ErrorResponse(400, L"Unsafe update file path.");

            std::filesystem::path fullPath = m_config.updateRoot / rel;
            std::ifstream in(fullPath, std::ios::binary);
            if (!in)
                return ErrorResponse(404, L"Update file not found.");

            HttpResponse response;
            response.status = 200;
            response.reason = "OK";
            response.contentType = "application/octet-stream";
            response.binary.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            return response;
        }

        return ErrorResponse(404, L"Update file id not found.");
    }

    static std::wstring QueryParam(const std::string& query, const std::string& key)
    {
        size_t pos = 0;
        while (pos <= query.size()) {
            size_t next = query.find('&', pos);
            std::string part = query.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
            size_t eq = part.find('=');
            std::string k = eq == std::string::npos ? part : part.substr(0, eq);
            if (k == key)
                return Utf8ToWide(eq == std::string::npos ? "" : part.substr(eq + 1));
            if (next == std::string::npos)
                break;
            pos = next + 1;
        }
        return L"";
    }

    std::mutex m_collaborationMutex;
    std::condition_variable m_collaborationChanged;
    std::atomic<uint32_t> m_collaborationVersion{ 1 };
    std::mutex m_sourceCacheMutex;
    std::condition_variable m_sourceCacheChanged;
    std::unordered_map<std::wstring, SourceCacheEntry> m_sourceCache;
    std::unordered_set<std::wstring> m_sourceCacheInFlight;
    uint32_t m_nextSourceCacheGeneration = 1;
    std::mutex m_sourceRefreshMutex;
    std::condition_variable m_sourceRefreshCv;
    std::thread m_sourceRefreshThread;
    std::atomic_bool m_sourceRefreshStop{ false };
    std::atomic<uint32_t> m_sourceFetchIntervalMs{ 60u * 1000u };
    std::atomic<uint32_t> m_ntisPollIntervalMs{ 2u * 1000u };
    std::atomic_bool m_serveStaleSourceCache{ true };
    ServerConfig m_config;
    Database m_database;
};

static bool LoadConfig(const std::filesystem::path& path, ServerConfig& config)
{
    std::ifstream in(path, std::ios::binary);
    json root = json::object();
    if (in)
        root = json::parse(in);
    if (root.contains("port"))
        config.port = root["port"].get<int>();
    if (root.contains("workerThreads"))
        config.workerThreads = root["workerThreads"].get<int>();
    if (root.contains("sessionMinutes"))
        config.sessionMinutes = root["sessionMinutes"].get<int>();
    if (root.contains("onlineTimeoutSeconds"))
        config.onlineTimeoutSeconds = std::clamp(root["onlineTimeoutSeconds"].get<int>(), 15, 3600);
    if (root.contains("sourceFetchIntervalSeconds"))
        config.sourceFetchIntervalSeconds = std::clamp(root["sourceFetchIntervalSeconds"].get<int>(), 15, 3600);
    if (root.contains("sourceFetchIntervalMs"))
        config.sourceFetchIntervalSeconds = static_cast<int>(ClampSourceFetchIntervalMs(root["sourceFetchIntervalMs"].get<uint64_t>()) / 1000u);
    if (root.contains("ntisPollIntervalSeconds"))
        config.ntisPollIntervalSeconds = std::clamp(root["ntisPollIntervalSeconds"].get<int>(), 1, 60);
    if (root.contains("serveStaleSourceCache"))
        config.serveStaleSourceCache = root["serveStaleSourceCache"].get<bool>();
    if (root.contains("databaseConnectionString"))
        config.databaseConnectionString = Utf8ToWide(root["databaseConnectionString"].get<std::string>());
    if (root.contains("ntisEventSnapshotUrl"))
        config.ntisEventSnapshotUrl = NormalizeAbsoluteUrl(Utf8ToWide(root["ntisEventSnapshotUrl"].get<std::string>()));
    if (root.contains("updateRoot"))
        config.updateRoot = Utf8ToWide(root["updateRoot"].get<std::string>());
    if (root.contains("manifestPath"))
        config.manifestPath = Utf8ToWide(root["manifestPath"].get<std::string>());
    if (root.contains("globalSettingsPath"))
        config.globalSettingsPath = Utf8ToWide(root["globalSettingsPath"].get<std::string>());

    return true;
}

static bool ReadRequest(SOCKET s, HttpRequest& req)
{
    std::string data;
    char buffer[8192];
    size_t headerEnd = std::string::npos;
    while ((headerEnd = data.find("\r\n\r\n")) == std::string::npos) {
        int n = recv(s, buffer, sizeof(buffer), 0);
        if (n <= 0)
            return false;
        data.append(buffer, buffer + n);
        if (data.size() > 1024 * 1024)
            return false;
    }

    std::string headerText = data.substr(0, headerEnd);
    std::istringstream headers(headerText);
    std::string line;
    if (!std::getline(headers, line))
        return false;
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::istringstream requestLine(line);
    std::string target;
    requestLine >> req.method >> target;
    size_t q = target.find('?');
    req.path = q == std::string::npos ? target : target.substr(0, q);
    req.query = q == std::string::npos ? "" : target.substr(q + 1);

    while (std::getline(headers, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        req.headers[Lower(Trim(line.substr(0, colon)))] = Trim(line.substr(colon + 1));
    }

    size_t contentLength = 0;
    auto cl = req.headers.find("content-length");
    if (cl != req.headers.end())
        contentLength = static_cast<size_t>(std::strtoull(cl->second.c_str(), nullptr, 10));
    if (contentLength > 32 * 1024 * 1024)
        return false;

    req.body = data.substr(headerEnd + 4);
    while (req.body.size() < contentLength) {
        int n = recv(s, buffer, sizeof(buffer), 0);
        if (n <= 0)
            return false;
        req.body.append(buffer, buffer + n);
    }
    if (req.body.size() > contentLength)
        req.body.resize(contentLength);
    return true;
}

static bool IsBinaryRequest(SOCKET s)
{
    char magic[4]{};
    int n = recv(s, magic, sizeof(magic), MSG_PEEK);
    return n == sizeof(magic) && std::memcmp(magic, "ERCB", 4) == 0;
}

static bool RecvExact(SOCKET s, unsigned char* data, size_t size)
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

static bool ReadBinaryRequest(SOCKET s, uint16_t& opcodeOut, std::vector<unsigned char>& payloadOut)
{
    unsigned char header[12]{};
    if (!RecvExact(s, header, sizeof(header)))
        return false;
    if (std::memcmp(header, "ERCB", 4) != 0)
        return false;

    uint16_t version = ReadU16Raw(header + 4);
    opcodeOut = ReadU16Raw(header + 6);
    uint32_t payloadLen = ReadU32Raw(header + 8);
    if (version != kBinaryProtocolVersion || payloadLen > kMaxBinaryPayload)
        return false;

    payloadOut.resize(payloadLen);
    return payloadLen == 0 || RecvExact(s, payloadOut.data(), payloadLen);
}

static void SendAll(SOCKET s, const std::string& data)
{
    const char* ptr = data.data();
    int remaining = static_cast<int>(data.size());
    while (remaining > 0) {
        int sent = send(s, ptr, remaining, 0);
        if (sent <= 0)
            return;
        ptr += sent;
        remaining -= sent;
    }
}

static void SendAll(SOCKET s, const std::vector<unsigned char>& data)
{
    const unsigned char* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        int chunk = static_cast<int>(std::min<size_t>(remaining, 64 * 1024));
        int sent = send(s, reinterpret_cast<const char*>(ptr), chunk, 0);
        if (sent <= 0)
            return;
        ptr += sent;
        remaining -= static_cast<size_t>(sent);
    }
}

static void SendBinaryResponse(SOCKET s, const BinaryResponse& response)
{
    std::vector<unsigned char> frame;
    frame.reserve(14 + response.payload.size());
    frame.push_back('E');
    frame.push_back('R');
    frame.push_back('C');
    frame.push_back('R');
    WriteU16(frame, kBinaryProtocolVersion);
    WriteU16(frame, response.opcode);
    WriteU16(frame, response.status);
    WriteU32(frame, static_cast<uint32_t>(response.payload.size()));
    frame.insert(frame.end(), response.payload.begin(), response.payload.end());
    SendAll(s, frame);
}

static void SendResponse(SOCKET s, HttpResponse response)
{
    const bool binary = !response.binary.empty();
    const size_t size = binary ? response.binary.size() : response.body.size();
    std::ostringstream header;
    header << "HTTP/1.1 " << response.status << ' ' << response.reason << "\r\n";
    header << "Content-Type: " << response.contentType << "\r\n";
    header << "Content-Length: " << size << "\r\n";
    header << "Access-Control-Allow-Origin: *\r\n";
    header << "Access-Control-Allow-Headers: Authorization, Content-Type\r\n";
    header << "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n";
    header << "Connection: close\r\n\r\n";
    SendAll(s, header.str());
    if (binary && !response.binary.empty())
        send(s, reinterpret_cast<const char*>(response.binary.data()), static_cast<int>(response.binary.size()), 0);
    else if (!response.body.empty())
        SendAll(s, response.body);
}

static void WorkerLoop(BlockingSocketQueue& queue, ErcServer& server)
{
    SOCKET s = INVALID_SOCKET;
    while (queue.Pop(s)) {
        if (IsBinaryRequest(s)) {
            uint16_t opcode = 0;
            std::vector<unsigned char> payload;
            if (ReadBinaryRequest(s, opcode, payload))
                SendBinaryResponse(s, server.HandleBinary(opcode, payload));
            else
                SendBinaryResponse(s, BinaryResponse{ 0, 400, {} });
        }
        else {
            HttpRequest request;
            if (ReadRequest(s, request))
                SendResponse(s, server.Handle(request));
            else
                SendResponse(s, ErrorResponse(400, L"Bad request."));
        }
        shutdown(s, SD_BOTH);
        closesocket(s);
    }
}

constexpr int kServerSettingsIntervalEdit = 5101;
constexpr int kServerSettingsApplyButton = 5102;
constexpr int kServerSettingsStaleCheck = 5103;
constexpr int kServerSettingsClearCacheButton = 5104;
constexpr int kServerSettingsStatusLabel = 5105;
constexpr int kServerSettingsNtisIntervalEdit = 5107;

struct ServerSettingsWindowState
{
    ErcServer* server = nullptr;
};

static std::wstring ServerSettingsStatusText(ErcServer& server)
{
    std::wstringstream text;
    text << L"Source cache entries: " << server.SourceCacheEntryCount()
        << L" | general interval: " << (server.SourceFetchIntervalMs() / 1000u) << L"s"
        << L" | NTIS poll: " << (server.NtisPollIntervalMs() / 1000u) << L"s";
    return text.str();
}

static void UpdateServerSettingsWindow(HWND hwnd)
{
    auto* state = reinterpret_cast<ServerSettingsWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || !state->server)
        return;

    if (HWND status = GetDlgItem(hwnd, kServerSettingsStatusLabel)) {
        const std::wstring text = ServerSettingsStatusText(*state->server);
        SetWindowTextW(status, text.c_str());
    }
    if (HWND stale = GetDlgItem(hwnd, kServerSettingsStaleCheck)) {
        SendMessageW(stale, BM_SETCHECK, state->server->ServeStaleSourceCache() ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static uint32_t ParseServerIntervalEditSeconds(HWND edit)
{
    wchar_t buffer[64]{};
    GetWindowTextW(edit, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    unsigned long seconds = std::wcstoul(buffer, &end, 10);
    if (end == buffer)
        seconds = 60;
    return ClampSourceFetchIntervalMs(static_cast<uint64_t>(seconds) * 1000ull) / 1000u;
}

static uint32_t ParseNtisIntervalEditSeconds(HWND edit)
{
    wchar_t buffer[64]{};
    GetWindowTextW(edit, buffer, static_cast<int>(_countof(buffer)));
    wchar_t* end = nullptr;
    unsigned long seconds = std::wcstoul(buffer, &end, 10);
    if (end == buffer)
        seconds = 2;
    return ClampNtisPollIntervalMs(static_cast<uint64_t>(seconds) * 1000ull) / 1000u;
}

static LRESULT CALLBACK ServerSettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_NCCREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* state = reinterpret_cast<ServerSettingsWindowState*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return TRUE;
    }

    case WM_CREATE: {
        auto* state = reinterpret_cast<ServerSettingsWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        ErcServer* server = state ? state->server : nullptr;
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        CreateWindowExW(0, L"STATIC", L"ERC Tools Server", WS_CHILD | WS_VISIBLE,
            18, 16, 260, 24, hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"STATIC", L"Source fetch interval (seconds)", WS_CHILD | WS_VISIBLE,
            18, 54, 220, 20, hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            18, 78, 110, 26, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsIntervalEdit)), GetModuleHandleW(nullptr), nullptr);
        HWND apply = CreateWindowExW(0, L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            140, 78, 86, 26, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsApplyButton)), GetModuleHandleW(nullptr), nullptr);
        CreateWindowExW(0, L"STATIC", L"NTIS snapshot poll (seconds)", WS_CHILD | WS_VISIBLE,
            250, 54, 220, 20, hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        HWND ntisEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            250, 78, 110, 26, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsNtisIntervalEdit)), GetModuleHandleW(nullptr), nullptr);
        HWND stale = CreateWindowExW(0, L"BUTTON", L"Serve stale cache if an upstream fetch fails", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            18, 118, 330, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsStaleCheck)), GetModuleHandleW(nullptr), nullptr);
        HWND clear = CreateWindowExW(0, L"BUTTON", L"Clear Source Cache", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            18, 154, 150, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsClearCacheButton)), GetModuleHandleW(nullptr), nullptr);
        HWND status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            18, 198, 600, 22, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServerSettingsStatusLabel)), GetModuleHandleW(nullptr), nullptr);

        for (HWND control : { edit, apply, ntisEdit, stale, clear, status })
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

        if (server) {
            const std::wstring seconds = std::to_wstring(server->SourceFetchIntervalMs() / 1000u);
            SetWindowTextW(edit, seconds.c_str());
            const std::wstring ntisSeconds = std::to_wstring(server->NtisPollIntervalMs() / 1000u);
            SetWindowTextW(ntisEdit, ntisSeconds.c_str());
            SendMessageW(stale, BM_SETCHECK, server->ServeStaleSourceCache() ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        UpdateServerSettingsWindow(hwnd);
        SetTimer(hwnd, 1, 1000, nullptr);
        return 0;
    }

    case WM_COMMAND: {
        auto* state = reinterpret_cast<ServerSettingsWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        ErcServer* server = state ? state->server : nullptr;
        const int id = LOWORD(wParam);
        if (!server)
            break;
        if (id == kServerSettingsApplyButton && HIWORD(wParam) == BN_CLICKED) {
            HWND edit = GetDlgItem(hwnd, kServerSettingsIntervalEdit);
            const uint32_t seconds = ParseServerIntervalEditSeconds(edit);
            HWND ntisEdit = GetDlgItem(hwnd, kServerSettingsNtisIntervalEdit);
            const uint32_t ntisSeconds = ParseNtisIntervalEditSeconds(ntisEdit);
            server->SetSourceFetchIntervalMs(seconds * 1000u);
            server->SetNtisPollIntervalMs(ntisSeconds * 1000u);
            const std::wstring normalized = std::to_wstring(seconds);
            SetWindowTextW(edit, normalized.c_str());
            const std::wstring normalizedNtis = std::to_wstring(ntisSeconds);
            SetWindowTextW(ntisEdit, normalizedNtis.c_str());
            UpdateServerSettingsWindow(hwnd);
            return 0;
        }
        if (id == kServerSettingsStaleCheck && HIWORD(wParam) == BN_CLICKED) {
            HWND stale = GetDlgItem(hwnd, kServerSettingsStaleCheck);
            server->SetServeStaleSourceCache(SendMessageW(stale, BM_GETCHECK, 0, 0) == BST_CHECKED);
            UpdateServerSettingsWindow(hwnd);
            return 0;
        }
        if (id == kServerSettingsClearCacheButton && HIWORD(wParam) == BN_CLICKED) {
            server->ClearSourceCache();
            UpdateServerSettingsWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_TIMER:
        UpdateServerSettingsWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        auto* state = reinterpret_cast<ServerSettingsWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        delete state;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void RunServerSettingsWindow(ErcServer& server)
{
    constexpr const wchar_t* className = L"ERCToolsServerSettingsWindow";
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ServerSettingsWndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);

    auto* state = new ServerSettingsWindowState{ &server };
    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        className,
        L"ERC Tools Server Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        650,
        280,
        nullptr,
        nullptr,
        instance,
        state);
    if (!hwnd) {
        delete state;
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc >= 3 && std::wstring(argv[1]) == L"--hash-password") {
        std::vector<unsigned char> salt(16);
        std::string hash;
        if (!RandomBytes(salt) || !DerivePasswordHash(argv[2], salt, 150000, hash)) {
            std::wcerr << L"Could not hash password.\n";
            return 1;
        }
        std::cout << "password_salt=" << HexFromBytes(salt) << "\n";
        std::cout << "password_hash=" << hash << "\n";
        std::cout << "password_iterations=150000\n";
        return 0;
    }

    ServerConfig config;
    std::filesystem::path configPath = argc > 1 ? argv[1] : L"server_config.json";
    try {
        LoadConfig(configPath, config);
    }
    catch (const std::exception& e) {
        std::wcerr << L"Config error: " << Utf8ToWide(e.what()) << L"\n";
        return 1;
    }

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::wcerr << L"WSAStartup failed.\n";
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::wcerr << L"socket failed.\n";
        WSACleanup();
        return 1;
    }

    DWORD ipv6Only = 0;
    setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&ipv6Only), sizeof(ipv6Only));

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(static_cast<u_short>(config.port));
    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
        listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::wcerr << L"bind/listen failed on port " << config.port << L".\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (config.workerThreads <= 0) {
        unsigned int hw = std::thread::hardware_concurrency();
        config.workerThreads = static_cast<int>(std::max(16u, hw > 0 ? hw * 2u : 16u));
    }

    ErcServer server(config);
    std::thread settingsWindowThread([&server]() { RunServerSettingsWindow(server); });
    settingsWindowThread.detach();

    BlockingSocketQueue queue;
    std::vector<std::thread> workers;
    for (int i = 0; i < config.workerThreads; ++i)
        workers.emplace_back([&]() { WorkerLoop(queue, server); });

    std::wcout << L"ERC Tools Server listening on port " << config.port << L" with " << config.workerThreads << L" workers.\n";
    std::wcout << L"National Highways source: NTIS Event Data.\n";
    for (;;) {
        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET)
            break;
        queue.Push(client);
    }

    queue.Stop();
    for (auto& worker : workers)
        worker.join();
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
