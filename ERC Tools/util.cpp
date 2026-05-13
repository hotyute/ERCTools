// =================================================================================
// FILE: util.cpp
// =================================================================================


#include "util.h"
#include "app_state.h"

void OpenConsole()
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

void ConsoleLog(const std::wstring& text)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return;

    std::wstring line = text + L"\r\n";
    DWORD written = 0;
    WriteConsoleW(h, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
}


static std::filesystem::path GetTrafficEnglandCacheFolder()
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
    return folder;
}

std::filesystem::path GetBoundaryCachePath()
{
    return GetTrafficEnglandCacheFolder() / L"uk_outline.geojson";
}

std::filesystem::path GetLaneImageCachePath(const std::wstring& imageUrl)
{
    std::filesystem::path folder = GetTrafficEnglandCacheFolder() / L"lane_images";
    CreateDirectoryW(folder.c_str(), nullptr);

    std::wstring extension = L".png";
    size_t query = imageUrl.find_first_of(L"?#");
    std::wstring clean = imageUrl.substr(0, query);
    size_t dot = clean.find_last_of(L'.');
    size_t slash = clean.find_last_of(L"/\\");
    if (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash)) {
        std::wstring candidate = clean.substr(dot);
        if (!candidate.empty() && candidate.size() <= 8)
            extension = candidate;
    }

    std::wstring hash = std::to_wstring(std::hash<std::wstring>{}(imageUrl));
    return folder / (L"lane_" + hash + extension);
}

bool SaveBinaryToFile(const std::filesystem::path& path, const std::vector<BYTE>& bytes)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;

    out.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    return out.good();
}

std::wstring Trim(const std::wstring& s)
{
    size_t start = 0;
    size_t end = s.size();

    while (start < end && iswspace(s[start]))
        ++start;
    while (end > start && iswspace(s[end - 1]))
        --end;

    return s.substr(start, end - start);
}

std::wstring ToLower(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return s;
}

std::wstring Utf8ToWide(const std::string& s)
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

std::wstring WinErrorText(DWORD err)
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

std::wstring GetWindowTextString(HWND hWnd)
{
    int len = GetWindowTextLengthW(hWnd);
    if (len <= 0)
        return {};

    std::vector<wchar_t> buf(static_cast<size_t>(len) + 1);
    GetWindowTextW(hWnd, buf.data(), len + 1);
    return std::wstring(buf.data());
}

void SetWindowTextSafe(HWND hWnd, const std::wstring& text)
{
    SetWindowTextW(hWnd, text.c_str());
}

std::wstring TimeTToText(std::time_t t)
{
    std::tm tmv{};
    localtime_s(&tmv, &t);

    wchar_t buf[64]{};
    wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

std::wstring JsonValueToText(const json& v)
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

    if (v.is_array()) {
        std::wstring joined;
        for (const auto& item : v) {
            std::wstring part = Trim(JsonValueToText(item));
            if (part.empty())
                continue;

            if (!joined.empty())
                joined += L" ";
            joined += part;
        }
        return joined;
    }

    if (v.is_object()) {
        static const char* preferredKeys[] = {
            "display", "text", "html", "value", "data", "rendered", "filter", "sort"
        };

        for (const char* key : preferredKeys) {
            auto it = v.find(key);
            if (it == v.end())
                continue;

            std::wstring value = Trim(JsonValueToText(*it));
            if (!value.empty())
                return value;
        }

        std::wstring joined;
        for (auto it = v.begin(); it != v.end(); ++it) {
            std::wstring part = Trim(JsonValueToText(*it));
            if (part.empty())
                continue;

            if (!joined.empty())
                joined += L" ";
            joined += part;
        }
        return joined;
    }

    return {};
}

bool TryGetDoubleFromJsonValue(const json& v, double& out)
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

static std::string NormalizeJsonKeyName(std::string key)
{
    std::string out;
    out.reserve(key.size());

    for (unsigned char ch : key) {
        if (std::isalnum(ch))
            out.push_back(static_cast<char>(std::tolower(ch)));
    }

    return out;
}

static json::const_iterator FindJsonKey(const json& obj, const char* key)
{
    auto it = obj.find(key);
    if (it != obj.end())
        return it;

    std::string wanted = NormalizeJsonKeyName(key);

    for (auto candidate = obj.begin(); candidate != obj.end(); ++candidate) {
        if (NormalizeJsonKeyName(candidate.key()) == wanted)
            return candidate;
    }

    return obj.end();
}

std::wstring PickString(const json& obj, std::initializer_list<const char*> keys)
{
    if (!obj.is_object())
        return {};

    for (const char* key : keys) {
        auto it = FindJsonKey(obj, key);
        if (it == obj.end())
            continue;

        std::wstring value = Trim(JsonValueToText(*it));
        if (!value.empty())
            return value;
    }

    return {};
}

bool PickDouble(const json& obj, std::initializer_list<const char*> keys, double& out)
{
    if (!obj.is_object())
        return false;

    for (const char* key : keys) {
        auto it = FindJsonKey(obj, key);
        if (it == obj.end())
            continue;

        if (TryGetDoubleFromJsonValue(*it, out))
            return true;
    }

    return false;
}

std::wstring PickDateText(const json& obj, std::initializer_list<const char*> keys)
{
    if (!obj.is_object())
        return {};

    for (const char* key : keys) {
        auto it = FindJsonKey(obj, key);
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

std::wstring BuildSeverityDisplay(const std::wstring& severity)
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

std::wstring SeverityBucket(const std::wstring& severity)
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

std::wstring BuildAlertSummary(const TrafficAlert& a)
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

std::wstring BuildAlertDetails(const TrafficAlert& a)
{
    std::wstring s;

    s += L"Title: ";
    s += a.title.empty() ? L"Traffic alert" : a.title;
    s += L"\r\n";

    s += L"Road: ";
    s += a.road.empty() ? L"" : a.road;
    s += L"\r\n";

    s += L"Region: ";
    s += a.region.empty() ? L"" : a.region;
    s += L"\r\n";

    s += L"Severity: ";
    s += BuildSeverityDisplay(a.severity);
    s += L"\r\n";

    s += L"Updated: ";
    s += a.updatedText.empty() ? L"" : a.updatedText;
    s += L"\r\n";

    s += L"Coordinates: ";
    if (a.hasLocation) {
        s += std::to_wstring(a.latitude);
        s += L", ";
        s += std::to_wstring(a.longitude);
    }
    else {
        s += L"";
    }
    s += L"\r\n\r\n";

    s += L"Description:\r\n";
    s += a.description.empty() ? L"No description provided." : a.description;
    s += L"\r\n";

    return s;
}

std::wstring NormalizeUrl(std::wstring url)
{
    url = Trim(url);
    if (url.empty())
        return {};

    if (url.find(L"://") == std::wstring::npos)
        url = L"https://" + url;

    return url;
}

std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty())
        return {};

    int needed = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
        return {};

    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], needed, nullptr, nullptr);
    return out;
}

std::string JsonEscape(const std::wstring& s)
{
    return json(WideToUtf8(s)).dump();
}

void EnableModernWindowFrame(HWND hwnd)
{
    if (!hwnd)
        return;

    const BOOL enabled = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &enabled, sizeof(enabled));
}

HFONT CreateUiFont(int pointSize, int weight)
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

void ApplyExplorerTheme(HWND hwnd)
{
    if (hwnd)
        SetWindowTheme(hwnd, L"Explorer", nullptr);
}
