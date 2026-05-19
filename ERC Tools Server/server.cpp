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

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cctype>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "bcrypt.lib")

using json = nlohmann::json;

namespace
{
struct ServerConfig
{
    int port = 8080;
    int workerThreads = 0;
    int sessionMinutes = 12 * 60;
    std::wstring databaseConnectionString =
        L"DRIVER={MySQL ODBC 8.0 Unicode Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=erc_tools;UID=erc_tools;PWD=change-me;OPTION=3;";
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
        if (!SQL_SUCCEEDED(ret))
            return Fail(errorOut, L"Could not connect to MySQL through ODBC.");

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
            errorOut = L"SQLExecute failed.";
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
        closeStmt();
        if (!SQL_SUCCEEDED(ret)) {
            errorOut = L"SQLExecute failed.";
            return false;
        }
        return true;
    }

private:
    bool PrepareAndBind(SQLHSTMT stmt, const std::wstring& sql, const std::vector<std::wstring>& params, std::wstring& errorOut)
    {
        SQLRETURN ret = SQLPrepareW(stmt, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql.c_str())), SQL_NTS);
        if (!SQL_SUCCEEDED(ret)) {
            errorOut = L"SQLPrepare failed.";
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
                errorOut = L"SQLBindParameter failed.";
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

    bool ValidateLogin(const std::wstring& username, const std::wstring& password, const std::wstring& position, const std::wstring& pod, UserRecord& userOut, std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        auto rows = db.Query(
            L"SELECT id, username, display_name, position, pod, password_salt, password_hash, password_iterations, active FROM users WHERE username = ? LIMIT 1",
            { username },
            errorOut);
        if (rows.empty()) {
            errorOut = L"Invalid username or password.";
            return false;
        }

        const auto& row = rows.front();
        if (row.size() < 9 || row[8] == L"0") {
            errorOut = L"This account is disabled.";
            return false;
        }

        std::vector<unsigned char> salt = BytesFromHex(row[5]);
        int iterations = row[7].empty() ? 150000 : _wtoi(row[7].c_str());
        std::string actualHash;
        if (salt.empty() || !DerivePasswordHash(password, salt, iterations, actualHash) ||
            !ConstantTimeEquals(actualHash, WideToUtf8(ToLower(row[6]))))
        {
            errorOut = L"Invalid username or password.";
            return false;
        }

        if (!position.empty() && ToLower(position) != ToLower(row[3])) {
            errorOut = L"Selected position does not match this account.";
            return false;
        }
        if (!pod.empty() && ToLower(pod) != ToLower(row[4])) {
            errorOut = L"Selected pod does not match this account.";
            return false;
        }

        userOut = UserRecord{ row[0], row[1], row[2], row[3], row[4] };
        db.Execute(L"UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE id = ?", { userOut.id }, errorOut);
        errorOut.clear();
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

        OdbcConnection db(m_config.databaseConnectionString);
        bool ok = db.Execute(
            L"INSERT INTO user_sessions (token_hash, user_id, expires_at, created_at) VALUES (?, ?, DATE_ADD(CURRENT_TIMESTAMP, INTERVAL ? MINUTE), CURRENT_TIMESTAMP)",
            { Utf8ToWide(tokenHash), user.id, std::to_wstring(m_config.sessionMinutes) },
            errorOut);
        if (!ok)
            return false;
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
        auto rows = db.Query(
            L"SELECT u.id, u.username, u.display_name, u.position, u.pod "
            L"FROM user_sessions s JOIN users u ON u.id = s.user_id "
            L"WHERE s.token_hash = ? AND s.expires_at > CURRENT_TIMESTAMP AND u.active = 1 LIMIT 1",
            { Utf8ToWide(tokenHash) },
            errorOut);
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

    json ChatMessages(std::wstring& errorOut)
    {
        OdbcConnection db(m_config.databaseConnectionString);
        auto rows = db.Query(
            L"SELECT c.author, c.body, DATE_FORMAT(c.created_at, '%Y-%m-%d %H:%i:%s'), COALESCE(u.position, '') "
            L"FROM chat_messages c LEFT JOIN users u ON u.id = c.user_id ORDER BY c.created_at DESC LIMIT 100",
            {},
            errorOut);
        json messages = json::array();
        for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
            messages.push_back({
                { "author", WideToUtf8((*it)[0]) },
                { "text", WideToUtf8((*it)[1]) },
                { "timestamp", WideToUtf8((*it)[2]) },
                { "position", WideToUtf8((*it)[3]) }
                });
        }
        return messages;
    }

    bool AddChatMessage(const UserRecord& user, const std::wstring& text, std::wstring& errorOut)
    {
        std::wstring author = user.displayName.empty() ? user.username : user.displayName;
        OdbcConnection db(m_config.databaseConnectionString);
        return db.Execute(
            L"INSERT INTO chat_messages (user_id, author, body, created_at) VALUES (?, ?, ?, CURRENT_TIMESTAMP)",
            { user.id, author, text },
            errorOut);
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
    ServerConfig m_config;
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

static HttpResponse ErrorResponse(int status, const std::wstring& message)
{
    return JsonResponse(status, { { "ok", false }, { "error", WideToUtf8(message) } });
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

class ErcServer
{
public:
    explicit ErcServer(ServerConfig config) : m_config(std::move(config)), m_database(m_config) {}

    HttpResponse Handle(const HttpRequest& req)
    {
        try {
            if (req.method == "OPTIONS")
                return JsonResponse(200, { { "ok", true } });
            if (req.method == "GET" && req.path == "/health")
                return JsonResponse(200, { { "ok", true }, { "service", "erc-tools-server" } });
            if (req.method == "POST" && req.path == "/api/auth/login")
                return HandleLogin(req);

            UserRecord user;
            std::wstring authError;
            if (!m_database.Authenticate(BearerToken(req), user, authError))
                return ErrorResponse(401, authError);

            if (req.method == "GET" && req.path == "/api/chat")
                return HandleGetChat();
            if (req.method == "POST" && req.path == "/api/chat")
                return HandlePostChat(req, user);
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
    HttpResponse HandleLogin(const HttpRequest& req)
    {
        json body;
        try {
            body = json::parse(req.body);
        }
        catch (...) {
            return ErrorResponse(400, L"Invalid login JSON.");
        }

        std::wstring username = PickWide(body, { "username" });
        std::wstring password = PickWide(body, { "password" });
        std::wstring position = PickWide(body, { "position" });
        std::wstring pod = PickWide(body, { "pod" });
        if (username.empty() || password.empty() || position.empty() || pod.empty())
            return ErrorResponse(400, L"Username, password, position and pod are required.");

        UserRecord user;
        std::wstring error;
        if (!m_database.ValidateLogin(username, password, position, pod, user, error))
            return ErrorResponse(401, error);

        std::wstring token;
        if (!m_database.CreateSession(user, token, error))
            return ErrorResponse(500, error);

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
        return JsonResponse(201, { { "ok", true } });
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
        return JsonResponse(201, { { "ok", true }, { "note", note } });
    }

    HttpResponse HandleUpdateNote(const HttpRequest& req, const UserRecord& user)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/notes/").size()));
        json body = json::parse(req.body.empty() ? "{}" : req.body);
        std::wstring error;
        if (!m_database.UpdateNote(user, id, body, error))
            return ErrorResponse(500, error);
        return JsonResponse(200, { { "ok", true } });
    }

    HttpResponse HandleDeleteNote(const HttpRequest& req, const UserRecord& user)
    {
        std::wstring id = Utf8ToWide(req.path.substr(std::string("/api/notes/").size()));
        std::wstring error;
        if (!m_database.DeleteNote(user, id, error))
            return ErrorResponse(500, error);
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

    ServerConfig m_config;
    Database m_database;
};

static bool LoadConfig(const std::filesystem::path& path, ServerConfig& config)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return true;
    json root = json::parse(in);
    if (root.contains("port"))
        config.port = root["port"].get<int>();
    if (root.contains("workerThreads"))
        config.workerThreads = root["workerThreads"].get<int>();
    if (root.contains("sessionMinutes"))
        config.sessionMinutes = root["sessionMinutes"].get<int>();
    if (root.contains("databaseConnectionString"))
        config.databaseConnectionString = Utf8ToWide(root["databaseConnectionString"].get<std::string>());
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
        HttpRequest request;
        if (ReadRequest(s, request))
            SendResponse(s, server.Handle(request));
        else
            SendResponse(s, ErrorResponse(400, L"Bad request."));
        shutdown(s, SD_BOTH);
        closesocket(s);
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

    if (config.workerThreads <= 0)
        config.workerThreads = std::max(4u, std::thread::hardware_concurrency());

    ErcServer server(config);
    BlockingSocketQueue queue;
    std::vector<std::thread> workers;
    for (int i = 0; i < config.workerThreads; ++i)
        workers.emplace_back([&]() { WorkerLoop(queue, server); });

    std::wcout << L"ERC Tools Server listening on port " << config.port << L" with " << config.workerThreads << L" workers.\n";
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
