// =================================================================================
// FILE: account_creator.cpp
// Server-side account creator for ERC Tools.
// =================================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <sqlext.h>
#include <bcrypt.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "bcrypt.lib")

using json = nlohmann::json;

namespace
{
constexpr int kPasswordIterations = 150000;

struct CreatorConfig
{
    std::wstring databaseConnectionString =
        L"DRIVER={Maria Unicode};SERVER=127.0.0.1;PORT=3306;DATABASE=erc_tools;UID=erc_tools;PWD=change-me;OPTION=3;";
};

struct AccountInput
{
    std::wstring username;
    std::wstring displayName;
    std::wstring password;
    std::wstring position;
    std::wstring pod;
    bool active = true;
    bool updateExisting = false;
    bool listOdbcDrivers = false;
    bool testConnection = false;
    bool printSql = false;
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

static std::wstring Trim(std::wstring s)
{
    while (!s.empty() && std::iswspace(s.front()))
        s.erase(s.begin());
    while (!s.empty() && std::iswspace(s.back()))
        s.pop_back();
    return s;
}

static std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

static std::wstring SqlString(const std::wstring& value)
{
    std::wstring escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back(L'\'');
    for (wchar_t ch : value) {
        if (ch == L'\'')
            escaped += L"''";
        else
            escaped.push_back(ch);
    }
    escaped.push_back(L'\'');
    return escaped;
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

static bool RandomBytes(std::vector<unsigned char>& bytes)
{
    return BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
}

static bool DerivePasswordHash(const std::wstring& password, const std::vector<unsigned char>& salt, int iterations, std::string& hashOut)
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        return false;

    int passwordBytes = WideCharToMultiByte(CP_UTF8, 0, password.data(), static_cast<int>(password.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8Password(static_cast<size_t>(std::max(passwordBytes, 0)), '\0');
    if (passwordBytes > 0)
        WideCharToMultiByte(CP_UTF8, 0, password.data(), static_cast<int>(password.size()), utf8Password.data(), passwordBytes, nullptr, nullptr);

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

static bool ListOdbcDrivers(std::wstring& errorOut)
{
    SQLHENV env = nullptr;
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env) != SQL_SUCCESS) {
        errorOut = L"SQLAllocHandle ENV failed.";
        return false;
    }

    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

    SQLWCHAR description[512]{};
    SQLWCHAR attributes[1024]{};
    SQLSMALLINT descriptionLength = 0;
    SQLSMALLINT attributesLength = 0;
    SQLUSMALLINT direction = SQL_FETCH_FIRST;
    bool found = false;

    while (true) {
        SQLRETURN ret = SQLDriversW(
            env,
            direction,
            description,
            _countof(description),
            &descriptionLength,
            attributes,
            _countof(attributes),
            &attributesLength);
        if (ret == SQL_NO_DATA)
            break;
        if (!SQL_SUCCEEDED(ret)) {
            errorOut = SqlDiagnostic(SQL_HANDLE_ENV, env);
            if (errorOut.empty())
                errorOut = L"SQLDrivers failed.";
            SQLFreeHandle(SQL_HANDLE_ENV, env);
            return false;
        }

        if (!found)
            std::wcout << L"Installed ODBC drivers:\n";
        found = true;
        std::wcout << L"  " << description << L"\n";
        direction = SQL_FETCH_NEXT;
    }

    SQLFreeHandle(SQL_HANDLE_ENV, env);

    if (!found)
        std::wcout << L"No ODBC drivers were reported by Windows.\n";
    return true;
}

class OdbcConnection
{
public:
    explicit OdbcConnection(std::wstring connectionString) : m_connectionString(std::move(connectionString)) {}
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

        if (!PrepareAndBind(stmt, sql, params, errorOut)) {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return rows;
        }

        SQLRETURN ret = SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
            errorOut = diagnostic.empty() ? L"SQLExecute failed." : diagnostic;
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
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

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
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

        if (!PrepareAndBind(stmt, sql, params, errorOut)) {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return false;
        }

        SQLRETURN ret = SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
            std::wstring diagnostic = SqlDiagnostic(SQL_HANDLE_STMT, stmt);
            errorOut = diagnostic.empty() ? L"SQLExecute failed." : diagnostic;
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return false;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
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

static bool LoadConfig(const std::filesystem::path& path, CreatorConfig& config, std::wstring& errorOut)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return true;

    try {
        json root = json::parse(in);
        if (root.contains("databaseConnectionString"))
            config.databaseConnectionString = Utf8ToWide(root["databaseConnectionString"].get<std::string>());
    }
    catch (const std::exception& e) {
        errorOut = L"Config error: " + Utf8ToWide(e.what());
        return false;
    }
    return true;
}

static std::wstring PromptLine(const wchar_t* label, const std::wstring& current = L"")
{
    std::wcout << label;
    if (!current.empty())
        std::wcout << L" [" << current << L"]";
    std::wcout << L": ";

    std::wstring value;
    std::getline(std::wcin, value);
    value = Trim(value);
    return value.empty() ? current : value;
}

static std::wstring PromptPassword(const std::wstring& current)
{
    if (!current.empty())
        return current;

    std::wcout << L"Password: ";
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    bool hide = input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &mode);
    if (hide)
        SetConsoleMode(input, mode & ~ENABLE_ECHO_INPUT);

    std::wstring password;
    std::getline(std::wcin, password);

    if (hide)
        SetConsoleMode(input, mode);
    std::wcout << L"\n";
    return password;
}

static std::optional<std::wstring> NormalizePosition(const std::wstring& value)
{
    std::wstring lower = ToLower(Trim(value));
    if (lower == L"administrator" || lower == L"admin")
        return L"Administrator";
    if (lower == L"supervisor" || lower == L"sup")
        return L"Supervisor";
    if (lower == L"manager" || lower == L"mgr")
        return L"Manager";
    if (lower == L"erc")
        return L"ERC";
    return std::nullopt;
}

static std::optional<std::wstring> NormalizePod(const std::wstring& value)
{
    std::wstring lower = ToLower(Trim(value));
    int number = 0;
    if (lower.rfind(L"pod ", 0) == 0)
        number = _wtoi(lower.c_str() + 4);
    else
        number = _wtoi(lower.c_str());
    if (number < 1 || number > 9)
        return std::nullopt;
    return L"Pod " + std::to_wstring(number);
}

static void ShowUsage()
{
    std::wcout
        << L"ERC Tools Account Creator\n\n"
        << L"Usage:\n"
        << L"  ERC Tools Account Creator.exe [server_config.json]\n"
        << L"  ERC Tools Account Creator.exe --config server_config.json --username sam --display-name \"Sam\" --password \"temp\" --position Administrator --pod \"Pod 1\"\n\n"
        << L"Options:\n"
        << L"  --config <path>          Server config JSON containing databaseConnectionString.\n"
        << L"  --list-odbc-drivers     List installed Windows ODBC driver names.\n"
        << L"  --test-connection       Test the configured MySQL ODBC connection and exit.\n"
        << L"  --print-sql             Print a MySQL INSERT statement instead of connecting through ODBC.\n"
        << L"  --username <value>       Unique login username.\n"
        << L"  --display-name <value>   Name shown in ERC Tools.\n"
        << L"  --password <value>       Initial password. If omitted, you will be prompted.\n"
        << L"  --position <value>       Administrator, Supervisor, Manager, or ERC.\n"
        << L"  --pod <value>            Pod 1 through Pod 9.\n"
        << L"  --inactive               Create/update as disabled.\n"
        << L"  --update-existing        Update the account if username already exists.\n";
}

static bool ReadArgs(int argc, wchar_t** argv, std::filesystem::path& configPath, AccountInput& input)
{
    configPath = L"server_config.json";
    bool sawConfig = false;

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        auto next = [&](std::wstring& out) -> bool {
            if (i + 1 >= argc)
                return false;
            out = argv[++i];
            return true;
            };

        if (arg == L"--help" || arg == L"-h" || arg == L"/?") {
            ShowUsage();
            return false;
        }
        if (arg == L"--config") {
            std::wstring value;
            if (!next(value))
                return false;
            configPath = value;
            sawConfig = true;
        }
        else if (arg == L"--list-odbc-drivers") {
            input.listOdbcDrivers = true;
        }
        else if (arg == L"--test-connection") {
            input.testConnection = true;
        }
        else if (arg == L"--print-sql") {
            input.printSql = true;
        }
        else if (arg == L"--username") {
            if (!next(input.username))
                return false;
        }
        else if (arg == L"--display-name") {
            if (!next(input.displayName))
                return false;
        }
        else if (arg == L"--password") {
            if (!next(input.password))
                return false;
        }
        else if (arg == L"--position") {
            if (!next(input.position))
                return false;
        }
        else if (arg == L"--pod") {
            if (!next(input.pod))
                return false;
        }
        else if (arg == L"--inactive") {
            input.active = false;
        }
        else if (arg == L"--update-existing") {
            input.updateExisting = true;
        }
        else if (!sawConfig && arg.rfind(L"--", 0) != 0) {
            configPath = arg;
            sawConfig = true;
        }
        else {
            std::wcerr << L"Unknown option: " << arg << L"\n";
            return false;
        }
    }

    return true;
}

static bool CompleteInteractiveInput(AccountInput& input)
{
    const bool needsPrompt = Trim(input.username).empty() ||
        Trim(input.displayName).empty() ||
        input.password.empty() ||
        !NormalizePosition(input.position).has_value() ||
        !NormalizePod(input.pod).has_value();

    if (needsPrompt)
        std::wcout << L"Create ERC Tools account\n\n";

    input.username = Trim(input.username);
    if (input.username.empty())
        input.username = PromptLine(L"Username");

    input.displayName = Trim(input.displayName);
    if (input.displayName.empty())
        input.displayName = PromptLine(L"Display Name");

    if (input.password.empty())
        input.password = PromptPassword(input.password);

    while (true) {
        std::wstring value = input.position.empty()
            ? PromptLine(L"Position (Administrator, Supervisor, Manager, ERC)")
            : input.position;
        auto normalized = NormalizePosition(value);
        if (normalized) {
            input.position = *normalized;
            break;
        }
        std::wcout << L"Please enter Administrator, Supervisor, Manager, or ERC.\n";
        input.position.clear();
    }

    while (true) {
        std::wstring value = input.pod.empty()
            ? PromptLine(L"Pod (Pod 1 - Pod 9)")
            : input.pod;
        auto normalized = NormalizePod(value);
        if (normalized) {
            input.pod = *normalized;
            break;
        }
        std::wcout << L"Please enter Pod 1 through Pod 9.\n";
        input.pod.clear();
    }

    input.username = Trim(input.username);
    input.displayName = Trim(input.displayName);
    if (input.username.empty() || input.displayName.empty() || input.password.empty()) {
        std::wcerr << L"Username, display name, and password are required.\n";
        return false;
    }

    return true;
}

static bool CreateOrUpdateAccount(const CreatorConfig& config, const AccountInput& input, std::wstring& errorOut)
{
    std::vector<unsigned char> salt(16);
    std::string passwordHash;
    if (!RandomBytes(salt) || !DerivePasswordHash(input.password, salt, kPasswordIterations, passwordHash)) {
        errorOut = L"Could not create password hash.";
        return false;
    }

    OdbcConnection db(config.databaseConnectionString);
    auto existing = db.Query(L"SELECT id FROM users WHERE username = ? LIMIT 1", { input.username }, errorOut);
    if (!errorOut.empty() && existing.empty())
        return false;

    const std::wstring active = input.active ? L"1" : L"0";
    const std::wstring saltHex = Utf8ToWide(HexFromBytes(salt));
    const std::wstring hashHex = Utf8ToWide(passwordHash);
    const std::wstring iterations = std::to_wstring(kPasswordIterations);

    if (!existing.empty()) {
        if (!input.updateExisting) {
            errorOut = L"Username already exists. Re-run with --update-existing if you want to replace its details/password.";
            return false;
        }

        return db.Execute(
            L"UPDATE users SET display_name = ?, position = ?, pod = ?, password_salt = ?, password_hash = ?, password_iterations = ?, active = ? WHERE username = ?",
            { input.displayName, input.position, input.pod, saltHex, hashHex, iterations, active, input.username },
            errorOut);
    }

    return db.Execute(
        L"INSERT INTO users (username, display_name, position, pod, password_salt, password_hash, password_iterations, active) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
        { input.username, input.displayName, input.position, input.pod, saltHex, hashHex, iterations, active },
        errorOut);
}

static bool PrintAccountSql(const AccountInput& input, std::wstring& errorOut)
{
    std::vector<unsigned char> salt(16);
    std::string passwordHash;
    if (!RandomBytes(salt) || !DerivePasswordHash(input.password, salt, kPasswordIterations, passwordHash)) {
        errorOut = L"Could not create password hash.";
        return false;
    }

    const std::wstring active = input.active ? L"1" : L"0";
    const std::wstring saltHex = Utf8ToWide(HexFromBytes(salt));
    const std::wstring hashHex = Utf8ToWide(passwordHash);
    const std::wstring iterations = std::to_wstring(kPasswordIterations);

    std::wcout
        << L"USE erc_tools;\n\n"
        << L"INSERT INTO users (username, display_name, position, pod, password_salt, password_hash, password_iterations, active)\n"
        << L"VALUES ("
        << SqlString(input.username) << L", "
        << SqlString(input.displayName) << L", "
        << SqlString(input.position) << L", "
        << SqlString(input.pod) << L", "
        << SqlString(saltHex) << L", "
        << SqlString(hashHex) << L", "
        << iterations << L", "
        << active << L")";

    if (input.updateExisting) {
        std::wcout
            << L"\nON DUPLICATE KEY UPDATE\n"
            << L"    display_name = VALUES(display_name),\n"
            << L"    position = VALUES(position),\n"
            << L"    pod = VALUES(pod),\n"
            << L"    password_salt = VALUES(password_salt),\n"
            << L"    password_hash = VALUES(password_hash),\n"
            << L"    password_iterations = VALUES(password_iterations),\n"
            << L"    active = VALUES(active)";
    }

    std::wcout << L";\n";
    return true;
}

static bool TestConnection(const CreatorConfig& config, std::wstring& errorOut)
{
    OdbcConnection db(config.databaseConnectionString);
    auto rows = db.Query(L"SELECT 1", {}, errorOut);
    if (!errorOut.empty())
        return false;
    return !rows.empty();
}
}

int wmain(int argc, wchar_t** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--help" || arg == L"-h" || arg == L"/?") {
            ShowUsage();
            return 0;
        }
    }

    std::filesystem::path configPath;
    AccountInput input;
    if (!ReadArgs(argc, argv, configPath, input)) {
        if (argc <= 1)
            ShowUsage();
        return 1;
    }

    if (input.listOdbcDrivers) {
        std::wstring driverError;
        if (!ListOdbcDrivers(driverError)) {
            std::wcerr << L"Could not list ODBC drivers: " << driverError << L"\n";
            return 1;
        }
        if (!input.testConnection && input.username.empty() && input.displayName.empty())
            return 0;
    }

    CreatorConfig config;
    std::wstring error;
    if ((!input.printSql || input.testConnection) && !LoadConfig(configPath, config, error)) {
        std::wcerr << error << L"\n";
        return 1;
    }

    if (input.testConnection) {
        if (!TestConnection(config, error)) {
            std::wcerr << L"Connection test failed: " << error << L"\n";
            return 1;
        }
        std::wcout << L"Connection test succeeded.\n";
        if (input.username.empty() && input.displayName.empty())
            return 0;
    }

    if (!CompleteInteractiveInput(input))
        return 1;

    if (input.printSql) {
        if (!PrintAccountSql(input, error)) {
            std::wcerr << L"SQL generation failed: " << error << L"\n";
            return 1;
        }
        return 0;
    }

    if (!CreateOrUpdateAccount(config, input, error)) {
        std::wcerr << L"Account creation failed: " << error << L"\n";
        return 1;
    }

    std::wcout << L"\nAccount " << (input.updateExisting ? L"saved" : L"created") << L": "
        << input.username << L" (" << input.position << L", " << input.pod << L")\n";
    return 0;
}
