// =================================================================================
// FILE: client_update.cpp
// =================================================================================

#include "net/client_update.h"
#include "net/http.h"
#include "core/util.h"

#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace
{
static std::wstring AppendApiPath(std::wstring base, const wchar_t* path)
{
    base = NormalizeUrl(base);
    while (!base.empty() && base.back() == L'/')
        base.pop_back();
    return base + path;
}

static bool IsAbsoluteHttpUrl(const std::wstring& value)
{
    std::wstring lower = ToLower(value);
    return lower.rfind(L"http://", 0) == 0 || lower.rfind(L"https://", 0) == 0;
}

static bool IsSafeRelativePath(const std::wstring& value)
{
    if (value.empty())
        return false;
    std::filesystem::path path(value);
    if (path.is_absolute())
        return false;
    for (const auto& part : path) {
        std::wstring text = part.wstring();
        if (text == L".." || text.find(L':') != std::wstring::npos)
            return false;
    }
    return true;
}

static std::wstring UrlEncodeComponent(const std::wstring& value)
{
    std::string utf8 = WideToUtf8(value);
    std::wstring encoded;
    const wchar_t* hex = L"0123456789ABCDEF";
    for (unsigned char ch : utf8) {
        bool safe = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~';
        if (safe) {
            encoded.push_back(static_cast<wchar_t>(ch));
        }
        else {
            encoded.push_back(L'%');
            encoded.push_back(hex[(ch >> 4) & 0x0F]);
            encoded.push_back(hex[ch & 0x0F]);
        }
    }
    return encoded;
}

static std::wstring HexFromBytes(const std::vector<BYTE>& bytes)
{
    const wchar_t* hex = L"0123456789abcdef";
    std::wstring out;
    out.reserve(bytes.size() * 2);
    for (BYTE b : bytes) {
        out.push_back(hex[(b >> 4) & 0x0F]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

static bool Sha256Hex(const std::vector<BYTE>& bytes, std::wstring& hashOut)
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0;
    DWORD cbData = 0;
    DWORD hashSize = 0;
    std::vector<BYTE> object;
    std::vector<BYTE> digest;

    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return false;
    auto closeAlg = [&]() { if (alg) BCryptCloseAlgorithmProvider(alg, 0); };

    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &cbData, 0) < 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize), sizeof(hashSize), &cbData, 0) < 0)
    {
        closeAlg();
        return false;
    }

    object.resize(objectSize);
    digest.resize(hashSize);
    if (BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) < 0) {
        closeAlg();
        return false;
    }
    auto closeHash = [&]() { if (hash) BCryptDestroyHash(hash); };

    bool ok = BCryptHashData(hash, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), 0) >= 0 &&
        BCryptFinishHash(hash, digest.data(), hashSize, 0) >= 0;
    if (ok)
        hashOut = HexFromBytes(digest);
    closeHash();
    closeAlg();
    return ok;
}

static std::wstring QuerySuffix(const ClientSession& session)
{
    std::wstring suffix = L"?version=";
    suffix += UrlEncodeComponent(kClientVersion);
    suffix += L"&platform=";
    suffix += UrlEncodeComponent(kClientPlatform);
    if (!session.position.empty()) {
        suffix += L"&position=";
        suffix += UrlEncodeComponent(session.position);
    }
    if (!session.pod.empty()) {
        suffix += L"&pod=";
        suffix += UrlEncodeComponent(session.pod);
    }
    return suffix;
}

static std::filesystem::path ModuleDirectory()
{
    wchar_t path[MAX_PATH * 4]{};
    DWORD n = GetModuleFileNameW(nullptr, path, _countof(path));
    if (n == 0 || n >= _countof(path))
        return std::filesystem::current_path();
    return std::filesystem::path(path).parent_path();
}

static std::filesystem::path ModulePath()
{
    wchar_t path[MAX_PATH * 4]{};
    DWORD n = GetModuleFileNameW(nullptr, path, _countof(path));
    if (n == 0 || n >= _countof(path))
        return {};
    return std::filesystem::path(path);
}

static std::wstring QuoteCmdPath(const std::filesystem::path& path)
{
    std::wstring value = path.wstring();
    std::wstring escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back(L'"');
    for (wchar_t ch : value) {
        if (ch == L'"')
            escaped += L"\"\"";
        else
            escaped.push_back(ch);
    }
    escaped.push_back(L'"');
    return escaped;
}
}

bool CheckAndStageClientUpdate(const std::wstring& serverBaseUrl, const ClientSession& session, ClientUpdateResult& resultOut)
{
    resultOut = ClientUpdateResult{};
    std::wstring server = NormalizeUrl(serverBaseUrl);
    if (server.empty()) {
        resultOut.error = L"No collaboration server configured.";
        return false;
    }

    std::string manifestBody;
    std::wstring error;
    if (!HttpGetTextWithHeaders(AppendApiPath(server, L"/api/updates/manifest") + QuerySuffix(session), BearerAuthHeader(session), manifestBody, error)) {
        resultOut.error = L"Update manifest check failed: " + error;
        return false;
    }

    json manifest;
    try {
        manifest = json::parse(manifestBody);
    }
    catch (const std::exception& e) {
        resultOut.error = L"Update manifest parse failed: " + Utf8ToWide(e.what());
        return false;
    }

    if (!manifest.is_object()) {
        resultOut.error = L"Update manifest is not an object.";
        return false;
    }

    resultOut.updateAvailable = manifest.value("updateAvailable", false);
    resultOut.version = PickString(manifest, { "version", "latestVersion" });
    resultOut.restartRequired = manifest.value("restartRequired", false);
    if (!resultOut.updateAvailable) {
        resultOut.ok = true;
        return true;
    }

    if (resultOut.version.empty())
        resultOut.version = L"unknown";

    const json* files = nullptr;
    auto filesIt = manifest.find("files");
    if (filesIt != manifest.end() && filesIt->is_array())
        files = &(*filesIt);
    if (!files || files->empty()) {
        resultOut.error = L"Update is available but no files were listed.";
        return false;
    }

    resultOut.stagingDir = GetUpdateCacheRoot() / resultOut.version;
    CreateDirectoryW(resultOut.stagingDir.c_str(), nullptr);

    for (const json& item : *files) {
        if (!item.is_object())
            continue;

        ClientUpdateFile file;
        file.id = PickString(item, { "id", "fileId", "name" });
        file.url = PickString(item, { "url", "downloadUrl" });
        file.target = PickString(item, { "target", "path" });
        file.applyMode = ToLower(PickString(item, { "applyMode", "mode" }));
        file.sha256 = ToLower(PickString(item, { "sha256", "hash" }));
        if (file.applyMode.empty())
            file.applyMode = resultOut.restartRequired ? L"restart" : L"hot-reload";
        if (file.id.empty())
            file.id = L"file-" + std::to_wstring(resultOut.files.size() + 1);
        if (file.target.empty() || !IsSafeRelativePath(file.target)) {
            resultOut.error = L"Unsafe update target path: " + file.target;
            return false;
        }

        std::wstring downloadUrl = file.url;
        if (downloadUrl.empty())
            downloadUrl = AppendApiPath(server, L"/api/updates/files/") + UrlEncodeComponent(file.id);
        else if (!IsAbsoluteHttpUrl(downloadUrl)) {
            if (!downloadUrl.empty() && downloadUrl.front() != L'/')
                downloadUrl = L"/" + downloadUrl;
            downloadUrl = AppendApiPath(server, downloadUrl.c_str());
        }

        std::vector<BYTE> bytes;
        if (!HttpGetBinary(downloadUrl, bytes, error)) {
            resultOut.error = L"Update file download failed: " + file.id + L": " + error;
            return false;
        }

        if (!file.sha256.empty()) {
            std::wstring actualHash;
            if (!Sha256Hex(bytes, actualHash) || actualHash != file.sha256) {
                resultOut.error = L"Update file hash mismatch: " + file.id;
                return false;
            }
        }

        file.stagedPath = resultOut.stagingDir / (file.id + L".staged");
        if (!SaveBinaryToFile(file.stagedPath, bytes)) {
            resultOut.error = L"Could not write staged update file: " + file.stagedPath.wstring();
            return false;
        }

        if (file.applyMode != L"hot-reload")
            resultOut.restartRequired = true;
        resultOut.files.push_back(std::move(file));
    }

    resultOut.ok = true;
    return true;
}

bool ApplyHotClientUpdate(const ClientUpdateResult& update, std::wstring& errorOut)
{
    errorOut.clear();
    std::filesystem::path cacheRoot = GetSettingsPath().parent_path();
    for (const ClientUpdateFile& file : update.files) {
        if (file.applyMode != L"hot-reload")
            continue;
        if (!IsSafeRelativePath(file.target)) {
            errorOut = L"Unsafe hot update target path: " + file.target;
            return false;
        }

        std::filesystem::path target = cacheRoot / file.target;
        std::filesystem::path parent = target.parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::filesystem::path tmp = target;
        tmp += L".tmp";
        std::error_code ec;
        std::filesystem::copy_file(file.stagedPath, tmp, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            errorOut = L"Could not copy hot update file: " + Utf8ToWide(ec.message());
            return false;
        }
        std::filesystem::rename(tmp, target, ec);
        if (ec) {
            std::filesystem::remove(target, ec);
            ec.clear();
            std::filesystem::rename(tmp, target, ec);
        }
        if (ec) {
            errorOut = L"Could not apply hot update file: " + Utf8ToWide(ec.message());
            return false;
        }
    }
    return true;
}

bool LaunchRestartClientUpdate(const ClientUpdateResult& update, std::wstring& errorOut)
{
    errorOut.clear();
    std::filesystem::path script = update.stagingDir / L"apply_update.cmd";
    std::wofstream out(script, std::ios::binary);
    if (!out) {
        errorOut = L"Could not create updater script.";
        return false;
    }

    DWORD pid = GetCurrentProcessId();
    std::filesystem::path appDir = ModuleDirectory();
    std::filesystem::path exePath = ModulePath();

    out << L"@echo off\r\n";
    out << L"setlocal\r\n";
    out << L":wait\r\n";
    out << L"tasklist /FI \"PID eq " << pid << L"\" | find \"" << pid << L"\" >nul\r\n";
    out << L"if not errorlevel 1 timeout /t 1 /nobreak >nul & goto wait\r\n";

    for (const ClientUpdateFile& file : update.files) {
        if (!IsSafeRelativePath(file.target)) {
            errorOut = L"Unsafe restart update target path: " + file.target;
            return false;
        }
        std::filesystem::path target = appDir / file.target;
        out << L"copy /Y " << QuoteCmdPath(file.stagedPath) << L" " << QuoteCmdPath(target) << L"\r\n";
    }
    out << L"start \"\" " << QuoteCmdPath(exePath) << L"\r\n";
    out.close();

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    std::wstring command = L"cmd.exe /d /c " + QuoteCmdPath(script);
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        errorOut = L"Could not launch updater: " + WinErrorText(GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}
