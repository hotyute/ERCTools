#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <d2d1.h>
#include <wrl/client.h>
#include <winhttp.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <uxtheme.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <iostream>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

using Microsoft::WRL::ComPtr;
using json = nlohmann::json;

constexpr UINT WM_APP_FEED_READY = WM_APP + 1;
constexpr UINT WM_APP_TILE_READY = WM_APP + 2;
constexpr UINT WM_APP_BOUNDARY_READY = WM_APP + 3;


constexpr int IDC_URL_EDIT = 1001;
constexpr int IDC_REFRESH_BTN = 1002;
constexpr int IDC_SEARCH_EDIT = 1003;
constexpr int IDC_SEVERITY_COMBO = 1004;
constexpr int IDC_LISTVIEW = 1005;
constexpr int IDC_DETAILS_EDIT = 1006;
constexpr int IDC_DOWNLOAD_BOUNDARY_BTN = 1007;
constexpr int IDC_HEADER_LABEL = 1008;
constexpr int IDC_URL_LABEL = 1009;
constexpr int IDC_SEARCH_LABEL = 1010;
constexpr int IDC_SEVERITY_LABEL = 1011;
constexpr int IDC_STATUS_BAR = 1012;

constexpr int kMinZoom = 2;
constexpr int kMaxZoom = 19;
constexpr int kDefaultZoom = 6;

constexpr double kDefaultCenterLat = 53.0;
constexpr double kDefaultCenterLon = -1.5;
constexpr double kMaxMercatorLat = 85.05112878;
constexpr double kPi = 3.14159265358979323846;

constexpr wchar_t kMainClassName[] = L"TrafficEnglandNativeMainWindow";
constexpr wchar_t kMapClassName[] = L"TrafficEnglandNativeMapView";
constexpr wchar_t kUkBoundarySourceUrl[] =
L"https://github.com/wmgeolab/geoBoundaries/raw/main/releaseData/gbOpen/GBR/ADM0/geoBoundaries-GBR-ADM0.geojson";

std::atomic_bool g_boundaryDownloadInProgress{ false };
std::atomic_bool g_appQuitting{ false };
std::atomic_bool g_fetchInProgress{ false };
ComPtr<ID2D1Factory> g_d2dFactory;
ComPtr<IWICImagingFactory> g_wicFactory;

// ============================================================
// Helpers
// ============================================================

template<typename T>
static T ClampValue(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

template<typename T>
static T MinValue(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
static T MaxValue(T a, T b)
{
    return a > b ? a : b;
}

struct GeoPoint
{
    double lat = 0.0;
    double lon = 0.0;
};

struct WorldPoint
{
    double x = 0.0;
    double y = 0.0;
};

struct TrafficAlert
{
    std::wstring id;
    std::wstring title;
    std::wstring description;
    std::wstring road;
    std::wstring region;
    std::wstring severity;
    std::wstring updatedText;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

static void OpenConsole()
{
    // If launched from an existing console, attach to it.
    // Otherwise create a new console window.
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }

    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleW(L"Traffic England Alerts Map - Debug Console");
}

static void ConsoleLog(const std::wstring& text)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return;

    std::wstring line = text + L"\r\n";
    DWORD written = 0;
    WriteConsoleW(h, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
}

static std::filesystem::path GetBoundaryCachePath()
{
    wchar_t localAppData[MAX_PATH * 4]{};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, _countof(localAppData));

    std::filesystem::path folder;
    if (n > 0 && n < _countof(localAppData)) {
        folder = std::filesystem::path(localAppData) / L"TrafficEnglandMap";
    }
    else {
        folder = std::filesystem::current_path() / L"TrafficEnglandMap";
    }

    CreateDirectoryW(folder.c_str(), nullptr);
    return folder / L"uk_outline.geojson";
}

static bool SaveBinaryToFile(const std::filesystem::path& path, const std::vector<BYTE>& bytes)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;

    out.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return out.good();
}

static std::wstring Trim(const std::wstring& s)
{
    size_t start = 0;
    size_t end = s.size();

    while (start < end && iswspace(s[start]))
        ++start;
    while (end > start && iswspace(s[end - 1]))
        --end;

    return s.substr(start, end - start);
}

static std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return s;
}

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty())
        return {};

    int needed = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (needed <= 0)
        return {};

    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], needed);
    return out;
}

static std::wstring WinErrorText(DWORD err)
{
    LPWSTR buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        err,
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::wstring msg = (size && buffer) ? std::wstring(buffer, size) : L"Unknown error";
    if (buffer)
        LocalFree(buffer);

    while (!msg.empty() &&
        (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L'.' || msg.back() == L' '))
    {
        msg.pop_back();
    }

    return msg;
}

static std::wstring GetWindowTextString(HWND hWnd)
{
    int len = GetWindowTextLengthW(hWnd);
    if (len <= 0)
        return {};

    std::vector<wchar_t> buf(static_cast<size_t>(len) + 1);
    GetWindowTextW(hWnd, buf.data(), len + 1);
    return std::wstring(buf.data());
}

static void SetWindowTextSafe(HWND hWnd, const std::wstring& text)
{
    SetWindowTextW(hWnd, text.c_str());
}

static std::wstring TimeTToText(std::time_t t)
{
    std::tm tmv{};
    localtime_s(&tmv, &t);

    wchar_t buf[64]{};
    wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

static std::wstring JsonValueToText(const json& v)
{
    if (v.is_string())
        return Utf8ToWide(v.get<std::string>());

    if (v.is_number_integer())
        return std::to_wstring(v.get<long long>());

    if (v.is_number_unsigned())
        return std::to_wstring(v.get<unsigned long long>());

    if (v.is_number_float())
        return std::to_wstring(v.get<double>());

    if (v.is_boolean())
        return v.get<bool>() ? L"true" : L"false";

    return {};
}

static bool TryGetDoubleFromJsonValue(const json& v, double& out)
{
    if (v.is_number()) {
        out = v.get<double>();
        return true;
    }

    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        char* end = nullptr;
        double d = std::strtod(s.c_str(), &end);
        if (end && end != s.c_str()) {
            out = d;
            return true;
        }
    }

    return false;
}

static std::wstring PickString(const json& obj, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it == obj.end())
            continue;

        std::wstring value = Trim(JsonValueToText(*it));
        if (!value.empty())
            return value;
    }

    return {};
}

static bool PickDouble(const json& obj, std::initializer_list<const char*> keys, double& out)
{
    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it == obj.end())
            continue;

        if (TryGetDoubleFromJsonValue(*it, out))
            return true;
    }

    return false;
}

static std::wstring PickDateText(const json& obj, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it == obj.end())
            continue;

        if (it->is_string()) {
            std::wstring s = Trim(Utf8ToWide(it->get<std::string>()));
            if (!s.empty())
                return s;
        }

        if (it->is_number()) {
            double n = it->get<double>();
            if (n > 1000000000000.0)
                n /= 1000.0;

            std::time_t t = static_cast<std::time_t>(n);
            return TimeTToText(t);
        }
    }

    return {};
}

static std::wstring BuildSeverityDisplay(const std::wstring& severity)
{
    std::wstring s = ToLower(Trim(severity));
    if (s.find(L"severe") != std::wstring::npos ||
        s.find(L"major") != std::wstring::npos ||
        s.find(L"critical") != std::wstring::npos ||
        s.find(L"high") != std::wstring::npos)
    {
        return L"Severe";
    }

    if (s.find(L"moderate") != std::wstring::npos ||
        s.find(L"medium") != std::wstring::npos)
    {
        return L"Moderate";
    }

    if (s.find(L"minor") != std::wstring::npos ||
        s.find(L"low") != std::wstring::npos ||
        s.find(L"info") != std::wstring::npos)
    {
        return L"Minor";
    }

    return L"Unknown";
}

static std::wstring SeverityBucket(const std::wstring& severity)
{
    std::wstring s = ToLower(Trim(severity));
    if (s.find(L"severe") != std::wstring::npos ||
        s.find(L"major") != std::wstring::npos ||
        s.find(L"critical") != std::wstring::npos ||
        s.find(L"high") != std::wstring::npos)
        return L"severe";

    if (s.find(L"moderate") != std::wstring::npos ||
        s.find(L"medium") != std::wstring::npos)
        return L"moderate";

    if (s.find(L"minor") != std::wstring::npos ||
        s.find(L"low") != std::wstring::npos ||
        s.find(L"info") != std::wstring::npos)
        return L"minor";

    return L"unknown";
}

static std::wstring BuildAlertSummary(const TrafficAlert& a)
{
    std::wstring s;
    if (!a.road.empty())
        s = a.road;
    else if (!a.region.empty())
        s = a.region;
    else
        s = L"Unknown location";

    if (!a.title.empty()) {
        s += L" - ";
        s += a.title;
    }

    return s;
}

static std::wstring BuildAlertDetails(const TrafficAlert& a)
{
    std::wstring s;

    s += L"Title: ";
    s += a.title.empty() ? L"Traffic alert" : a.title;
    s += L"\r\n";

    s += L"Road: ";
    s += a.road.empty() ? L"" : a.road;
    s += L"\r\n";

    s += L"Region: ";
    s += a.region.empty() ? L"" : a.region;
    s += L"\r\n";

    s += L"Severity: ";
    s += BuildSeverityDisplay(a.severity);
    s += L"\r\n";

    s += L"Updated: ";
    s += a.updatedText.empty() ? L"" : a.updatedText;
    s += L"\r\n";

    s += L"Coordinates: ";
    if (a.hasLocation) {
        s += std::to_wstring(a.latitude);
        s += L", ";
        s += std::to_wstring(a.longitude);
    }
    else {
        s += L"";
    }
    s += L"\r\n\r\n";

    s += L"Description:\r\n";
    s += a.description.empty() ? L"No description provided." : a.description;
    s += L"\r\n";

    return s;
}

static std::wstring NormalizeUrl(std::wstring url)
{
    url = Trim(url);
    if (url.empty())
        return {};

    if (url.rfind(L"http://", 0) == 0 || url.rfind(L"https://", 0) == 0)
        return url;

    return L"https://" + url;
}

static std::wstring HtmlDecode(const std::wstring& text)
{
    std::wstring out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'&') {
            size_t sem = text.find(L';', i + 1);
            if (sem != std::wstring::npos && (sem - i) <= 12) {
                std::wstring ent = text.substr(i + 1, sem - i - 1);
                std::wstring low = ToLower(ent);

                if (low == L"amp") out.push_back(L'&');
                else if (low == L"lt") out.push_back(L'<');
                else if (low == L"gt") out.push_back(L'>');
                else if (low == L"quot") out.push_back(L'"');
                else if (low == L"apos" || low == L"#39") out.push_back(L'\'');
                else if (low == L"nbsp") out.push_back(L' ');
                else if (low.size() > 2 && low[0] == L'#') {
                    int code = 0;
                    if (low.size() > 3 && low[1] == L'x') {
                        code = static_cast<int>(wcstol(low.c_str() + 2, nullptr, 16));
                    }
                    else {
                        code = static_cast<int>(wcstol(low.c_str() + 1, nullptr, 10));
                    }

                    if (code > 0)
                        out.push_back(static_cast<wchar_t>(code));
                }
                else {
                    out += L"&";
                    out += ent;
                    out += L";";
                }

                i = sem;
                continue;
            }
        }

        out.push_back(text[i]);
    }

    return out;
}

static std::wstring StripHtmlTags(std::wstring text)
{
    text = std::regex_replace(
        text,
        std::wregex(LR"(<\s*br\s*/?\s*>)", std::regex_constants::icase),
        L"\n");

    text = std::regex_replace(
        text,
        std::wregex(LR"(<[^>]+>)", std::regex_constants::icase),
        L" ");

    text = HtmlDecode(text);

    std::wstring out;
    out.reserve(text.size());

    bool lastSpace = false;

    for (wchar_t ch : text) {
        if (ch == L'\r')
            continue;

        if (ch == L'\n') {
            if (!out.empty() && out.back() != L'\n')
                out.push_back(L'\n');
            lastSpace = false;
            continue;
        }

        if (iswspace(ch)) {
            if (!lastSpace) {
                out.push_back(L' ');
                lastSpace = true;
            }
        }
        else {
            out.push_back(ch);
            lastSpace = false;
        }
    }

    return Trim(out);
}

static std::vector<TrafficAlert> ParseHtmlTrafficAlerts(const std::wstring& html)
{
    std::vector<TrafficAlert> out;

    std::wregex rowRe(LR"(<tr\b[^>]*>([\s\S]*?)</tr>)", std::regex_constants::icase);
    std::wregex cellRe(LR"(<t[dh]\b[^>]*>([\s\S]*?)</t[dh]>)", std::regex_constants::icase);

    size_t idCounter = 0;

    for (std::wsregex_iterator rowIt(html.begin(), html.end(), rowRe), rowEnd;
        rowIt != rowEnd;
        ++rowIt)
    {
        std::wstring rowHtml = (*rowIt)[1].str();

        std::vector<std::wstring> cells;
        for (std::wsregex_iterator cellIt(rowHtml.begin(), rowHtml.end(), cellRe), cellEnd;
            cellIt != cellEnd;
            ++cellIt)
        {
            cells.push_back(StripHtmlTags((*cellIt)[1].str()));
        }

        if (cells.size() < 4)
            continue;

        std::wstring road = Trim(cells[0]);
        std::wstring type = Trim(cells[1]);
        std::wstring severity = Trim(cells[2]);
        std::wstring description = Trim(cells[3]);

        // Skip the table header row
        if (ToLower(road) == L"road" && ToLower(type) == L"type")
            continue;

        if (road.empty() || description.empty())
            continue;

        TrafficAlert a;
        a.id = L"html-" + std::to_wstring(++idCounter);
        a.road = road;
        a.title = type.empty() ? L"Traffic alert" : type;
        a.severity = severity.empty() ? L"Unknown" : severity;
        a.description = description;
        a.updatedText = L"";
        a.region = L"";
        a.hasLocation = false;

        out.push_back(std::move(a));
    }

    return out;
}


static void EnableModernWindowFrame(HWND hwnd)
{
    if (!hwnd)
        return;

    const BOOL enabled = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &enabled, sizeof(enabled));
}

static HFONT CreateUiFont(int pointSize = 10, int weight = FW_NORMAL)
{
    HDC hdc = GetDC(nullptr);
    const int dpiY = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc)
        ReleaseDC(nullptr, hdc);

    LOGFONTW lf{};
    lf.lfHeight = -MulDiv(pointSize, dpiY, 72);
    lf.lfWeight = weight;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    return CreateFontIndirectW(&lf);
}

static void ApplyExplorerTheme(HWND hwnd)
{
    if (hwnd)
        SetWindowTheme(hwnd, L"Explorer", nullptr);
}

// ============================================================
// HTTP
// ============================================================

struct InternetHandle
{
    HINTERNET h = nullptr;

    InternetHandle() = default;
    explicit InternetHandle(HINTERNET handle) : h(handle) {}
    ~InternetHandle()
    {
        if (h)
            WinHttpCloseHandle(h);
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};


static bool QueryHttpStatus(HINTERNET request, DWORD& statusOut, std::wstring& errorOut)
{
    statusOut = 0;
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status,
        &statusSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        errorOut = L"Could not read HTTP status: " + WinErrorText(GetLastError());
        return false;
    }

    statusOut = status;
    return true;
}

static bool EnsureHttpSuccess(HINTERNET request, std::wstring& errorOut)
{
    DWORD status = 0;
    if (!QueryHttpStatus(request, status, errorOut))
        return false;

    if (status >= 200 && status < 300)
        return true;

    wchar_t reason[512]{};
    DWORD reasonSize = sizeof(reason);
    if (!WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_TEXT,
        WINHTTP_HEADER_NAME_BY_INDEX,
        reason,
        &reasonSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        reason[0] = L'\0';
    }

    errorOut = L"HTTP " + std::to_wstring(status);
    if (reason[0] != L'\0') {
        errorOut += L" ";
        errorOut += reason;
    }

    if (status >= 300 && status < 400) {
        wchar_t location[2048]{};
        DWORD locationSize = sizeof(location);
        if (WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_LOCATION,
            WINHTTP_HEADER_NAME_BY_INDEX,
            location,
            &locationSize,
            WINHTTP_NO_HEADER_INDEX) && location[0] != L'\0')
        {
            errorOut += L" (redirect to ";
            errorOut += location;
            errorOut += L")";
        }
    }

    return false;
}

static void ConfigureSecureProtocols(HINTERNET session)
{
    DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    secureProtocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
}

static void ConfigureRedirects(HINTERNET request)
{
#ifdef WINHTTP_OPTION_REDIRECT_POLICY
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
#endif
}

static bool HttpGetText(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);

    wchar_t host[2048]{};
    wchar_t path[4096]{};
    wchar_t extra[4096]{};

    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse URL: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring objectName(path, parts.dwUrlPathLength);
    std::wstring extraInfo(extra, parts.dwExtraInfoLength);
    std::wstring fullPath = objectName + extraInfo;
    if (fullPath.empty())
        fullPath = L"/";

    InternetHandle session(WinHttpOpen(
        L"TrafficEnglandNative/1.1 (+https://www.trafficengland.com/traffic-alerts)",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));

    if (!session.h) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session.h, 15000, 15000, 15000, 15000);

    ConfigureSecureProtocols(session.h);

    INTERNET_PORT port = parts.nPort;
    if (port == 0)
        port = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    InternetHandle connect(WinHttpConnect(session.h, hostName.c_str(), port, 0));
    if (!connect.h) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connect.h,
        L"GET",
        fullPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags));

    if (!request.h) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    ConfigureRedirects(request.h);

    std::wstring headers =
        L"Accept: text/html,application/xhtml+xml,application/geo+json,application/json,text/json,text/plain,*/*;q=0.8\r\n"
        L"Accept-Encoding: identity\r\n"
        L"Accept-Language: en-GB,en;q=0.9\r\n"
        L"Cache-Control: no-cache\r\n";

    WinHttpAddRequestHeaders(request.h, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        errorOut = L"WinHttpSendRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!WinHttpReceiveResponse(request.h, nullptr)) {
        errorOut = L"WinHttpReceiveResponse failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!EnsureHttpSuccess(request.h, errorOut))
        return false;

    std::vector<unsigned char> bytes;
    bytes.reserve(4096);

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }

        if (available == 0)
            break;

        size_t oldSize = bytes.size();
        bytes.resize(oldSize + available);

        DWORD read = 0;
        if (!WinHttpReadData(request.h, bytes.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }

        bytes.resize(oldSize + read);
    }

    if (bytes.size() >= 3 &&
        bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
    {
        bytes.erase(bytes.begin(), bytes.begin() + 3);
    }

    bodyOut.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

static bool HttpGetBinary(const std::wstring& inputUrl, std::vector<BYTE>& bodyOut, std::wstring& errorOut)
{
    bodyOut.clear();
    errorOut.clear();

    std::wstring url = NormalizeUrl(inputUrl);
    if (url.empty()) {
        errorOut = L"Empty URL.";
        return false;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);

    wchar_t host[2048]{};
    wchar_t path[4096]{};
    wchar_t extra[4096]{};

    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = _countof(extra);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        errorOut = L"Could not parse URL: " + WinErrorText(GetLastError());
        return false;
    }

    if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
        errorOut = L"Only http:// and https:// URLs are supported.";
        return false;
    }

    std::wstring hostName(host, parts.dwHostNameLength);
    std::wstring objectName(path, parts.dwUrlPathLength);
    std::wstring extraInfo(extra, parts.dwExtraInfoLength);
    std::wstring fullPath = objectName + extraInfo;
    if (fullPath.empty())
        fullPath = L"/";

    InternetHandle session(WinHttpOpen(
        L"TrafficEnglandNative/1.1 (+https://www.trafficengland.com/traffic-alerts)",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));

    if (!session.h) {
        errorOut = L"WinHttpOpen failed: " + WinErrorText(GetLastError());
        return false;
    }

    WinHttpSetTimeouts(session.h, 15000, 15000, 15000, 15000);
    ConfigureSecureProtocols(session.h);

    INTERNET_PORT port = parts.nPort;
    if (port == 0)
        port = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    InternetHandle connect(WinHttpConnect(session.h, hostName.c_str(), port, 0));
    if (!connect.h) {
        errorOut = L"WinHttpConnect failed: " + WinErrorText(GetLastError());
        return false;
    }

    DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connect.h,
        L"GET",
        fullPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags));

    if (!request.h) {
        errorOut = L"WinHttpOpenRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    ConfigureRedirects(request.h);

    std::wstring headers =
        L"Accept: image/png,image/*;q=0.9,*/*;q=0.8\r\n"
        L"Accept-Encoding: identity\r\n";

    WinHttpAddRequestHeaders(request.h, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        errorOut = L"WinHttpSendRequest failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!WinHttpReceiveResponse(request.h, nullptr)) {
        errorOut = L"WinHttpReceiveResponse failed: " + WinErrorText(GetLastError());
        return false;
    }

    if (!EnsureHttpSuccess(request.h, errorOut))
        return false;

    std::vector<BYTE> bytes;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available)) {
            errorOut = L"WinHttpQueryDataAvailable failed: " + WinErrorText(GetLastError());
            return false;
        }

        if (available == 0)
            break;

        size_t oldSize = bytes.size();
        bytes.resize(oldSize + available);

        DWORD read = 0;
        if (!WinHttpReadData(request.h, bytes.data() + oldSize, available, &read)) {
            errorOut = L"WinHttpReadData failed: " + WinErrorText(GetLastError());
            return false;
        }

        bytes.resize(oldSize + read);
    }

    bodyOut = std::move(bytes);
    return true;
}

static bool ExtractRingFromCoords(const json& coords, std::vector<GeoPoint>& ring)
{
    ring.clear();

    if (!coords.is_array())
        return false;

    for (const auto& pos : coords) {
        if (!pos.is_array() || pos.size() < 2)
            continue;

        double lon = 0.0;
        double lat = 0.0;

        if (!TryGetDoubleFromJsonValue(pos[0], lon))
            continue;
        if (!TryGetDoubleFromJsonValue(pos[1], lat))
            continue;

        ring.push_back({ lat, lon });
    }

    if (ring.size() >= 3) {
        const GeoPoint& a = ring.front();
        const GeoPoint& b = ring.back();
        if (std::abs(a.lat - b.lat) < 1e-12 && std::abs(a.lon - b.lon) < 1e-12)
            ring.pop_back();
    }

    return ring.size() >= 3;
}

static void CollectBoundaryRingsFromGeometry(const json& geom, std::vector<std::vector<GeoPoint>>& rings)
{
    if (!geom.is_object())
        return;

    const std::string type = geom.value("type", "");
    auto coordIt = geom.find("coordinates");
    if (coordIt == geom.end() || !coordIt->is_array())
        return;

    if (type == "Polygon") {
        for (const auto& ringCoords : *coordIt) {
            std::vector<GeoPoint> ring;
            if (ExtractRingFromCoords(ringCoords, ring))
                rings.push_back(std::move(ring));
        }
    }
    else if (type == "MultiPolygon") {
        for (const auto& polygon : *coordIt) {
            if (!polygon.is_array())
                continue;

            for (const auto& ringCoords : polygon) {
                std::vector<GeoPoint> ring;
                if (ExtractRingFromCoords(ringCoords, ring))
                    rings.push_back(std::move(ring));
            }
        }
    }
}

static void CollectBoundaryRingsFromNode(const json& node, std::vector<std::vector<GeoPoint>>& rings)
{
    if (node.is_array()) {
        for (const auto& item : node)
            CollectBoundaryRingsFromNode(item, rings);
        return;
    }

    if (!node.is_object())
        return;

    const std::string type = node.value("type", "");

    if (type == "FeatureCollection") {
        auto it = node.find("features");
        if (it != node.end() && it->is_array()) {
            for (const auto& feature : *it)
                CollectBoundaryRingsFromNode(feature, rings);
        }
        return;
    }

    if (type == "Feature") {
        auto it = node.find("geometry");
        if (it != node.end() && it->is_object())
            CollectBoundaryRingsFromGeometry(*it, rings);
        return;
    }

    CollectBoundaryRingsFromGeometry(node, rings);
}

// ============================================================
// Feed parsing
// ============================================================

static TrafficAlert ParseAlertObject(const json& obj)
{
    TrafficAlert a;

    const json* props = &obj;
    if (obj.contains("properties") && obj["properties"].is_object())
        props = &obj["properties"];

    a.id = PickString(*props, { "id", "incidentId", "alertId", "uuid", "eventId" });
    if (a.id.empty())
        a.id = PickString(obj, { "id", "incidentId", "alertId", "uuid", "eventId" });

    a.title = PickString(*props, { "title", "headline", "summary", "name" });
    if (a.title.empty())
        a.title = PickString(obj, { "title", "headline", "summary", "name" });

    a.description = PickString(*props, { "description", "details", "message", "fullText" });
    if (a.description.empty())
        a.description = PickString(obj, { "description", "details", "message", "fullText" });

    a.road = PickString(*props, { "road", "roadName", "route" });
    if (a.road.empty())
        a.road = PickString(obj, { "road", "roadName", "route" });

    a.region = PickString(*props, { "region", "area", "county", "district", "location" });
    if (a.region.empty())
        a.region = PickString(obj, { "region", "area", "county", "district", "location" });

    a.severity = PickString(*props, { "severity", "impact", "level", "priority" });
    if (a.severity.empty())
        a.severity = PickString(obj, { "severity", "impact", "level", "priority" });

    a.updatedText = PickDateText(*props, { "updated", "lastUpdated", "timestamp", "created", "published" });
    if (a.updatedText.empty())
        a.updatedText = PickDateText(obj, { "updated", "lastUpdated", "timestamp", "created", "published" });

    double lat = 0.0;
    double lon = 0.0;
    bool hasLat = PickDouble(*props, { "latitude", "lat", "y" }, lat);
    bool hasLon = PickDouble(*props, { "longitude", "lon", "lng", "long", "x" }, lon);

    if (!(hasLat && hasLon)) {
        hasLat = PickDouble(obj, { "latitude", "lat", "y" }, lat);
        hasLon = PickDouble(obj, { "longitude", "lon", "lng", "long", "x" }, lon);
    }

    if (!(hasLat && hasLon) && obj.contains("geometry") && obj["geometry"].is_object()) {
        const json& geom = obj["geometry"];
        std::wstring geomType = PickString(geom, { "type" });
        if (ToLower(geomType) == L"point" &&
            geom.contains("coordinates") &&
            geom["coordinates"].is_array())
        {
            const json& coords = geom["coordinates"];
            if (coords.size() >= 2) {
                TryGetDoubleFromJsonValue(coords[0], lon);
                TryGetDoubleFromJsonValue(coords[1], lat);
                hasLat = true;
                hasLon = true;
            }
        }
    }

    if (hasLat && hasLon) {
        a.latitude = lat;
        a.longitude = lon;
        a.hasLocation = true;
    }

    static std::atomic<unsigned long long> s_idCounter{ 0 };
    if (a.id.empty())
        a.id = L"alert-" + std::to_wstring(++s_idCounter);

    if (a.title.empty())
        a.title = L"Traffic alert";

    if (a.severity.empty())
        a.severity = L"Unknown";

    return a;
}

static std::vector<TrafficAlert> ParseTrafficAlerts(const std::string& text, std::wstring& errorOut)
{
    errorOut.clear();

    if (text.empty()) {
        errorOut = L"Empty response.";
        return {};
    }

    // Try JSON / GeoJSON first
    try {
        json root = json::parse(text);

        std::vector<TrafficAlert> out;
        bool recognized = false;

        auto addIfObject = [&](const json& item)
            {
                if (item.is_object())
                    out.push_back(ParseAlertObject(item));
            };

        if (root.is_array()) {
            recognized = true;
            for (const auto& item : root)
                addIfObject(item);
        }
        else if (root.is_object()) {
            if (root.contains("features") && root["features"].is_array()) {
                recognized = true;
                for (const auto& item : root["features"])
                    addIfObject(item);
            }
            else if (root.contains("alerts") && root["alerts"].is_array()) {
                recognized = true;
                for (const auto& item : root["alerts"])
                    addIfObject(item);
            }
            else if (root.contains("data") && root["data"].is_array()) {
                recognized = true;
                for (const auto& item : root["data"])
                    addIfObject(item);
            }
            else if (root.contains("geometry") ||
                root.contains("latitude") ||
                root.contains("lat") ||
                root.contains("title") ||
                root.contains("description") ||
                root.contains("headline"))
            {
                recognized = true;
                out.push_back(ParseAlertObject(root));
            }
        }

        if (recognized && !out.empty())
            return out;

        if (recognized && out.empty())
            errorOut = L"JSON parsed, but no alerts were found.";
    }
    catch (...) {
        // Not JSON, so try HTML below
    }

    // HTML table fallback
    std::wstring html = Utf8ToWide(text);
    std::vector<TrafficAlert> htmlAlerts = ParseHtmlTrafficAlerts(html);

    if (!htmlAlerts.empty())
        return htmlAlerts;

    if (errorOut.empty())
        errorOut = L"Could not parse the response as JSON or as an HTML alerts table.";

    return {};
}

static std::vector<TrafficAlert> SampleAlerts()
{
    std::vector<TrafficAlert> out;
    std::time_t now = std::time(nullptr);

    out.push_back({
        L"sample-1",
        L"Queueing traffic on the M1",
        L"Slow traffic northbound due to congestion.",
        L"M1",
        L"East Midlands",
        L"Moderate",
        TimeTToText(now - 300),
        52.0570, -0.7550, true
        });

    out.push_back({
        L"sample-2",
        L"Lane closed on the M25",
        L"One lane is closed for roadworks.",
        L"M25",
        L"Greater London",
        L"Severe",
        TimeTToText(now - 420),
        51.6090, -0.4300, true
        });

    out.push_back({
        L"sample-3",
        L"Incident on A1",
        L"Delays likely near the junction.",
        L"A1",
        L"North East",
        L"Minor",
        TimeTToText(now - 180),
        54.9700, -1.6170, true
        });

    return out;
}

struct TileKey
{
    int z{};
    int x{};
    int y{};
};

static bool operator==(const TileKey& a, const TileKey& b) noexcept
{
    return a.z == b.z && a.x == b.x && a.y == b.y;
}

struct TileKeyHash
{
    std::size_t operator()(const TileKey& k) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(k.z);
        std::size_t h2 = std::hash<int>{}(k.x);
        std::size_t h3 = std::hash<int>{}(k.y);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct TileEntry
{
    std::mutex mutex;
    bool loading = false;
    bool ready = false;
    bool failed = false;
    ULONGLONG lastAttemptMs = 0;
    std::vector<BYTE> bytes;
    ComPtr<ID2D1Bitmap> bitmap;
};

// ============================================================
// MapView
// ============================================================

class MapView
{
public:
    using SelectCallback = std::function<void(const std::wstring&)>;

    bool Create(HWND parent, int x, int y, int w, int h)
    {
        if (!RegisterClass())
            return false;

        m_hwnd = CreateWindowExW(
            0,
            kMapClassName,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            x, y, w, h,
            parent,
            nullptr,
            GetModuleHandleW(nullptr),
            this);

        return m_hwnd != nullptr;
    }

    HWND Hwnd() const
    {
        return m_hwnd;
    }

    void SetSelectCallback(SelectCallback cb)
    {
        m_onSelect = std::move(cb);
    }

    void SetAlerts(const std::vector<TrafficAlert>& alerts)
    {
        m_alerts = alerts;
        Invalidate();
    }

    void SetSelectedId(const std::wstring& id)
    {
        m_selectedId = id;
        Invalidate();
    }

    void CenterOnAlert(const std::wstring& id)
    {
        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (m_alerts[i].id == id && m_alerts[i].hasLocation) {
                m_centerLat = m_alerts[i].latitude;
                m_centerLon = m_alerts[i].longitude;
                m_zoom = MaxValue(m_zoom, 11);
                NormalizeCenter();
                Invalidate();
                return;
            }
        }
    }

    void FitToAlerts()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        std::vector<GeoPoint> pts;
        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (m_alerts[i].hasLocation)
                pts.push_back({ m_alerts[i].latitude, m_alerts[i].longitude });
        }

        if (pts.empty()) {
            m_centerLat = kDefaultCenterLat;
            m_centerLon = kDefaultCenterLon;
            m_zoom = kDefaultZoom;
            Invalidate();
            return;
        }

        if (pts.size() == 1) {
            m_centerLat = pts[0].lat;
            m_centerLon = pts[0].lon;
            m_zoom = 12;
            NormalizeCenter();
            Invalidate();
            return;
        }

        double minLat = pts[0].lat;
        double maxLat = pts[0].lat;
        double minLon = pts[0].lon;
        double maxLon = pts[0].lon;

        for (size_t i = 1; i < pts.size(); ++i) {
            minLat = MinValue(minLat, pts[i].lat);
            maxLat = MaxValue(maxLat, pts[i].lat);
            minLon = MinValue(minLon, pts[i].lon);
            maxLon = MaxValue(maxLon, pts[i].lon);
        }

        for (int z = kMaxZoom; z >= kMinZoom; --z) {
            WorldPoint a = GeoToWorld(maxLat, minLon, z);
            WorldPoint b = GeoToWorld(minLat, maxLon, z);

            double spanX = std::abs(b.x - a.x);
            double spanY = std::abs(b.y - a.y);

            if (spanX <= width * 0.85 && spanY <= height * 0.85) {
                m_zoom = z;
                WorldPoint centerWorld;
                centerWorld.x = (a.x + b.x) * 0.5;
                centerWorld.y = (a.y + b.y) * 0.5;

                GeoPoint centerGeo = WorldToGeo(centerWorld.x, centerWorld.y, z);
                m_centerLat = centerGeo.lat;
                m_centerLon = centerGeo.lon;
                NormalizeCenter();
                Invalidate();
                return;
            }
        }

        m_centerLat = (minLat + maxLat) * 0.5;
        m_centerLon = (minLon + maxLon) * 0.5;
        m_zoom = kDefaultZoom;
        NormalizeCenter();
        Invalidate();
    }

    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            if (errorOut)
                *errorOut = L"Could not open boundary file: " + path.wstring();
            return false;
        }

        json root;
        try {
            in >> root;
        }
        catch (const std::exception& e) {
            if (errorOut)
                *errorOut = L"Boundary JSON parse failed: " + Utf8ToWide(e.what());
            return false;
        }

        std::vector<std::vector<GeoPoint>> rings;
        CollectBoundaryRingsFromNode(root, rings);

        if (rings.empty()) {
            if (errorOut)
                *errorOut = L"No boundary rings found in GeoJSON file.";
            return false;
        }

        m_ukBoundaryRings = std::move(rings);
        Invalidate();
        return true;
    }

private:
    static bool RegisterClass()
    {
        static bool registered = false;
        if (registered)
            return true;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MapWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kMapClassName;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        registered = true;
        return true;
    }

    static LRESULT CALLBACK MapWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MapView* self = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MapView*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        }
        else {
            self = reinterpret_cast<MapView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
            return self->HandleMessage(msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            return 0;

        case WM_SIZE:
            if (m_rt) {
                UINT w = static_cast<UINT>(std::max<LONG>(1L, LOWORD(lParam)));
                UINT h = static_cast<UINT>(std::max<LONG>(1L, HIWORD(lParam)));
                m_rt->Resize(D2D1::SizeU(w, h));
            }
            return 0;

        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_LBUTTONDOWN:
            SetCapture(m_hwnd);
            m_mouseDown.x = GET_X_LPARAM(lParam);
            m_mouseDown.y = GET_Y_LPARAM(lParam);
            m_lastMouse = m_mouseDown;
            m_dragging = false;
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), static_cast<UINT>(wParam));
            return 0;

        case WM_LBUTTONUP:
            OnLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_MOUSEWHEEL:
            OnMouseWheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;

        case WM_APP_TILE_READY:
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;

        case WM_DESTROY:
            ClearTileCache();
            DiscardDeviceResources();
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    static WorldPoint GeoToWorld(double lat, double lon, int zoom)
    {
        lat = ClampValue(lat, -kMaxMercatorLat, kMaxMercatorLat);
        lon = ClampValue(lon, -180.0, 180.0);

        double worldSize = 256.0 * static_cast<double>(1 << zoom);
        double x = (lon + 180.0) / 360.0 * worldSize;
        double sinLat = std::sin(lat * kPi / 180.0);
        double y = (0.5 - std::log((1.0 + sinLat) / (1.0 - sinLat)) / (4.0 * kPi)) * worldSize;

        return { x, y };
    }

    static GeoPoint WorldToGeo(double x, double y, int zoom)
    {
        double worldSize = 256.0 * static_cast<double>(1 << zoom);
        double lon = x / worldSize * 360.0 - 180.0;
        double n = kPi - 2.0 * kPi * y / worldSize;
        double lat = 180.0 / kPi * std::atan(std::sinh(n));
        return { lat, lon };
    }

    GeoPoint ScreenToGeo(int sx, int sy) const
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        WorldPoint center = GeoToWorld(m_centerLat, m_centerLon, m_zoom);
        double worldX = center.x + (sx - width * 0.5);
        double worldY = center.y + (sy - height * 0.5);

        return WorldToGeo(worldX, worldY, m_zoom);
    }

    D2D1_POINT_2F GeoToScreen(double lat, double lon) const
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        WorldPoint center = GeoToWorld(m_centerLat, m_centerLon, m_zoom);
        WorldPoint p = GeoToWorld(lat, lon, m_zoom);

        float x = static_cast<float>((p.x - center.x) + width * 0.5);
        float y = static_cast<float>((p.y - center.y) + height * 0.5);

        return D2D1::Point2F(x, y);
    }

    void NormalizeCenter()
    {
        while (m_centerLon < -180.0) m_centerLon += 360.0;
        while (m_centerLon > 180.0)  m_centerLon -= 360.0;
        m_centerLat = ClampValue(m_centerLat, -kMaxMercatorLat, kMaxMercatorLat);
        m_zoom = ClampValue(m_zoom, kMinZoom, kMaxZoom);
    }

    void Invalidate()
    {
        if (m_hwnd)
            InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void MoveCenterByPixels(int dx, int dy)
    {
        WorldPoint center = GeoToWorld(m_centerLat, m_centerLon, m_zoom);
        center.x += dx;
        center.y += dy;

        GeoPoint geo = WorldToGeo(center.x, center.y, m_zoom);
        m_centerLat = geo.lat;
        m_centerLon = geo.lon;
        NormalizeCenter();
    }

    std::wstring HitTestAlert(int x, int y) const
    {
        std::wstring bestId;
        double bestDist = 14.0;

        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (!m_alerts[i].hasLocation)
                continue;

            D2D1_POINT_2F pt = GeoToScreen(m_alerts[i].latitude, m_alerts[i].longitude);
            double dx = pt.x - x;
            double dy = pt.y - y;
            double d = std::sqrt(dx * dx + dy * dy);

            if (d < bestDist) {
                bestDist = d;
                bestId = m_alerts[i].id;
            }
        }

        return bestId;
    }

    void OnMouseMove(int x, int y, UINT buttons)
    {
        if ((buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            POINT pt{ x, y };
            int dx = pt.x - m_lastMouse.x;
            int dy = pt.y - m_lastMouse.y;

            if (!m_dragging) {
                int adx = std::abs(pt.x - m_mouseDown.x);
                int ady = std::abs(pt.y - m_mouseDown.y);
                if (adx + ady > 3)
                    m_dragging = true;
            }

            if (m_dragging && (dx != 0 || dy != 0)) {
                MoveCenterByPixels(-dx, -dy);
                m_lastMouse = pt;
                Invalidate();
            }
        }
    }

    void OnLeftButtonUp(int x, int y)
    {
        if (GetCapture() == m_hwnd)
            ReleaseCapture();

        if (!m_dragging) {
            std::wstring id = HitTestAlert(x, y);
            if (!id.empty() && m_onSelect)
                m_onSelect(id);
        }

        m_dragging = false;
    }

    void OnMouseWheel(int screenX, int screenY, short delta)
    {
        POINT pt{ screenX, screenY };
        ScreenToClient(m_hwnd, &pt);

        int newZoom = m_zoom + ((delta > 0) ? 1 : -1);
        newZoom = ClampValue(newZoom, kMinZoom, kMaxZoom);
        if (newZoom == m_zoom)
            return;

        GeoPoint geoUnderCursor = ScreenToGeo(pt.x, pt.y);

        m_zoom = newZoom;

        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        WorldPoint target = GeoToWorld(geoUnderCursor.lat, geoUnderCursor.lon, m_zoom);
        WorldPoint newCenter;
        newCenter.x = target.x - (pt.x - width * 0.5);
        newCenter.y = target.y - (pt.y - height * 0.5);

        GeoPoint centerGeo = WorldToGeo(newCenter.x, newCenter.y, m_zoom);
        m_centerLat = centerGeo.lat;
        m_centerLon = centerGeo.lon;
        NormalizeCenter();
        Invalidate();
    }

    void EnsureDeviceResources()
    {
        if (m_rt)
            return;

        if (!g_d2dFactory)
            return;

        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        UINT width = static_cast<UINT>(std::max<LONG>(1L, rc.right - rc.left));
        UINT height = static_cast<UINT>(std::max<LONG>(1L, rc.bottom - rc.top));

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            m_hwnd,
            D2D1::SizeU(width, height));

        if (FAILED(g_d2dFactory->CreateHwndRenderTarget(rtProps, hwndProps, &m_rt)))
            return;

        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.10f, 0.10f, 0.95f), &m_severeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.62f, 0.18f, 0.95f), &m_moderateBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.52f, 0.90f, 0.95f), &m_minorBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.20f, 0.70f, 0.55f, 0.95f), &m_unknownBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.92f, 0.2f, 0.55f), &m_selectedBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.88f, 0.92f, 0.96f), &m_placeholderBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.35f, 0.35f), &m_borderBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.30f, 0.55f, 0.25f, 0.18f), &m_outlineFillBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.28f, 0.12f, 0.92f), &m_outlineStrokeBrush);
    }

    void DiscardDeviceResources()
    {
        m_selectedBrush.Reset();
        m_unknownBrush.Reset();
        m_minorBrush.Reset();
        m_moderateBrush.Reset();
        m_severeBrush.Reset();
        m_placeholderBrush.Reset();
        m_borderBrush.Reset();
        m_rt.Reset();
        m_outlineFillBrush.Reset();
        m_outlineStrokeBrush.Reset();
    }

    void ClearTileCache()
    {
        std::lock_guard<std::mutex> lk(m_tileMutex);
        m_tiles.clear();
    }

    static std::wstring BuildTileUrl(int z, int x, int y)
    {
        wchar_t buf[256]{};
        swprintf_s(buf, L"https://tile.openstreetmap.org/%d/%d/%d.png", z, x, y);
        return buf;
    }

    std::shared_ptr<TileEntry> GetOrCreateTile(const TileKey& key)
    {
        std::lock_guard<std::mutex> lk(m_tileMutex);
        auto& entry = m_tiles[key];
        if (!entry)
            entry = std::make_shared<TileEntry>();
        return entry;
    }

    void RequestTile(const TileKey& key)
    {
        auto entry = GetOrCreateTile(key);

        {
            std::lock_guard<std::mutex> lk(entry->mutex);
            const ULONGLONG now = GetTickCount64();

            if (entry->ready || entry->loading)
                return;

            // Throttle retries if a tile recently failed.
            if (entry->failed && (now - entry->lastAttemptMs) < 30000)
                return;

            entry->loading = true;
            entry->lastAttemptMs = now;
        }

        HWND hwnd = m_hwnd;

        std::thread([hwnd, key, entry]() {
            std::vector<BYTE> bytes;
            std::wstring error;
            bool ok = HttpGetBinary(BuildTileUrl(key.z, key.x, key.y), bytes, error);

            {
                std::lock_guard<std::mutex> lk(entry->mutex);
                entry->loading = false;
                if (ok) {
                    entry->bytes = std::move(bytes);
                    entry->ready = true;
                    entry->failed = false;
                }
                else {
                    entry->failed = true;
                }
            }

            if (hwnd && !g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_TILE_READY, 0, 0);
            }).detach();
    }

    ComPtr<ID2D1Bitmap> CreateBitmapFromBytes(const std::vector<BYTE>& bytes)
    {
        if (!m_rt || !g_wicFactory || bytes.empty())
            return {};

        ComPtr<IWICStream> stream;
        if (FAILED(g_wicFactory->CreateStream(&stream)))
            return {};

        if (FAILED(stream->InitializeFromMemory(
            const_cast<BYTE*>(bytes.data()),
            static_cast<DWORD>(bytes.size()))))
        {
            return {};
        }

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(g_wicFactory->CreateDecoderFromStream(
            stream.Get(),
            nullptr,
            WICDecodeMetadataCacheOnLoad,
            &decoder)))
        {
            return {};
        }

        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, &frame)))
            return {};

        ComPtr<ID2D1Bitmap> bmp;
        if (FAILED(m_rt->CreateBitmapFromWicBitmap(frame.Get(), nullptr, &bmp)))
            return {};

        return bmp;
    }

    ID2D1SolidColorBrush* BrushForSeverity(const std::wstring& severity)
    {
        std::wstring b = SeverityBucket(severity);
        if (b == L"severe") return m_severeBrush.Get();
        if (b == L"moderate") return m_moderateBrush.Get();
        if (b == L"minor") return m_minorBrush.Get();
        return m_unknownBrush.Get();
    }

    void DrawMarkers()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        int width = static_cast<int>(rc.right - rc.left);
        int height = static_cast<int>(rc.bottom - rc.top);

        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (!m_alerts[i].hasLocation)
                continue;

            D2D1_POINT_2F p = GeoToScreen(m_alerts[i].latitude, m_alerts[i].longitude);
            if (p.x < -20.0f || p.y < -20.0f || p.x > width + 20.0f || p.y > height + 20.0f)
                continue;

            bool selected = (m_alerts[i].id == m_selectedId);
            ID2D1SolidColorBrush* sevBrush = BrushForSeverity(m_alerts[i].severity);

            float outerR = selected ? 10.0f : 7.0f;
            float innerR = selected ? 6.0f : 5.0f;

            D2D1_ELLIPSE outer = D2D1::Ellipse(p, outerR, outerR);
            D2D1_ELLIPSE inner = D2D1::Ellipse(p, innerR, innerR);

            if (selected)
                m_rt->FillEllipse(outer, m_selectedBrush.Get());

            m_rt->FillEllipse(inner, sevBrush);
            m_rt->DrawEllipse(outer, m_borderBrush.Get(), selected ? 2.0f : 1.0f);
        }
    }

    void DrawTiles()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        int width = std::max(1, static_cast<int>(rc.right - rc.left));
        int height = std::max(1, static_cast<int>(rc.bottom - rc.top));

        WorldPoint centerWorld = GeoToWorld(m_centerLat, m_centerLon, m_zoom);
        double originX = centerWorld.x - width * 0.5;
        double originY = centerWorld.y - height * 0.5;

        int startTileX = static_cast<int>(std::floor(originX / 256.0)) - 1;
        int endTileX = static_cast<int>(std::floor((originX + width) / 256.0)) + 1;
        int startTileY = static_cast<int>(std::floor(originY / 256.0)) - 1;
        int endTileY = static_cast<int>(std::floor((originY + height) / 256.0)) + 1;

        int tilesPerAxis = 1 << m_zoom;

        for (int ty = startTileY; ty <= endTileY; ++ty) {
            if (ty < 0 || ty >= tilesPerAxis)
                continue;

            for (int tx = startTileX; tx <= endTileX; ++tx) {
                int wrappedX = tx;
                while (wrappedX < 0) wrappedX += tilesPerAxis;
                while (wrappedX >= tilesPerAxis) wrappedX -= tilesPerAxis;

                TileKey key{ m_zoom, wrappedX, ty };
                auto entry = GetOrCreateTile(key);

                D2D1_RECT_F dest = D2D1::RectF(
                    static_cast<float>(tx * 256.0 - originX),
                    static_cast<float>(ty * 256.0 - originY),
                    static_cast<float>(tx * 256.0 - originX + 256.0),
                    static_cast<float>(ty * 256.0 - originY + 256.0));

                ComPtr<ID2D1Bitmap> bmp;
                std::vector<BYTE> bytesCopy;

                {
                    std::lock_guard<std::mutex> lk(entry->mutex);
                    if (entry->bitmap) {
                        bmp = entry->bitmap;
                    }
                    else if (entry->ready && !entry->bytes.empty()) {
                        bytesCopy = entry->bytes;
                    }
                }

                if (!bmp && !bytesCopy.empty()) {
                    bmp = CreateBitmapFromBytes(bytesCopy);
                    if (bmp) {
                        std::lock_guard<std::mutex> lk(entry->mutex);
                        if (!entry->bitmap)
                            entry->bitmap = bmp;
                    }
                }

                if (bmp) {
                    m_rt->DrawBitmap(bmp.Get(), dest);
                }
                else {
                    RequestTile(key);
                    m_rt->FillRectangle(dest, m_placeholderBrush.Get());
                    m_rt->DrawRectangle(dest, m_borderBrush.Get(), 0.5f);
                }
            }
        }
    }

    void OnPaint()
    {
        PAINTSTRUCT ps{};
        BeginPaint(m_hwnd, &ps);

        EnsureDeviceResources();
        if (m_rt) {
            m_rt->BeginDraw();
            m_rt->Clear(D2D1::ColorF(0.80f, 0.91f, 0.98f, 1.0f));

            DrawTiles();
            DrawUkBoundary();
            DrawMarkers();

            HRESULT hr = m_rt->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET)
                DiscardDeviceResources();
        }

        EndPaint(m_hwnd, &ps);
    }

    void DrawClosedPolygon(const GeoPoint* pts, size_t count)
    {
        if (!m_rt || !g_d2dFactory || !pts || count < 3)
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        sink->BeginFigure(GeoToScreen(pts[0].lat, pts[0].lon), D2D1_FIGURE_BEGIN_FILLED);

        for (size_t i = 1; i < count; ++i) {
            sink->AddLine(GeoToScreen(pts[i].lat, pts[i].lon));
        }

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        if (FAILED(sink->Close()))
            return;

        if (m_outlineFillBrush)
            m_rt->FillGeometry(geom.Get(), m_outlineFillBrush.Get());

        if (m_outlineStrokeBrush)
            m_rt->DrawGeometry(geom.Get(), m_outlineStrokeBrush.Get(), 2.0f);
    }

    void DrawUkOutline()
    {
        static const GeoPoint greatBritain[] = {
            { 50.05, -5.75 }, { 50.30, -5.30 }, { 50.48, -4.90 }, { 50.65, -4.20 },
            { 50.83, -3.50 }, { 51.05, -2.90 }, { 51.30, -2.40 }, { 51.55, -2.00 },
            { 51.85, -1.65 }, { 52.20, -1.20 }, { 52.55, -0.95 }, { 52.90, -0.90 },
            { 53.25, -1.00 }, { 53.65, -1.35 }, { 54.00, -1.85 }, { 54.35, -2.50 },
            { 54.60, -3.10 }, { 54.95, -3.60 }, { 55.30, -4.10 }, { 55.65, -4.65 },
            { 55.95, -5.10 }, { 56.20, -5.05 }, { 56.35, -4.60 }, { 56.45, -4.00 },
            { 56.40, -3.30 }, { 56.30, -2.60 }, { 56.10, -1.90 }, { 55.90, -1.30 },
            { 55.70, -0.65 }, { 55.45, -0.15 }, { 55.05,  0.10 }, { 54.65,  0.25 },
            { 54.20,  0.30 }, { 53.75,  0.24 }, { 53.30,  0.15 }, { 52.85,  0.15 },
            { 52.35,  0.05 }, { 51.95, -0.15 }, { 51.50, -0.40 }, { 51.10, -0.70 },
            { 50.80, -1.05 }, { 50.50, -1.45 }, { 50.28, -1.95 }, { 50.12, -2.60 },
            { 50.05, -3.40 }, { 50.02, -4.40 }, { 50.05, -5.20 }
        };

        static const GeoPoint northernIreland[] = {
            { 54.00, -8.15 }, { 54.25, -8.00 }, { 54.55, -7.70 }, { 54.85, -7.30 },
            { 55.10, -6.90 }, { 55.25, -6.40 }, { 55.20, -6.00 }, { 54.95, -5.90 },
            { 54.60, -6.05 }, { 54.25, -6.45 }, { 54.05, -6.95 }, { 54.00, -7.50 },
            { 54.00, -8.15 }
        };

        DrawClosedPolygon(greatBritain, _countof(greatBritain));
        DrawClosedPolygon(northernIreland, _countof(northernIreland));
    }

    void DrawBoundaryRing(const std::vector<GeoPoint>& ring)
    {
        if (!m_rt || ring.size() < 3)
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

        sink->BeginFigure(
            GeoToScreen(ring[0].lat, ring[0].lon),
            D2D1_FIGURE_BEGIN_FILLED);

        for (size_t i = 1; i < ring.size(); ++i)
            sink->AddLine(GeoToScreen(ring[i].lat, ring[i].lon));

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        if (FAILED(sink->Close()))
            return;

        if (m_outlineFillBrush)
            m_rt->FillGeometry(geom.Get(), m_outlineFillBrush.Get());

        if (m_outlineStrokeBrush)
            m_rt->DrawGeometry(geom.Get(), m_outlineStrokeBrush.Get(), 2.0f);
    }

    void DrawUkBoundary()
    {
        for (const auto& ring : m_ukBoundaryRings)
            DrawBoundaryRing(ring);
    }

    HWND m_hwnd = nullptr;
    std::vector<TrafficAlert> m_alerts;
    std::wstring m_selectedId;
    SelectCallback m_onSelect;

    int m_zoom = kDefaultZoom;
    double m_centerLat = kDefaultCenterLat;
    double m_centerLon = kDefaultCenterLon;

    POINT m_mouseDown{};
    POINT m_lastMouse{};
    bool m_dragging = false;

    ComPtr<ID2D1HwndRenderTarget> m_rt;
    ComPtr<ID2D1SolidColorBrush> m_severeBrush;
    ComPtr<ID2D1SolidColorBrush> m_moderateBrush;
    ComPtr<ID2D1SolidColorBrush> m_minorBrush;
    ComPtr<ID2D1SolidColorBrush> m_unknownBrush;
    ComPtr<ID2D1SolidColorBrush> m_selectedBrush;
    ComPtr<ID2D1SolidColorBrush> m_placeholderBrush;
    ComPtr<ID2D1SolidColorBrush> m_borderBrush;
    ComPtr<ID2D1SolidColorBrush> m_outlineFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_outlineStrokeBrush;
    std::vector<std::vector<GeoPoint>> m_ukBoundaryRings;

    std::mutex m_tileMutex;
    std::unordered_map<TileKey, std::shared_ptr<TileEntry>, TileKeyHash> m_tiles;
    
};

// ============================================================
// MainWindow
// ============================================================

struct FeedResult
{
    bool ok = false;
    std::wstring error;
    std::vector<TrafficAlert> alerts;
};

struct BoundaryDownloadResult
{
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
};

class MainWindow
{
public:
    bool Create(HINSTANCE hInst)
    {
        m_hInst = hInst;
        if (!RegisterClass())
            return false;

        m_hwnd = CreateWindowExW(
            0,
            kMainClassName,
            L"Traffic England Alerts Map",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1600,
            950,
            nullptr,
            nullptr,
            hInst,
            this);

        return m_hwnd != nullptr;
    }

    int Run(int nCmdShow)
    {
        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

private:
    static constexpr const wchar_t* kBaseTitle = L"Traffic England Alerts Map";

    static bool RegisterClass()
    {
        static bool registered = false;
        if (registered)
            return true;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = MainWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kMainClassName;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        registered = true;
        return true;
    }

    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        }
        else {
            self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
            return self->HandleMessage(msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            OnCreate();
            return 0;

        case WM_SIZE:
            Layout();
            return 0;

        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_NOTIFY:
            OnNotify(reinterpret_cast<NMHDR*>(lParam));
            return 0;

        case WM_TIMER:
            if (wParam == 1)
                RefreshFeedAsync();
            return 0;

        case WM_APP_FEED_READY:
            OnFeedReady(reinterpret_cast<FeedResult*>(lParam));
            return 0;

        case WM_APP_BOUNDARY_READY:
            OnBoundaryReady(reinterpret_cast<BoundaryDownloadResult*>(lParam));
            return 0;

        case WM_DESTROY:
            g_appQuitting.store(true);
            KillTimer(m_hwnd, 1);
            if (m_font) {
                DeleteObject(m_font);
                m_font = nullptr;
            }
            if (m_headerFont) {
                DeleteObject(m_headerFont);
                m_headerFont = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void OnCreate()
    {
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icc);

        EnableModernWindowFrame(m_hwnd);

        m_font = CreateUiFont(10);
        m_headerFont = CreateUiFont(16, FW_SEMIBOLD);

        m_headerLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Traffic England Alerts",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_HEADER_LABEL),
            m_hInst,
            nullptr);

        m_urlLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Alerts endpoint",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_URL_LABEL),
            m_hInst,
            nullptr);

        m_searchLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Search",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEARCH_LABEL),
            m_hInst,
            nullptr);

        m_severityLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Severity",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEVERITY_LABEL),
            m_hInst,
            nullptr);

        m_urlEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_URL_EDIT),
            m_hInst,
            nullptr);

        m_refreshBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_REFRESH_BTN),
            m_hInst,
            nullptr);

        m_boundaryBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Download UK boundary",
            WS_CHILD | WS_VISIBLE | BS_COMMANDLINK,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_DOWNLOAD_BOUNDARY_BTN),
            m_hInst,
            nullptr);

        m_searchEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEARCH_EDIT),
            m_hInst,
            nullptr);

        m_severityCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEVERITY_COMBO),
            m_hInst,
            nullptr);

        m_listView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_LISTVIEW),
            m_hInst,
            nullptr);

        m_detailsEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_DETAILS_EDIT),
            m_hInst,
            nullptr);

        m_statusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            L"Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_STATUS_BAR),
            m_hInst,
            nullptr);

        if (!m_headerLabel || !m_urlLabel || !m_searchLabel || !m_severityLabel ||
            !m_urlEdit || !m_refreshBtn || !m_boundaryBtn || !m_searchEdit || !m_severityCombo || !m_listView || !m_detailsEdit || !m_statusBar)
        {
            MessageBoxW(m_hwnd, L"Failed to create one or more child controls.", L"Traffic England Alerts Map", MB_ICONERROR);
            return;
        }

        for (HWND h : { m_urlLabel, m_searchLabel, m_severityLabel, m_urlEdit, m_refreshBtn, m_boundaryBtn, m_searchEdit, m_severityCombo, m_listView, m_detailsEdit, m_statusBar }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_headerLabel, WM_SETFONT, reinterpret_cast<WPARAM>(m_headerFont), TRUE);

        SendMessageW(m_searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter by road, region, or description"));
        SendMessageW(m_urlEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"https://www.trafficengland.com/traffic-alerts"));
        SendMessageW(m_boundaryBtn, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(L"Optional: cache a high-detail UK outline for the native map."));

        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Severe"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Moderate"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minor"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unknown"));
        SendMessageW(m_severityCombo, CB_SETCURSEL, 0, 0);

        SendMessageW(m_listView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        std::wstring s0 = L"Severity";
        col.pszText = const_cast<LPWSTR>(s0.c_str());
        col.cx = 90;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&col));

        std::wstring s1 = L"Alert";
        col.pszText = const_cast<LPWSTR>(s1.c_str());
        col.cx = 260;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 1, reinterpret_cast<LPARAM>(&col));

        std::wstring s2 = L"Updated";
        col.pszText = const_cast<LPWSTR>(s2.c_str());
        col.cx = 160;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 2, reinterpret_cast<LPARAM>(&col));

        m_map.Create(m_hwnd, 0, 0, 100, 100);
        if (!m_map.Hwnd()) {
            MessageBoxW(m_hwnd, L"Could not create the native map view.", L"Traffic England Alerts Map", MB_ICONERROR);
            return;
        }

        std::wstring boundaryError;
        if (!m_map.LoadUkBoundaryFromFile(GetBoundaryCachePath(), &boundaryError)) {
            OutputDebugStringW((L"Boundary cache load: " + boundaryError + L"\n").c_str());
        }

        m_map.SetSelectCallback([this](const std::wstring& id) {
            SelectAlertById(id, true);
            });

        SetWindowTextW(m_urlEdit, L"https://www.trafficengland.com/traffic-alerts");

        Layout();
        SetStatusText(L"Ready.");
        SetTimer(m_hwnd, 1, 5 * 60 * 1000, nullptr);

        RefreshFeedAsync();
    }

    void Layout()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        const int pad = 16;
        const int labelH = 18;
        const int controlH = 28;
        const int topBarH = 118;
        const int statusH = 24;

        MoveWindow(m_headerLabel, pad, 12, std::max<LONG>(200L, width - pad * 2), 28, TRUE);

        const int endpointY = 52;
        const LONG refreshW = 132;
        MoveWindow(m_urlLabel, pad, endpointY, 160, labelH, TRUE);
        MoveWindow(m_urlEdit, pad, endpointY + labelH + 2, std::max<LONG>(220L, width - refreshW - pad * 3), controlH, TRUE);
        MoveWindow(m_refreshBtn, width - refreshW - pad, endpointY + labelH + 2, refreshW, controlH, TRUE);

        int bodyTop = topBarH;
        int leftW = 440;
        int detailsH = 250;

        int leftX = pad;
        int leftY = bodyTop + pad;
        int leftInnerW = leftW - pad * 2;

        MoveWindow(m_searchLabel, leftX, leftY, leftInnerW, labelH, TRUE);
        MoveWindow(m_searchEdit, leftX, leftY + labelH + 2, leftInnerW, controlH, TRUE);

        const int severityY = leftY + labelH + controlH + 12;
        MoveWindow(m_severityLabel, leftX, severityY, leftInnerW, labelH, TRUE);
        MoveWindow(m_severityCombo, leftX, severityY + labelH + 2, leftInnerW, 180, TRUE);

        const int boundaryY = severityY + labelH + controlH + 16;
        MoveWindow(m_boundaryBtn, leftX, boundaryY, leftInnerW, 56, TRUE);

        int listTop = boundaryY + 68;
        int bodyHeight = height - bodyTop - statusH - pad * 2;
        int listHeight = std::max(120, bodyHeight - (listTop - leftY) - detailsH - 12);

        MoveWindow(m_listView, leftX, listTop, leftInnerW, listHeight, TRUE);
        MoveWindow(m_detailsEdit, leftX, listTop + listHeight + 10, leftInnerW, detailsH, TRUE);

        int mapX = leftW + pad;
        int mapY = bodyTop + pad;
        LONG mapW = std::max<LONG>(100L, width - mapX - pad);
        LONG mapH = std::max<LONG>(100L, height - mapY - statusH - pad);

        MoveWindow(m_map.Hwnd(), mapX, mapY, mapW, mapH, TRUE);

        SendMessageW(m_statusBar, WM_SIZE, 0, 0);

        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 0, 94);
        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 1, std::max(120, leftInnerW - 264));
        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 2, 160);
    }

    void OnCommand(int id, int code)
    {
        switch (id) {
        case IDC_REFRESH_BTN:
            if (code == BN_CLICKED)
                RefreshFeedAsync();
            break;

        case IDC_SEARCH_EDIT:
            if (code == EN_CHANGE) {
                size_t visible = ApplyFilters(false);
                SetStatusText(L"Showing " + std::to_wstring(visible) + L" alert(s).");
            }
            break;

        case IDC_SEVERITY_COMBO:
            if (code == CBN_SELCHANGE) {
                size_t visible = ApplyFilters(false);
                SetStatusText(L"Showing " + std::to_wstring(visible) + L" alert(s).");
            }
            break;
        case IDC_DOWNLOAD_BOUNDARY_BTN:
            if (code == BN_CLICKED)
                DownloadBoundaryFromGitHubAsync();
            break;
        }
    }

    void OnNotify(NMHDR* nmh)
    {
        if (!nmh)
            return;

        if (nmh->hwndFrom == m_listView && nmh->code == LVN_ITEMCHANGED) {
            if (m_programmaticSelection)
                return;

            NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
            if ((lv->uChanged & LVIF_STATE) && (lv->uNewState & LVIS_SELECTED)) {
                int selected = static_cast<int>(SendMessageW(m_listView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (selected >= 0 && selected < static_cast<int>(m_filteredAlerts.size())) {
                    const std::wstring& id = m_filteredAlerts[static_cast<size_t>(selected)].id;
                    SelectAlertById(id, true);
                }
            }
        }
    }

    void SetStatusText(const std::wstring& text)
    {
        std::wstring title = kBaseTitle;
        if (!text.empty()) {
            title += L" - ";
            title += text;
        }
        SetWindowTextW(m_hwnd, title.c_str());
        if (m_statusBar)
            SendMessageW(m_statusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }

    void RefreshFeedAsync()
    {
        if (g_fetchInProgress.exchange(true)) {
            SetStatusText(L"Already fetching alerts...");
            return;
        }

        std::wstring url = NormalizeUrl(GetWindowTextString(m_urlEdit));
        if (url.empty()) {
            g_fetchInProgress.store(false);
            SetStatusText(L"Please enter a feed URL.");
            return;
        }

        SetStatusText(L"Fetching alerts...");

        HWND hwnd = m_hwnd;

        std::thread([hwnd, url]() {
            auto* result = new FeedResult{};
            std::string body;
            std::wstring error;

            if (HttpGetText(url, body, error)) {
                std::wstring parseError;
                std::vector<TrafficAlert> alerts = ParseTrafficAlerts(body, parseError);

                if (!alerts.empty()) {
                    result->ok = true;
                    result->alerts = std::move(alerts);
                }
                else {
                    result->ok = false;
                    result->error = parseError.empty()
                        ? L"Feed could not be parsed. Showing sample data."
                        : parseError + L" Showing sample data.";
                    result->alerts = SampleAlerts();
                }
            }
            else {
                result->ok = false;
                result->error = error + L" Showing sample data.";
                result->alerts = SampleAlerts();
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_fetchInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_FEED_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_fetchInProgress.store(false);
            }
            }).detach();
    }

    void OnFeedReady(FeedResult* result)
    {
        g_fetchInProgress.store(false);

        if (!result)
            return;

        m_allAlerts = result->alerts;
        std::sort(m_allAlerts.begin(), m_allAlerts.end(),
            [](const TrafficAlert& a, const TrafficAlert& b)
            {
                std::wstring ar = ToLower(Trim(a.road.empty() ? a.region : a.road));
                std::wstring br = ToLower(Trim(b.road.empty() ? b.region : b.road));

                if (ar != br)
                    return ar < br;

                return ToLower(Trim(a.title)) < ToLower(Trim(b.title));
            });
        delete result;

        size_t visible = ApplyFilters(true);

        if (m_allAlerts.empty()) {
            SetStatusText(L"No alerts available.");
        }
        else {
            SetStatusText(L"Loaded " + std::to_wstring(visible) + L" alert(s).");
        }
    }

    bool TextFilterMatches(const TrafficAlert& a) const
    {
        std::wstring q = ToLower(Trim(GetWindowTextString(m_searchEdit)));
        if (q.empty())
            return true;

        std::wstring hay =
            ToLower(a.id + L" " + a.title + L" " + a.description + L" " +
                a.road + L" " + a.region + L" " + a.severity);

        return hay.find(q) != std::wstring::npos;
    }

    bool SeverityFilterMatches(const TrafficAlert& a) const
    {
        int idx = static_cast<int>(SendMessageW(m_severityCombo, CB_GETCURSEL, 0, 0));
        if (idx <= 0)
            return true;

        std::wstring filter;
        switch (idx) {
        case 1: filter = L"severe"; break;
        case 2: filter = L"moderate"; break;
        case 3: filter = L"minor"; break;
        case 4: filter = L"unknown"; break;
        default: return true;
        }

        return SeverityBucket(a.severity) == filter;
    }

    size_t ApplyFilters(bool fitMap)
    {
        std::wstring previousSelected = m_selectedId;

        m_filteredAlerts.clear();
        for (size_t i = 0; i < m_allAlerts.size(); ++i) {
            if (TextFilterMatches(m_allAlerts[i]) && SeverityFilterMatches(m_allAlerts[i]))
                m_filteredAlerts.push_back(m_allAlerts[i]);
        }

        m_programmaticSelection = true;
        SendMessageW(m_listView, LVM_DELETEALLITEMS, 0, 0);

        for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
            const TrafficAlert& a = m_filteredAlerts[i];

            std::wstring sev = BuildSeverityDisplay(a.severity);
            std::wstring summary = BuildAlertSummary(a);
            std::wstring updated = a.updatedText.empty() ? L"" : a.updatedText;

            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(sev.c_str());

            int row = static_cast<int>(
                SendMessageW(m_listView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item))
                );

            if (row >= 0) {
                LVITEMW sub{};
                sub.iSubItem = 1;
                sub.pszText = const_cast<LPWSTR>(summary.c_str());
                SendMessageW(m_listView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));

                sub.iSubItem = 2;
                sub.pszText = const_cast<LPWSTR>(updated.c_str());
                SendMessageW(m_listView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));
            }
        }

        m_programmaticSelection = false;

        m_map.SetAlerts(m_filteredAlerts);

        if (m_filteredAlerts.empty()) {
            m_selectedId.clear();
            m_map.SetSelectedId(L"");
            SetWindowTextSafe(m_detailsEdit, L"No alerts match the current filters.");
        }
        else {
            bool foundPrevious = false;
            for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
                if (m_filteredAlerts[i].id == previousSelected) {
                    foundPrevious = true;
                    break;
                }
            }

            if (foundPrevious)
                m_selectedId = previousSelected;
            else
                m_selectedId = m_filteredAlerts.front().id;

            SelectAlertById(m_selectedId, false);
        }

        if (fitMap)
            m_map.FitToAlerts();

        return m_filteredAlerts.size();
    }

    int FindAlertIndexById(const std::wstring& id) const
    {
        for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
            if (m_filteredAlerts[i].id == id)
                return static_cast<int>(i);
        }

        return -1;
    }

    void SelectAlertById(const std::wstring& id, bool centerMap)
    {
        int idx = FindAlertIndexById(id);
        if (idx < 0)
            return;

        m_selectedId = id;

        const TrafficAlert& a = m_filteredAlerts[static_cast<size_t>(idx)];
        SetWindowTextSafe(m_detailsEdit, BuildAlertDetails(a));
        m_map.SetSelectedId(id);

        if (centerMap)
            m_map.CenterOnAlert(id);

        m_programmaticSelection = true;

        LVITEMW state{};
        state.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        state.state = LVIS_SELECTED | LVIS_FOCUSED;
        state.iItem = idx;
        state.iSubItem = 0;

        SendMessageW(m_listView, LVM_SETITEMSTATE, idx, reinterpret_cast<LPARAM>(&state));
        SendMessageW(m_listView, LVM_ENSUREVISIBLE, idx, TRUE);

        m_programmaticSelection = false;
    }

    void DownloadBoundaryFromGitHubAsync()
    {
        if (g_boundaryDownloadInProgress.exchange(true)) {
            SetStatusText(L"Boundary download already in progress...");
            return;
        }

        SetStatusText(L"Downloading UK boundary from geoBoundaries...");

        HWND hwnd = m_hwnd;

        std::thread([hwnd]() {
            auto* result = new BoundaryDownloadResult{};
            std::vector<BYTE> bytes;
            std::wstring error;

            if (!HttpGetBinary(kUkBoundarySourceUrl, bytes, error)) {
                result->ok = false;
                result->error = L"Boundary download failed: " + error;
            }
            else {
                std::filesystem::path path = GetBoundaryCachePath();
                if (!SaveBinaryToFile(path, bytes)) {
                    result->ok = false;
                    result->error = L"Boundary downloaded but could not be saved locally.";
                }
                else {
                    result->ok = true;
                    result->filePath = path;
                }
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_boundaryDownloadInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_BOUNDARY_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_boundaryDownloadInProgress.store(false);
            }
            }).detach();
    }

    void OnBoundaryReady(BoundaryDownloadResult* result)
    {
        g_boundaryDownloadInProgress.store(false);

        if (!result)
            return;

        if (!result->ok) {
            SetStatusText(result->error);
            delete result;
            return;
        }

        std::wstring loadError;
        if (m_map.LoadUkBoundaryFromFile(result->filePath, &loadError)) {
            SetStatusText(L"UK boundary downloaded and loaded.");
            
        }
        else {
            SetStatusText(L"Boundary downloaded, but could not load it.");
            OutputDebugStringW((L"Boundary load failed: " + loadError + L"\n").c_str());
        }

        delete result;
    }

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInst = nullptr;
    HFONT m_font = nullptr;
    HFONT m_headerFont = nullptr;

    HWND m_headerLabel = nullptr;
    HWND m_urlLabel = nullptr;
    HWND m_searchLabel = nullptr;
    HWND m_severityLabel = nullptr;
    HWND m_statusBar = nullptr;
    HWND m_urlEdit = nullptr;
    HWND m_refreshBtn = nullptr;
    HWND m_searchEdit = nullptr;
    HWND m_severityCombo = nullptr;
    HWND m_listView = nullptr;
    HWND m_detailsEdit = nullptr;
    HWND m_boundaryBtn = nullptr;

    MapView m_map;

    std::vector<TrafficAlert> m_allAlerts;
    std::vector<TrafficAlert> m_filteredAlerts;
    std::wstring m_selectedId;
    bool m_programmaticSelection = false;
};

// ============================================================
// Direct2D / WIC factory init
// ============================================================

static bool InitGraphicsFactories()
{
    if (!g_d2dFactory) {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&g_d2dFactory))))
            return false;
    }

    if (!g_wicFactory) {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_wicFactory))))
            return false;
    }

    return true;
}

// ============================================================
// Entry point
// ============================================================

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    OpenConsole();
    ConsoleLog(L"Application starting...");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"COM initialization failed.", L"Traffic England Alerts Map", MB_ICONERROR);
        return 0;
    }

    if (!InitGraphicsFactories()) {
        MessageBoxW(nullptr, L"Could not initialize Direct2D / WIC.", L"Traffic England Alerts Map", MB_ICONERROR);
        CoUninitialize();
        return 0;
    }

    MainWindow app;
    if (!app.Create(hInstance)) {
        MessageBoxW(nullptr, L"Could not create main window.", L"Traffic England Alerts Map", MB_ICONERROR);
        CoUninitialize();
        return 0;
    }

    int ret = app.Run(nCmdShow);
    CoUninitialize();
    return ret;
}