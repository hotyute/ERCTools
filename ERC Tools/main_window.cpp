// =================================================================================
// FILE: main_window.cpp
// =================================================================================


#include "main_window.h"
#include "app_state.h"
#include "client_update.h"
#include "http.h"
#include "map_view.h"
#include "parsing.h"
#include "util.h"


constexpr int IDC_URL_EDIT = 1001;
constexpr int IDC_SEARCH_EDIT = 1003;
constexpr int IDC_SEVERITY_COMBO = 1004;
constexpr int IDC_LISTVIEW = 1005;
constexpr int IDC_DETAILS_EDIT = 1006;
constexpr int IDC_SEARCH_LABEL = 1010;
constexpr int IDC_SEVERITY_LABEL = 1011;
constexpr int IDC_STATUS_BAR = 1012;
constexpr int IDC_SERVER_EDIT = 1013;
constexpr int IDC_CHAT_HISTORY = 1015;
constexpr int IDC_CHAT_EDIT = 1016;
constexpr int IDC_CHAT_SEND_BTN = 1017;
constexpr int IDC_PANEL_TAB_BTN = 1021;
constexpr int IDM_FILE_SETTINGS = 2001;
constexpr int IDM_FILE_EXIT = 2002;
constexpr int IDM_ABOUT = 2003;
constexpr int IDM_ROADS_INCIDENT_FILTERS = 2004;
constexpr int IDM_ROADS_INCIDENT_NOTIFICATIONS = 2005;
constexpr int IDM_EARTHQUAKES_LIST = 2006;
constexpr int IDM_EARTHQUAKE_NOTIFICATIONS = 2007;
constexpr int IDM_SHOW_EARTHQUAKES = 2008;
constexpr int IDM_VIEW_NOTIFICATION_HISTORY = 2009;
constexpr int IDM_ROADS_TEMPLATES_WIZARD = 2010;
constexpr int IDM_ROADS_EDIT_TEMPLATES = 2011;
constexpr int IDM_EARTHQUAKES_TEMPLATES_WIZARD = 2012;
constexpr int IDM_EARTHQUAKES_EDIT_TEMPLATES = 2013;
constexpr int IDM_EARTHQUAKE_OVERLAY_NONE = 2014;
constexpr int IDM_EARTHQUAKE_OVERLAY_MAG_REGION = 2015;
constexpr int IDM_WEATHER_SYSTEMS_LIST = 2016;
constexpr int IDM_WEATHER_SYSTEM_NOTIFICATIONS = 2017;
constexpr int IDM_WEATHER_SYSTEMS_TEMPLATES_WIZARD = 2018;
constexpr int IDM_WEATHER_SYSTEMS_EDIT_TEMPLATES = 2019;
constexpr int IDM_SHOW_WEATHER_SYSTEMS = 2020;
constexpr int IDM_WEATHER_SYSTEM_OVERLAY_NONE = 2021;
constexpr int IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND = 2022;
constexpr int IDC_SETTINGS_ALERT_FILTER = 2101;
constexpr int IDC_SETTINGS_ALERT_ORDER = 2102;
constexpr int IDC_SETTINGS_BOUNDARY_BTN = 2103;
constexpr int IDC_SETTINGS_CLOSE_BTN = 2104;
constexpr int IDC_SETTINGS_FILTER_LABEL = 2105;
constexpr int IDC_SETTINGS_ORDER_LABEL = 2106;
constexpr int IDC_SETTINGS_ENDPOINT_LABEL = 2107;
constexpr int IDC_SETTINGS_SERVER_LABEL = 2108;
constexpr int IDC_SETTINGS_REFRESH_OFF_RADIO = 2109;
constexpr int IDC_SETTINGS_REFRESH_ON_RADIO = 2110;
constexpr int IDC_SETTINGS_REFRESH_INTERVAL_EDIT = 2111;
constexpr int IDC_SETTINGS_REFRESH_LABEL = 2112;
constexpr int IDC_SETTINGS_REFRESH_INTERVAL_LABEL = 2113;
constexpr int IDC_SETTINGS_WORLD_LABEL = 2114;
constexpr int IDC_SETTINGS_WORLD_OFF_RADIO = 2115;
constexpr int IDC_SETTINGS_WORLD_ON_RADIO = 2116;
constexpr int IDC_SETTINGS_SYNC_LABEL = 2117;
constexpr int IDC_SETTINGS_SYNC_LOCAL_RADIO = 2118;
constexpr int IDC_SETTINGS_SYNC_SERVER_RADIO = 2119;
constexpr int IDC_SETTINGS_WORLD_BOUNDARY_BTN = 2120;
constexpr int IDC_INCIDENT_FILTERS_TITLE_LABEL = 2201;
constexpr int IDC_INCIDENT_FILTERS_DESC_LABEL = 2202;
constexpr int IDC_INCIDENT_FILTERS_SEVERITY_LABEL = 2203;
constexpr int IDC_INCIDENT_FILTERS_SEVERE_CHECK = 2204;
constexpr int IDC_INCIDENT_FILTERS_MODERATE_CHECK = 2205;
constexpr int IDC_INCIDENT_FILTERS_MINOR_CHECK = 2206;
constexpr int IDC_INCIDENT_FILTERS_UNKNOWN_CHECK = 2207;
constexpr int IDC_INCIDENT_FILTERS_TYPE_LABEL = 2208;
constexpr int IDC_INCIDENT_FILTERS_UNPLANNED_CHECK = 2209;
constexpr int IDC_INCIDENT_FILTERS_PLANNED_CHECK = 2210;
constexpr int IDC_INCIDENT_FILTERS_CLOSE_BTN = 2211;
constexpr int IDC_INCIDENT_NOTIFICATIONS_TITLE_LABEL = 2301;
constexpr int IDC_INCIDENT_NOTIFICATIONS_DESC_LABEL = 2302;
constexpr int IDC_INCIDENT_NOTIFICATIONS_ROADS_LABEL = 2303;
constexpr int IDC_INCIDENT_NOTIFICATIONS_ROADS_EDIT = 2304;
constexpr int IDC_INCIDENT_NOTIFICATIONS_LANES_LABEL = 2305;
constexpr int IDC_INCIDENT_NOTIFICATIONS_LANES_EDIT = 2306;
constexpr int IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_LABEL = 2307;
constexpr int IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_EDIT = 2308;
constexpr int IDC_INCIDENT_NOTIFICATIONS_CLOSE_BTN = 2309;
constexpr int IDC_INCIDENT_NOTIFICATIONS_REGIONS_LABEL = 2310;
constexpr int IDC_INCIDENT_NOTIFICATIONS_REGIONS_BTN = 2311;
constexpr int IDC_INCIDENT_NOTIFICATIONS_DELAY_LABEL = 2312;
constexpr int IDC_INCIDENT_NOTIFICATIONS_DELAY_EDIT = 2313;
constexpr int IDC_INCIDENT_NOTIFICATIONS_AND_RADIO = 2314;
constexpr int IDC_INCIDENT_NOTIFICATIONS_OR_RADIO = 2315;
constexpr int IDC_INCIDENT_NOTIFICATIONS_LOCATION_EXCLUSIONS_LABEL = 2316;
constexpr int IDC_INCIDENT_NOTIFICATIONS_LOCATION_EXCLUSIONS_EDIT = 2317;
constexpr int IDC_INCIDENT_NOTIFICATIONS_ROAD_EXCLUSIONS_LABEL = 2318;
constexpr int IDC_INCIDENT_NOTIFICATIONS_ROAD_EXCLUSIONS_EDIT = 2319;
constexpr int IDC_NOTIFICATION_REGIONS_LIST = 2401;
constexpr int IDC_NOTIFICATION_REGIONS_NEW_BTN = 2402;
constexpr int IDC_NOTIFICATION_REGIONS_EDIT_BTN = 2403;
constexpr int IDC_NOTIFICATION_REGIONS_DELETE_BTN = 2404;
constexpr int IDC_NOTIFICATION_REGIONS_CLOSE_BTN = 2405;
constexpr int IDC_NOTIFICATION_REGION_NAME_EDIT = 2411;
constexpr int IDC_NOTIFICATION_REGION_ROADS_EDIT = 2412;
constexpr int IDC_NOTIFICATION_REGION_ALL_ROADS_CHECK = 2413;
constexpr int IDC_NOTIFICATION_REGION_DRAW_BTN = 2414;
constexpr int IDC_NOTIFICATION_REGION_CLEAR_BTN = 2415;
constexpr int IDC_NOTIFICATION_REGION_CLOSE_BTN = 2416;
constexpr int IDC_NOTIFICATION_REGION_POINTS_LABEL = 2417;
constexpr int IDC_NOTIFICATION_REGION_UNDO_BTN = 2418;
constexpr int IDC_NOTIFICATION_REGION_FINISH_BTN = 2419;
constexpr int IDC_EARTHQUAKE_LIST_MAG_EDIT = 2501;
constexpr int IDC_EARTHQUAKE_LIST_TIME_EDIT = 2502;
constexpr int IDC_EARTHQUAKE_LIST_REGION_BTN = 2503;
constexpr int IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN = 2504;
constexpr int IDC_EARTHQUAKE_LIST_LISTVIEW = 2505;
constexpr int IDC_EARTHQUAKE_LIST_CLOSE_BTN = 2506;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT = 2521;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN = 2522;
constexpr int IDC_WEATHER_SYSTEMS_LIST_LISTVIEW = 2541;
constexpr int IDC_WEATHER_SYSTEMS_LIST_REFRESH_BTN = 2542;
constexpr int IDC_WEATHER_SYSTEMS_LIST_CLOSE_BTN = 2543;
constexpr int IDC_WEATHER_SYSTEM_NOTIFICATIONS_WIND_EDIT = 2551;
constexpr int IDC_WEATHER_SYSTEM_NOTIFICATIONS_CLOSE_BTN = 2552;
constexpr int IDC_TEMPLATES_WIZARD_TITLE = 2601;
constexpr int IDC_TEMPLATES_WIZARD_DESC = 2602;
constexpr int IDC_TEMPLATES_WIZARD_LIST = 2603;
constexpr int IDC_TEMPLATES_WIZARD_VARIABLES = 2604;
constexpr int IDC_TEMPLATES_WIZARD_PREVIEW = 2605;
constexpr int IDC_TEMPLATES_WIZARD_PREV = 2606;
constexpr int IDC_TEMPLATES_WIZARD_NEXT = 2607;
constexpr int IDC_TEMPLATES_WIZARD_COPY = 2608;
constexpr int IDC_TEMPLATES_WIZARD_CLOSE = 2609;
constexpr int IDC_TEMPLATES_WIZARD_COPY_LOCATION = 2610;
constexpr int IDC_TEMPLATES_EDITOR_LIST = 2621;
constexpr int IDC_TEMPLATES_EDITOR_NAME = 2622;
constexpr int IDC_TEMPLATES_EDITOR_BODY = 2623;
constexpr int IDC_TEMPLATES_EDITOR_NEW = 2624;
constexpr int IDC_TEMPLATES_EDITOR_SAVE = 2625;
constexpr int IDC_TEMPLATES_EDITOR_DELETE = 2626;
constexpr int IDC_TEMPLATES_EDITOR_CLOSE = 2627;
constexpr const wchar_t* kSettingsClassName = L"TrafficEnglandSettingsWindow";
constexpr const wchar_t* kIncidentFiltersClassName = L"TrafficEnglandIncidentFiltersWindow";
constexpr const wchar_t* kIncidentNotificationsClassName = L"TrafficEnglandIncidentNotificationsWindow";
constexpr const wchar_t* kNotificationRegionsClassName = L"TrafficEnglandNotificationRegionsWindow";
constexpr const wchar_t* kNotificationRegionEditorClassName = L"TrafficEnglandNotificationRegionEditorWindow";
constexpr const wchar_t* kEarthquakeListClassName = L"TrafficEnglandEarthquakeListWindow";
constexpr const wchar_t* kEarthquakeNotificationsClassName = L"TrafficEnglandEarthquakeNotificationsWindow";
constexpr const wchar_t* kWeatherSystemsListClassName = L"TrafficEnglandWeatherSystemsListWindow";
constexpr const wchar_t* kWeatherSystemNotificationsClassName = L"TrafficEnglandWeatherSystemNotificationsWindow";
constexpr const wchar_t* kTemplatesWizardClassName = L"TrafficEnglandTemplatesWizardWindow";
constexpr const wchar_t* kTemplatesEditorClassName = L"TrafficEnglandTemplatesEditorWindow";
constexpr UINT WM_APP_NOTIFY_ICON = WM_APP + 20;
constexpr UINT WM_APP_UPDATE_READY = WM_APP + 21;
constexpr UINT WM_APP_SETTINGS_SYNC_READY = WM_APP + 22;
constexpr UINT kNotificationIconId = 1;
constexpr UINT_PTR kAlertRefreshTimerId = 1;
constexpr UINT_PTR kServerPollTimerId = 2;
constexpr UINT_PTR kInAppNotificationTimerId = 3;
constexpr UINT_PTR kEarthquakeRefreshTimerId = 4;
constexpr UINT_PTR kWeatherSystemsRefreshTimerId = 5;
constexpr const wchar_t* kWeatherSystemsSourceUrl = L"https://www.tropicalstormrisk.com/tracker/dynamic/main.html";

struct FeedResult
{
    bool ok = false;
    std::wstring error;
    std::vector<TrafficAlert> alerts;
};

enum class BoundaryDownloadKind
{
    Uk,
    World
};

struct BoundaryDownloadResult
{
    BoundaryDownloadKind kind = BoundaryDownloadKind::Uk;
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
};

struct EarthquakeResult
{
    bool ok = false;
    bool notify = false;
    std::wstring error;
    std::wstring statusText;
    std::vector<EarthquakeEvent> events;
};

struct WeatherSystemsResult
{
    bool ok = false;
    bool notify = false;
    std::wstring error;
    std::wstring statusText;
    std::vector<WeatherSystemEvent> systems;
};

enum class ServerAction
{
    Poll,
    SendChat,
    SendNote,
    UpdateNote,
    DeleteNote
};

struct ServerResult
{
    ServerAction action = ServerAction::Poll;
    bool ok = false;
    bool chatOk = false;
    bool notesOk = false;
    std::wstring error;
    std::vector<ChatMessage> chat;
    std::vector<MapNote> notes;
};

struct GlobalSettingsResult
{
    bool ok = false;
    std::wstring error;
    json settings;
};

struct ReportTemplate
{
    std::wstring name;
    std::wstring body;
};

struct JunctionTemplateData
{
    std::wstring display;
    std::wstring number;
    std::wstring data;
    size_t sourcePosition = 0;
};

struct IncidentNotificationState
{
    std::wstring signature;
    std::wstring line;
};

struct EarthquakeNotificationState
{
    std::wstring signature;
    std::wstring line;
};

struct WeatherSystemNotificationState
{
    std::wstring signature;
    std::wstring line;
};

enum class TemplateContext
{
    Roads,
    Earthquakes,
    WeatherSystems
};

enum class PolygonCaptureTarget
{
    None,
    IncidentRegion,
    EarthquakeRegion
};

class MainWindow;

struct NotificationRegionEditorContext
{
    MainWindow* owner = nullptr;
    size_t index = 0;
};

static std::wstring AppendPath(std::wstring base, const wchar_t* path)
{
    base = NormalizeUrl(base);
    while (!base.empty() && base.back() == L'/')
        base.pop_back();
    return base + path;
}

static std::wstring UrlEncodePathSegment(const std::wstring& value)
{
    std::string utf8 = WideToUtf8(value);
    std::wstring encoded;
    encoded.reserve(utf8.size());
    const wchar_t* hex = L"0123456789ABCDEF";
    for (unsigned char ch : utf8) {
        const bool safe =
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.' || ch == '~';
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

static std::wstring AppendNoteIdPath(const std::wstring& server, const std::wstring& noteId)
{
    return AppendPath(server, L"/api/notes/") + UrlEncodePathSegment(noteId);
}

static std::string BuildNoteJsonBody(const MapNote& note)
{
    std::string body = "{";
    if (!note.id.empty() && note.id.rfind(L"local-", 0) != 0)
        body += "\"id\":" + JsonEscape(note.id) + ",";
    body += "\"author\":" + JsonEscape(note.author.empty() ? L"ERCTools" : note.author) +
        ",\"text\":" + JsonEscape(note.text) +
        ",\"latitude\":" + std::to_string(note.latitude) +
        ",\"longitude\":" + std::to_string(note.longitude) +
        ",\"lat\":" + std::to_string(note.latitude) +
        ",\"lon\":" + std::to_string(note.longitude) +
        ",\"x\":" + std::to_string(note.longitude) +
        ",\"y\":" + std::to_string(note.latitude) + "}";
    return body;
}

static int MaxInt(int a, int b)
{
    return a > b ? a : b;
}

static int MinInt(int a, int b)
{
    return a < b ? a : b;
}

static LONG MaxLong(LONG a, LONG b)
{
    return a > b ? a : b;
}

static HMENU ControlId(int id)
{
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

static constexpr COLORREF kUiBackground = RGB(246, 248, 251);
static constexpr COLORREF kUiSurface = RGB(255, 255, 255);
static constexpr COLORREF kUiText = RGB(22, 34, 49);
static constexpr COLORREF kUiMutedText = RGB(86, 99, 115);
static constexpr COLORREF kUiSelection = RGB(226, 240, 255);

static COLORREF SeverityLabelColor(const std::wstring& severity)
{
    std::wstring bucket = SeverityBucket(severity);
    if (bucket == L"severe")
        return RGB(190, 44, 44);
    if (bucket == L"moderate")
        return RGB(190, 111, 23);
    if (bucket == L"minor")
        return RGB(24, 114, 205);
    return RGB(74, 124, 102);
}

static HBRUSH ModernWindowBrush()
{
    static HBRUSH brush = CreateSolidBrush(kUiBackground);
    return brush;
}

static HBRUSH ModernInputBrush()
{
    static HBRUSH brush = CreateSolidBrush(kUiSurface);
    return brush;
}

static LRESULT HandleModernCtlColor(UINT msg, WPARAM wParam)
{
    HDC hdc = reinterpret_cast<HDC>(wParam);
    if (!hdc)
        return FALSE;

    SetTextColor(hdc, kUiText);
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN) {
        SetBkColor(hdc, kUiBackground);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(ModernWindowBrush());
    }

    SetBkColor(hdc, kUiSurface);
    SetBkMode(hdc, OPAQUE);
    return reinterpret_cast<LRESULT>(ModernInputBrush());
}

static void ApplyModernEditChrome(HWND hwnd)
{
    if (!hwnd)
        return;
    SendMessageW(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
}

static SIZE MeasureControlText(HWND hwnd, int wrapWidth = 0)
{
    SIZE size{};
    if (!hwnd)
        return size;

    std::wstring text = GetWindowTextString(hwnd);
    if (text.empty())
        return size;

    HDC dc = GetDC(hwnd);
    if (!dc)
        return size;

    HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;

    if (wrapWidth > 0) {
        RECT textRect{ 0, 0, wrapWidth, 0 };
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);
        size.cx = textRect.right - textRect.left;
        size.cy = textRect.bottom - textRect.top;
    }
    else {
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    }

    if (oldFont)
        SelectObject(dc, oldFont);
    ReleaseDC(hwnd, dc);
    return size;
}

static int PreferredControlWidth(HWND hwnd, int padding, int minimum = 0, int maximum = 0)
{
    SIZE textSize = MeasureControlText(hwnd);
    int width = MaxInt(minimum, static_cast<int>(textSize.cx) + padding);
    if (maximum > 0)
        width = MinInt(width, maximum);
    return width;
}

static int PreferredControlHeight(HWND hwnd, int padding, int minimum = 0, int wrapWidth = 0)
{
    SIZE textSize = MeasureControlText(hwnd, wrapWidth > padding ? wrapWidth - padding : 0);
    return MaxInt(minimum, static_cast<int>(textSize.cy) + padding);
}

static void SizeControlToText(HWND hwnd, int horizontalPadding, int verticalPadding, int minimumWidth = 0, int maximumWidth = 0, int minimumHeight = 0)
{
    if (!hwnd)
        return;

    const int width = PreferredControlWidth(hwnd, horizontalPadding, minimumWidth, maximumWidth);
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        width,
        PreferredControlHeight(hwnd, verticalPadding, minimumHeight, width),
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static int AutoLabelWidth(HWND hwnd, int maximumWidth = 0)
{
    return PreferredControlWidth(hwnd, 4, 0, maximumWidth);
}

static int AutoLabelHeight(HWND hwnd, int minimumHeight = 22, int wrapWidth = 0)
{
    return PreferredControlHeight(hwnd, 6, minimumHeight, wrapWidth);
}

static void SizeLabelToText(HWND hwnd, int maximumWidth = 0)
{
    SizeControlToText(hwnd, 4, 6, 0, maximumWidth, 22);
}

static void MoveLabelToText(HWND hwnd, int x, int y, int maximumWidth = 0)
{
    const int width = AutoLabelWidth(hwnd, maximumWidth);
    MoveWindow(hwnd, x, y, width, AutoLabelHeight(hwnd, 22, width), TRUE);
}

static BOOL CALLBACK AutoFitChildEnumProc(HWND child, LPARAM param)
{
    RECT childRect{};
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0 || !GetWindowRect(child, &childRect))
        return TRUE;

    HWND parent = GetParent(child);
    POINT topLeft{ childRect.left, childRect.top };
    POINT bottomRight{ childRect.right, childRect.bottom };
    ScreenToClient(parent, &topLeft);
    ScreenToClient(parent, &bottomRight);

    RECT* bounds = reinterpret_cast<RECT*>(param);
    bounds->right = MaxLong(bounds->right, bottomRight.x);
    bounds->bottom = MaxLong(bounds->bottom, bottomRight.y);
    return TRUE;
}

static void AutoFitWindowToChildren(HWND hwnd, int padding = 28)
{
    if (!hwnd)
        return;

    RECT childBounds{ 0, 0, 0, 0 };
    EnumChildWindows(hwnd, AutoFitChildEnumProc, reinterpret_cast<LPARAM>(&childBounds));
    if (childBounds.right <= 0 || childBounds.bottom <= 0)
        return;

    int desiredClientW = MaxInt(260, childBounds.right + padding);
    int desiredClientH = MaxInt(160, childBounds.bottom + padding);

    RECT windowRect{ 0, 0, desiredClientW, desiredClientH };
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&windowRect, style, GetMenu(hwnd) != nullptr, exStyle);

    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

template <typename Fn>
static void ScheduleBackgroundTask(Fn&& fn)
{
    std::thread(std::forward<Fn>(fn)).detach();
}

static bool TryParseRefreshIntervalMilliseconds(const std::wstring& text, UINT& millisecondsOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double amount = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(amount) || amount <= 0.0)
        return false;

    std::wstring unit = ToLower(Trim(end ? end : L""));
    double multiplier = 1000.0;
    if (unit.empty() || unit == L"s" || unit == L"sec" || unit == L"secs" || unit == L"second" || unit == L"seconds") {
        multiplier = 1000.0;
    }
    else if (unit == L"m" || unit == L"min" || unit == L"mins" || unit == L"minute" || unit == L"minutes") {
        multiplier = 60.0 * 1000.0;
    }
    else if (unit == L"ms" || unit == L"millisecond" || unit == L"milliseconds") {
        multiplier = 1.0;
    }
    else {
        return false;
    }

    double milliseconds = amount * multiplier;
    if (milliseconds < 1000.0 || milliseconds > static_cast<double>(USER_TIMER_MAXIMUM))
        return false;

    millisecondsOut = static_cast<UINT>(milliseconds);
    return true;
}

static std::vector<std::wstring> SplitCommaSeparatedTokens(const std::wstring& text)
{
    std::vector<std::wstring> tokens;
    std::wstring token;
    bool quoted = false;

    for (wchar_t ch : text) {
        if (ch == L'"') {
            quoted = !quoted;
            continue;
        }

        if (ch == L',' && !quoted) {
            std::wstring trimmed = Trim(token);
            if (!trimmed.empty())
                tokens.push_back(std::move(trimmed));
            token.clear();
            continue;
        }

        token.push_back(ch);
    }

    std::wstring trimmed = Trim(token);
    if (!trimmed.empty())
        tokens.push_back(std::move(trimmed));
    return tokens;
}

static bool StartsWithNoCase(const std::wstring& text, const std::wstring& prefix)
{
    std::wstring lowerText = ToLower(Trim(text));
    std::wstring lowerPrefix = ToLower(Trim(prefix));
    return !lowerPrefix.empty() && lowerText.rfind(lowerPrefix, 0) == 0;
}

static bool IsMotorwayRoadName(const std::wstring& road)
{
    std::wstring value = ToLower(Trim(road));
    if (value.empty())
        return false;

    if (value.size() >= 2 && value[0] == L'm' && iswdigit(value[1]))
        return true;

    return value.find(L"(m)") != std::wstring::npos;
}

static bool RoadTokenMatches(const std::wstring& road, const std::wstring& token)
{
    std::wstring normalized = ToLower(Trim(token));
    if (normalized.empty())
        return false;

    if (normalized == L"*" || normalized == L"all")
        return true;

    if (normalized == L"motorway" || normalized == L"motorways" ||
        normalized == L"m*" || normalized == L"m%")
    {
        return IsMotorwayRoadName(road);
    }

    if (normalized.back() == L'*' || normalized.back() == L'%') {
        normalized.pop_back();
        return StartsWithNoCase(road, normalized);
    }

    return ToLower(Trim(road)) == normalized;
}

static void ReplaceAllText(std::wstring& text, const std::wstring& from, const std::wstring& to)
{
    if (from.empty())
        return;

    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::wstring DecodeBasicHtmlEntities(std::wstring text)
{
    ReplaceAllText(text, L"&amp;", L"&");
    ReplaceAllText(text, L"&lt;", L"<");
    ReplaceAllText(text, L"&gt;", L">");
    ReplaceAllText(text, L"&quot;", L"\"");
    ReplaceAllText(text, L"&#39;", L"'");
    ReplaceAllText(text, L"&nbsp;", L" ");
    return text;
}

static std::wstring StripTemplateHtmlTags(const std::wstring& html)
{
    std::wstring text = html;
    text = std::regex_replace(text, std::wregex(LR"(<\s*(?:br|hr)\b[^>]*>)", std::regex_constants::icase), L" ");
    text = std::regex_replace(text, std::wregex(LR"(</\s*(?:p|div|td|tr|li|h[1-6])\s*>)", std::regex_constants::icase), L" ");
    text = std::regex_replace(text, std::wregex(LR"(<[^>]+>)"), L" ");
    text = DecodeBasicHtmlEntities(text);
    std::wstring compact;
    compact.reserve(text.size());
    bool lastSpace = false;
    for (wchar_t ch : text) {
        bool isSpace = iswspace(ch) != 0;
        if (isSpace) {
            if (!lastSpace)
                compact.push_back(L' ');
            lastSpace = true;
        }
        else {
            compact.push_back(ch);
            lastSpace = false;
        }
    }
    return Trim(compact);
}

static std::wstring ExtractLabeledNotificationField(const std::wstring& description, const wchar_t* label)
{
    std::wstring pattern = LR"((?:^|[\r\n])\s*)";
    pattern += label;
    pattern += LR"(\s*:\s*([^\r\n]+))";

    std::wsmatch m;
    std::wregex lineRe(pattern, std::regex_constants::icase);
    if (std::regex_search(description, m, lineRe) && m.size() > 1)
        return Trim(m[1].str());

    return L"";
}

static bool TryParseDurationMinutes(const std::wstring& text, double& minutesOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    std::wsmatch m;
    std::wregex re(LR"((\d+(?:\.\d+)?)\s*(hours?|hrs?|h|minutes?|mins?|m)\b)", std::regex_constants::icase);
    if (!std::regex_search(value, m, re) || m.size() < 3)
        return false;

    wchar_t* end = nullptr;
    double amount = std::wcstod(m[1].str().c_str(), &end);
    if (end == m[1].str().c_str() || !std::isfinite(amount) || amount < 0.0)
        return false;

    std::wstring unit = ToLower(m[2].str());
    if (unit == L"h" || unit == L"hr" || unit == L"hrs" || unit == L"hour" || unit == L"hours")
        amount *= 60.0;

    minutesOut = amount;
    return true;
}

static bool TryParseDoubleText(const std::wstring& text, double& valueOut)
{
    std::wstring value = Trim(text);
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed) || !Trim(end ? end : L"").empty())
        return false;

    valueOut = parsed;
    return true;
}

static bool TryExtractAlertDelayMinutes(const TrafficAlert& alert, double& minutesOut)
{
    std::wstring delayText = ExtractLabeledNotificationField(alert.description, L"Delay");
    if (!delayText.empty() && TryParseDurationMinutes(delayText, minutesOut))
        return true;

    std::wsmatch m;
    std::wregex re(LR"(\bDelay\s*:\s*(.*?)(?=\s+(?:Location|Reason|Status|Time To Clear|Return To Normal|Lanes Closed)\s*:|$))", std::regex_constants::icase);
    if (std::regex_search(alert.description, m, re) && m.size() > 1)
        return TryParseDurationMinutes(m[1].str(), minutesOut);

    return false;
}

static bool PointInPolygon(double lat, double lon, const std::vector<GeoPoint>& points)
{
    if (points.size() < 3)
        return false;

    bool inside = false;
    size_t j = points.size() - 1;
    for (size_t i = 0; i < points.size(); ++i) {
        const double yi = points[i].lat;
        const double yj = points[j].lat;
        const double xi = points[i].lon;
        const double xj = points[j].lon;

        const bool intersects = ((yi > lat) != (yj > lat)) &&
            (lon < (xj - xi) * (lat - yi) / ((yj - yi) == 0.0 ? 1e-12 : (yj - yi)) + xi);
        if (intersects)
            inside = !inside;
        j = i;
    }
    return inside;
}

static bool TryParseDateTimeFilter(const std::wstring& text, long long& timeMsOut)
{
    std::wstring value = Trim(text);
    if (value.empty()) {
        timeMsOut = 0;
        return true;
    }

    std::tm tm{};
    int y = 0, mon = 0, d = 0, h = 0, min = 0;
    int count = swscanf_s(value.c_str(), L"%d-%d-%d %d:%d", &y, &mon, &d, &h, &min);
    if (count < 3)
        count = swscanf_s(value.c_str(), L"%d/%d/%d %d:%d", &y, &mon, &d, &h, &min);
    if (count < 3)
        return false;

    tm.tm_year = y - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = d;
    tm.tm_hour = count >= 4 ? h : 0;
    tm.tm_min = count >= 5 ? min : 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;

    std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1))
        return false;

    timeMsOut = static_cast<long long>(t) * 1000LL;
    return true;
}

static std::wstring EarthquakeTimeText(long long timeMs)
{
    if (timeMs <= 0)
        return L"";

    std::time_t t = static_cast<std::time_t>(timeMs / 1000LL);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buf[64]{};
    wcsftime(buf, _countof(buf), L"%Y-%m-%d %H:%M", &tm);
    return buf;
}

static std::vector<EarthquakeEvent> ParseEarthquakeEvents(const std::string& body)
{
    std::vector<EarthquakeEvent> out;
    json root = json::parse(body);
    if (!root.is_object())
        return out;

    auto featuresIt = root.find("features");
    if (featuresIt == root.end() || !featuresIt->is_array())
        return out;

    for (const json& feature : *featuresIt) {
        if (!feature.is_object())
            continue;

        const json* propsPtr = nullptr;
        const json* geomPtr = nullptr;
        auto propsIt = feature.find("properties");
        if (propsIt != feature.end() && propsIt->is_object())
            propsPtr = &(*propsIt);
        auto geomIt = feature.find("geometry");
        if (geomIt != feature.end() && geomIt->is_object())
            geomPtr = &(*geomIt);
        const json& props = propsPtr ? *propsPtr : feature;
        const json& geom = geomPtr ? *geomPtr : feature;
        EarthquakeEvent event;
        event.id = PickString(feature, { "id" });
        event.place = PickString(props, { "place", "title" });
        PickDouble(props, { "mag", "magnitude" }, event.magnitude);
        event.timeMs = props.value("time", 0LL);
        event.timeText = EarthquakeTimeText(event.timeMs);

        auto coordsIt = geom.find("coordinates");
        if (coordsIt != geom.end() && coordsIt->is_array() && coordsIt->size() >= 2) {
            TryGetDoubleFromJsonValue((*coordsIt)[0], event.longitude);
            TryGetDoubleFromJsonValue((*coordsIt)[1], event.latitude);
            if (coordsIt->size() >= 3)
                TryGetDoubleFromJsonValue((*coordsIt)[2], event.depthKm);
            event.hasLocation = std::isfinite(event.latitude) && std::isfinite(event.longitude);
        }

        if (event.id.empty())
            event.id = event.place + L"|" + std::to_wstring(event.timeMs);
        out.push_back(std::move(event));
    }

    return out;
}

static bool IsValidMapCoordinate(double lat, double lon);

static bool TryParseHemisphereCoordinate(const std::wstring& text, double& valueOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    wchar_t hemisphere = 0;
    if (!value.empty()) {
        wchar_t last = value.back();
        if (last == L'n' || last == L's' || last == L'e' || last == L'w') {
            hemisphere = last;
            value.pop_back();
        }
    }

    value = Trim(value);
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed))
        return false;

    if (hemisphere == L's' || hemisphere == L'w')
        parsed = -parsed;
    valueOut = parsed;
    return true;
}

static bool TryParseKnots(const std::wstring& text, double& knotsOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed))
        return false;

    knotsOut = parsed;
    return true;
}

static std::vector<std::wstring> ExtractHtmlTableCells(const std::wstring& rowHtml)
{
    std::vector<std::wstring> cells;
    std::wregex cellRe(LR"(<\s*t[hd]\b[^>]*>([\s\S]*?)</\s*t[hd]\s*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(rowHtml.begin(), rowHtml.end(), cellRe), end; it != end; ++it) {
        std::wstring cell = StripTemplateHtmlTags((*it)[1].str());
        if (!cell.empty())
            cells.push_back(std::move(cell));
    }
    return cells;
}

static std::wstring ExtractWeatherSystemsStatusText(const std::wstring& htmlText)
{
    std::wstring text = StripTemplateHtmlTags(htmlText);
    std::wsmatch m;
    std::wregex statusRe(LR"(Tropical Storm Tracker:\s*(.*?GMT))", std::regex_constants::icase);
    if (std::regex_search(text, m, statusRe) && m.size() > 1)
        return Trim(m[1].str());
    return L"";
}

static std::vector<WeatherSystemEvent> ParseWeatherSystemEvents(const std::string& body, std::wstring& statusTextOut)
{
    std::vector<WeatherSystemEvent> systems;
    std::wstring html = Utf8ToWide(body);
    statusTextOut = ExtractWeatherSystemsStatusText(html);

    std::wregex rowRe(LR"(<\s*tr\b[^>]*>([\s\S]*?)</\s*tr\s*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), rowRe), end; it != end; ++it) {
        std::vector<std::wstring> cells = ExtractHtmlTableCells((*it)[1].str());
        if (cells.size() < 10)
            continue;

        std::wstring first = ToLower(Trim(cells[0]));
        if (first == L"system" || first == L"current data")
            continue;

        WeatherSystemEvent system;
        system.name = Trim(cells[0]);
        system.basin = Trim(cells[1]);
        system.windText = Trim(cells[4]);
        system.category = Trim(cells[5]);
        system.forecastWindText = Trim(cells[8]);
        system.forecastCategory = Trim(cells[9]);
        system.updatedText = statusTextOut;

        double lat = 0.0;
        double lon = 0.0;
        if (TryParseHemisphereCoordinate(cells[2], lat) &&
            TryParseHemisphereCoordinate(cells[3], lon) &&
            IsValidMapCoordinate(lat, lon))
        {
            system.latitude = lat;
            system.longitude = lon;
            system.hasLocation = true;
        }

        double forecastLat = 0.0;
        double forecastLon = 0.0;
        if (TryParseHemisphereCoordinate(cells[6], forecastLat) &&
            TryParseHemisphereCoordinate(cells[7], forecastLon) &&
            IsValidMapCoordinate(forecastLat, forecastLon))
        {
            system.forecastLatitude = forecastLat;
            system.forecastLongitude = forecastLon;
            system.hasForecastLocation = true;
        }

        TryParseKnots(system.windText, system.windKnots);
        TryParseKnots(system.forecastWindText, system.forecastWindKnots);

        system.id = system.name + L"|" + system.basin;
        if (!system.updatedText.empty())
            system.id += L"|" + system.updatedText;

        if (!system.name.empty())
            systems.push_back(std::move(system));
    }

    return systems;
}

static GeoPolygon GeoPolygonFromJson(const json& value)
{
    GeoPolygon polygon;
    if (!value.is_object())
        return polygon;

    polygon.name = PickString(value, { "name" });
    polygon.roadFilter = PickString(value, { "roadFilter", "roads" });
    auto allIt = value.find("allRoads");
    if (allIt != value.end() && allIt->is_boolean())
        polygon.allRoads = allIt->get<bool>();

    auto pointsIt = value.find("points");
    if (pointsIt != value.end() && pointsIt->is_array()) {
        for (const json& item : *pointsIt) {
            if (!item.is_object())
                continue;
            double lat = 0.0;
            double lon = 0.0;
            if (PickDouble(item, { "lat", "latitude" }, lat) &&
                PickDouble(item, { "lon", "longitude" }, lon))
            {
                polygon.points.push_back({ lat, lon });
            }
        }
    }

    return polygon;
}

static json GeoPolygonToJson(const GeoPolygon& polygon)
{
    json value = json::object();
    value["name"] = WideToUtf8(polygon.name);
    value["roadFilter"] = WideToUtf8(polygon.roadFilter);
    value["allRoads"] = polygon.allRoads;
    value["points"] = json::array();
    for (const GeoPoint& point : polygon.points) {
        value["points"].push_back({
            { "lat", point.lat },
            { "lon", point.lon }
            });
    }
    return value;
}

static std::vector<ChatMessage> ParseChatMessages(const json& root)
{
    std::vector<ChatMessage> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("chat") && root["chat"].is_array())
        arr = &root["chat"];
    else if (root.contains("messages") && root["messages"].is_array())
        arr = &root["messages"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        ChatMessage msg;
        msg.author = PickString(item, { "author", "user", "name" });
        msg.position = PickString(item, { "position", "role" });
        msg.text = PickString(item, { "text", "message", "body" });
        msg.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        if (!msg.text.empty())
            out.push_back(std::move(msg));
    }

    return out;
}

static bool IsValidMapCoordinate(double lat, double lon)
{
    return std::isfinite(lat) && std::isfinite(lon) &&
        lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

static bool IsLikelyUkCoordinate(double lat, double lon)
{
    return lat >= 48.0 && lat <= 62.0 && lon >= -12.0 && lon <= 6.0;
}

static std::vector<MapNote> ParseMapNotes(const json& root)
{
    std::vector<MapNote> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("notes") && root["notes"].is_array())
        arr = &root["notes"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        MapNote note;
        note.id = PickString(item, { "id", "noteId" });
        note.author = PickString(item, { "author", "user", "name" });
        note.text = PickString(item, { "text", "note", "body" });
        note.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        bool hasLat = PickDouble(item, { "lat", "latitude" }, note.latitude);
        bool hasLon = PickDouble(item, { "lon", "lng", "longitude" }, note.longitude);
        if (hasLat && hasLon) {
            if (IsLikelyUkCoordinate(note.longitude, note.latitude) && !IsLikelyUkCoordinate(note.latitude, note.longitude)) {
                std::swap(note.latitude, note.longitude);
            }
        }
        if (!(hasLat && hasLon)) {
            double x = 0.0;
            double y = 0.0;
            if (PickDouble(item, { "x" }, x) && PickDouble(item, { "y" }, y)) {
                // Some collaborators send x/y in lon/lat order or lat/lon order.
                if (IsValidMapCoordinate(y, x)) {
                    note.latitude = y;
                    note.longitude = x;
                    hasLat = true;
                    hasLon = true;
                }
                else if (IsValidMapCoordinate(x, y)) {
                    note.latitude = x;
                    note.longitude = y;
                    hasLat = true;
                    hasLon = true;
                }
            }
        }
        if (hasLat && hasLon && IsValidMapCoordinate(note.latitude, note.longitude) && !note.text.empty())
            out.push_back(std::move(note));
    }

    return out;
}

static bool MapNotesEqual(const std::vector<MapNote>& a, const std::vector<MapNote>& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id ||
            a[i].author != b[i].author ||
            a[i].text != b[i].text ||
            a[i].timestamp != b[i].timestamp ||
            std::abs(a[i].latitude - b[i].latitude) > 1e-9 ||
            std::abs(a[i].longitude - b[i].longitude) > 1e-9)
        {
            return false;
        }
    }

    return true;
}

static void MergePendingLocalNotes(std::vector<MapNote>& serverNotes, const std::vector<MapNote>& currentNotes)
{
    for (const auto& note : currentNotes) {
        if (note.timestamp != L"pending")
            continue;

        const bool alreadyPresent = std::any_of(serverNotes.begin(), serverNotes.end(), [&](const MapNote& serverNote) {
            return serverNote.text == note.text &&
                serverNote.author == note.author &&
                std::abs(serverNote.latitude - note.latitude) <= 1e-9 &&
                std::abs(serverNote.longitude - note.longitude) <= 1e-9;
            });

        if (!alreadyPresent)
            serverNotes.push_back(note);
    }
}

static bool FetchTrafficEnglandAlerts(std::vector<TrafficAlert>& alertsOut, std::wstring& errorOut, bool unplannedOnly, const std::wstring& order)
{
    alertsOut.clear();
    errorOut.clear();

    constexpr size_t kPageSize = 100;
    constexpr size_t kMaxPages = 20;

    for (size_t page = 0; page < kMaxPages; ++page) {
        const size_t start = page * kPageSize;
        std::string body;
        std::wstring httpError;
        if (!HttpGetText(BuildTrafficEnglandAlertsApiUrl(start, kPageSize, unplannedOnly, order), body, httpError)) {
            errorOut = L"Traffic England alerts API failed: " + httpError;
            return false;
        }

        std::wstring parseError;
        std::vector<TrafficAlert> pageAlerts = ParseTrafficAlerts(body, parseError);
        if (pageAlerts.empty()) {
            if (page == 0) {
                errorOut = parseError.empty()
                    ? L"Traffic England alerts API returned no alerts."
                    : L"Traffic England alerts API could not be parsed: " + parseError;
                return false;
            }
            break;
        }

        const size_t returned = pageAlerts.size();
        alertsOut.insert(alertsOut.end(), pageAlerts.begin(), pageAlerts.end());

        if (returned < kPageSize)
            break;
    }

    return !alertsOut.empty();
}

class MainWindow
{
public:
    explicit MainWindow(ClientSession session) : m_session(std::move(session)) {}

    bool Create(HINSTANCE hInst)
    {
        m_hInst = hInst;
        if (!RegisterClass())
            return false;

        m_hwnd = CreateWindowExW(
            0,
            kMainClassName,
            L"ERC Tools",
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
    static constexpr const wchar_t* kBaseTitle = L"ERC Tools";

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
        wc.hbrBackground = ModernWindowBrush();
        wc.lpszClassName = kMainClassName;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        registered = true;
        return true;
    }

    HWND CreateAutoLabel(HWND parent, int id, const wchar_t* text, int x = 0, int y = 0, HFONT font = nullptr, int maximumWidth = 0)
    {
        // Static text in this UI should size itself from its current font/text by default.
        // Pass maximumWidth when a label is constrained by layout so the preferred height wraps.
        HWND label = CreateWindowExW(
            0,
            L"STATIC",
            text,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, 0, 0,
            parent,
            ControlId(id),
            m_hInst,
            nullptr);
        if (!label)
            return nullptr;

        SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font ? font : m_font), TRUE);
        ApplyExplorerTheme(label);
        SizeLabelToText(label, maximumWidth);
        return label;
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

        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);

        case WM_SIZE:
            Layout();
            return 0;

        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_NOTIFY:
            return OnNotify(reinterpret_cast<NMHDR*>(lParam));

        case WM_TIMER:
            if (wParam == kAlertRefreshTimerId)
                RefreshFeedAsync();
            else if (wParam == kServerPollTimerId)
                PollServerAsync();
            else if (wParam == kEarthquakeRefreshTimerId)
                FetchEarthquakesAsync(true);
            else if (wParam == kWeatherSystemsRefreshTimerId)
                FetchWeatherSystemsAsync(true);
            else if (wParam == kInAppNotificationTimerId) {
                KillTimer(m_hwnd, kInAppNotificationTimerId);
                m_map.ClearActiveNotification();
            }
            return 0;

        case WM_APP_NOTIFY_ICON:
            return 0;

        case WM_APP_FEED_READY:
            OnFeedReady(reinterpret_cast<FeedResult*>(lParam));
            return 0;

        case WM_APP_BOUNDARY_READY:
            OnBoundaryReady(reinterpret_cast<BoundaryDownloadResult*>(lParam));
            return 0;

        case WM_APP_SERVER_READY:
            OnServerReady(reinterpret_cast<ServerResult*>(lParam));
            return 0;

        case WM_APP_EARTHQUAKE_READY:
            OnEarthquakeReady(reinterpret_cast<EarthquakeResult*>(lParam));
            return 0;

        case WM_APP_WEATHER_READY:
            OnWeatherSystemsReady(reinterpret_cast<WeatherSystemsResult*>(lParam));
            return 0;

        case WM_APP_UPDATE_READY:
            OnClientUpdateReady(reinterpret_cast<ClientUpdateResult*>(lParam));
            return 0;

        case WM_APP_SETTINGS_SYNC_READY:
            OnGlobalSettingsSyncReady(reinterpret_cast<GlobalSettingsResult*>(lParam));
            return 0;

        case WM_DESTROY:
            g_appQuitting.store(true);
            SaveSettings();
            RemoveNotificationIcon();
            KillTimer(m_hwnd, kAlertRefreshTimerId);
            KillTimer(m_hwnd, kServerPollTimerId);
            KillTimer(m_hwnd, kInAppNotificationTimerId);
            KillTimer(m_hwnd, kEarthquakeRefreshTimerId);
            KillTimer(m_hwnd, kWeatherSystemsRefreshTimerId);
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

        m_font = CreateUiFont(11);
        m_headerFont = CreateUiFont(18, FW_SEMIBOLD);

        LoadSettings();
        EnsureDefaultReportTemplates();
        EnsureDefaultEarthquakeReportTemplates();
        EnsureDefaultWeatherSystemReportTemplates();
        ModernizeReportTemplates();
        CreateMainMenu();

        m_searchLabel = CreateAutoLabel(m_hwnd, IDC_SEARCH_LABEL, L"Search");
        m_severityLabel = CreateAutoLabel(m_hwnd, IDC_SEVERITY_LABEL, L"Severity");

        m_panelTabBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"\x25C0",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_PANEL_TAB_BTN),
            m_hInst,
            nullptr);

        m_searchEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_SEARCH_EDIT),
            m_hInst,
            nullptr);

        m_severityCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_SEVERITY_COMBO),
            m_hInst,
            nullptr);

        m_listView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_LISTVIEW),
            m_hInst,
            nullptr);

        m_detailsEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_DETAILS_EDIT),
            m_hInst,
            nullptr);

        m_statusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            L"Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_STATUS_BAR),
            m_hInst,
            nullptr);

        if (!m_searchLabel || !m_severityLabel ||
            !m_panelTabBtn || !m_searchEdit || !m_severityCombo || !m_listView || !m_detailsEdit || !m_statusBar)
        {
            MessageBoxW(m_hwnd, L"Failed to create one or more child controls.", L"ERC Tools", MB_ICONERROR);
            return;
        }

        for (HWND h : { m_panelTabBtn, m_searchEdit, m_severityCombo, m_listView, m_detailsEdit, m_statusBar }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        for (HWND h : { m_searchEdit, m_detailsEdit })
            ApplyModernEditChrome(h);

        SendMessageW(m_searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter by road, region, or description"));

        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Severe"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Moderate"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minor"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unknown"));
        SendMessageW(m_severityCombo, CB_SETCURSEL, 0, 0);

        SendMessageW(m_listView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);
        ListView_SetBkColor(m_listView, kUiSurface);
        ListView_SetTextBkColor(m_listView, CLR_NONE);
        ListView_SetTextColor(m_listView, kUiText);

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
            MessageBoxW(m_hwnd, L"Could not create the native map view.", L"ERC Tools", MB_ICONERROR);
            return;
        }

        std::wstring boundaryError;
        if (!m_map.LoadUkBoundaryFromFile(GetBoundaryCachePath(), &boundaryError)) {
            OutputDebugStringW((L"Boundary cache load: " + boundaryError + L"\n").c_str());
        }
        std::wstring worldBoundaryError;
        if (!m_map.LoadWorldBoundaryFromFile(GetWorldBoundaryCachePath(), &worldBoundaryError)) {
            OutputDebugStringW((L"World boundary cache load: " + worldBoundaryError + L"\n").c_str());
        }

        m_map.SetSelectCallback([this](const std::wstring& id) {
            SelectAlertById(id, true);
            });
        m_map.SetNoteCreateCallback([this](const std::wstring& text, double lat, double lon) {
            CreateMapNote(text, lat, lon);
            });
        m_map.SetNoteUpdateCallback([this](size_t index, const std::wstring& text) {
            UpdateMapNote(index, text);
            });
        m_map.SetNoteDeleteCallback([this](size_t index) {
            DeleteMapNote(index);
            });
        m_map.SetRefreshCallback([this]() {
            RefreshFeedAsync();
            });
        m_map.SetPolygonPointCallback([this](double lat, double lon) {
            OnMapPolygonPoint(lat, lon);
            });
        m_map.SetPolygonPointMoveCallback([this](size_t polygonIndex, size_t pointIndex, double lat, double lon) {
            OnMapPolygonPointMoved(polygonIndex, pointIndex, lat, lon);
            });
        m_map.SetPolygonPointDeleteCallback([this](size_t polygonIndex, size_t pointIndex) {
            OnMapPolygonPointDeleted(polygonIndex, pointIndex);
            });
        m_map.SetPolygonClearCallback([this](size_t polygonIndex) {
            OnMapPolygonCleared(polygonIndex);
            });
        m_map.SetNotificationHistoryClearCallback([this]() {
            ClearNotificationHistory();
            });
        m_map.SetChatSendCallback([this](const std::wstring& text) {
            SendChatTextAsync(text);
            });
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        m_map.SetEarthquakeOverlayVisible(m_showEarthquakes && m_showEarthquakeOverlayLabels);
        m_map.SetWeatherSystemOverlayVisible(m_showWeatherSystems && m_showWeatherSystemOverlayLabels);
        m_map.SetDisplayWorldMap(m_displayWorldMap);
        RenderChatHistory();
        RenderNotificationHistory();

        Layout();
        SetStatusText(L"Ready.");
        ApplyRefreshTimer();
        if (IsOnlineMode())
            SetTimer(m_hwnd, kServerPollTimerId, 8 * 1000, nullptr);
        SetTimer(m_hwnd, kEarthquakeRefreshTimerId, 10 * 60 * 1000, nullptr);
        SetTimer(m_hwnd, kWeatherSystemsRefreshTimerId, 10 * 60 * 1000, nullptr);

        RefreshFeedAsync();
        FetchEarthquakesAsync(true);
        FetchWeatherSystemsAsync(true);
        if (IsOnlineMode()) {
            PollServerAsync();
            CheckForClientUpdateAsync();
            if (m_syncSettingsFromServer)
                SyncGlobalSettingsFromServerAsync();
        }
        else {
            SetStatusText(L"Offline mode ready.");
        }
    }

    void Layout()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = MaxLong(1L, rc.right - rc.left);
        LONG height = MaxLong(1L, rc.bottom - rc.top);

        const int pad = 16;
        const int labelH = 22;
        const int controlH = 32;
        const int statusH = 24;

        const int topY = 12;
        const int topBarH = topY;
        int bodyTop = topBarH;
        int leftW = m_isSidePanelVisible ? 440 : 0;
        int detailsH = 185;

        int leftX = pad;
        int leftY = bodyTop + pad;
        int leftInnerW = MaxInt(10, leftW - pad * 2);

        const int panelShow = m_isSidePanelVisible ? SW_SHOW : SW_HIDE;
        for (HWND h : { m_searchLabel, m_searchEdit, m_severityLabel, m_severityCombo, m_listView, m_detailsEdit })
            ShowWindow(h, panelShow);

        if (m_isSidePanelVisible) {
            const int searchLabelW = AutoLabelWidth(m_searchLabel, leftInnerW);
            const int searchLabelH = AutoLabelHeight(m_searchLabel, labelH, searchLabelW);
            MoveLabelToText(m_searchLabel, leftX, leftY, leftInnerW);
            MoveWindow(m_searchEdit, leftX, leftY + searchLabelH + 2, leftInnerW, controlH, TRUE);

            const int severityY = leftY + searchLabelH + controlH + 12;
            const int severityLabelW = AutoLabelWidth(m_severityLabel, leftInnerW);
            const int severityLabelH = AutoLabelHeight(m_severityLabel, labelH, severityLabelW);
            MoveLabelToText(m_severityLabel, leftX, severityY, leftInnerW);
            MoveWindow(m_severityCombo, leftX, severityY + severityLabelH + 2, leftInnerW, 180, TRUE);

            int listTop = severityY + severityLabelH + controlH + 18;
            int bodyHeight = static_cast<int>(height) - bodyTop - statusH - pad * 2;
            int listHeight = MaxInt(150, bodyHeight - (listTop - leftY) - detailsH - 20);

            MoveWindow(m_listView, leftX, listTop, leftInnerW, listHeight, TRUE);
            int detailsTop = listTop + listHeight + 10;
            MoveWindow(m_detailsEdit, leftX, detailsTop, leftInnerW, detailsH, TRUE);
        }

        int mapX = (m_isSidePanelVisible ? leftW + pad : pad);
        int mapY = bodyTop + pad;
        LONG mapW = MaxLong(100L, width - mapX - pad);
        LONG mapH = MaxLong(100L, height - mapY - statusH - pad);

        MoveWindow(m_map.Hwnd(), mapX, mapY, mapW, mapH, TRUE);

        const int tabW = 24;
        const int tabH = 72;
        int tabX = m_isSidePanelVisible ? (leftW - tabW / 2) : 0;
        int tabY = bodyTop + MaxInt(60, static_cast<int>((height - bodyTop - statusH) / 2 - tabH / 2));
        MoveWindow(m_panelTabBtn, tabX, tabY, tabW, tabH, TRUE);

        SendMessageW(m_statusBar, WM_SIZE, 0, 0);

        if (m_isSidePanelVisible) {
            SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 0, 94);
            SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 1, MaxInt(120, leftInnerW - 264));
            SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 2, 160);
        }
    }

    void OnCommand(int id, int code)
    {
        switch (id) {
        case IDC_PANEL_TAB_BTN:
            if (code == BN_CLICKED) {
                m_isSidePanelVisible = !m_isSidePanelVisible;
                SetWindowTextW(m_panelTabBtn, m_isSidePanelVisible ? L"\x25C0" : L"\x25B6");
                Layout();
            }
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
        case IDM_FILE_SETTINGS:
            ShowSettingsWindow();
            break;

        case IDM_FILE_EXIT:
            DestroyWindow(m_hwnd);
            break;

        case IDM_ROADS_INCIDENT_FILTERS:
            ShowIncidentFiltersWindow();
            break;

        case IDM_ROADS_INCIDENT_NOTIFICATIONS:
            ShowIncidentNotificationsWindow();
            break;

        case IDM_ROADS_TEMPLATES_WIZARD:
            ShowTemplatesWizardWindow();
            break;

        case IDM_ROADS_EDIT_TEMPLATES:
            ShowTemplatesEditorWindow();
            break;

        case IDM_EARTHQUAKES_LIST:
            ShowEarthquakeListWindow();
            break;

        case IDM_EARTHQUAKE_NOTIFICATIONS:
            ShowEarthquakeNotificationsWindow();
            break;

        case IDM_EARTHQUAKES_TEMPLATES_WIZARD:
            ShowEarthquakeTemplatesWizardWindow();
            break;

        case IDM_EARTHQUAKES_EDIT_TEMPLATES:
            ShowEarthquakeTemplatesEditorWindow();
            break;

        case IDM_SHOW_EARTHQUAKES:
            ToggleShowEarthquakes();
            break;
        case IDM_EARTHQUAKE_OVERLAY_NONE:
            SetEarthquakeOverlayLabels(false);
            break;
        case IDM_EARTHQUAKE_OVERLAY_MAG_REGION:
            SetEarthquakeOverlayLabels(true);
            break;

        case IDM_WEATHER_SYSTEMS_LIST:
            ShowWeatherSystemsListWindow();
            break;

        case IDM_WEATHER_SYSTEM_NOTIFICATIONS:
            ShowWeatherSystemNotificationsWindow();
            break;

        case IDM_WEATHER_SYSTEMS_TEMPLATES_WIZARD:
            ShowWeatherSystemsTemplatesWizardWindow();
            break;

        case IDM_WEATHER_SYSTEMS_EDIT_TEMPLATES:
            ShowWeatherSystemsTemplatesEditorWindow();
            break;

        case IDM_SHOW_WEATHER_SYSTEMS:
            ToggleShowWeatherSystems();
            break;

        case IDM_WEATHER_SYSTEM_OVERLAY_NONE:
            SetWeatherSystemOverlayLabels(false);
            break;

        case IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND:
            SetWeatherSystemOverlayLabels(true);
            break;

        case IDM_VIEW_NOTIFICATION_HISTORY:
            ToggleNotificationHistory();
            break;

        case IDM_ABOUT:
            ShowAboutDialog();
            break;
        }
    }

    LRESULT OnNotify(NMHDR* nmh)
    {
        if (!nmh)
            return 0;

        if (nmh->hwndFrom == m_listView && nmh->code == NM_CUSTOMDRAW)
            return OnAlertListCustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(nmh));

        if (nmh->hwndFrom == m_listView && nmh->code == LVN_ITEMCHANGED) {
            if (m_programmaticSelection)
                return 0;

            NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
            if ((lv->uChanged & LVIF_STATE) && (lv->uNewState & LVIS_SELECTED)) {
                int selected = static_cast<int>(SendMessageW(m_listView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (selected >= 0 && selected < static_cast<int>(m_filteredAlerts.size())) {
                    const std::wstring& id = m_filteredAlerts[static_cast<size_t>(selected)].id;
                    SelectAlertById(id, true);
                }
            }
        }

        return 0;
    }

    LRESULT OnAlertListCustomDraw(NMLVCUSTOMDRAW* cd)
    {
        if (!cd)
            return CDRF_DODEFAULT;

        switch (cd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
        {
            const bool selected = (cd->nmcd.uItemState & CDIS_SELECTED) != 0;
            const bool hot = (cd->nmcd.uItemState & CDIS_HOT) != 0;
            cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
            cd->clrTextBk = selected ? kUiSelection : (hot ? RGB(240, 246, 253) : kUiSurface);
            return CDRF_NOTIFYSUBITEMDRAW;
        }
        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        {
            const int row = static_cast<int>(cd->nmcd.dwItemSpec);
            const bool selected = (cd->nmcd.uItemState & CDIS_SELECTED) != 0;
            if (row >= 0 && row < static_cast<int>(m_filteredAlerts.size()) && cd->iSubItem == 0 && !selected)
                cd->clrText = SeverityLabelColor(m_filteredAlerts[static_cast<size_t>(row)].severity);
            else
                cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
            cd->clrTextBk = selected ? kUiSelection : kUiSurface;
            return CDRF_DODEFAULT;
        }
        }

        return CDRF_DODEFAULT;
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

    std::wstring SessionDisplayName() const
    {
        if (!m_session.displayName.empty())
            return m_session.displayName;
        if (!m_session.username.empty())
            return m_session.username;
        return L"ERCTools";
    }

    bool IsOnlineMode() const
    {
        return IsOnlineSession(m_session);
    }

    std::wstring MapNoteAuthor() const
    {
        return IsOnlineMode() ? SessionDisplayName() : L"";
    }

    void LoadSettings()
    {
        std::ifstream in(GetSettingsPath(), std::ios::binary);
        if (!in)
            return;

        try {
            json root = json::parse(in);
            if (!root.is_object())
                return;

            const json* settings = &root;
            auto settingsIt = root.find("settings");
            if (settingsIt != root.end() && settingsIt->is_object())
                settings = &(*settingsIt);

            auto readString = [&](const char* key, std::wstring& target) {
                auto it = settings->find(key);
                if (it != settings->end() && it->is_string())
                    target = Utf8ToWide(it->get<std::string>());
                };
            auto readBool = [&](const char* key, bool& target) {
                auto it = settings->find(key);
                if (it == settings->end())
                    return;
                if (it->is_boolean())
                    target = it->get<bool>();
                else if (it->is_number_integer())
                    target = it->get<int>() != 0;
                };
            auto readDouble = [&](const char* key, double& target) {
                auto it = settings->find(key);
                if (it != settings->end() && it->is_number())
                    target = it->get<double>();
                };
            auto readUInt = [&](const char* key, UINT& target) {
                auto it = settings->find(key);
                if (it != settings->end() && it->is_number_unsigned())
                    target = static_cast<UINT>(it->get<unsigned int>());
                else if (it != settings->end() && it->is_number_integer() && it->get<int>() >= 0)
                    target = static_cast<UINT>(it->get<int>());
                };

            readString("alertsEndpoint", m_alertsEndpoint);
            m_alertsEndpoint = NormalizeUrl(m_alertsEndpoint);
            readString("serverBaseUrl", m_serverBaseUrl);
            m_serverBaseUrl = NormalizeUrl(m_serverBaseUrl);
            {
                std::wstring lowerServer = ToLower(m_serverBaseUrl);
                if (lowerServer == L"http://localhost:8080" || lowerServer == L"https://localhost:8080" ||
                    lowerServer == L"http://213.254.181.35:8080" || lowerServer == L"https://213.254.181.35:8080")
                {
                    m_serverBaseUrl = L"http://213.254.181.35:8081";
                }
            }
            readString("alertOrder", m_alertOrder);
            readBool("alertFilterUnplannedOnly", m_alertFilterUnplannedOnly);
            readBool("periodicRefreshEnabled", m_periodicRefreshEnabled);
            readBool("showNotificationHistory", m_showNotificationHistory);
            readBool("displayWorldMap", m_displayWorldMap);
            readBool("syncSettingsFromServer", m_syncSettingsFromServer);
            readString("refreshIntervalText", m_refreshIntervalText);
            readUInt("refreshIntervalMs", m_refreshIntervalMs);
            UINT parsedRefreshMs = 0;
            if (TryParseRefreshIntervalMilliseconds(m_refreshIntervalText, parsedRefreshMs))
                m_refreshIntervalMs = parsedRefreshMs;

            readBool("incidentFilterSevere", m_incidentFilterSevere);
            readBool("incidentFilterModerate", m_incidentFilterModerate);
            readBool("incidentFilterMinor", m_incidentFilterMinor);
            readBool("incidentFilterUnknown", m_incidentFilterUnknown);
            readBool("incidentFilterUnplanned", m_incidentFilterUnplanned);
            readBool("incidentFilterPlanned", m_incidentFilterPlanned);
            readString("incidentNotifyRoads", m_incidentNotifyRoads);
            readString("incidentNotifyRoadExclusions", m_incidentNotifyRoadExclusions);
            readString("incidentNotifyLaneThresholdText", m_incidentNotifyLaneThresholdText);
            readDouble("incidentNotifyLaneThreshold", m_incidentNotifyLaneThreshold);
            double parsedThreshold = 0.0;
            if (TryParsePercentThreshold(m_incidentNotifyLaneThresholdText, parsedThreshold))
                m_incidentNotifyLaneThreshold = parsedThreshold;
            readString("incidentNotifyDelayThresholdText", m_incidentNotifyDelayThresholdText);
            readDouble("incidentNotifyDelayThresholdMinutes", m_incidentNotifyDelayThresholdMinutes);
            double parsedDelayMinutes = 0.0;
            if (TryParseDurationMinutes(m_incidentNotifyDelayThresholdText, parsedDelayMinutes))
                m_incidentNotifyDelayThresholdMinutes = parsedDelayMinutes;
            readBool("incidentNotifyThresholdUseOr", m_incidentNotifyThresholdUseOr);
            readString("incidentNotifyReasonExclusions", m_incidentNotifyReasonExclusions);
            readString("incidentNotifyLocationExclusions", m_incidentNotifyLocationExclusions);
            auto regionsIt = settings->find("incidentNotificationRegions");
            if (regionsIt != settings->end() && regionsIt->is_array()) {
                m_incidentNotificationRegions.clear();
                for (const json& item : *regionsIt) {
                    GeoPolygon polygon = GeoPolygonFromJson(item);
                    if (!polygon.name.empty() || !polygon.points.empty())
                        m_incidentNotificationRegions.push_back(std::move(polygon));
                }
            }
            auto templatesIt = settings->find("roadReportTemplates");
            if (templatesIt != settings->end() && templatesIt->is_array()) {
                m_reportTemplatesConfigured = true;
                m_reportTemplates.clear();
                for (const json& item : *templatesIt) {
                    if (!item.is_object())
                        continue;
                    ReportTemplate reportTemplate;
                    auto nameIt = item.find("name");
                    if (nameIt != item.end())
                        reportTemplate.name = JsonValueToText(*nameIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.body.empty())
                        m_reportTemplates.push_back(std::move(reportTemplate));
                }
            }
            auto earthquakeTemplatesIt = settings->find("earthquakeReportTemplates");
            if (earthquakeTemplatesIt != settings->end() && earthquakeTemplatesIt->is_array()) {
                m_earthquakeReportTemplatesConfigured = true;
                m_earthquakeReportTemplates.clear();
                for (const json& item : *earthquakeTemplatesIt) {
                    if (!item.is_object())
                        continue;
                    ReportTemplate reportTemplate;
                    auto nameIt = item.find("name");
                    if (nameIt != item.end())
                        reportTemplate.name = JsonValueToText(*nameIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.body.empty())
                        m_earthquakeReportTemplates.push_back(std::move(reportTemplate));
                }
            }
            auto weatherTemplatesIt = settings->find("weatherSystemReportTemplates");
            if (weatherTemplatesIt != settings->end() && weatherTemplatesIt->is_array()) {
                m_weatherSystemReportTemplatesConfigured = true;
                m_weatherSystemReportTemplates.clear();
                for (const json& item : *weatherTemplatesIt) {
                    if (!item.is_object())
                        continue;
                    ReportTemplate reportTemplate;
                    auto nameIt = item.find("name");
                    if (nameIt != item.end())
                        reportTemplate.name = JsonValueToText(*nameIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.body.empty())
                        m_weatherSystemReportTemplates.push_back(std::move(reportTemplate));
                }
            }

            readBool("showEarthquakes", m_showEarthquakes);
            readBool("showEarthquakeOverlayLabels", m_showEarthquakeOverlayLabels);
            readBool("showWeatherSystems", m_showWeatherSystems);
            readBool("showWeatherSystemOverlayLabels", m_showWeatherSystemOverlayLabels);
            readString("earthquakeListMagnitudeText", m_earthquakeListMagnitudeText);
            readString("earthquakeListTimeText", m_earthquakeListTimeText);
            readString("earthquakeNotificationMagnitudeText", m_earthquakeNotificationMagnitudeText);
            readDouble("earthquakeNotificationMagnitude", m_earthquakeNotificationMagnitude);
            double parsedMag = 0.0;
            if (TryParseDoubleText(m_earthquakeNotificationMagnitudeText, parsedMag))
                m_earthquakeNotificationMagnitude = parsedMag;
            readString("weatherSystemNotificationWindText", m_weatherSystemNotificationWindText);
            readDouble("weatherSystemNotificationWindKnots", m_weatherSystemNotificationWindKnots);
            double parsedWind = 0.0;
            if (TryParseDoubleText(m_weatherSystemNotificationWindText, parsedWind))
                m_weatherSystemNotificationWindKnots = parsedWind;

            auto earthquakeRegionIt = settings->find("earthquakeFilterRegion");
            if (earthquakeRegionIt != settings->end() && earthquakeRegionIt->is_array()) {
                m_earthquakeFilterRegion.clear();
                for (const json& item : *earthquakeRegionIt) {
                    if (!item.is_object())
                        continue;
                    double lat = 0.0;
                    double lon = 0.0;
                    if (PickDouble(item, { "lat", "latitude" }, lat) &&
                        PickDouble(item, { "lon", "longitude" }, lon))
                    {
                        m_earthquakeFilterRegion.push_back({ lat, lon });
                    }
                }
            }
        }
        catch (...) {
            OutputDebugStringW(L"Settings file could not be parsed; using defaults.\n");
        }
    }

    void EnsureDefaultReportTemplates()
    {
        if (!m_reportTemplates.empty() || m_reportTemplatesConfigured)
            return;

        ReportTemplate reportTemplate;
        reportTemplate.name = L"National Highways incident";
        reportTemplate.body = L"$DATE: NATIONAL HIGHWAYS REPORTS: A $TITLE on the $ROAD $DIRECTION %JUNCTIONS_WITH_DATA with %LANECLOSURES closed. Expect delays and congestion, avoid the area if possible, find alternate routes, monitor local traffic and media for updates. Allow extra time for your journey.";
        m_reportTemplates.push_back(std::move(reportTemplate));
        m_reportTemplatesConfigured = true;
    }

    void EnsureDefaultEarthquakeReportTemplates()
    {
        if (!m_earthquakeReportTemplates.empty() || m_earthquakeReportTemplatesConfigured)
            return;

        ReportTemplate reportTemplate;
        reportTemplate.name = L"Earthquake report";
        reportTemplate.body = L"$DATE: EARTHQUAKE REPORTS: An earthquake of magnitude $MAGNITUDE was recorded near $PLACE at $TIME. Coordinates: %LATITUDE, %LONGITUDE. Depth: %DEPTH km.";
        m_earthquakeReportTemplates.push_back(std::move(reportTemplate));
        m_earthquakeReportTemplatesConfigured = true;
    }

    void EnsureDefaultWeatherSystemReportTemplates()
    {
        if (!m_weatherSystemReportTemplates.empty() || m_weatherSystemReportTemplatesConfigured)
            return;

        ReportTemplate reportTemplate;
        reportTemplate.name = L"Weather system report";
        reportTemplate.body = L"$DATE: WEATHER SYSTEM REPORTS: $SYSTEM is active in the $BASIN basin as a $CATEGORY with winds of $WIND. Current position: %LATITUDE, %LONGITUDE. 24-hour projection: %FORECAST_LATITUDE, %FORECAST_LONGITUDE with $FORECAST_WIND winds.";
        m_weatherSystemReportTemplates.push_back(std::move(reportTemplate));
        m_weatherSystemReportTemplatesConfigured = true;
    }

    std::vector<ReportTemplate>& TemplatesForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplates;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplates;
        return m_reportTemplates;
    }

    const std::vector<ReportTemplate>& TemplatesForContext(TemplateContext context) const
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplates;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplates;
        return m_reportTemplates;
    }

    bool& TemplatesConfiguredForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplatesConfigured;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplatesConfigured;
        return m_reportTemplatesConfigured;
    }

    std::wstring DefaultTemplateBodyForContext(TemplateContext context) const
    {
        if (context == TemplateContext::Earthquakes)
            return L"$DATE: EARTHQUAKE REPORTS: An earthquake of magnitude $MAGNITUDE was recorded near $PLACE at $TIME. Coordinates: %LATITUDE, %LONGITUDE. Depth: %DEPTH km.";
        if (context == TemplateContext::WeatherSystems)
            return L"$DATE: WEATHER SYSTEM REPORTS: $SYSTEM is active in the $BASIN basin as a $CATEGORY with winds of $WIND. Current position: %LATITUDE, %LONGITUDE.";
        return L"$DATE: NATIONAL HIGHWAYS REPORTS: A $TITLE on the $ROAD $DIRECTION %JUNCTIONS_WITH_DATA with %LANECLOSURES closed. Allow extra time for your journey.";
    }

    void EnsureDefaultTemplatesForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            EnsureDefaultEarthquakeReportTemplates();
        else if (context == TemplateContext::WeatherSystems)
            EnsureDefaultWeatherSystemReportTemplates();
        else
            EnsureDefaultReportTemplates();
    }

    void ModernizeReportTemplates()
    {
        for (ReportTemplate& reportTemplate : m_reportTemplates) {
            ReplaceAllText(
                reportTemplate.body,
                L"between %JUNCTION1 (%JUNCTIONDATA1) and %JUNCTION2 (%JUNCTIONDATA2)",
                L"%JUNCTIONS_WITH_DATA");
        }
    }

    void SaveSettings() const
    {
        try {
            json root = json::object();
            {
                std::ifstream in(GetSettingsPath(), std::ios::binary);
                if (in) {
                    try {
                        root = json::parse(in);
                        if (!root.is_object())
                            root = json::object();
                    }
                    catch (...) {
                        root = json::object();
                    }
                }
            }

            root["version"] = 1;
            json& settings = root["settings"];
            if (!settings.is_object())
                settings = json::object();

            settings["alertsEndpoint"] = WideToUtf8(m_alertsEndpoint);
            settings["serverBaseUrl"] = WideToUtf8(m_serverBaseUrl);
            settings["alertOrder"] = WideToUtf8(m_alertOrder);
            settings["alertFilterUnplannedOnly"] = m_alertFilterUnplannedOnly;
            settings["periodicRefreshEnabled"] = m_periodicRefreshEnabled;
            settings["showNotificationHistory"] = m_showNotificationHistory;
            settings["displayWorldMap"] = m_displayWorldMap;
            settings["syncSettingsFromServer"] = m_syncSettingsFromServer;
            settings["refreshIntervalText"] = WideToUtf8(m_refreshIntervalText);
            settings["refreshIntervalMs"] = m_refreshIntervalMs;
            settings["incidentFilterSevere"] = m_incidentFilterSevere;
            settings["incidentFilterModerate"] = m_incidentFilterModerate;
            settings["incidentFilterMinor"] = m_incidentFilterMinor;
            settings["incidentFilterUnknown"] = m_incidentFilterUnknown;
            settings["incidentFilterUnplanned"] = m_incidentFilterUnplanned;
            settings["incidentFilterPlanned"] = m_incidentFilterPlanned;
            settings["incidentNotifyRoads"] = WideToUtf8(m_incidentNotifyRoads);
            settings["incidentNotifyRoadExclusions"] = WideToUtf8(m_incidentNotifyRoadExclusions);
            settings["incidentNotifyLaneThresholdText"] = WideToUtf8(m_incidentNotifyLaneThresholdText);
            settings["incidentNotifyLaneThreshold"] = m_incidentNotifyLaneThreshold;
            settings["incidentNotifyDelayThresholdText"] = WideToUtf8(m_incidentNotifyDelayThresholdText);
            settings["incidentNotifyDelayThresholdMinutes"] = m_incidentNotifyDelayThresholdMinutes;
            settings["incidentNotifyThresholdUseOr"] = m_incidentNotifyThresholdUseOr;
            settings["incidentNotifyReasonExclusions"] = WideToUtf8(m_incidentNotifyReasonExclusions);
            settings["incidentNotifyLocationExclusions"] = WideToUtf8(m_incidentNotifyLocationExclusions);
            settings["incidentNotificationRegions"] = json::array();
            for (const GeoPolygon& polygon : m_incidentNotificationRegions)
                settings["incidentNotificationRegions"].push_back(GeoPolygonToJson(polygon));
            settings["roadReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_reportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["roadReportTemplates"].push_back(std::move(item));
            }
            settings["earthquakeReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_earthquakeReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["earthquakeReportTemplates"].push_back(std::move(item));
            }
            settings["weatherSystemReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_weatherSystemReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["weatherSystemReportTemplates"].push_back(std::move(item));
            }
            settings["showEarthquakes"] = m_showEarthquakes;
            settings["showEarthquakeOverlayLabels"] = m_showEarthquakeOverlayLabels;
            settings["showWeatherSystems"] = m_showWeatherSystems;
            settings["showWeatherSystemOverlayLabels"] = m_showWeatherSystemOverlayLabels;
            settings["earthquakeListMagnitudeText"] = WideToUtf8(m_earthquakeListMagnitudeText);
            settings["earthquakeListTimeText"] = WideToUtf8(m_earthquakeListTimeText);
            settings["earthquakeFilterRegion"] = json::array();
            for (const GeoPoint& point : m_earthquakeFilterRegion) {
                settings["earthquakeFilterRegion"].push_back({
                    { "lat", point.lat },
                    { "lon", point.lon }
                    });
            }
            settings["earthquakeNotificationMagnitudeText"] = WideToUtf8(m_earthquakeNotificationMagnitudeText);
            settings["earthquakeNotificationMagnitude"] = m_earthquakeNotificationMagnitude;
            settings["weatherSystemNotificationWindText"] = WideToUtf8(m_weatherSystemNotificationWindText);
            settings["weatherSystemNotificationWindKnots"] = m_weatherSystemNotificationWindKnots;

            std::ofstream out(GetSettingsPath(), std::ios::binary | std::ios::trunc);
            if (out)
                out << root.dump();
        }
        catch (...) {
            OutputDebugStringW(L"Settings file could not be saved.\n");
        }
    }

    bool MergeSyncedSettingsIntoLocalSettings(const json& syncedSettings, std::wstring& errorOut) const
    {
        if (!syncedSettings.is_object()) {
            errorOut = L"Synced settings response was not an object.";
            return false;
        }

        try {
            json root = json::object();
            {
                std::ifstream in(GetSettingsPath(), std::ios::binary);
                if (in) {
                    try {
                        root = json::parse(in);
                        if (!root.is_object())
                            root = json::object();
                    }
                    catch (...) {
                        root = json::object();
                    }
                }
            }

            root["version"] = 1;
            json& localSettings = root["settings"];
            if (!localSettings.is_object())
                localSettings = json::object();

            for (auto it = syncedSettings.begin(); it != syncedSettings.end(); ++it)
                localSettings[it.key()] = it.value();

            std::ofstream out(GetSettingsPath(), std::ios::binary | std::ios::trunc);
            if (!out) {
                errorOut = L"Could not write the local settings file.";
                return false;
            }
            out << root.dump();
            return true;
        }
        catch (const std::exception& e) {
            errorOut = L"Could not merge synced settings: " + Utf8ToWide(e.what());
            return false;
        }
    }

    void ApplySettingsToRuntime()
    {
        m_alertsEndpoint = NormalizeUrl(m_alertsEndpoint);
        m_serverBaseUrl = NormalizeUrl(m_serverBaseUrl);
        EnsureDefaultReportTemplates();
        EnsureDefaultEarthquakeReportTemplates();
        EnsureDefaultWeatherSystemReportTemplates();
        ModernizeReportTemplates();

        ApplyRefreshTimer();
        UpdateNotificationHistoryMenu();
        UpdateEarthquakeMenu();
        UpdateWeatherSystemsMenu();
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        m_map.SetDisplayWorldMap(m_displayWorldMap);

        SortAlertsForCurrentOrder();
        if (m_listView)
            ApplyFilters(false);

        RebuildFilteredEarthquakes();
        RenderEarthquakeListRows();
        ApplyEarthquakeVisibility();
        m_filteredWeatherSystems = m_allWeatherSystems;
        RenderWeatherSystemsListRows();
        ApplyWeatherSystemVisibility();
        RenderNotificationHistory();
        SyncSettingsControls();
    }

    void SyncGlobalSettingsFromServerAsync()
    {
        if (!IsOnlineMode()) {
            SetStatusText(L"Sync Settings needs an online session.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            SetStatusText(L"Set the collaboration server before syncing settings.");
            return;
        }

        SetStatusText(L"Syncing settings from server...");
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, authHeaders]() {
            auto* result = new GlobalSettingsResult{};
            std::string response;
            std::wstring error;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/settings/global"), authHeaders, response, error)) {
                try {
                    json root = json::parse(response.empty() ? "{}" : response);
                    if (root.is_object()) {
                        auto settingsIt = root.find("settings");
                        result->settings = (settingsIt != root.end() && settingsIt->is_object())
                            ? *settingsIt
                            : root;
                        result->ok = result->settings.is_object();
                        if (!result->ok)
                            result->error = L"Server settings response did not include a settings object.";
                    }
                    else {
                        result->error = L"Server settings response was not an object.";
                    }
                }
                catch (const std::exception& e) {
                    result->error = L"Settings sync parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->error = L"Settings sync failed: " + error;
            }

            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SETTINGS_SYNC_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void OnGlobalSettingsSyncReady(GlobalSettingsResult* result)
    {
        std::unique_ptr<GlobalSettingsResult> holder(result);
        if (!result)
            return;

        if (!result->ok) {
            SetStatusText(result->error.empty() ? L"Settings sync failed." : result->error);
            return;
        }

        std::wstring error;
        if (!MergeSyncedSettingsIntoLocalSettings(result->settings, error)) {
            SetStatusText(error);
            return;
        }

        LoadSettings();
        ApplySettingsToRuntime();
        SaveSettings();
        SetStatusText(L"Settings synced from server.");
    }

    void ApplyRefreshTimer()
    {
        KillTimer(m_hwnd, kAlertRefreshTimerId);
        if (m_periodicRefreshEnabled)
            SetTimer(m_hwnd, kAlertRefreshTimerId, m_refreshIntervalMs, nullptr);
    }

    void RefreshFeedAsync()
    {
        if (g_fetchInProgress.exchange(true)) {
            SetStatusText(L"Already fetching alerts...");
            return;
        }

        std::wstring url = NormalizeUrl(m_alertsEndpoint);
        if (url.empty()) {
            g_fetchInProgress.store(false);
            SetStatusText(L"Please enter a feed URL.");
            return;
        }

        SetStatusText(L"Fetching alerts...");

        HWND hwnd = m_hwnd;
        const bool unplannedOnly = m_alertFilterUnplannedOnly;
        const std::wstring order = m_alertOrder;

        ScheduleBackgroundTask([hwnd, url, unplannedOnly, order]() {
            auto* result = new FeedResult{};
            std::string body;
            std::wstring error;

            if (IsTrafficEnglandAlertsPageUrl(url)) {
                std::vector<TrafficAlert> alerts;
                if (FetchTrafficEnglandAlerts(alerts, error, unplannedOnly, order)) {
                    result->ok = true;
                    result->alerts = std::move(alerts);
                }
                else {
                    result->ok = false;
                    result->error = error + L" Showing sample data.";
                    result->alerts = SampleAlerts();
                }
            }
            else if (HttpGetText(url, body, error)) {
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
            });
    }

    void OnFeedReady(FeedResult* result)
    {
        g_fetchInProgress.store(false);

        if (!result)
            return;

        const bool feedOk = result->ok;
        m_allAlerts = result->alerts;
        DownloadMissingLaneImagesAsync(m_allAlerts);
        SortAlertsForCurrentOrder();
        delete result;

        const bool fitMap = !m_hasLoadedAlerts;
        size_t visible = ApplyFilters(fitMap);
        m_hasLoadedAlerts = true;

        if (m_allAlerts.empty()) {
            SetStatusText(L"No alerts available.");
        }
        else {
            SetStatusText(L"Loaded " + std::to_wstring(visible) + L" alert(s).");
        }

        if (feedOk)
            NotifyForMatchingIncidents(m_allAlerts);
    }

    bool SeverityAllowedForIncidentNotification(const TrafficAlert& alert) const
    {
        std::wstring bucket = SeverityBucket(alert.severity);
        if (bucket == L"severe") return m_incidentFilterSevere;
        if (bucket == L"moderate") return m_incidentFilterModerate;
        if (bucket == L"minor") return m_incidentFilterMinor;
        return m_incidentFilterUnknown;
    }

    bool IsPlannedIncident(const TrafficAlert& alert) const
    {
        std::wstring text = ToLower(alert.eventType + L" " + alert.title + L" " + alert.description);
        return text.find(L"roadworks") != std::wstring::npos ||
            text.find(L"road works") != std::wstring::npos ||
            text.find(L"planned") != std::wstring::npos;
    }

    bool IncidentTypeAllowedForNotification(const TrafficAlert& alert) const
    {
        return IsPlannedIncident(alert) ? m_incidentFilterPlanned : m_incidentFilterUnplanned;
    }

    std::wstring AlertReasonForNotification(const TrafficAlert& alert) const
    {
        std::wstring reason = ExtractLabeledNotificationField(alert.description, L"Reason");
        if (!reason.empty())
            return reason;
        return alert.title;
    }

    std::wstring AlertLocationForNotification(const TrafficAlert& alert) const
    {
        std::wstring location = ExtractLabeledNotificationField(alert.description, L"Location");
        if (!location.empty())
            return location;
        return alert.region;
    }

    bool RoadListMatches(const std::wstring& road, const std::wstring& filter, bool emptyMatchesAll) const
    {
        std::vector<std::wstring> roads = SplitCommaSeparatedTokens(filter);
        if (roads.empty())
            return emptyMatchesAll;

        for (const std::wstring& token : roads) {
            if (RoadTokenMatches(road, token))
                return true;
        }
        return false;
    }

    bool AlertMatchesIncidentNotificationRegion(const TrafficAlert& alert) const
    {
        if (!alert.hasLocation)
            return false;

        std::wstring road = alert.road.empty() ? alert.region : alert.road;
        for (const GeoPolygon& polygon : m_incidentNotificationRegions) {
            if (polygon.points.size() < 3 || !PointInPolygon(alert.latitude, alert.longitude, polygon.points))
                continue;
            if (polygon.allRoads || RoadListMatches(road, polygon.roadFilter, true))
                return true;
        }
        return false;
    }

    bool RoadMatchesIncidentNotification(const TrafficAlert& alert) const
    {
        std::wstring road = alert.road.empty() ? alert.region : alert.road;
        if (RoadListMatches(road, m_incidentNotifyRoads, m_incidentNotificationRegions.empty()))
            return true;
        return AlertMatchesIncidentNotificationRegion(alert);
    }

    bool RoadExcludedFromIncidentNotification(const TrafficAlert& alert) const
    {
        std::wstring road = alert.road.empty() ? alert.region : alert.road;
        return RoadListMatches(road, m_incidentNotifyRoadExclusions, false);
    }

    bool ReasonExcludedFromNotification(const TrafficAlert& alert) const
    {
        std::wstring reason = ToLower(AlertReasonForNotification(alert));
        if (reason.empty())
            return false;

        for (const std::wstring& exclusion : SplitCommaSeparatedTokens(m_incidentNotifyReasonExclusions)) {
            std::wstring value = ToLower(Trim(exclusion));
            if (!value.empty() && reason.find(value) != std::wstring::npos)
                return true;
        }

        return false;
    }

    bool LocationExcludedFromNotification(const TrafficAlert& alert) const
    {
        std::wstring location = ToLower(AlertLocationForNotification(alert));
        if (location.empty())
            return false;

        for (const std::wstring& exclusion : SplitCommaSeparatedTokens(m_incidentNotifyLocationExclusions)) {
            std::wstring value = ToLower(Trim(exclusion));
            if (!value.empty() && location.find(value) != std::wstring::npos)
                return true;
        }

        return false;
    }

    double ClosedLanePercentage(const TrafficAlert& alert) const
    {
        if (alert.lanesClosed <= 0 || alert.lanesTotal <= 0)
            return 0.0;
        return (static_cast<double>(alert.lanesClosed) * 100.0) / static_cast<double>(alert.lanesTotal);
    }

    bool LaneThresholdMatchesIncidentNotification(const TrafficAlert& alert) const
    {
        if (m_incidentNotifyLaneThreshold <= 0.0)
            return true;
        if (alert.lanesClosed <= 0 || alert.lanesTotal <= 0)
            return false;
        return ClosedLanePercentage(alert) + 0.0001 >= m_incidentNotifyLaneThreshold;
    }

    bool DelayThresholdMatchesIncidentNotification(const TrafficAlert& alert) const
    {
        if (m_incidentNotifyDelayThresholdMinutes <= 0.0)
            return true;

        double delayMinutes = 0.0;
        return TryExtractAlertDelayMinutes(alert, delayMinutes) &&
            delayMinutes + 0.0001 >= m_incidentNotifyDelayThresholdMinutes;
    }

    bool IncidentThresholdsMatchNotification(const TrafficAlert& alert) const
    {
        bool laneMatch = LaneThresholdMatchesIncidentNotification(alert);
        bool delayMatch = DelayThresholdMatchesIncidentNotification(alert);
        return m_incidentNotifyThresholdUseOr ? (laneMatch || delayMatch) : (laneMatch && delayMatch);
    }

    bool AlertMatchesIncidentNotification(const TrafficAlert& alert) const
    {
        return RoadMatchesIncidentNotification(alert) &&
            !RoadExcludedFromIncidentNotification(alert) &&
            SeverityAllowedForIncidentNotification(alert) &&
            IncidentTypeAllowedForNotification(alert) &&
            IncidentThresholdsMatchNotification(alert) &&
            !ReasonExcludedFromNotification(alert) &&
            !LocationExcludedFromNotification(alert);
    }

    std::wstring IncidentNotificationStableKey(const TrafficAlert& alert) const
    {
        if (!alert.id.empty())
            return alert.id;

        std::wstring key = alert.road.empty() ? alert.region : alert.road;
        key += L"|";
        key += AlertLocationForNotification(alert);
        key += L"|";
        key += AlertReasonForNotification(alert);
        if (Trim(key) == L"||")
            return BuildAlertSummary(alert);
        return key;
    }

    std::wstring IncidentNotificationSignature(const TrafficAlert& alert) const
    {
        return alert.updatedText + L"|" +
            alert.title + L"|" +
            alert.description + L"|" +
            alert.severity + L"|" +
            alert.eventType + L"|" +
            std::to_wstring(alert.lanesClosed) + L"/" +
            std::to_wstring(alert.lanesTotal);
    }

    std::wstring IncidentNotificationLine(const TrafficAlert& alert) const
    {
        std::wstring road = alert.road.empty() ? L"Unknown road" : alert.road;
        std::wstring reason = AlertReasonForNotification(alert);
        std::wstring line = road;
        if (!reason.empty()) {
            line += L" - ";
            line += reason;
        }
        if (alert.lanesTotal > 0) {
            line += L" (";
            line += std::to_wstring(alert.lanesClosed);
            line += L" of ";
            line += std::to_wstring(alert.lanesTotal);
            line += L" lanes closed)";
        }
        double delayMinutes = 0.0;
        if (TryExtractAlertDelayMinutes(alert, delayMinutes)) {
            line += L" (delay ";
            line += std::to_wstring(static_cast<int>(std::round(delayMinutes)));
            line += L" min)";
        }
        return line;
    }

    void PublishIncidentNotificationBatch(
        const std::vector<std::wstring>& lines,
        const std::wstring& singleTitleSuffix,
        const std::wstring& pluralTitle)
    {
        if (lines.empty())
            return;

        std::wstring title;
        std::wstring body;
        if (lines.size() == 1) {
            title = singleTitleSuffix;
            body = lines.front();
        }
        else {
            title = std::to_wstring(lines.size()) + L" " + pluralTitle;
            const size_t displayCount = MinValue<size_t>(lines.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += lines[i];
            }
            if (lines.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body);
    }

    void NotifyForMatchingIncidents(const std::vector<TrafficAlert>& alerts)
    {
        std::unordered_set<std::wstring> currentIncidentKeys;
        std::vector<std::wstring> newLines;
        std::vector<std::wstring> updateLines;
        std::vector<std::wstring> removedLines;

        for (const TrafficAlert& alert : alerts) {
            std::wstring stableKey = IncidentNotificationStableKey(alert);
            currentIncidentKeys.insert(stableKey);

            if (!AlertMatchesIncidentNotification(alert)) {
                auto existing = m_notifiedIncidentStates.find(stableKey);
                if (m_haveIncidentNotificationSnapshot && existing != m_notifiedIncidentStates.end()) {
                    removedLines.push_back(existing->second.line);
                    m_notifiedIncidentStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = IncidentNotificationSignature(alert);
            std::wstring line = IncidentNotificationLine(alert);
            auto existing = m_notifiedIncidentStates.find(stableKey);
            if (existing == m_notifiedIncidentStates.end()) {
                newLines.push_back(line);
                m_notifiedIncidentStates[stableKey] = IncidentNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back(line);
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveIncidentNotificationSnapshot) {
            for (auto it = m_notifiedIncidentStates.begin(); it != m_notifiedIncidentStates.end();) {
                if (currentIncidentKeys.find(it->first) == currentIncidentKeys.end()) {
                    removedLines.push_back(it->second.line);
                    it = m_notifiedIncidentStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveIncidentNotificationSnapshot = true;

        PublishIncidentNotificationBatch(newLines, L"Incident notification", L"matching incident notifications");
        PublishIncidentNotificationBatch(updateLines, L"Incident update", L"incident updates");
        PublishIncidentNotificationBatch(removedLines, L"Incident removed", L"incident removals");
    }

    void EnsureNotificationIcon()
    {
        if (m_notificationIconAdded || !m_hwnd)
            return;

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = m_hwnd;
        nid.uID = kNotificationIconId;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_APP_NOTIFY_ICON;
        nid.hIcon = LoadIconW(nullptr, IDI_INFORMATION);
        wcsncpy_s(nid.szTip, L"ERC Tools", _TRUNCATE);

        if (Shell_NotifyIconW(NIM_ADD, &nid)) {
            nid.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &nid);
            m_notificationIconAdded = true;
        }
    }

    void RemoveNotificationIcon()
    {
        if (!m_notificationIconAdded)
            return;

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = m_hwnd;
        nid.uID = kNotificationIconId;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        m_notificationIconAdded = false;
    }

    void ShowWindowsIncidentNotification(const std::wstring& title, const std::wstring& body)
    {
        EnsureNotificationIcon();
        if (!m_notificationIconAdded)
            return;

        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = m_hwnd;
        nid.uID = kNotificationIconId;
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_WARNING;
        wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
        wcsncpy_s(nid.szInfo, body.c_str(), _TRUNCATE);
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    void ShowInAppIncidentNotification(const std::wstring& title, const std::wstring& body)
    {
        AppNotification notification;
        notification.title = title;
        notification.body = body;
        notification.timestamp = TimeTToText(std::time(nullptr));
        m_map.SetActiveNotification(notification);
        KillTimer(m_hwnd, kInAppNotificationTimerId);
        SetTimer(m_hwnd, kInAppNotificationTimerId, 10 * 1000, nullptr);
    }

    void RenderNotificationHistory()
    {
        m_map.SetNotificationHistory(m_notificationHistory);
    }

    void ClearNotificationHistory()
    {
        m_notificationHistory.clear();
        RenderNotificationHistory();
        SetStatusText(L"Notification history cleared.");
    }

    void AddNotificationHistory(const std::wstring& title, const std::wstring& body)
    {
        AppNotification entry;
        entry.title = title;
        entry.body = body;
        entry.timestamp = TimeTToText(std::time(nullptr));
        m_notificationHistory.insert(m_notificationHistory.begin(), std::move(entry));
        if (m_notificationHistory.size() > 100)
            m_notificationHistory.resize(100);
        RenderNotificationHistory();
    }

    void PublishNotification(const std::wstring& title, const std::wstring& body)
    {
        AddNotificationHistory(title, body);
        ShowWindowsIncidentNotification(title, body);
        ShowInAppIncidentNotification(title, body);
    }

    void DownloadMissingLaneImagesAsync(const std::vector<TrafficAlert>& alerts)
    {
        std::vector<std::wstring> urls;
        for (const TrafficAlert& alert : alerts) {
            for (const std::wstring& url : alert.laneImageUrls) {
                if (url.empty() || std::find(urls.begin(), urls.end(), url) != urls.end())
                    continue;
                if (std::filesystem::exists(GetLaneImageCachePath(url)))
                    continue;
                urls.push_back(url);
            }
        }

        if (urls.empty())
            return;

        HWND hwnd = m_hwnd;
        HWND mapHwnd = m_map.Hwnd();
        ScheduleBackgroundTask([hwnd, mapHwnd, urls = std::move(urls)]() {
            for (const std::wstring& url : urls) {
                if (g_appQuitting.load())
                    return;

                std::vector<BYTE> bytes;
                std::wstring error;
                if (HttpGetBinary(url, bytes, error))
                    SaveBinaryToFile(GetLaneImageCachePath(url), bytes);
            }

            if (mapHwnd && !g_appQuitting.load() && IsWindow(mapHwnd))
                InvalidateRect(mapHwnd, nullptr, FALSE);
            else if (hwnd && !g_appQuitting.load() && IsWindow(hwnd))
                InvalidateRect(hwnd, nullptr, FALSE);
            });
    }

    bool TextFilterMatches(const TrafficAlert& a) const
    {
        std::wstring q = ToLower(Trim(GetWindowTextString(m_searchEdit)));
        if (q.empty())
            return true;

        std::wstring hay =
            ToLower(a.id + L" " + a.title + L" " + a.description + L" " +
                a.road + L" " + a.region + L" " + a.severity + L" " + a.eventType);

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
            std::wstring updated = a.updatedText.empty() ? L"" : a.updatedText;

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


    std::wstring ServerBaseUrl() const
    {
        return NormalizeUrl(m_serverBaseUrl);
    }

    void RenderChatHistory()
    {
        if (m_map.Hwnd())
            m_map.SetChatMessages(m_chatMessages);
    }

    void PollServerAsync()
    {
        if (!IsOnlineMode())
            return;

        if (m_serverRequestInProgress.exchange(true))
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            m_serverRequestInProgress.store(false);
            return;
        }

        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, authHeaders]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::Poll;

            std::string chatBody;
            std::wstring chatError;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/chat"), authHeaders, chatBody, chatError)) {
                try {
                    result->chat = ParseChatMessages(json::parse(chatBody));
                    result->chatOk = true;
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    result->error = L"Chat parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->error = L"Chat poll failed: " + chatError;
            }

            std::string noteBody;
            std::wstring noteError;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/notes"), authHeaders, noteBody, noteError)) {
                try {
                    result->notes = ParseMapNotes(json::parse(noteBody));
                    result->notesOk = true;
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Notes parse failed: " + Utf8ToWide(e.what());
                }
            }
            else if (result->error.empty()) {
                result->error = L"Notes poll failed: " + noteError;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            });
    }

    void CheckForClientUpdateAsync()
    {
        if (!IsOnlineMode())
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty())
            return;

        HWND hwnd = m_hwnd;
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, session]() {
            auto* result = new ClientUpdateResult{};
            CheckAndStageClientUpdate(server, session, *result);
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_UPDATE_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void OnClientUpdateReady(ClientUpdateResult* result)
    {
        std::unique_ptr<ClientUpdateResult> holder(result);
        if (!result)
            return;

        if (!result->ok) {
            if (!result->error.empty())
                SetStatusText(L"Update check failed: " + result->error);
            return;
        }

        if (!result->updateAvailable) {
            SetStatusText(L"Ready.");
            return;
        }

        std::wstring error;
        if (!result->restartRequired) {
            if (ApplyHotClientUpdate(*result, error)) {
                SetStatusText(L"Updated to " + result->version + L" while running.");
            }
            else {
                SetStatusText(L"Hot update failed: " + error);
            }
            return;
        }

        std::wstring prompt = L"ERC Tools update " + result->version + L" has been downloaded and requires a restart.\n\nRestart now to apply it?";
        int choice = MessageBoxW(m_hwnd, prompt.c_str(), L"ERC Tools Update", MB_YESNO | MB_ICONQUESTION);
        if (choice != IDYES) {
            SetStatusText(L"Update downloaded; restart deferred.");
            return;
        }

        if (!LaunchRestartClientUpdate(*result, error)) {
            SetStatusText(L"Could not launch updater: " + error);
            return;
        }

        SetStatusText(L"Restarting to apply update...");
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }

    void SendChatTextAsync(const std::wstring& inputText)
    {
        std::wstring text = Trim(inputText);
        if (text.empty())
            return;

        std::wstring author = SessionDisplayName();
        ChatMessage local{ author, m_session.position, text, L"pending" };
        m_chatMessages.push_back(local);
        RenderChatHistory();

        if (!IsOnlineMode()) {
            SetStatusText(L"Offline mode: chat kept locally.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, text, author, authHeaders]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendChat;
            std::string response;
            std::wstring error;
            std::string body = "{\"author\":" + JsonEscape(author) + ",\"text\":" + JsonEscape(text) + "}";
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/chat"), body, authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    static bool IsLocalOnlyNoteId(const std::wstring& id)
    {
        return id.empty() || id.rfind(L"local-", 0) == 0;
    }

    void CreateMapNote(const std::wstring& text, double lat, double lon)
    {
        MapNote note;
        note.id = L"local-" + std::to_wstring(GetTickCount64());
        note.author = MapNoteAuthor();
        note.text = text;
        note.timestamp = IsOnlineMode() ? L"pending" : L"";
        note.latitude = lat;
        note.longitude = lon;
        m_notes.push_back(note);
        m_map.SetNotes(m_notes);
        SetStatusText(IsOnlineMode() ? L"Map note added." : L"Map note added locally.");

        if (!IsOnlineMode())
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            SetStatusText(L"Map note added locally.");
            return;
        }

        HWND hwnd = m_hwnd;
        std::string body = BuildNoteJsonBody(note);
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, body, authHeaders]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendNote;
            std::string response;
            std::wstring error;
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/notes"), body, authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void UpdateMapNote(size_t index, const std::wstring& text)
    {
        if (index >= m_notes.size())
            return;

        MapNote& note = m_notes[index];
        note.text = text;
        if (!IsOnlineMode()) {
            note.timestamp.clear();
            m_map.SetNotes(m_notes);
            SetStatusText(L"Map note updated locally.");
            return;
        }

        if (!IsLocalOnlyNoteId(note.id)) {
            note.timestamp = L"pending edit";
            m_pendingNoteEdits[note.id] = note;
        }
        else {
            note.timestamp = L"pending";
        }
        m_map.SetNotes(m_notes);
        SetStatusText(L"Map note updated.");

        if (IsLocalOnlyNoteId(note.id))
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty())
            return;

        HWND hwnd = m_hwnd;
        std::wstring noteId = note.id;
        std::string body = BuildNoteJsonBody(note);
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, noteId, body, authHeaders]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::UpdateNote;
            std::string response;
            std::wstring error;
            std::wstring endpoint = AppendNoteIdPath(server, noteId);
            result->ok = HttpPutJsonTextWithHeaders(endpoint, body, authHeaders, response, error);
            if (!result->ok)
                result->ok = HttpPatchJsonTextWithHeaders(endpoint, body, authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void DeleteMapNote(size_t index)
    {
        if (index >= m_notes.size())
            return;

        MapNote note = m_notes[index];
        if (!IsLocalOnlyNoteId(note.id)) {
            m_deletedNoteIds.insert(note.id);
            m_pendingNoteEdits.erase(note.id);
        }
        m_notes.erase(m_notes.begin() + static_cast<std::ptrdiff_t>(index));
        m_map.SetNotes(m_notes);
        SetStatusText(IsOnlineMode() ? L"Map note removed." : L"Map note removed locally.");

        if (!IsOnlineMode())
            return;

        if (IsLocalOnlyNoteId(note.id))
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty())
            return;

        HWND hwnd = m_hwnd;
        std::wstring noteId = note.id;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ScheduleBackgroundTask([hwnd, server, noteId, authHeaders]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::DeleteNote;
            std::string response;
            std::wstring error;
            result->ok = HttpDeleteTextWithHeaders(AppendNoteIdPath(server, noteId), authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void ReconcilePendingNoteEdits(const std::vector<MapNote>& serverNotes)
    {
        for (auto it = m_pendingNoteEdits.begin(); it != m_pendingNoteEdits.end();) {
            const auto found = std::find_if(serverNotes.begin(), serverNotes.end(), [&](const MapNote& note) {
                return note.id == it->first && note.text == it->second.text;
                });
            if (found != serverNotes.end())
                it = m_pendingNoteEdits.erase(it);
            else
                ++it;
        }
    }

    void ApplyLocalNoteOverrides(std::vector<MapNote>& serverNotes)
    {
        serverNotes.erase(std::remove_if(serverNotes.begin(), serverNotes.end(), [&](const MapNote& note) {
            return !note.id.empty() && m_deletedNoteIds.find(note.id) != m_deletedNoteIds.end();
            }), serverNotes.end());

        for (const auto& [id, pendingNote] : m_pendingNoteEdits) {
            bool replaced = false;
            for (MapNote& note : serverNotes) {
                if (note.id == id) {
                    note = pendingNote;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                serverNotes.push_back(pendingNote);
        }

        MergePendingLocalNotes(serverNotes, m_notes);
    }

    void OnServerReady(ServerResult* result)
    {
        m_serverRequestInProgress.store(false);
        if (!result)
            return;

        if (result->action == ServerAction::Poll && result->ok) {
            if (result->chatOk && !result->chat.empty()) {
                m_chatMessages = std::move(result->chat);
                RenderChatHistory();
            }

            if (result->notesOk) {
                ReconcilePendingNoteEdits(result->notes);
                ApplyLocalNoteOverrides(result->notes);
                if (!MapNotesEqual(m_notes, result->notes)) {
                    m_notes = std::move(result->notes);
                    m_map.SetNotes(m_notes);
                }
            }
        }
        else if (result->action == ServerAction::SendChat) {
            SetStatusText(result->ok ? L"Chat message sent." : L"Chat send failed; kept locally.");
            PollServerAsync();
        }
        else if (result->action == ServerAction::SendNote) {
            SetStatusText(result->ok ? L"Map note shared." : L"Note share failed; kept locally.");
            PollServerAsync();
        }
        else if (result->action == ServerAction::UpdateNote) {
            SetStatusText(result->ok ? L"Map note synced." : L"Note update kept locally.");
            PollServerAsync();
        }
        else if (result->action == ServerAction::DeleteNote) {
            SetStatusText(result->ok ? L"Map note deleted." : L"Note delete kept locally.");
            PollServerAsync();
        }

        delete result;
    }

    void DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind kind = BoundaryDownloadKind::Uk)
    {
        if (g_boundaryDownloadInProgress.exchange(true)) {
            SetStatusText(L"Boundary download already in progress...");
            return;
        }

        SetStatusText(kind == BoundaryDownloadKind::World
            ? L"Downloading world boundaries from geoBoundaries..."
            : L"Downloading UK boundary from geoBoundaries...");

        HWND hwnd = m_hwnd;
        const std::wstring sourceUrl = kind == BoundaryDownloadKind::World ? kWorldBoundarySourceUrl : kUkBoundarySourceUrl;
        const std::filesystem::path cachePath = kind == BoundaryDownloadKind::World ? GetWorldBoundaryCachePath() : GetBoundaryCachePath();

        ScheduleBackgroundTask([hwnd, kind, sourceUrl, cachePath]() {
            auto* result = new BoundaryDownloadResult{};
            result->kind = kind;
            std::vector<BYTE> bytes;
            std::wstring error;

            if (!HttpGetBinary(sourceUrl, bytes, error)) {
                result->ok = false;
                result->error = L"Boundary download failed: " + error;
            }
            else {
                if (!SaveBinaryToFile(cachePath, bytes)) {
                    result->ok = false;
                    result->error = L"Boundary downloaded but could not be saved locally.";
                }
                else {
                    result->ok = true;
                    result->filePath = cachePath;
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
            });
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
        const bool loaded = result->kind == BoundaryDownloadKind::World
            ? m_map.LoadWorldBoundaryFromFile(result->filePath, &loadError)
            : m_map.LoadUkBoundaryFromFile(result->filePath, &loadError);
        if (loaded) {
            SetStatusText(result->kind == BoundaryDownloadKind::World
                ? L"World boundaries downloaded and loaded."
                : L"UK boundary downloaded and loaded.");
        }
        else {
            SetStatusText(L"Boundary downloaded, but could not load it.");
            OutputDebugStringW((L"Boundary load failed: " + loadError + L"\n").c_str());
        }

        delete result;
    }

    void CreateMainMenu()
    {
        HMENU menu = CreateMenu();
        HMENU fileMenu = CreatePopupMenu();
        HMENU roadsMenu = CreatePopupMenu();
        HMENU earthquakesMenu = CreatePopupMenu();
        HMENU weatherMenu = CreatePopupMenu();
        HMENU viewMenu = CreatePopupMenu();
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_SETTINGS, L"Settings...");
        AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXIT, L"Exit");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_FILTERS, L"Incident Filters...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_NOTIFICATIONS, L"Incident Notifications...");
        AppendMenuW(roadsMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_TEMPLATES_WIZARD, L"Templates Wizard...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_EDIT_TEMPLATES, L"Edit Templates...");
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKES_LIST, L"Earthquakes List...");
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKE_NOTIFICATIONS, L"Earthquake Notifications...");
        AppendMenuW(earthquakesMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKES_TEMPLATES_WIZARD, L"Templates Wizard...");
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKES_EDIT_TEMPLATES, L"Edit Templates...");
        AppendMenuW(earthquakesMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(earthquakesMenu, m_showEarthquakes ? MF_CHECKED : MF_UNCHECKED, IDM_SHOW_EARTHQUAKES, L"Show Earthquakes");
        HMENU earthquakeOverlayMenu = CreatePopupMenu();
        const UINT overlayEnabled = m_showEarthquakes ? MF_ENABLED : MF_GRAYED;
        AppendMenuW(earthquakeOverlayMenu, overlayEnabled | (m_showEarthquakeOverlayLabels ? MF_UNCHECKED : MF_CHECKED), IDM_EARTHQUAKE_OVERLAY_NONE, L"None");
        AppendMenuW(earthquakeOverlayMenu, overlayEnabled | (m_showEarthquakeOverlayLabels ? MF_CHECKED : MF_UNCHECKED), IDM_EARTHQUAKE_OVERLAY_MAG_REGION, L"Magnitude and Region");
        AppendMenuW(earthquakesMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(earthquakeOverlayMenu), L"Earthquake Overlays");
        MENUITEMINFOW showEarthquakesInfo{};
        showEarthquakesInfo.cbSize = sizeof(showEarthquakesInfo);
        showEarthquakesInfo.fMask = MIIM_FTYPE;
        showEarthquakesInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(earthquakesMenu, IDM_SHOW_EARTHQUAKES, FALSE, &showEarthquakesInfo);
        MENUITEMINFOW earthquakeOverlayInfo{};
        earthquakeOverlayInfo.cbSize = sizeof(earthquakeOverlayInfo);
        earthquakeOverlayInfo.fMask = MIIM_FTYPE;
        earthquakeOverlayInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(earthquakeOverlayMenu, IDM_EARTHQUAKE_OVERLAY_NONE, FALSE, &earthquakeOverlayInfo);
        SetMenuItemInfoW(earthquakeOverlayMenu, IDM_EARTHQUAKE_OVERLAY_MAG_REGION, FALSE, &earthquakeOverlayInfo);
        AppendMenuW(weatherMenu, MF_STRING, IDM_WEATHER_SYSTEMS_LIST, L"Weather Systems List...");
        AppendMenuW(weatherMenu, MF_STRING, IDM_WEATHER_SYSTEM_NOTIFICATIONS, L"Weather System Notifications...");
        AppendMenuW(weatherMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(weatherMenu, MF_STRING, IDM_WEATHER_SYSTEMS_TEMPLATES_WIZARD, L"Templates Wizard...");
        AppendMenuW(weatherMenu, MF_STRING, IDM_WEATHER_SYSTEMS_EDIT_TEMPLATES, L"Edit Templates...");
        AppendMenuW(weatherMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(weatherMenu, m_showWeatherSystems ? MF_CHECKED : MF_UNCHECKED, IDM_SHOW_WEATHER_SYSTEMS, L"Show Weather Systems");
        HMENU weatherOverlayMenu = CreatePopupMenu();
        const UINT weatherOverlayEnabled = m_showWeatherSystems ? MF_ENABLED : MF_GRAYED;
        AppendMenuW(weatherOverlayMenu, weatherOverlayEnabled | (m_showWeatherSystemOverlayLabels ? MF_UNCHECKED : MF_CHECKED), IDM_WEATHER_SYSTEM_OVERLAY_NONE, L"None");
        AppendMenuW(weatherOverlayMenu, weatherOverlayEnabled | (m_showWeatherSystemOverlayLabels ? MF_CHECKED : MF_UNCHECKED), IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND, L"Name and Wind");
        AppendMenuW(weatherMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(weatherOverlayMenu), L"Weather System Overlays");
        MENUITEMINFOW showWeatherInfo{};
        showWeatherInfo.cbSize = sizeof(showWeatherInfo);
        showWeatherInfo.fMask = MIIM_FTYPE;
        showWeatherInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(weatherMenu, IDM_SHOW_WEATHER_SYSTEMS, FALSE, &showWeatherInfo);
        MENUITEMINFOW weatherOverlayInfo{};
        weatherOverlayInfo.cbSize = sizeof(weatherOverlayInfo);
        weatherOverlayInfo.fMask = MIIM_FTYPE;
        weatherOverlayInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(weatherOverlayMenu, IDM_WEATHER_SYSTEM_OVERLAY_NONE, FALSE, &weatherOverlayInfo);
        SetMenuItemInfoW(weatherOverlayMenu, IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND, FALSE, &weatherOverlayInfo);
        AppendMenuW(viewMenu, m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_NOTIFICATION_HISTORY, L"Notification History");
        MENUITEMINFOW historyInfo{};
        historyInfo.cbSize = sizeof(historyInfo);
        historyInfo.fMask = MIIM_FTYPE;
        historyInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(viewMenu, IDM_VIEW_NOTIFICATION_HISTORY, FALSE, &historyInfo);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"File");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu), L"View");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(roadsMenu), L"Roads");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(earthquakesMenu), L"Earthquakes");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(weatherMenu), L"Weather Systems");
        AppendMenuW(menu, MF_STRING, IDM_ABOUT, L"About");
        SetMenu(m_hwnd, menu);
    }

    void ShowAboutDialog()
    {
        MessageBoxW(
            m_hwnd,
            L"ERC Tools\n\nView live alerts on a UK map, collaborate with local responders, and share map notes.",
            L"About ERC Tools",
            MB_OK | MB_ICONINFORMATION);
    }

    static LRESULT CALLBACK IncidentFiltersWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleIncidentFiltersMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleIncidentFiltersMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateIncidentFiltersControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnIncidentFiltersCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowIncidentFiltersWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = IncidentFiltersWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kIncidentFiltersClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_incidentFiltersWnd || !IsWindow(m_incidentFiltersWnd)) {
            m_incidentFiltersWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kIncidentFiltersClassName,
                L"Incident Filters",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                470,
                360,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        SyncIncidentFilterControls();
        ShowWindow(m_incidentFiltersWnd, SW_SHOW);
        SetForegroundWindow(m_incidentFiltersWnd);
    }

    void CreateIncidentFiltersControls(HWND parent)
    {
        CreateAutoLabel(parent, IDC_INCIDENT_FILTERS_TITLE_LABEL, L"Incident Filters", 18, 18, m_headerFont);
        HWND descLabel = CreateAutoLabel(
            parent,
            IDC_INCIDENT_FILTERS_DESC_LABEL,
            L"Choose which road incident categories should be available for filtering. These controls are ready for the next filtering step.",
            18,
            54,
            nullptr,
            416);
        const int descH = AutoLabelHeight(descLabel, 44, 416);

        CreateAutoLabel(parent, IDC_INCIDENT_FILTERS_SEVERITY_LABEL, L"Severity", 18, 54 + descH + 18);
        const int severityY = 54 + descH + 46;
        m_incidentSevereCheck = CreateWindowExW(0, L"BUTTON", L"Severe", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, severityY, 120, 24, parent, ControlId(IDC_INCIDENT_FILTERS_SEVERE_CHECK), m_hInst, nullptr);
        m_incidentModerateCheck = CreateWindowExW(0, L"BUTTON", L"Moderate", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 148, severityY, 130, 24, parent, ControlId(IDC_INCIDENT_FILTERS_MODERATE_CHECK), m_hInst, nullptr);
        m_incidentMinorCheck = CreateWindowExW(0, L"BUTTON", L"Minor", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 288, severityY, 110, 24, parent, ControlId(IDC_INCIDENT_FILTERS_MINOR_CHECK), m_hInst, nullptr);
        m_incidentUnknownCheck = CreateWindowExW(0, L"BUTTON", L"Unknown", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, severityY + 32, 130, 24, parent, ControlId(IDC_INCIDENT_FILTERS_UNKNOWN_CHECK), m_hInst, nullptr);

        CreateAutoLabel(parent, IDC_INCIDENT_FILTERS_TYPE_LABEL, L"Incident type", 18, severityY + 78);
        const int typeY = severityY + 106;
        m_incidentUnplannedCheck = CreateWindowExW(0, L"BUTTON", L"Unplanned incidents", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, typeY, 180, 24, parent, ControlId(IDC_INCIDENT_FILTERS_UNPLANNED_CHECK), m_hInst, nullptr);
        m_incidentPlannedCheck = CreateWindowExW(0, L"BUTTON", L"Planned roadworks", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 218, typeY, 170, 24, parent, ControlId(IDC_INCIDENT_FILTERS_PLANNED_CHECK), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 280, 102, 32, parent, ControlId(IDC_INCIDENT_FILTERS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentSevereCheck, m_incidentModerateCheck, m_incidentMinorCheck, m_incidentUnknownCheck, m_incidentUnplannedCheck, m_incidentPlannedCheck, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SizeControlToText(m_incidentSevereCheck, 34, 6, 120, 0, 24);
        SizeControlToText(m_incidentModerateCheck, 34, 6, 130, 0, 24);
        SizeControlToText(m_incidentMinorCheck, 34, 6, 110, 0, 24);
        SizeControlToText(m_incidentUnknownCheck, 34, 6, 130, 0, 24);
        SizeControlToText(m_incidentUnplannedCheck, 34, 6, 180, 0, 24);
        SizeControlToText(m_incidentPlannedCheck, 34, 6, 170, 0, 24);
        SyncIncidentFilterControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncIncidentFilterControls()
    {
        m_syncingControls = true;
        if (m_incidentSevereCheck)
            SendMessageW(m_incidentSevereCheck, BM_SETCHECK, m_incidentFilterSevere ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentModerateCheck)
            SendMessageW(m_incidentModerateCheck, BM_SETCHECK, m_incidentFilterModerate ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentMinorCheck)
            SendMessageW(m_incidentMinorCheck, BM_SETCHECK, m_incidentFilterMinor ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentUnknownCheck)
            SendMessageW(m_incidentUnknownCheck, BM_SETCHECK, m_incidentFilterUnknown ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentUnplannedCheck)
            SendMessageW(m_incidentUnplannedCheck, BM_SETCHECK, m_incidentFilterUnplanned ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentPlannedCheck)
            SendMessageW(m_incidentPlannedCheck, BM_SETCHECK, m_incidentFilterPlanned ? BST_CHECKED : BST_UNCHECKED, 0);
        m_syncingControls = false;
    }

    void OnIncidentFiltersCommand(int id, int code)
    {
        if (m_syncingControls)
            return;

        if (code == BN_CLICKED) {
            if (id == IDC_INCIDENT_FILTERS_CLOSE_BTN) {
                ShowWindow(m_incidentFiltersWnd, SW_HIDE);
                return;
            }

            if (id == IDC_INCIDENT_FILTERS_SEVERE_CHECK)
                m_incidentFilterSevere = SendMessageW(m_incidentSevereCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            else if (id == IDC_INCIDENT_FILTERS_MODERATE_CHECK)
                m_incidentFilterModerate = SendMessageW(m_incidentModerateCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            else if (id == IDC_INCIDENT_FILTERS_MINOR_CHECK)
                m_incidentFilterMinor = SendMessageW(m_incidentMinorCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            else if (id == IDC_INCIDENT_FILTERS_UNKNOWN_CHECK)
                m_incidentFilterUnknown = SendMessageW(m_incidentUnknownCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            else if (id == IDC_INCIDENT_FILTERS_UNPLANNED_CHECK)
                m_incidentFilterUnplanned = SendMessageW(m_incidentUnplannedCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            else if (id == IDC_INCIDENT_FILTERS_PLANNED_CHECK)
                m_incidentFilterPlanned = SendMessageW(m_incidentPlannedCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;

            SetStatusText(L"Incident filter selections updated.");
            SaveSettings();
        }
    }

    static LRESULT CALLBACK IncidentNotificationsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleIncidentNotificationsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleIncidentNotificationsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateIncidentNotificationsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnIncidentNotificationsCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowIncidentNotificationsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = IncidentNotificationsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kIncidentNotificationsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_incidentNotificationsWnd || !IsWindow(m_incidentNotificationsWnd)) {
            m_incidentNotificationsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kIncidentNotificationsClassName,
                L"Incident Notifications",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                540,
                660,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        SyncIncidentNotificationControls();
        ShowWindow(m_incidentNotificationsWnd, SW_SHOW);
        SetForegroundWindow(m_incidentNotificationsWnd);
    }

    void CreateIncidentNotificationsControls(HWND parent)
    {
        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_TITLE_LABEL, L"Incident Notifications", 18, 18, m_headerFont);
        HWND descLabel = CreateAutoLabel(
            parent,
            IDC_INCIDENT_NOTIFICATIONS_DESC_LABEL,
            L"Define which incidents should trigger a notification. Roads and exclusions are comma separated; lane threshold accepts percentages such as 50%.",
            18,
            54,
            nullptr,
            476);
        const int descH = AutoLabelHeight(descLabel, 44, 476);

        const int left = 18;
        const int editX = 18;
        const int editW = 476;
        int y = 54 + descH + 18;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_ROADS_LABEL, L"Roads to notify on", left, y);
        y += 26;
        m_incidentNotifyRoadsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, editW, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_ROADS_EDIT), m_hInst, nullptr);
        y += 42;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_ROAD_EXCLUSIONS_LABEL, L"Roads to exclude", left, y);
        y += 26;
        m_incidentNotifyRoadExclusionsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, editW, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_ROAD_EXCLUSIONS_EDIT), m_hInst, nullptr);
        y += 42;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_LANES_LABEL, L"Closed-lane threshold", left, y);
        y += 26;
        m_incidentNotifyLaneThresholdEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, 120, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_LANES_EDIT), m_hInst, nullptr);
        y += 42;

        m_incidentNotifyAndRadio = CreateWindowExW(0, L"BUTTON", L"AND", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, editX, y, 72, 24, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_AND_RADIO), m_hInst, nullptr);
        m_incidentNotifyOrRadio = CreateWindowExW(0, L"BUTTON", L"OR", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, editX + 84, y, 72, 24, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_OR_RADIO), m_hInst, nullptr);
        y += 36;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_DELAY_LABEL, L"Delay threshold", left, y);
        y += 26;
        m_incidentNotifyDelayThresholdEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, 120, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_DELAY_EDIT), m_hInst, nullptr);
        y += 42;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_REGIONS_LABEL, L"Notification regions", left, y);
        y += 26;
        m_incidentNotifyRegionsBtn = CreateWindowExW(0, L"BUTTON", L"Manage regions...", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, editX, y, 168, 32, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_REGIONS_BTN), m_hInst, nullptr);
        y += 48;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_LABEL, L"Reason exclusions", left, y);
        y += 26;
        m_incidentNotifyReasonExclusionsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, editW, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_EDIT), m_hInst, nullptr);
        y += 42;

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_LOCATION_EXCLUSIONS_LABEL, L"Location exclusions", left, y);
        y += 26;
        m_incidentNotifyLocationExclusionsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, editX, y, editW, 26, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_LOCATION_EXCLUSIONS_EDIT), m_hInst, nullptr);

        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 392, y + 42, 102, 32, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentNotifyRoadsEdit, m_incidentNotifyRoadExclusionsEdit, m_incidentNotifyLaneThresholdEdit, m_incidentNotifyAndRadio, m_incidentNotifyOrRadio, m_incidentNotifyDelayThresholdEdit, m_incidentNotifyRegionsBtn, m_incidentNotifyReasonExclusionsEdit, m_incidentNotifyLocationExclusionsEdit, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SendMessageW(m_incidentNotifyRoadsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M*, A*, A1(M), A2, A15, A16, A17, A20, A4, A52"));
        SendMessageW(m_incidentNotifyRoadExclusionsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M25, A1(M)"));
        SendMessageW(m_incidentNotifyLaneThresholdEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"50%"));
        SendMessageW(m_incidentNotifyDelayThresholdEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1 hour"));
        SendMessageW(m_incidentNotifyReasonExclusionsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Road Management"));
        SendMessageW(m_incidentNotifyLocationExclusionsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"entry, exit"));
        SyncIncidentNotificationControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncIncidentNotificationControls()
    {
        m_syncingControls = true;
        if (m_incidentNotifyRoadsEdit)
            SetWindowTextSafe(m_incidentNotifyRoadsEdit, m_incidentNotifyRoads);
        if (m_incidentNotifyRoadExclusionsEdit)
            SetWindowTextSafe(m_incidentNotifyRoadExclusionsEdit, m_incidentNotifyRoadExclusions);
        if (m_incidentNotifyLaneThresholdEdit)
            SetWindowTextSafe(m_incidentNotifyLaneThresholdEdit, m_incidentNotifyLaneThresholdText);
        if (m_incidentNotifyDelayThresholdEdit)
            SetWindowTextSafe(m_incidentNotifyDelayThresholdEdit, m_incidentNotifyDelayThresholdText);
        if (m_incidentNotifyAndRadio)
            SendMessageW(m_incidentNotifyAndRadio, BM_SETCHECK, m_incidentNotifyThresholdUseOr ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_incidentNotifyOrRadio)
            SendMessageW(m_incidentNotifyOrRadio, BM_SETCHECK, m_incidentNotifyThresholdUseOr ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentNotifyReasonExclusionsEdit)
            SetWindowTextSafe(m_incidentNotifyReasonExclusionsEdit, m_incidentNotifyReasonExclusions);
        if (m_incidentNotifyLocationExclusionsEdit)
            SetWindowTextSafe(m_incidentNotifyLocationExclusionsEdit, m_incidentNotifyLocationExclusions);
        m_syncingControls = false;
    }

    void OnIncidentNotificationsCommand(int id, int code)
    {
        if (m_syncingControls)
            return;

        if (id == IDC_INCIDENT_NOTIFICATIONS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_incidentNotificationsWnd, SW_HIDE);
            return;
        }
        if (id == IDC_INCIDENT_NOTIFICATIONS_REGIONS_BTN && code == BN_CLICKED) {
            ShowNotificationRegionsWindow();
            return;
        }
        if ((id == IDC_INCIDENT_NOTIFICATIONS_AND_RADIO || id == IDC_INCIDENT_NOTIFICATIONS_OR_RADIO) && code == BN_CLICKED) {
            m_incidentNotifyThresholdUseOr = (id == IDC_INCIDENT_NOTIFICATIONS_OR_RADIO);
            SaveSettings();
            return;
        }

        if (code != EN_CHANGE && code != EN_KILLFOCUS)
            return;

        if (id == IDC_INCIDENT_NOTIFICATIONS_ROADS_EDIT) {
            m_incidentNotifyRoads = GetWindowTextString(m_incidentNotifyRoadsEdit);
            SaveSettings();
        }
        else if (id == IDC_INCIDENT_NOTIFICATIONS_ROAD_EXCLUSIONS_EDIT) {
            m_incidentNotifyRoadExclusions = GetWindowTextString(m_incidentNotifyRoadExclusionsEdit);
            SaveSettings();
        }
        else if (id == IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_EDIT) {
            m_incidentNotifyReasonExclusions = GetWindowTextString(m_incidentNotifyReasonExclusionsEdit);
            SaveSettings();
        }
        else if (id == IDC_INCIDENT_NOTIFICATIONS_LOCATION_EXCLUSIONS_EDIT) {
            m_incidentNotifyLocationExclusions = GetWindowTextString(m_incidentNotifyLocationExclusionsEdit);
            SaveSettings();
        }
        else if (id == IDC_INCIDENT_NOTIFICATIONS_LANES_EDIT) {
            std::wstring thresholdText = Trim(GetWindowTextString(m_incidentNotifyLaneThresholdEdit));
            double parsed = 0.0;
            if (TryParsePercentThreshold(thresholdText, parsed)) {
                m_incidentNotifyLaneThresholdText = thresholdText;
                m_incidentNotifyLaneThreshold = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_incidentNotifyLaneThresholdEdit, m_incidentNotifyLaneThresholdText);
                SetStatusText(L"Lane threshold should be a percentage such as 50%.");
            }
        }
        else if (id == IDC_INCIDENT_NOTIFICATIONS_DELAY_EDIT) {
            std::wstring thresholdText = Trim(GetWindowTextString(m_incidentNotifyDelayThresholdEdit));
            double parsed = 0.0;
            if (TryParseDurationMinutes(thresholdText, parsed)) {
                m_incidentNotifyDelayThresholdText = thresholdText;
                m_incidentNotifyDelayThresholdMinutes = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_incidentNotifyDelayThresholdEdit, m_incidentNotifyDelayThresholdText);
                SetStatusText(L"Delay threshold should be a duration such as 1 hour or 60 minutes.");
            }
        }
    }

    static LRESULT CALLBACK NotificationRegionsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleNotificationRegionsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleNotificationRegionsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateNotificationRegionsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnNotificationRegionsCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowNotificationRegionsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = NotificationRegionsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kNotificationRegionsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_notificationRegionsWnd || !IsWindow(m_notificationRegionsWnd)) {
            m_notificationRegionsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kNotificationRegionsClassName,
                L"Incident Notification Regions",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                430,
                360,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        SyncNotificationRegionsList();
        ShowWindow(m_notificationRegionsWnd, SW_SHOW);
        SetForegroundWindow(m_notificationRegionsWnd);
    }

    void CreateNotificationRegionsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Notification regions", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Each region can notify for every road inside it, or only the roads listed for that region.", 18, 54, nullptr, 364);
        m_notificationRegionsList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 100, 376, 150, parent, ControlId(IDC_NOTIFICATION_REGIONS_LIST), m_hInst, nullptr);
        HWND addBtn = CreateWindowExW(0, L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 268, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_NEW_BTN), m_hInst, nullptr);
        HWND editBtn = CreateWindowExW(0, L"BUTTON", L"Edit", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 108, 268, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_EDIT_BTN), m_hInst, nullptr);
        HWND deleteBtn = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 198, 268, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_DELETE_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 292, 268, 102, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_notificationRegionsList, addBtn, editBtn, deleteBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SyncNotificationRegionsList();
        AutoFitWindowToChildren(parent);
    }

    void SyncNotificationRegionsList()
    {
        if (!m_notificationRegionsList)
            return;

        int previous = static_cast<int>(SendMessageW(m_notificationRegionsList, LB_GETCURSEL, 0, 0));
        SendMessageW(m_notificationRegionsList, LB_RESETCONTENT, 0, 0);
        for (size_t i = 0; i < m_incidentNotificationRegions.size(); ++i) {
            const GeoPolygon& polygon = m_incidentNotificationRegions[i];
            std::wstring line = polygon.name.empty() ? (L"Region " + std::to_wstring(i + 1)) : polygon.name;
            line += L" - ";
            line += std::to_wstring(polygon.points.size());
            line += L" point(s)";
            if (!polygon.allRoads && !polygon.roadFilter.empty()) {
                line += L" - ";
                line += polygon.roadFilter;
            }
            SendMessageW(m_notificationRegionsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
        if (!m_incidentNotificationRegions.empty()) {
            previous = ClampValue(previous, 0, static_cast<int>(m_incidentNotificationRegions.size()) - 1);
            SendMessageW(m_notificationRegionsList, LB_SETCURSEL, previous, 0);
        }
    }

    int SelectedNotificationRegionIndex() const
    {
        if (!m_notificationRegionsList)
            return -1;
        int index = static_cast<int>(SendMessageW(m_notificationRegionsList, LB_GETCURSEL, 0, 0));
        if (index < 0 || index >= static_cast<int>(m_incidentNotificationRegions.size()))
            return -1;
        return index;
    }

    void OnNotificationRegionsCommand(int id, int code)
    {
        if (id == IDC_NOTIFICATION_REGIONS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_notificationRegionsWnd, SW_HIDE);
            return;
        }
        if (id == IDC_NOTIFICATION_REGIONS_LIST && code == LBN_DBLCLK) {
            int index = SelectedNotificationRegionIndex();
            if (index >= 0)
                OpenNotificationRegionEditor(static_cast<size_t>(index));
            return;
        }
        if (code != BN_CLICKED)
            return;

        if (id == IDC_NOTIFICATION_REGIONS_NEW_BTN) {
            GeoPolygon polygon;
            polygon.name = L"Region " + std::to_wstring(m_incidentNotificationRegions.size() + 1);
            polygon.allRoads = true;
            m_incidentNotificationRegions.push_back(std::move(polygon));
            m_map.SetNotificationPolygons(m_incidentNotificationRegions);
            SyncNotificationRegionsList();
            SaveSettings();
            OpenNotificationRegionEditor(m_incidentNotificationRegions.size() - 1);
        }
        else if (id == IDC_NOTIFICATION_REGIONS_EDIT_BTN) {
            int index = SelectedNotificationRegionIndex();
            if (index >= 0)
                OpenNotificationRegionEditor(static_cast<size_t>(index));
        }
        else if (id == IDC_NOTIFICATION_REGIONS_DELETE_BTN) {
            int index = SelectedNotificationRegionIndex();
            if (index >= 0) {
                m_incidentNotificationRegions.erase(m_incidentNotificationRegions.begin() + index);
                m_map.SetNotificationPolygons(m_incidentNotificationRegions);
                if (m_polygonCaptureTarget == PolygonCaptureTarget::IncidentRegion &&
                    m_activeIncidentRegionIndex == static_cast<size_t>(index))
                {
                    StopPolygonCapture();
                }
                SyncNotificationRegionsList();
                SaveSettings();
            }
        }
    }

    static LRESULT CALLBACK NotificationRegionEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        NotificationRegionEditorContext* ctx = reinterpret_cast<NotificationRegionEditorContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ctx = reinterpret_cast<NotificationRegionEditorContext*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
        }

        if (ctx && ctx->owner)
            return ctx->owner->HandleNotificationRegionEditorMessage(ctx, hwnd, msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleNotificationRegionEditorMessage(NotificationRegionEditorContext* ctx, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateNotificationRegionEditorControls(hwnd, ctx->index);
            return 0;
        case WM_COMMAND:
            OnNotificationRegionEditorCommand(ctx, hwnd, LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_NCDESTROY:
            delete ctx;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void OpenNotificationRegionEditor(size_t index)
    {
        if (index >= m_incidentNotificationRegions.size())
            return;

        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = NotificationRegionEditorWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kNotificationRegionEditorClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        auto* ctx = new NotificationRegionEditorContext{};
        ctx->owner = this;
        ctx->index = index;
        HWND hwnd = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            kNotificationRegionEditorClassName,
            L"Notification Region",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            430,
            410,
            m_hwnd,
            nullptr,
            m_hInst,
            ctx);
        if (hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        else {
            delete ctx;
        }
    }

    void CreateNotificationRegionEditorControls(HWND parent, size_t index)
    {
        const GeoPolygon& polygon = m_incidentNotificationRegions[index];
        CreateAutoLabel(parent, 0, L"Notification region", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Name", 18, 58);
        HWND nameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", polygon.name.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 84, 376, 26, parent, ControlId(IDC_NOTIFICATION_REGION_NAME_EDIT), m_hInst, nullptr);
        HWND allRoads = CreateWindowExW(0, L"BUTTON", L"All roads inside polygon", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, 122, 220, 24, parent, ControlId(IDC_NOTIFICATION_REGION_ALL_ROADS_CHECK), m_hInst, nullptr);
        SendMessageW(allRoads, BM_SETCHECK, polygon.allRoads ? BST_CHECKED : BST_UNCHECKED, 0);
        CreateAutoLabel(parent, 0, L"Specific roads", 18, 156);
        HWND roadsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", polygon.roadFilter.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 182, 376, 26, parent, ControlId(IDC_NOTIFICATION_REGION_ROADS_EDIT), m_hInst, nullptr);
        HWND pointsLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 18, 220, 376, 24, parent, ControlId(IDC_NOTIFICATION_REGION_POINTS_LABEL), m_hInst, nullptr);
        HWND drawBtn = CreateWindowExW(0, L"BUTTON", L"Edit on map", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 254, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_DRAW_BTN), m_hInst, nullptr);
        HWND undoBtn = CreateWindowExW(0, L"BUTTON", L"Undo point", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 154, 254, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_UNDO_BTN), m_hInst, nullptr);
        HWND clearBtn = CreateWindowExW(0, L"BUTTON", L"Clear points", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 296, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_CLEAR_BTN), m_hInst, nullptr);
        HWND finishBtn = CreateWindowExW(0, L"BUTTON", L"Finish edit", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 154, 296, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_FINISH_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 292, 296, 102, 32, parent, ControlId(IDC_NOTIFICATION_REGION_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { nameEdit, allRoads, roadsEdit, pointsLabel, drawBtn, undoBtn, clearBtn, finishBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(roadsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M*, A*, A1(M), A2, A52"));
        UpdateNotificationRegionEditorPointsLabel(parent, index);
        AutoFitWindowToChildren(parent);
    }

    void UpdateNotificationRegionEditorPointsLabel(HWND hwnd, size_t index)
    {
        if (index >= m_incidentNotificationRegions.size())
            return;
        HWND label = GetDlgItem(hwnd, IDC_NOTIFICATION_REGION_POINTS_LABEL);
        if (!label)
            return;
        std::wstring text = L"Polygon points: " + std::to_wstring(m_incidentNotificationRegions[index].points.size());
        SetWindowTextSafe(label, text);
    }

    void OnNotificationRegionEditorCommand(NotificationRegionEditorContext* ctx, HWND hwnd, int id, int code)
    {
        if (!ctx || ctx->index >= m_incidentNotificationRegions.size())
            return;

        GeoPolygon& polygon = m_incidentNotificationRegions[ctx->index];
        if (id == IDC_NOTIFICATION_REGION_CLOSE_BTN && code == BN_CLICKED) {
            DestroyWindow(hwnd);
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_DRAW_BTN && code == BN_CLICKED) {
            m_polygonCaptureTarget = PolygonCaptureTarget::IncidentRegion;
            m_activeIncidentRegionIndex = ctx->index;
            m_map.SetPolygonCaptureActive(true);
            m_map.SetActiveNotificationPolygonIndex(ctx->index);
            m_map.SetDraftPolygon({});
            SetStatusText(L"Editing polygon for " + (polygon.name.empty() ? L"this region" : polygon.name) + L". Click to add points, drag points to move them, right-click a point to delete it.");
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_UNDO_BTN && code == BN_CLICKED) {
            if (!polygon.points.empty()) {
                polygon.points.pop_back();
                m_map.SetNotificationPolygons(m_incidentNotificationRegions);
                UpdateNotificationRegionEditorPointsLabel(hwnd, ctx->index);
                SyncNotificationRegionsList();
                SaveSettings();
                SetStatusText(L"Removed last polygon point.");
            }
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_FINISH_BTN && code == BN_CLICKED) {
            if (m_polygonCaptureTarget == PolygonCaptureTarget::IncidentRegion &&
                m_activeIncidentRegionIndex == ctx->index)
            {
                StopPolygonCapture();
                SetStatusText(L"Finished editing notification region polygon.");
            }
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_CLEAR_BTN && code == BN_CLICKED) {
            polygon.points.clear();
            m_map.SetNotificationPolygons(m_incidentNotificationRegions);
            UpdateNotificationRegionEditorPointsLabel(hwnd, ctx->index);
            SyncNotificationRegionsList();
            SaveSettings();
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_ALL_ROADS_CHECK && code == BN_CLICKED) {
            polygon.allRoads = SendMessageW(GetDlgItem(hwnd, IDC_NOTIFICATION_REGION_ALL_ROADS_CHECK), BM_GETCHECK, 0, 0) == BST_CHECKED;
            SyncNotificationRegionsList();
            SaveSettings();
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_NAME_EDIT && code == EN_CHANGE) {
            polygon.name = Trim(GetWindowTextString(GetDlgItem(hwnd, IDC_NOTIFICATION_REGION_NAME_EDIT)));
            SyncNotificationRegionsList();
            SaveSettings();
            return;
        }
        if (id == IDC_NOTIFICATION_REGION_ROADS_EDIT && code == EN_CHANGE) {
            polygon.roadFilter = GetWindowTextString(GetDlgItem(hwnd, IDC_NOTIFICATION_REGION_ROADS_EDIT));
            SyncNotificationRegionsList();
            SaveSettings();
            return;
        }
    }

    void StopPolygonCapture()
    {
        m_polygonCaptureTarget = PolygonCaptureTarget::None;
        m_activeIncidentRegionIndex = static_cast<size_t>(-1);
        m_map.SetPolygonCaptureActive(false);
        m_map.SetActiveNotificationPolygonIndex(static_cast<size_t>(-1));
        m_map.SetDraftPolygon({});
    }

    void OnMapPolygonPoint(double lat, double lon)
    {
        if (m_polygonCaptureTarget == PolygonCaptureTarget::IncidentRegion) {
            if (m_activeIncidentRegionIndex >= m_incidentNotificationRegions.size()) {
                StopPolygonCapture();
                return;
            }
            GeoPolygon& polygon = m_incidentNotificationRegions[m_activeIncidentRegionIndex];
            polygon.points.push_back({ lat, lon });
            m_map.SetNotificationPolygons(m_incidentNotificationRegions);
            SyncNotificationRegionsList();
            SaveSettings();
            SetStatusText(L"Added polygon point " + std::to_wstring(polygon.points.size()) + L" to " + (polygon.name.empty() ? L"region" : polygon.name) + L".");
            return;
        }

        if (m_polygonCaptureTarget == PolygonCaptureTarget::EarthquakeRegion) {
            m_earthquakeFilterRegion.push_back({ lat, lon });
            m_map.SetDraftPolygon(m_earthquakeFilterRegion);
            ApplyEarthquakeListFilters();
            SetStatusText(L"Added earthquake region point " + std::to_wstring(m_earthquakeFilterRegion.size()) + L".");
        }
    }

    void OnMapPolygonPointMoved(size_t polygonIndex, size_t pointIndex, double lat, double lon)
    {
        if (polygonIndex >= m_incidentNotificationRegions.size() ||
            pointIndex >= m_incidentNotificationRegions[polygonIndex].points.size())
            return;

        m_incidentNotificationRegions[polygonIndex].points[pointIndex] = { lat, lon };
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        SyncNotificationRegionsList();
        SaveSettings();
    }

    void OnMapPolygonPointDeleted(size_t polygonIndex, size_t pointIndex)
    {
        if (polygonIndex >= m_incidentNotificationRegions.size() ||
            pointIndex >= m_incidentNotificationRegions[polygonIndex].points.size())
            return;

        m_incidentNotificationRegions[polygonIndex].points.erase(m_incidentNotificationRegions[polygonIndex].points.begin() + pointIndex);
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        SyncNotificationRegionsList();
        SaveSettings();
        SetStatusText(L"Removed polygon point.");
    }

    void OnMapPolygonCleared(size_t polygonIndex)
    {
        if (polygonIndex >= m_incidentNotificationRegions.size())
            return;

        m_incidentNotificationRegions[polygonIndex].points.clear();
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        SyncNotificationRegionsList();
        SaveSettings();
        SetStatusText(L"Cleared notification polygon.");
    }

    const TrafficAlert* FindSelectedAlert() const
    {
        if (!m_selectedId.empty()) {
            for (const TrafficAlert& alert : m_allAlerts) {
                if (alert.id == m_selectedId)
                    return &alert;
            }
            for (const TrafficAlert& alert : m_filteredAlerts) {
                if (alert.id == m_selectedId)
                    return &alert;
            }
        }

        if (m_listView) {
            int selected = static_cast<int>(SendMessageW(m_listView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredAlerts.size()))
                return &m_filteredAlerts[static_cast<size_t>(selected)];
        }

        return nullptr;
    }

    const TrafficAlert* FindTemplateWizardAlert() const
    {
        if (!m_templateWizardAlertId.empty()) {
            for (const TrafficAlert& alert : m_allAlerts) {
                if (alert.id == m_templateWizardAlertId)
                    return &alert;
            }
        }
        return FindSelectedAlert();
    }

    const EarthquakeEvent* FindSelectedEarthquake() const
    {
        if (m_earthquakeListView) {
            int selected = static_cast<int>(SendMessageW(m_earthquakeListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredEarthquakes.size()))
                return &m_filteredEarthquakes[static_cast<size_t>(selected)];
        }

        if (!m_filteredEarthquakes.empty())
            return &m_filteredEarthquakes.front();
        if (!m_allEarthquakes.empty())
            return &m_allEarthquakes.front();
        return nullptr;
    }

    const WeatherSystemEvent* FindSelectedWeatherSystem() const
    {
        if (m_weatherSystemsListView) {
            int selected = static_cast<int>(SendMessageW(m_weatherSystemsListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredWeatherSystems.size()))
                return &m_filteredWeatherSystems[static_cast<size_t>(selected)];
        }

        if (!m_filteredWeatherSystems.empty())
            return &m_filteredWeatherSystems.front();
        if (!m_allWeatherSystems.empty())
            return &m_allWeatherSystems.front();
        return nullptr;
    }

    static std::wstring CurrentDateText()
    {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        wchar_t buffer[32]{};
        if (std::wcsftime(buffer, _countof(buffer), L"%d/%m/%Y", &local) == 0)
            return L"";
        return buffer;
    }

    static std::wstring RoadSlugForRoadsOrg(std::wstring road)
    {
        std::wstring slug;
        for (wchar_t ch : ToLower(road)) {
            if (iswalnum(ch))
                slug.push_back(ch);
        }
        return slug;
    }

    static std::wstring JoinTemplateItems(const std::vector<std::wstring>& items, const std::wstring& separator = L", ")
    {
        std::wstring text;
        for (const std::wstring& item : items) {
            if (item.empty())
                continue;
            if (!text.empty())
                text += separator;
            text += item;
        }
        return text;
    }

    static std::wstring JoinTemplateItemsAsPhrase(const std::vector<std::wstring>& items)
    {
        if (items.empty())
            return L"";
        if (items.size() == 1)
            return items.front();
        if (items.size() == 2)
            return items[0] + L" and " + items[1];

        std::wstring text;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0)
                text += (i + 1 == items.size()) ? L" and " : L", ";
            text += items[i];
        }
        return text;
    }

    static bool PushUniqueText(std::vector<std::wstring>& values, const std::wstring& value)
    {
        std::wstring trimmed = Trim(value);
        if (trimmed.empty())
            return false;

        std::wstring lower = ToLower(trimmed);
        for (const std::wstring& existing : values) {
            if (ToLower(existing) == lower)
                return false;
        }

        values.push_back(std::move(trimmed));
        return true;
    }

    static std::wstring CompactTemplateWhitespace(const std::wstring& text)
    {
        std::wstring compact;
        compact.reserve(text.size());
        bool lastSpace = false;
        for (wchar_t ch : text) {
            bool isSpace = iswspace(ch) != 0;
            if (isSpace) {
                if (!lastSpace)
                    compact.push_back(L' ');
                lastSpace = true;
            }
            else {
                compact.push_back(ch);
                lastSpace = false;
            }
        }
        std::wstring value = Trim(compact);
        std::wstring normalized;
        normalized.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == L' ') {
                wchar_t previous = normalized.empty() ? L'\0' : normalized.back();
                wchar_t next = (i + 1 < value.size()) ? value[i + 1] : L'\0';
                if (previous == L'(' || next == L')' || next == L',' || next == L';')
                    continue;
            }
            normalized.push_back(value[i]);
        }
        return Trim(normalized);
    }

    static std::vector<std::wstring> ExtractRoadRefsFromText(const std::wstring& text, const std::wstring& currentRoad)
    {
        std::vector<std::wstring> refs;
        std::wstring current = ToLower(Trim(currentRoad));
        std::wregex roadRefRe(LR"(\b([AMB]\d{1,4}[A-Z]?(?:\([A-Z]+\))?)\b)", std::regex_constants::icase);
        for (std::wsregex_iterator it(text.begin(), text.end(), roadRefRe), end; it != end; ++it) {
            std::wstring ref = Trim((*it)[1].str());
            if (!current.empty() && ToLower(ref) == current)
                continue;
            PushUniqueText(refs, ref);
        }
        return refs;
    }

    static bool TrySplitLeadingBracketedRoadSignItem(const std::wstring& item, std::wstring& bracketedRoadOut, std::wstring& remainderOut)
    {
        static const std::wregex bracketedRoadRe(LR"(^\s*(\([AMB]\d{1,4}[A-Z]?(?:\([A-Z]+\))?\))\s*,?\s*(.*?)\s*$)", std::regex_constants::icase);
        std::wsmatch m;
        if (!std::regex_match(item, m, bracketedRoadRe) || m.size() < 3)
            return false;

        bracketedRoadOut = CompactTemplateWhitespace(m[1].str());
        remainderOut = CompactTemplateWhitespace(m[2].str());
        return !bracketedRoadOut.empty();
    }

    static bool IsBracketedRoadSignItem(const std::wstring& item)
    {
        std::wstring bracketedRoad;
        std::wstring remainder;
        return TrySplitLeadingBracketedRoadSignItem(item, bracketedRoad, remainder) && remainder.empty();
    }

    static std::vector<std::wstring> MergeBracketedRoadSignItems(const std::vector<std::wstring>& items)
    {
        std::vector<std::wstring> merged;
        for (const std::wstring& item : items) {
            std::wstring bracketedRoad;
            std::wstring remainder;
            if (TrySplitLeadingBracketedRoadSignItem(item, bracketedRoad, remainder)) {
                if (!merged.empty())
                    merged.back() = CompactTemplateWhitespace(merged.back() + L" " + bracketedRoad);
                else
                    PushUniqueText(merged, bracketedRoad);

                if (!remainder.empty())
                    PushUniqueText(merged, remainder);
                continue;
            }

            PushUniqueText(merged, item);
        }
        return merged;
    }

    static std::vector<std::wstring> ExtractRoadsOrgSignPanelItems(const std::wstring& fragment, const std::wstring& currentRoad)
    {
        std::wstring text = fragment;
        text = std::regex_replace(text, std::wregex(LR"(<\s*span\b[^>]*\bclass\s*=\s*(['"])[^'"]*\bsr-only\b[^'"]*\1[^>]*>[\s\S]*?</\s*span\s*>)", std::regex_constants::icase), L" ");
        text = std::regex_replace(text, std::wregex(LR"(<\s*i\b[^>]*>[\s\S]*?</\s*i\s*>)", std::regex_constants::icase), L" ");
        text = std::regex_replace(text, std::wregex(LR"(<\s*img\b[^>]*>)", std::regex_constants::icase), L" ");
        text = std::regex_replace(text, std::wregex(LR"(<\s*hr\b[^>]*>)", std::regex_constants::icase), L"\n");
        text = std::regex_replace(text, std::wregex(LR"(<\s*br\b[^>]*>)", std::regex_constants::icase), L"\n");
        text = std::regex_replace(text, std::wregex(LR"(<\s*/\s*(?:div|p|li)\s*>)", std::regex_constants::icase), L"\n");
        text = std::regex_replace(text, std::wregex(LR"(<[^>]+>)"), L" ");
        text = DecodeBasicHtmlEntities(text);

        std::vector<std::wstring> items;
        size_t pos = 0;
        while (pos <= text.size()) {
            size_t next = text.find_first_of(L"\r\n", pos);
            std::wstring line = CompactTemplateWhitespace(text.substr(pos, next == std::wstring::npos ? std::wstring::npos : next - pos));
            if (!line.empty()) {
                std::wstring lower = ToLower(line);
                if (lower != L"link" && lower != L"details" && lower != L"lanes" && lower != L"signs" &&
                    lower != L"image" && lower.find(L"image:") != 0)
                {
                    if (IsBracketedRoadSignItem(line) && !items.empty()) {
                        std::wstring previous = items.back();
                        items.pop_back();
                        PushUniqueText(items, previous + L" " + line);
                    }
                    else {
                        PushUniqueText(items, line);
                    }
                }
            }
            if (next == std::wstring::npos)
                break;
            pos = next + 1;
            while (pos < text.size() && (text[pos] == L'\r' || text[pos] == L'\n'))
                ++pos;
        }

        std::wstring current = ToLower(Trim(currentRoad));
        if (!current.empty()) {
            items.erase(
                std::remove_if(items.begin(), items.end(), [&](const std::wstring& item) {
                    return ToLower(item) == current;
                    }),
                items.end());
        }
        return MergeBracketedRoadSignItems(items);
    }

    static bool TryStandaloneSignRoad(const std::wstring& item, std::wstring& roadOut)
    {
        std::wstring value = CompactTemplateWhitespace(item);
        std::wregex roadRe(LR"(^\s*[AMB]\d{1,4}[A-Z]?(?:\([A-Z]+\))?(?:\s+\([AMB]\d{1,4}[A-Z]?(?:\([A-Z]+\))?\))*\s*$)", std::regex_constants::icase);
        if (!std::regex_match(value, roadRe))
            return false;

        roadOut = std::move(value);
        return !roadOut.empty();
    }

    static std::wstring FormatRoadsOrgSignGroup(const std::wstring& road, const std::vector<std::wstring>& destinations)
    {
        std::wstring body = JoinTemplateItems(destinations);
        if (road.empty())
            return body;
        if (body.empty())
            return road;
        return road + L" " + body;
    }

    static std::wstring FormatRoadsOrgSignPanelItems(const std::vector<std::wstring>& items)
    {
        std::vector<std::wstring> normalizedItems = MergeBracketedRoadSignItems(items);
        std::vector<std::wstring> groups;
        std::vector<std::wstring> pendingDestinations;
        std::wstring activeRoad;

        auto flush = [&](const std::wstring& road) {
            if (pendingDestinations.empty())
                return;
            PushUniqueText(groups, FormatRoadsOrgSignGroup(road, pendingDestinations));
            pendingDestinations.clear();
            };

        for (const std::wstring& item : normalizedItems) {
            std::wstring road;
            if (TryStandaloneSignRoad(item, road)) {
                if (pendingDestinations.empty()) {
                    activeRoad = road;
                }
                else if (activeRoad.empty()) {
                    flush(road);
                }
                else {
                    flush(activeRoad);
                    activeRoad = road;
                }
            }
            else {
                pendingDestinations.push_back(item);
            }
        }

        flush(activeRoad);
        return JoinTemplateItems(groups, L" - ");
    }

    static std::wstring ExtractRoadsOrgDataFromFragment(const std::wstring& fragment, const std::wstring& currentRoad)
    {
        std::vector<std::wstring> signPanelItems = ExtractRoadsOrgSignPanelItems(fragment, currentRoad);
        if (!signPanelItems.empty())
            return FormatRoadsOrgSignPanelItems(signPanelItems);

        std::wstring text = StripTemplateHtmlTags(fragment);
        std::vector<std::wstring> refs = ExtractRoadRefsFromText(text, currentRoad);
        if (!refs.empty())
            return JoinTemplateItems(refs);

        std::wregex titleRe(LR"(title\s*=\s*(['"])(.*?)\1)", std::regex_constants::icase);
        std::wsmatch m;
        if (std::regex_search(fragment, m, titleRe) && m.size() > 2) {
            std::wstring title = DecodeBasicHtmlEntities(m[2].str());
            size_t paren = title.find(L'(');
            if (paren != std::wstring::npos)
                title = Trim(title.substr(0, paren));
            return title;
        }

        return L"";
    }

    static bool TryFetchRoadsOrgPage(const std::wstring& slug, std::wstring& htmlOut)
    {
        std::vector<std::wstring> urls = {
            L"https://www.roads.org.uk/motorway/" + slug + L"/",
            L"https://www.roads.org.uk/motorway/" + slug,
            L"https://www.roads.org.uk/" + slug
        };

        for (const std::wstring& url : urls) {
            std::string body;
            std::wstring error;
            if (HttpGetText(url, body, error)) {
                htmlOut = Utf8ToWide(body);
                return true;
            }
        }

        htmlOut.clear();
        return false;
    }

    static std::wstring ExtractRoadsOrgDetailHref(const std::wstring& rowHtml, const std::wstring& slug)
    {
        std::wstring pattern = LR"(href\s*=\s*(['"])((?:/motorway/)";
        pattern += slug;
        pattern += LR"(/[^'"]+))\1)";
        std::wregex hrefRe(pattern, std::regex_constants::icase);
        std::wsmatch m;
        if (std::regex_search(rowHtml, m, hrefRe) && m.size() > 2)
            return m[2].str();
        return L"";
    }

    static bool RowMatchesRoadsOrgJunction(const std::wstring& rowHtml, const std::wstring& junctionNumber)
    {
        std::wregex jctCellRe(LR"(<td\b[^>]*class\s*=\s*(['"])[^'"]*\bjct\b[^'"]*\1[^>]*>([\s\S]*?)</td>)", std::regex_constants::icase);
        std::wsmatch m;
        if (!std::regex_search(rowHtml, m, jctCellRe) || m.size() <= 2)
            return false;

        std::wstring cellText = StripTemplateHtmlTags(m[2].str());
        std::wregex numberRe(LR"(\b(?:J\s*)?(\d+[A-Za-z]?)\b)", std::regex_constants::icase);
        std::wsmatch numberMatch;
        if (!std::regex_search(cellText, numberMatch, numberRe) || numberMatch.size() <= 1)
            return false;

        return ToLower(numberMatch[1].str()) == ToLower(junctionNumber);
    }

    static bool TryFindRoadsOrgJunctionRow(const std::wstring& html, const std::wstring& junctionNumber, std::wstring& rowHtmlOut, std::wstring& detailHrefOut)
    {
        std::wregex rowRe(LR"(<tr\b[^>]*>([\s\S]*?)</tr>)", std::regex_constants::icase);
        for (std::wsregex_iterator it(html.begin(), html.end(), rowRe), end; it != end; ++it) {
            std::wstring row = (*it)[0].str();
            if (!RowMatchesRoadsOrgJunction(row, junctionNumber))
                continue;

            rowHtmlOut = std::move(row);
            detailHrefOut.clear();
            return true;
        }
        rowHtmlOut.clear();
        detailHrefOut.clear();
        return false;
    }

    static std::wstring ExtractRoadsOrgDetailOtherRoutes(const std::wstring& html, const std::wstring& currentRoad)
    {
        std::wstring text = StripTemplateHtmlTags(html);
        std::wregex otherRoutesRe(LR"(Other routes\s+(.+?)\s+(?:Authority|Restricted turns|Services|Layout and signage))", std::regex_constants::icase);
        std::wsmatch m;
        if (std::regex_search(text, m, otherRoutesRe) && m.size() > 1) {
            std::vector<std::wstring> refs = ExtractRoadRefsFromText(m[1].str(), currentRoad);
            if (!refs.empty())
                return JoinTemplateItems(refs);
            return Trim(m[1].str());
        }

        return L"";
    }

    std::wstring FetchRoadsOrgJunctionData(const std::wstring& road, const JunctionTemplateData& junction, const std::wstring& direction) const
    {
        std::wstring slug = RoadSlugForRoadsOrg(road);
        if (slug.empty() || junction.number.empty())
            return L"";

        std::wstring html;
        if (!TryFetchRoadsOrgPage(slug, html))
            return L"";

        std::wstring rowHtml;
        std::wstring detailHref;
        std::wstring rowData;
        if (TryFindRoadsOrgJunctionRow(html, junction.number, rowHtml, detailHref)) {
            detailHref = ExtractRoadsOrgDetailHref(rowHtml, slug);

            std::vector<std::wstring> signPanels;
            std::wregex signPanelRe(LR"(<td\b[^>]*class\s*=\s*(['"])[^'"]*\bsignpanel\b[^'"]*\1[^>]*>([\s\S]*?)</td>)", std::regex_constants::icase);
            for (std::wsregex_iterator it(rowHtml.begin(), rowHtml.end(), signPanelRe), end; it != end; ++it)
                signPanels.push_back((*it)[2].str());

            std::wstring lowerDirection = ToLower(direction);
            const bool forwardDirection =
                lowerDirection == L"northbound" || lowerDirection == L"westbound" || lowerDirection == L"anticlockwise";
            const bool reverseDirection =
                lowerDirection == L"southbound" || lowerDirection == L"eastbound" || lowerDirection == L"clockwise";
            if (!signPanels.empty() &&
                forwardDirection)
            {
                rowData = ExtractRoadsOrgDataFromFragment(signPanels.front(), road);
            }
            else if (signPanels.size() > 1 &&
                reverseDirection)
            {
                rowData = ExtractRoadsOrgDataFromFragment(signPanels[1], road);
            }
            else if (!forwardDirection && !reverseDirection && !signPanels.empty()) {
                rowData = ExtractRoadsOrgDataFromFragment(signPanels.front(), road);
            }
        }

        return rowData;
    }

    static std::wstring ExtractDirectionFromLocation(const std::wstring& location)
    {
        std::wregex directionRe(LR"(\b(anticlockwise|clockwise|northbound|southbound|eastbound|westbound)\b)", std::regex_constants::icase);
        std::wsmatch m;
        if (std::regex_search(location, m, directionRe) && m.size() > 1)
            return m[1].str();
        return L"";
    }

    static std::wstring ReadBalancedParenthetical(const std::wstring& text, size_t openPos, size_t& endPos)
    {
        endPos = openPos;
        if (openPos >= text.size() || text[openPos] != L'(')
            return L"";

        int depth = 0;
        const size_t valueStart = openPos + 1;
        for (size_t i = openPos; i < text.size(); ++i) {
            if (text[i] == L'(')
                ++depth;
            else if (text[i] == L')') {
                --depth;
                if (depth == 0) {
                    endPos = i + 1;
                    return Trim(text.substr(valueStart, i - valueStart));
                }
            }
        }

        endPos = text.size();
        return L"";
    }

    static std::wstring JunctionDisplayName(const std::wstring& matchText, const std::wstring& number)
    {
        std::wstring lowerMatch = ToLower(matchText);
        std::wregex jPrefixRe(LR"(\bJ\s*\d)", std::regex_constants::icase);
        if (std::regex_search(matchText, jPrefixRe))
            return L"J" + number;
        if (lowerMatch.find(L"junction") != std::wstring::npos)
            return L"Junction " + number;
        return L"J" + number;
    }

    static bool AddExtractedJunction(
        std::vector<JunctionTemplateData>& junctions,
        std::unordered_set<std::wstring>& seenNumbers,
        const std::wstring& display,
        const std::wstring& number,
        const std::wstring& data,
        size_t sourcePosition)
    {
        std::wstring normalizedNumber = ToLower(Trim(number));
        if (normalizedNumber.empty() || seenNumbers.find(normalizedNumber) != seenNumbers.end())
            return false;

        seenNumbers.insert(normalizedNumber);
        JunctionTemplateData junction;
        junction.display = display.empty() ? L"J" + number : display;
        junction.number = number;
        junction.data = data;
        junction.sourcePosition = sourcePosition;
        junctions.push_back(std::move(junction));
        return true;
    }

    static std::vector<JunctionTemplateData> ExtractJunctionsFromLocation(const std::wstring& location)
    {
        std::vector<JunctionTemplateData> junctions;
        std::unordered_set<std::wstring> seenNumbers;

        std::wregex explicitJunctionRe(LR"(\b(?:junctions?|j)\s*(?:\.?\s*)?(?:J\s*)?(\d+[A-Za-z]?)\b)", std::regex_constants::icase);
        for (std::wsregex_iterator it(location.begin(), location.end(), explicitJunctionRe), end; it != end; ++it) {
            std::wstring number = (*it)[1].str();
            std::wstring matchText = (*it)[0].str();
            size_t matchPos = static_cast<size_t>((*it).position(0));
            size_t cursor = matchPos + static_cast<size_t>((*it).length(0));
            while (cursor < location.size() && iswspace(location[cursor]))
                ++cursor;

            std::wstring data;
            if (cursor < location.size() && location[cursor] == L'(') {
                size_t endPos = cursor;
                data = ReadBalancedParenthetical(location, cursor, endPos);
            }

            AddExtractedJunction(
                junctions,
                seenNumbers,
                JunctionDisplayName(matchText, number),
                number,
                data,
                matchPos);
        }

        std::wregex pluralPhraseRe(LR"(\bjunctions?\s+((?:J?\s*\d+[A-Za-z]?\s*(?:(?:,|and|&|to|-)\s*)?)+))", std::regex_constants::icase);
        for (std::wsregex_iterator it(location.begin(), location.end(), pluralPhraseRe), end; it != end; ++it) {
            std::wstring phrase = (*it)[1].str();
            size_t phrasePos = static_cast<size_t>((*it).position(1));
            std::wregex numberRe(LR"(\b(J?)\s*(\d+[A-Za-z]?)\b)", std::regex_constants::icase);
            for (std::wsregex_iterator numberIt(phrase.begin(), phrase.end(), numberRe), numberEnd; numberIt != numberEnd; ++numberIt) {
                std::wstring number = (*numberIt)[2].str();
                bool hasJPrefix = !(*numberIt)[1].str().empty();
                std::wstring display = hasJPrefix ? L"J" + number : L"Junction " + number;
                AddExtractedJunction(
                    junctions,
                    seenNumbers,
                    display,
                    number,
                    L"",
                    phrasePos + static_cast<size_t>((*numberIt).position(0)));
            }
        }

        std::sort(junctions.begin(), junctions.end(), [](const JunctionTemplateData& a, const JunctionTemplateData& b) {
            return a.sourcePosition < b.sourcePosition;
            });
        return junctions;
    }

    static void SetTemplateVariable(std::vector<std::pair<std::wstring, std::wstring>>& variables, const std::wstring& key, const std::wstring& value)
    {
        for (auto& item : variables) {
            if (item.first == key) {
                item.second = value;
                return;
            }
        }
        variables.push_back({ key, value });
    }

    std::wstring TemplateVariableValue(const std::wstring& key) const
    {
        for (const auto& item : m_templateWizardVariables) {
            if (item.first == key)
                return item.second;
        }
        return L"";
    }

    std::vector<std::pair<std::wstring, std::wstring>> BuildTemplateVariables(const TrafficAlert& alert) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        std::wstring road = alert.road.empty() ? alert.region : alert.road;
        std::wstring location = AlertLocationForNotification(alert);
        std::wstring direction = ExtractDirectionFromLocation(location);
        std::vector<JunctionTemplateData> junctions = ExtractJunctionsFromLocation(location);

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        SetTemplateVariable(variables, L"$TITLE", alert.title.empty() ? AlertReasonForNotification(alert) : alert.title);
        SetTemplateVariable(variables, L"$ROAD", road);
        SetTemplateVariable(variables, L"$DIRECTION", direction);
        SetTemplateVariable(variables, L"%LOCATION", location);
        if (alert.hasLocation) {
            wchar_t latitude[48]{};
            wchar_t longitude[48]{};
            swprintf_s(latitude, L"%.5f", alert.latitude);
            swprintf_s(longitude, L"%.5f", alert.longitude);
            SetTemplateVariable(variables, L"%LATITUDE", latitude);
            SetTemplateVariable(variables, L"%LONGITUDE", longitude);
        }

        std::vector<std::wstring> junctionNames;
        std::vector<std::wstring> junctionsWithData;
        for (size_t i = 0; i < junctions.size(); ++i) {
            std::wstring suffix = std::to_wstring(i + 1);
            std::wstring junctionName = junctions[i].display;
            std::wstring junctionData = junctions[i].data;
            if (junctionData.empty())
                junctionData = FetchRoadsOrgJunctionData(road, junctions[i], direction);

            SetTemplateVariable(variables, L"%JUNCTION" + suffix, junctionName);
            SetTemplateVariable(variables, L"%JUNCTIONDATA" + suffix, junctionData);
            junctionNames.push_back(junctionName);
            junctionsWithData.push_back(junctionData.empty() ? junctionName : junctionName + L" (" + junctionData + L")");
        }
        SetTemplateVariable(variables, L"%JUNCTIONCOUNT", std::to_wstring(junctions.size()));
        SetTemplateVariable(variables, L"%JUNCTIONS", JoinTemplateItemsAsPhrase(junctionNames));
        std::wstring junctionsWithDataText = JoinTemplateItemsAsPhrase(junctionsWithData);
        if (!junctionsWithDataText.empty()) {
            std::wstring lowerLocation = ToLower(location);
            if (lowerLocation.find(L"between") != std::wstring::npos)
                junctionsWithDataText = L"between " + junctionsWithDataText;
            else if (lowerLocation.find(L" at ") != std::wstring::npos || StartsWithNoCase(lowerLocation, L"at "))
                junctionsWithDataText = L"at " + junctionsWithDataText;
        }
        SetTemplateVariable(variables, L"%JUNCTIONS_WITH_DATA", junctionsWithDataText);

        std::wstring laneClosures;
        if (alert.lanesTotal > 0) {
            laneClosures = std::to_wstring(alert.lanesClosed) + L" of " +
                std::to_wstring(alert.lanesTotal) + L" lanes";
        }
        else {
            laneClosures = ExtractLabeledNotificationField(alert.description, L"Lanes Closed");
        }
        SetTemplateVariable(variables, L"%LANECLOSURES", laneClosures);
        return variables;
    }

    std::vector<std::pair<std::wstring, std::wstring>> BuildEarthquakeTemplateVariables(const EarthquakeEvent& event) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        wchar_t magnitude[32]{};
        wchar_t latitude[48]{};
        wchar_t longitude[48]{};
        wchar_t depth[32]{};
        swprintf_s(magnitude, L"%.1f", event.magnitude);
        swprintf_s(latitude, L"%.5f", event.latitude);
        swprintf_s(longitude, L"%.5f", event.longitude);
        swprintf_s(depth, L"%.1f", event.depthKm);

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        SetTemplateVariable(variables, L"$MAGNITUDE", magnitude);
        SetTemplateVariable(variables, L"$PLACE", event.place.empty() ? L"unknown region" : event.place);
        SetTemplateVariable(variables, L"$TIME", event.timeText);
        SetTemplateVariable(variables, L"%LATITUDE", latitude);
        SetTemplateVariable(variables, L"%LONGITUDE", longitude);
        SetTemplateVariable(variables, L"%DEPTH", depth);
        return variables;
    }

    std::vector<std::pair<std::wstring, std::wstring>> BuildWeatherSystemTemplateVariables(const WeatherSystemEvent& system) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        wchar_t latitude[48]{};
        wchar_t longitude[48]{};
        wchar_t forecastLatitude[48]{};
        wchar_t forecastLongitude[48]{};
        wchar_t windKnots[32]{};
        wchar_t forecastWindKnots[32]{};
        swprintf_s(latitude, L"%.5f", system.latitude);
        swprintf_s(longitude, L"%.5f", system.longitude);
        swprintf_s(forecastLatitude, L"%.5f", system.forecastLatitude);
        swprintf_s(forecastLongitude, L"%.5f", system.forecastLongitude);
        swprintf_s(windKnots, L"%.0f", system.windKnots);
        swprintf_s(forecastWindKnots, L"%.0f", system.forecastWindKnots);
        std::wstring windText = system.windText.empty() ? std::wstring(windKnots) + L" kts" : system.windText;
        std::wstring forecastWindText = system.forecastWindText.empty() ? std::wstring(forecastWindKnots) + L" kts" : system.forecastWindText;

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        SetTemplateVariable(variables, L"$SYSTEM", system.name.empty() ? L"Weather system" : system.name);
        SetTemplateVariable(variables, L"$BASIN", system.basin);
        SetTemplateVariable(variables, L"$CATEGORY", system.category);
        SetTemplateVariable(variables, L"$WIND", windText);
        SetTemplateVariable(variables, L"$WIND_KNOTS", windKnots);
        SetTemplateVariable(variables, L"$FORECAST_CATEGORY", system.forecastCategory);
        SetTemplateVariable(variables, L"$FORECAST_WIND", forecastWindText);
        SetTemplateVariable(variables, L"$FORECAST_WIND_KNOTS", forecastWindKnots);
        SetTemplateVariable(variables, L"$UPDATED", system.updatedText);
        SetTemplateVariable(variables, L"%LATITUDE", latitude);
        SetTemplateVariable(variables, L"%LONGITUDE", longitude);
        SetTemplateVariable(variables, L"%FORECAST_LATITUDE", forecastLatitude);
        SetTemplateVariable(variables, L"%FORECAST_LONGITUDE", forecastLongitude);
        return variables;
    }

    std::wstring FormatTemplateVariablesForEdit() const
    {
        std::wstring text;
        for (const auto& item : m_templateWizardVariables) {
            if (!text.empty())
                text += L"\r\n";
            text += item.first + L" = " + item.second;
        }
        return text;
    }

    void LoadTemplateVariablesFromEdit()
    {
        if (!m_templateWizardVariablesEdit)
            return;

        std::wstring text = GetWindowTextString(m_templateWizardVariablesEdit);
        size_t pos = 0;
        while (pos <= text.size()) {
            size_t next = text.find_first_of(L"\r\n", pos);
            std::wstring line = Trim(text.substr(pos, next == std::wstring::npos ? std::wstring::npos : next - pos));
            if (!line.empty()) {
                size_t sep = line.find(L'=');
                if (sep == std::wstring::npos)
                    sep = line.find(L':');
                if (sep != std::wstring::npos) {
                    std::wstring key = Trim(line.substr(0, sep));
                    std::wstring value = Trim(line.substr(sep + 1));
                    SetTemplateVariable(m_templateWizardVariables, key, value);
                }
            }
            if (next == std::wstring::npos)
                break;
            pos = next + 1;
            while (pos < text.size() && (text[pos] == L'\r' || text[pos] == L'\n'))
                ++pos;
        }
    }

    static bool HasPrefixNoCase(const std::wstring& value, const std::wstring& prefix)
    {
        return StartsWithNoCase(value, prefix);
    }

    static bool NeedsAnArticle(const std::wstring& nextWord)
    {
        std::wstring word = ToLower(Trim(nextWord));
        if (word.empty())
            return false;

        while (!word.empty() && !iswalnum(word.front()))
            word.erase(word.begin());
        if (word.empty())
            return false;

        if (HasPrefixNoCase(word, L"honest") || HasPrefixNoCase(word, L"honour") ||
            HasPrefixNoCase(word, L"hour") || HasPrefixNoCase(word, L"heir"))
            return true;

        if (HasPrefixNoCase(word, L"user") || HasPrefixNoCase(word, L"unit") ||
            HasPrefixNoCase(word, L"university") || HasPrefixNoCase(word, L"unicorn") ||
            HasPrefixNoCase(word, L"euro") || HasPrefixNoCase(word, L"one"))
            return false;

        if (word.size() == 1) {
            wchar_t ch = static_cast<wchar_t>(towupper(word[0]));
            return wcschr(L"AEFHILMNORSX", ch) != nullptr;
        }

        bool allCaps = true;
        for (wchar_t ch : nextWord) {
            if (iswalpha(ch) && !iswupper(ch)) {
                allCaps = false;
                break;
            }
        }
        if (allCaps) {
            wchar_t ch = static_cast<wchar_t>(towupper(nextWord[0]));
            return wcschr(L"AEFHILMNORSX", ch) != nullptr;
        }

        wchar_t first = word[0];
        return first == L'a' || first == L'e' || first == L'i' || first == L'o' || first == L'u';
    }

    static std::wstring FixIndefiniteArticles(std::wstring text)
    {
        std::wregex articleRe(LR"(\b(A|An|a|an)\s+([A-Za-z0-9][A-Za-z0-9'\-]*))");
        std::wstring output;
        size_t cursor = 0;
        for (std::wsregex_iterator it(text.begin(), text.end(), articleRe), end; it != end; ++it) {
            const auto& match = *it;
            const size_t pos = static_cast<size_t>(match.position(0));
            output.append(text, cursor, pos - cursor);
            std::wstring originalArticle = match[1].str();
            std::wstring word = match[2].str();
            bool useAn = NeedsAnArticle(word);
            bool upper = !originalArticle.empty() && iswupper(originalArticle[0]);
            output += upper ? (useAn ? L"An" : L"A") : (useAn ? L"an" : L"a");
            output += L" ";
            output += word;
            cursor = pos + static_cast<size_t>(match.length(0));
        }
        output.append(text, cursor, std::wstring::npos);
        return output;
    }

    std::wstring RenderReportTemplate(const ReportTemplate& reportTemplate) const
    {
        std::wstring output = reportTemplate.body;
        std::vector<std::pair<std::wstring, std::wstring>> variables = m_templateWizardVariables;
        std::sort(variables.begin(), variables.end(), [](const auto& a, const auto& b) {
            return a.first.size() > b.first.size();
            });
        for (const auto& item : variables)
            ReplaceAllText(output, item.first, item.second);
        return FixIndefiniteArticles(output);
    }

    bool CopyTextToClipboard(const std::wstring& text, HWND owner)
    {
        if (!OpenClipboard(owner))
            return false;
        EmptyClipboard();

        const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!mem) {
            CloseClipboard();
            return false;
        }

        void* ptr = GlobalLock(mem);
        if (!ptr) {
            GlobalFree(mem);
            CloseClipboard();
            return false;
        }
        CopyMemory(ptr, text.c_str(), bytes);
        GlobalUnlock(mem);

        if (!SetClipboardData(CF_UNICODETEXT, mem)) {
            GlobalFree(mem);
            CloseClipboard();
            return false;
        }
        CloseClipboard();
        return true;
    }

    static LRESULT CALLBACK TemplatesWizardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleTemplatesWizardMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleTemplatesWizardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateTemplatesWizardControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnTemplatesWizardCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::Roads;
        EnsureDefaultTemplatesForContext(m_templateWizardContext);
        const auto& templates = TemplatesForContext(m_templateWizardContext);
        if (templates.empty()) {
            MessageBoxW(m_hwnd, L"No templates are configured. Open Roads > Edit Templates to add one.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        const TrafficAlert* alert = FindSelectedAlert();
        if (!alert) {
            MessageBoxW(m_hwnd, L"Select an incident first, then open the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        m_templateWizardAlertId = alert->id;
        m_templateWizardEarthquakeId.clear();
        m_templateWizardWeatherSystemId.clear();
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables = BuildTemplateVariables(*alert);

        ShowTemplatesWizardWindowShell(L"Road Templates Wizard");
    }

    void ShowEarthquakeTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::Earthquakes;
        EnsureDefaultTemplatesForContext(m_templateWizardContext);
        const auto& templates = TemplatesForContext(m_templateWizardContext);
        if (templates.empty()) {
            MessageBoxW(m_hwnd, L"No templates are configured. Open Earthquakes > Edit Templates to add one.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        const EarthquakeEvent* event = FindSelectedEarthquake();
        if (!event) {
            MessageBoxW(m_hwnd, L"Select an earthquake in the Earthquakes List first, then open the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        m_templateWizardAlertId.clear();
        m_templateWizardEarthquakeId = EarthquakeStableKey(*event);
        m_templateWizardWeatherSystemId.clear();
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables = BuildEarthquakeTemplateVariables(*event);

        ShowTemplatesWizardWindowShell(L"Earthquake Templates Wizard");
    }

    void ShowWeatherSystemsTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::WeatherSystems;
        EnsureDefaultTemplatesForContext(m_templateWizardContext);
        const auto& templates = TemplatesForContext(m_templateWizardContext);
        if (templates.empty()) {
            MessageBoxW(m_hwnd, L"No templates are configured. Open Weather Systems > Edit Templates to add one.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        const WeatherSystemEvent* system = FindSelectedWeatherSystem();
        if (!system) {
            MessageBoxW(m_hwnd, L"Select a weather system in the Weather Systems List first, then open the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return;
        }

        m_templateWizardAlertId.clear();
        m_templateWizardEarthquakeId.clear();
        m_templateWizardWeatherSystemId = WeatherSystemStableKey(*system);
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables = BuildWeatherSystemTemplateVariables(*system);

        ShowTemplatesWizardWindowShell(L"Weather System Templates Wizard");
    }

    void ShowTemplatesWizardWindowShell(const wchar_t* title)
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = TemplatesWizardWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kTemplatesWizardClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_templatesWizardWnd || !IsWindow(m_templatesWizardWnd)) {
            m_templatesWizardWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kTemplatesWizardClassName,
                title,
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                720,
                460,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        else {
            SetWindowTextSafe(m_templatesWizardWnd, title);
        }

        PopulateTemplatesWizardList();
        SetWindowTextSafe(m_templateWizardVariablesEdit, FormatTemplateVariablesForEdit());
        RenderTemplatesWizardStep();
        ShowWindow(m_templatesWizardWnd, SW_SHOW);
        SetForegroundWindow(m_templatesWizardWnd);
    }

    void CreateTemplatesWizardControls(HWND parent)
    {
        CreateAutoLabel(parent, IDC_TEMPLATES_WIZARD_TITLE, L"Templates Wizard", 18, 18, m_headerFont);
        m_templateWizardDesc = CreateAutoLabel(parent, IDC_TEMPLATES_WIZARD_DESC, L"", 18, 54, nullptr, 580);
        m_templateWizardList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 92, 584, 220, parent, ControlId(IDC_TEMPLATES_WIZARD_LIST), m_hInst, nullptr);
        m_templateWizardVariablesEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 18, 92, 584, 220, parent, ControlId(IDC_TEMPLATES_WIZARD_VARIABLES), m_hInst, nullptr);
        m_templateWizardPreviewEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 18, 92, 584, 220, parent, ControlId(IDC_TEMPLATES_WIZARD_PREVIEW), m_hInst, nullptr);
        m_templateWizardPrevBtn = CreateWindowExW(0, L"BUTTON", L"Previous", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 188, 344, 102, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_PREV), m_hInst, nullptr);
        m_templateWizardNextBtn = CreateWindowExW(0, L"BUTTON", L"Next", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 298, 344, 102, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_NEXT), m_hInst, nullptr);
        m_templateWizardCopyBtn = CreateWindowExW(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 408, 344, 72, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_COPY), m_hInst, nullptr);
        m_templateWizardCopyLocationBtn = CreateWindowExW(0, L"BUTTON", L"Copy Coords", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 488, 344, 118, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_COPY_LOCATION), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 614, 344, 68, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_CLOSE), m_hInst, nullptr);

        for (HWND h : { m_templateWizardDesc, m_templateWizardList, m_templateWizardVariablesEdit, m_templateWizardPreviewEdit, m_templateWizardPrevBtn, m_templateWizardNextBtn, m_templateWizardCopyBtn, m_templateWizardCopyLocationBtn, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        PopulateTemplatesWizardList();
        RenderTemplatesWizardStep();
        AutoFitWindowToChildren(parent);
    }

    void PopulateTemplatesWizardList()
    {
        if (!m_templateWizardList)
            return;
        const auto& templates = TemplatesForContext(m_templateWizardContext);
        SendMessageW(m_templateWizardList, LB_RESETCONTENT, 0, 0);
        for (const ReportTemplate& reportTemplate : templates)
            SendMessageW(m_templateWizardList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(reportTemplate.name.c_str()));
        if (!templates.empty()) {
            m_templateWizardTemplateIndex = MinValue<size_t>(m_templateWizardTemplateIndex, templates.size() - 1);
            SendMessageW(m_templateWizardList, LB_SETCURSEL, static_cast<WPARAM>(m_templateWizardTemplateIndex), 0);
        }
    }

    void RenderTemplatesWizardStep()
    {
        if (!m_templatesWizardWnd)
            return;

        const bool chooseStep = m_templateWizardStep == 0;
        const bool variableStep = m_templateWizardStep == 1;
        const bool reviewStep = m_templateWizardStep == 2;
        ShowWindow(m_templateWizardList, chooseStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardVariablesEdit, variableStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardPreviewEdit, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardCopyBtn, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardCopyLocationBtn, reviewStep ? SW_SHOW : SW_HIDE);
        EnableWindow(m_templateWizardPrevBtn, m_templateWizardStep > 0);
        SetWindowTextSafe(m_templateWizardNextBtn, reviewStep ? L"Finish" : L"Next");

        if (chooseStep) {
            const wchar_t* description = L"Choose the report template to use for the selected incident.";
            if (m_templateWizardContext == TemplateContext::Earthquakes)
                description = L"Choose the report template to use for the selected earthquake.";
            else if (m_templateWizardContext == TemplateContext::WeatherSystems)
                description = L"Choose the report template to use for the selected weather system.";
            SetWindowTextSafe(m_templateWizardDesc, description);
        }
        else if (variableStep)
            SetWindowTextSafe(m_templateWizardDesc, L"Review and edit the variables only. Leave the template text itself unchanged here.");
        else {
            SetWindowTextSafe(m_templateWizardDesc, L"Review the completed message, then copy it to the clipboard.");
            const auto& templates = TemplatesForContext(m_templateWizardContext);
            if (!templates.empty()) {
                m_templateWizardTemplateIndex = MinValue<size_t>(m_templateWizardTemplateIndex, templates.size() - 1);
                SetWindowTextSafe(m_templateWizardPreviewEdit, RenderReportTemplate(templates[m_templateWizardTemplateIndex]));
            }
        }
    }

    void OnTemplatesWizardCommand(int id, int code)
    {
        if (id == IDC_TEMPLATES_WIZARD_CLOSE && code == BN_CLICKED) {
            ShowWindow(m_templatesWizardWnd, SW_HIDE);
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_LIST && code == LBN_SELCHANGE) {
            int selected = static_cast<int>(SendMessageW(m_templateWizardList, LB_GETCURSEL, 0, 0));
            const auto& templates = TemplatesForContext(m_templateWizardContext);
            if (selected >= 0 && selected < static_cast<int>(templates.size()))
                m_templateWizardTemplateIndex = static_cast<size_t>(selected);
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_PREV && code == BN_CLICKED) {
            if (m_templateWizardStep > 0)
                --m_templateWizardStep;
            RenderTemplatesWizardStep();
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_NEXT && code == BN_CLICKED) {
            if (m_templateWizardStep == 0) {
                int selected = static_cast<int>(SendMessageW(m_templateWizardList, LB_GETCURSEL, 0, 0));
                const auto& templates = TemplatesForContext(m_templateWizardContext);
                if (selected >= 0 && selected < static_cast<int>(templates.size()))
                    m_templateWizardTemplateIndex = static_cast<size_t>(selected);
                SetWindowTextSafe(m_templateWizardVariablesEdit, FormatTemplateVariablesForEdit());
                m_templateWizardStep = 1;
            }
            else if (m_templateWizardStep == 1) {
                LoadTemplateVariablesFromEdit();
                m_templateWizardStep = 2;
            }
            else {
                ShowWindow(m_templatesWizardWnd, SW_HIDE);
            }
            RenderTemplatesWizardStep();
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_COPY && code == BN_CLICKED) {
            std::wstring text = GetWindowTextString(m_templateWizardPreviewEdit);
            if (CopyTextToClipboard(text, m_templatesWizardWnd))
                SetStatusText(L"Template message copied to clipboard.");
            else
                SetStatusText(L"Could not copy template message to clipboard.");
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_COPY_LOCATION && code == BN_CLICKED) {
            std::wstring latitude = TemplateVariableValue(L"%LATITUDE");
            std::wstring longitude = TemplateVariableValue(L"%LONGITUDE");
            std::wstring coords;
            if (!latitude.empty() && !longitude.empty())
                coords = latitude + L", " + longitude;

            if (coords.empty()) {
                SetStatusText(L"No coordinates are available for this report.");
            }
            else if (CopyTextToClipboard(coords, m_templatesWizardWnd))
                SetStatusText(L"Coordinates copied to clipboard.");
            else
                SetStatusText(L"Could not copy coordinates to clipboard.");
            return;
        }
    }

    static LRESULT CALLBACK TemplatesEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleTemplatesEditorMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleTemplatesEditorMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateTemplatesEditorControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnTemplatesEditorCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowTemplatesEditorWindow()
    {
        m_templateEditorContext = TemplateContext::Roads;
        EnsureDefaultTemplatesForContext(m_templateEditorContext);
        ShowTemplatesEditorWindowShell(L"Edit Road Templates");
    }

    void ShowEarthquakeTemplatesEditorWindow()
    {
        m_templateEditorContext = TemplateContext::Earthquakes;
        EnsureDefaultTemplatesForContext(m_templateEditorContext);
        ShowTemplatesEditorWindowShell(L"Edit Earthquake Templates");
    }

    void ShowWeatherSystemsTemplatesEditorWindow()
    {
        m_templateEditorContext = TemplateContext::WeatherSystems;
        EnsureDefaultTemplatesForContext(m_templateEditorContext);
        ShowTemplatesEditorWindowShell(L"Edit Weather System Templates");
    }

    void ShowTemplatesEditorWindowShell(const wchar_t* title)
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = TemplatesEditorWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kTemplatesEditorClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_templatesEditorWnd || !IsWindow(m_templatesEditorWnd)) {
            m_templatesEditorWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kTemplatesEditorClassName,
                title,
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                760,
                560,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        else {
            SetWindowTextSafe(m_templatesEditorWnd, title);
        }

        SyncTemplatesEditorList();
        ShowWindow(m_templatesEditorWnd, SW_SHOW);
        SetForegroundWindow(m_templatesEditorWnd);
    }

    void CreateTemplatesEditorControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Edit Templates", 18, 18, m_headerFont);
        m_templateEditorList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 64, 214, 340, parent, ControlId(IDC_TEMPLATES_EDITOR_LIST), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Name", 252, 64);
        m_templateEditorNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 252, 90, 450, 26, parent, ControlId(IDC_TEMPLATES_EDITOR_NAME), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Template", 252, 132);
        m_templateEditorBodyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 252, 158, 450, 246, parent, ControlId(IDC_TEMPLATES_EDITOR_BODY), m_hInst, nullptr);
        HWND newBtn = CreateWindowExW(0, L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_NEW), m_hInst, nullptr);
        HWND saveBtn = CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 252, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_SAVE), m_hInst, nullptr);
        HWND deleteBtn = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 348, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_DELETE), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 614, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_CLOSE), m_hInst, nullptr);

        for (HWND h : { m_templateEditorList, m_templateEditorNameEdit, m_templateEditorBodyEdit, newBtn, saveBtn, deleteBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SyncTemplatesEditorList();
        AutoFitWindowToChildren(parent);
    }

    int SelectedTemplateEditorIndex() const
    {
        if (!m_templateEditorList)
            return -1;
        int selected = static_cast<int>(SendMessageW(m_templateEditorList, LB_GETCURSEL, 0, 0));
        if (selected < 0 || selected >= static_cast<int>(TemplatesForContext(m_templateEditorContext).size()))
            return -1;
        return selected;
    }

    void SyncTemplatesEditorList()
    {
        if (!m_templateEditorList)
            return;

        int previous = SelectedTemplateEditorIndex();
        const auto& templates = TemplatesForContext(m_templateEditorContext);
        SendMessageW(m_templateEditorList, LB_RESETCONTENT, 0, 0);
        for (const ReportTemplate& reportTemplate : templates)
            SendMessageW(m_templateEditorList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(reportTemplate.name.c_str()));

        if (!templates.empty()) {
            previous = ClampValue(previous, 0, static_cast<int>(templates.size()) - 1);
            SendMessageW(m_templateEditorList, LB_SETCURSEL, previous, 0);
            LoadSelectedTemplateIntoEditor();
        }
        else {
            SetWindowTextSafe(m_templateEditorNameEdit, L"");
            SetWindowTextSafe(m_templateEditorBodyEdit, L"");
        }
    }

    void LoadSelectedTemplateIntoEditor()
    {
        int selected = SelectedTemplateEditorIndex();
        if (selected < 0)
            return;
        const ReportTemplate& reportTemplate = TemplatesForContext(m_templateEditorContext)[static_cast<size_t>(selected)];
        SetWindowTextSafe(m_templateEditorNameEdit, reportTemplate.name);
        SetWindowTextSafe(m_templateEditorBodyEdit, reportTemplate.body);
    }

    void OnTemplatesEditorCommand(int id, int code)
    {
        if (id == IDC_TEMPLATES_EDITOR_CLOSE && code == BN_CLICKED) {
            ShowWindow(m_templatesEditorWnd, SW_HIDE);
            return;
        }
        if (id == IDC_TEMPLATES_EDITOR_LIST && code == LBN_SELCHANGE) {
            LoadSelectedTemplateIntoEditor();
            return;
        }
        if (code != BN_CLICKED)
            return;

        if (id == IDC_TEMPLATES_EDITOR_NEW) {
            auto& templates = TemplatesForContext(m_templateEditorContext);
            ReportTemplate reportTemplate;
            reportTemplate.name = L"Template " + std::to_wstring(templates.size() + 1);
            reportTemplate.body = DefaultTemplateBodyForContext(m_templateEditorContext);
            templates.push_back(std::move(reportTemplate));
            TemplatesConfiguredForContext(m_templateEditorContext) = true;
            SaveSettings();
            SyncTemplatesEditorList();
            SendMessageW(m_templateEditorList, LB_SETCURSEL, static_cast<WPARAM>(templates.size() - 1), 0);
            LoadSelectedTemplateIntoEditor();
        }
        else if (id == IDC_TEMPLATES_EDITOR_SAVE) {
            int selected = SelectedTemplateEditorIndex();
            if (selected < 0)
                return;
            ReportTemplate& reportTemplate = TemplatesForContext(m_templateEditorContext)[static_cast<size_t>(selected)];
            reportTemplate.name = Trim(GetWindowTextString(m_templateEditorNameEdit));
            reportTemplate.body = GetWindowTextString(m_templateEditorBodyEdit);
            if (reportTemplate.name.empty())
                reportTemplate.name = L"Template " + std::to_wstring(selected + 1);
            SaveSettings();
            SyncTemplatesEditorList();
            SendMessageW(m_templateEditorList, LB_SETCURSEL, static_cast<WPARAM>(selected), 0);
            SetStatusText(L"Template saved.");
        }
        else if (id == IDC_TEMPLATES_EDITOR_DELETE) {
            int selected = SelectedTemplateEditorIndex();
            if (selected < 0)
                return;
            auto& templates = TemplatesForContext(m_templateEditorContext);
            templates.erase(templates.begin() + selected);
            TemplatesConfiguredForContext(m_templateEditorContext) = true;
            SaveSettings();
            SyncTemplatesEditorList();
            SetStatusText(L"Template removed.");
        }
    }

    bool TryParsePercentThreshold(const std::wstring& text, double& valueOut) const
    {
        std::wstring value = Trim(text);
        if (!value.empty() && value.back() == L'%')
            value.pop_back();
        value = Trim(value);
        if (value.empty())
            return false;

        wchar_t* end = nullptr;
        double parsed = std::wcstod(value.c_str(), &end);
        if (end == value.c_str() || !std::isfinite(parsed))
            return false;
        if (Trim(end ? end : L"").empty() && parsed >= 0.0 && parsed <= 100.0) {
            valueOut = parsed;
            return true;
        }
        return false;
    }

    std::wstring EarthquakeQueryUrl() const
    {
        // Equivalent GeoJSON query for the requested USGS map extent:
        // https://earthquake.usgs.gov/earthquakes/map/?extent=-82.94033,-81.5625&extent=82.9834,480.9375
        return L"https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=-82.94033&maxlatitude=82.9834&minlongitude=-180&maxlongitude=180&orderby=time&limit=20000";
    }

    void FetchEarthquakesAsync(bool notify)
    {
        if (m_earthquakeFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        std::wstring url = EarthquakeQueryUrl();
        ScheduleBackgroundTask([hwnd, url, notify]() {
            auto* result = new EarthquakeResult{};
            result->notify = notify;
            std::string body;
            std::wstring error;
            if (HttpGetText(url, body, error)) {
                try {
                    result->events = ParseEarthquakeEvents(body);
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    result->ok = false;
                    result->error = L"Earthquake parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->ok = false;
                result->error = L"Earthquake fetch failed: " + error;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            if (!PostMessageW(hwnd, WM_APP_EARTHQUAKE_READY, 0, reinterpret_cast<LPARAM>(result)))
                delete result;
            });
    }

    void OnEarthquakeReady(EarthquakeResult* result)
    {
        m_earthquakeFetchInProgress.store(false);
        if (!result)
            return;

        if (!result->ok) {
            if (m_earthquakeListWnd && IsWindowVisible(m_earthquakeListWnd))
                SetStatusText(result->error);
            delete result;
            return;
        }

        m_allEarthquakes = std::move(result->events);
        ApplyEarthquakeListFilters();
        if (result->notify)
            NotifyForMatchingEarthquakes(m_allEarthquakes);
        delete result;
    }

    void FetchWeatherSystemsAsync(bool notify)
    {
        if (g_weatherSystemsFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        ScheduleBackgroundTask([hwnd, notify]() {
            auto* result = new WeatherSystemsResult{};
            result->notify = notify;
            std::string body;
            std::wstring error;
            if (HttpGetText(kWeatherSystemsSourceUrl, body, error)) {
                try {
                    result->systems = ParseWeatherSystemEvents(body, result->statusText);
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    result->ok = false;
                    result->error = L"Weather systems parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->ok = false;
                result->error = L"Weather systems fetch failed: " + error;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            if (!PostMessageW(hwnd, WM_APP_WEATHER_READY, 0, reinterpret_cast<LPARAM>(result)))
                delete result;
            });
    }

    void OnWeatherSystemsReady(WeatherSystemsResult* result)
    {
        g_weatherSystemsFetchInProgress.store(false);
        if (!result)
            return;

        if (!result->ok) {
            if (m_weatherSystemsListWnd && IsWindowVisible(m_weatherSystemsListWnd))
                SetStatusText(result->error);
            delete result;
            return;
        }

        m_allWeatherSystems = std::move(result->systems);
        m_filteredWeatherSystems = m_allWeatherSystems;
        RenderWeatherSystemsListRows();
        ApplyWeatherSystemVisibility();
        if (result->notify)
            NotifyForMatchingWeatherSystems(m_allWeatherSystems);
        if (m_weatherSystemsListWnd && IsWindowVisible(m_weatherSystemsListWnd)) {
            std::wstring status = L"Showing " + std::to_wstring(m_filteredWeatherSystems.size()) + L" weather system(s).";
            if (!result->statusText.empty())
                status += L" " + result->statusText;
            SetStatusText(status);
        }
        delete result;
    }

    void ApplyEarthquakeVisibility()
    {
        m_map.SetEarthquakeOverlayVisible(m_showEarthquakes && m_showEarthquakeOverlayLabels);
        if (m_showEarthquakes)
            m_map.SetEarthquakes(m_filteredEarthquakes);
        else
            m_map.SetEarthquakes({});
    }

    void UpdateEarthquakeMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (menu) {
            CheckMenuItem(menu, IDM_SHOW_EARTHQUAKES, MF_BYCOMMAND | (m_showEarthquakes ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(menu, IDM_EARTHQUAKE_OVERLAY_NONE, MF_BYCOMMAND | (m_showEarthquakeOverlayLabels ? MF_UNCHECKED : MF_CHECKED));
            CheckMenuItem(menu, IDM_EARTHQUAKE_OVERLAY_MAG_REGION, MF_BYCOMMAND | (m_showEarthquakeOverlayLabels ? MF_CHECKED : MF_UNCHECKED));
            EnableMenuItem(menu, IDM_EARTHQUAKE_OVERLAY_NONE, MF_BYCOMMAND | (m_showEarthquakes ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(menu, IDM_EARTHQUAKE_OVERLAY_MAG_REGION, MF_BYCOMMAND | (m_showEarthquakes ? MF_ENABLED : MF_GRAYED));
        }
    }

    void UpdateWeatherSystemsMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (menu) {
            CheckMenuItem(menu, IDM_SHOW_WEATHER_SYSTEMS, MF_BYCOMMAND | (m_showWeatherSystems ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NONE, MF_BYCOMMAND | (m_showWeatherSystemOverlayLabels ? MF_UNCHECKED : MF_CHECKED));
            CheckMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND, MF_BYCOMMAND | (m_showWeatherSystemOverlayLabels ? MF_CHECKED : MF_UNCHECKED));
            EnableMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NONE, MF_BYCOMMAND | (m_showWeatherSystems ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND, MF_BYCOMMAND | (m_showWeatherSystems ? MF_ENABLED : MF_GRAYED));
        }
    }

    void UpdateNotificationHistoryMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (menu)
            CheckMenuItem(menu, IDM_VIEW_NOTIFICATION_HISTORY, MF_BYCOMMAND | (m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED));
    }

    void ToggleNotificationHistory()
    {
        m_showNotificationHistory = !m_showNotificationHistory;
        UpdateNotificationHistoryMenu();
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        RenderNotificationHistory();
        SaveSettings();
    }

    void ToggleShowEarthquakes()
    {
        m_showEarthquakes = !m_showEarthquakes;
        UpdateEarthquakeMenu();
        ApplyEarthquakeListFilters();
        SaveSettings();
        if (m_showEarthquakes && m_allEarthquakes.empty())
            FetchEarthquakesAsync(false);
    }

    void SetEarthquakeOverlayLabels(bool visible)
    {
        if (m_showEarthquakeOverlayLabels == visible)
            return;

        m_showEarthquakeOverlayLabels = visible;
        UpdateEarthquakeMenu();
        ApplyEarthquakeVisibility();
        SaveSettings();
    }

    void ApplyWeatherSystemVisibility()
    {
        m_map.SetWeatherSystemOverlayVisible(m_showWeatherSystems && m_showWeatherSystemOverlayLabels);
        if (m_showWeatherSystems)
            m_map.SetWeatherSystems(m_filteredWeatherSystems);
        else
            m_map.SetWeatherSystems({});
    }

    void ToggleShowWeatherSystems()
    {
        m_showWeatherSystems = !m_showWeatherSystems;
        UpdateWeatherSystemsMenu();
        ApplyWeatherSystemVisibility();
        SaveSettings();
        if (m_showWeatherSystems && m_allWeatherSystems.empty())
            FetchWeatherSystemsAsync(false);
    }

    void SetWeatherSystemOverlayLabels(bool visible)
    {
        if (m_showWeatherSystemOverlayLabels == visible)
            return;

        m_showWeatherSystemOverlayLabels = visible;
        UpdateWeatherSystemsMenu();
        ApplyWeatherSystemVisibility();
        SaveSettings();
    }

    bool EarthquakeMatchesNotification(const EarthquakeEvent& event) const
    {
        return event.magnitude + 0.0001 >= m_earthquakeNotificationMagnitude;
    }

    std::wstring EarthquakeNotificationLine(const EarthquakeEvent& event) const
    {
        std::wstring line = L"M";
        wchar_t mag[32]{};
        swprintf_s(mag, L"%.1f", event.magnitude);
        line += mag;
        if (!event.place.empty()) {
            line += L" - ";
            line += event.place;
        }
        if (!event.timeText.empty()) {
            line += L" (";
            line += event.timeText;
            line += L")";
        }
        return line;
    }

    std::wstring EarthquakeStableKey(const EarthquakeEvent& event) const
    {
        if (!event.id.empty())
            return event.id;
        std::wstring key = event.place;
        key += L"|";
        key += std::to_wstring(event.timeMs);
        return key;
    }

    std::wstring EarthquakeSignature(const EarthquakeEvent& event) const
    {
        std::wstring signature = std::to_wstring(event.timeMs);
        signature += L"|";
        signature += event.place;
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(event.magnitude * 100.0)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(event.latitude * 100000.0)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(event.longitude * 100000.0)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(event.depthKm * 10.0)));
        return signature;
    }

    void PublishEarthquakeNotificationBatch(
        const std::vector<std::wstring>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        if (lines.empty())
            return;

        std::wstring title;
        std::wstring body;
        if (lines.size() == 1) {
            title = singleTitle;
            body = lines.front();
        }
        else {
            title = std::to_wstring(lines.size()) + L" " + pluralSuffix;
            const size_t displayCount = MinValue<size_t>(lines.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += lines[i];
            }
            if (lines.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body);
    }

    void NotifyForMatchingEarthquakes(const std::vector<EarthquakeEvent>& events)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<std::wstring> newLines;
        std::vector<std::wstring> updateLines;
        std::vector<std::wstring> removedLines;

        for (const EarthquakeEvent& event : events) {
            std::wstring key = EarthquakeStableKey(event);
            currentKeys.insert(key);

            if (!EarthquakeMatchesNotification(event)) {
                auto existing = m_notifiedEarthquakeStates.find(key);
                if (m_haveEarthquakeNotificationSnapshot && existing != m_notifiedEarthquakeStates.end()) {
                    removedLines.push_back(existing->second.line);
                    m_notifiedEarthquakeStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = EarthquakeSignature(event);
            std::wstring line = EarthquakeNotificationLine(event);
            auto existing = m_notifiedEarthquakeStates.find(key);
            if (existing == m_notifiedEarthquakeStates.end()) {
                newLines.push_back(line);
                m_notifiedEarthquakeStates[key] = EarthquakeNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back(line);
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveEarthquakeNotificationSnapshot) {
            for (auto it = m_notifiedEarthquakeStates.begin(); it != m_notifiedEarthquakeStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back(it->second.line);
                    it = m_notifiedEarthquakeStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveEarthquakeNotificationSnapshot = true;

        PublishEarthquakeNotificationBatch(newLines, L"Earthquake notification", L"earthquake notifications");
        PublishEarthquakeNotificationBatch(updateLines, L"Earthquake update", L"earthquake updates");
        PublishEarthquakeNotificationBatch(removedLines, L"Earthquake removed", L"earthquake removals");
    }

    bool WeatherSystemMatchesNotification(const WeatherSystemEvent& system) const
    {
        return system.windKnots + 0.0001 >= m_weatherSystemNotificationWindKnots;
    }

    std::wstring WeatherSystemStableKey(const WeatherSystemEvent& system) const
    {
        if (!system.id.empty())
            return system.id;
        return system.name + L"|" + system.basin;
    }

    std::wstring WeatherSystemSignature(const WeatherSystemEvent& system) const
    {
        std::wstring signature = system.name;
        signature += L"|";
        signature += system.basin;
        signature += L"|";
        signature += system.category;
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.windKnots)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.latitude * 1000.0)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.longitude * 1000.0)));
        signature += L"|";
        signature += system.updatedText;
        return signature;
    }

    std::wstring WeatherSystemNotificationLine(const WeatherSystemEvent& system) const
    {
        std::wstring line = system.name.empty() ? L"Weather system" : system.name;
        if (!system.basin.empty()) {
            line += L" - ";
            line += system.basin;
        }
        if (!system.category.empty()) {
            line += L" ";
            line += system.category;
        }
        if (!system.windText.empty()) {
            line += L" ";
            line += system.windText;
        }
        return line;
    }

    void PublishWeatherSystemNotificationBatch(
        const std::vector<std::wstring>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        if (lines.empty())
            return;

        std::wstring title;
        std::wstring body;
        if (lines.size() == 1) {
            title = singleTitle;
            body = lines.front();
        }
        else {
            title = std::to_wstring(lines.size()) + L" " + pluralSuffix;
            const size_t displayCount = MinValue<size_t>(lines.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += lines[i];
            }
            if (lines.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body);
    }

    void NotifyForMatchingWeatherSystems(const std::vector<WeatherSystemEvent>& systems)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<std::wstring> newLines;
        std::vector<std::wstring> updateLines;
        std::vector<std::wstring> removedLines;

        for (const WeatherSystemEvent& system : systems) {
            std::wstring key = WeatherSystemStableKey(system);
            currentKeys.insert(key);

            if (!WeatherSystemMatchesNotification(system)) {
                auto existing = m_notifiedWeatherSystemStates.find(key);
                if (m_haveWeatherSystemNotificationSnapshot && existing != m_notifiedWeatherSystemStates.end()) {
                    removedLines.push_back(existing->second.line);
                    m_notifiedWeatherSystemStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = WeatherSystemSignature(system);
            std::wstring line = WeatherSystemNotificationLine(system);
            auto existing = m_notifiedWeatherSystemStates.find(key);
            if (existing == m_notifiedWeatherSystemStates.end()) {
                newLines.push_back(line);
                m_notifiedWeatherSystemStates[key] = WeatherSystemNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back(line);
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveWeatherSystemNotificationSnapshot) {
            for (auto it = m_notifiedWeatherSystemStates.begin(); it != m_notifiedWeatherSystemStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back(it->second.line);
                    it = m_notifiedWeatherSystemStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveWeatherSystemNotificationSnapshot = true;

        PublishWeatherSystemNotificationBatch(newLines, L"Weather system notification", L"weather system notifications");
        PublishWeatherSystemNotificationBatch(updateLines, L"Weather system update", L"weather system updates");
        PublishWeatherSystemNotificationBatch(removedLines, L"Weather system removed", L"weather system removals");
    }

    static LRESULT CALLBACK EarthquakeListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleEarthquakeListMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleEarthquakeListMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateEarthquakeListControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnEarthquakeListCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            if (m_polygonCaptureTarget == PolygonCaptureTarget::EarthquakeRegion)
                StopPolygonCapture();
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowEarthquakeListWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = EarthquakeListWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kEarthquakeListClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_earthquakeListWnd || !IsWindow(m_earthquakeListWnd)) {
            m_earthquakeListWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kEarthquakeListClassName,
                L"Earthquakes List",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                820,
                540,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        ApplyEarthquakeListFilters();
        ShowWindow(m_earthquakeListWnd, SW_SHOW);
        SetForegroundWindow(m_earthquakeListWnd);
        if (m_allEarthquakes.empty())
            FetchEarthquakesAsync(false);
    }

    void CreateEarthquakeListControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Earthquakes List", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Minimum magnitude", 18, 58);
        m_earthquakeListMagnitudeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 84, 120, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_MAG_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"After date/time", 160, 58);
        m_earthquakeListTimeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 160, 84, 170, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_TIME_EDIT), m_hInst, nullptr);
        m_earthquakeListRegionBtn = CreateWindowExW(0, L"BUTTON", L"Draw region", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 350, 80, 118, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_REGION_BTN), m_hInst, nullptr);
        m_earthquakeListClearRegionBtn = CreateWindowExW(0, L"BUTTON", L"Clear region", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 478, 80, 118, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 672, 80, 102, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_earthquakeListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 126, 756, 350, parent, ControlId(IDC_EARTHQUAKE_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { m_earthquakeListMagnitudeEdit, m_earthquakeListTimeEdit, m_earthquakeListRegionBtn, m_earthquakeListClearRegionBtn, closeBtn, m_earthquakeListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_earthquakeListMagnitudeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2.5"));
        SendMessageW(m_earthquakeListTimeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2026-05-14 09:00"));
        SetWindowTextSafe(m_earthquakeListMagnitudeEdit, m_earthquakeListMagnitudeText);
        SetWindowTextSafe(m_earthquakeListTimeEdit, m_earthquakeListTimeText);
        SendMessageW(m_earthquakeListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        std::wstring c0 = L"Mag";
        col.pszText = const_cast<LPWSTR>(c0.c_str());
        col.cx = 70;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&col));
        std::wstring c1 = L"Time";
        col.pszText = const_cast<LPWSTR>(c1.c_str());
        col.cx = 150;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 1, reinterpret_cast<LPARAM>(&col));
        std::wstring c2 = L"Region";
        col.pszText = const_cast<LPWSTR>(c2.c_str());
        col.cx = 420;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 2, reinterpret_cast<LPARAM>(&col));
        std::wstring c3 = L"Depth km";
        col.pszText = const_cast<LPWSTR>(c3.c_str());
        col.cx = 90;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 3, reinterpret_cast<LPARAM>(&col));
        AutoFitWindowToChildren(parent);
    }

    void StoreEarthquakeListFiltersFromControls()
    {
        if (m_earthquakeListMagnitudeEdit)
            m_earthquakeListMagnitudeText = Trim(GetWindowTextString(m_earthquakeListMagnitudeEdit));
        if (m_earthquakeListTimeEdit)
            m_earthquakeListTimeText = Trim(GetWindowTextString(m_earthquakeListTimeEdit));
    }

    bool EarthquakeMatchesListFilters(const EarthquakeEvent& event) const
    {
        double minMagnitude = 0.0;
        if (!m_earthquakeListMagnitudeText.empty() &&
            TryParseDoubleText(m_earthquakeListMagnitudeText, minMagnitude) &&
            event.magnitude + 0.0001 < minMagnitude)
        {
            return false;
        }

        long long afterMs = 0;
        if (!m_earthquakeListTimeText.empty() &&
            TryParseDateTimeFilter(m_earthquakeListTimeText, afterMs) &&
            event.timeMs < afterMs)
        {
            return false;
        }

        if (m_earthquakeFilterRegion.size() >= 3) {
            if (!event.hasLocation || !PointInPolygon(event.latitude, event.longitude, m_earthquakeFilterRegion))
                return false;
        }

        return true;
    }

    void RebuildFilteredEarthquakes()
    {
        m_filteredEarthquakes.clear();
        for (const EarthquakeEvent& event : m_allEarthquakes) {
            if (!EarthquakeMatchesListFilters(event))
                continue;
            m_filteredEarthquakes.push_back(event);
        }
    }

    void RenderEarthquakeListRows()
    {
        if (!m_earthquakeListView)
            return;

        SendMessageW(m_earthquakeListView, LVM_DELETEALLITEMS, 0, 0);
        int row = 0;
        for (const EarthquakeEvent& event : m_filteredEarthquakes) {
            wchar_t magText[32]{};
            swprintf_s(magText, L"%.1f", event.magnitude);
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = magText;
            int inserted = static_cast<int>(SendMessageW(m_earthquakeListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (inserted >= 0) {
                LVITEMW sub{};
                sub.iSubItem = 1;
                sub.pszText = const_cast<LPWSTR>(event.timeText.c_str());
                SendMessageW(m_earthquakeListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
                sub.iSubItem = 2;
                sub.pszText = const_cast<LPWSTR>(event.place.c_str());
                SendMessageW(m_earthquakeListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
                wchar_t depthText[32]{};
                swprintf_s(depthText, L"%.1f", event.depthKm);
                sub.iSubItem = 3;
                sub.pszText = depthText;
                SendMessageW(m_earthquakeListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
                ++row;
            }
        }
    }

    void ApplyEarthquakeListFilters()
    {
        StoreEarthquakeListFiltersFromControls();
        RebuildFilteredEarthquakes();
        RenderEarthquakeListRows();
        ApplyEarthquakeVisibility();
        SaveSettings();

        if (m_earthquakeListWnd && IsWindowVisible(m_earthquakeListWnd))
            SetStatusText(L"Showing " + std::to_wstring(m_filteredEarthquakes.size()) + L" earthquake(s).");
    }

    void OnEarthquakeListCommand(int id, int code)
    {
        if (id == IDC_EARTHQUAKE_LIST_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_earthquakeListWnd, SW_HIDE);
            if (m_polygonCaptureTarget == PolygonCaptureTarget::EarthquakeRegion)
                StopPolygonCapture();
            return;
        }
        if (id == IDC_EARTHQUAKE_LIST_REGION_BTN && code == BN_CLICKED) {
            m_earthquakeFilterRegion.clear();
            m_polygonCaptureTarget = PolygonCaptureTarget::EarthquakeRegion;
            m_activeIncidentRegionIndex = static_cast<size_t>(-1);
            m_map.SetActiveNotificationPolygonIndex(static_cast<size_t>(-1));
            m_map.SetDraftPolygon(m_earthquakeFilterRegion);
            m_map.SetPolygonCaptureActive(true);
            SetStatusText(L"Click the map to draw the earthquake filter region. Drag to pan.");
            return;
        }
        if (id == IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN && code == BN_CLICKED) {
            m_earthquakeFilterRegion.clear();
            if (m_polygonCaptureTarget == PolygonCaptureTarget::EarthquakeRegion)
                StopPolygonCapture();
            else
                m_map.SetDraftPolygon({});
            ApplyEarthquakeListFilters();
            return;
        }
        if ((id == IDC_EARTHQUAKE_LIST_MAG_EDIT || id == IDC_EARTHQUAKE_LIST_TIME_EDIT) &&
            (code == EN_CHANGE || code == EN_KILLFOCUS))
        {
            ApplyEarthquakeListFilters();
        }
    }

    static LRESULT CALLBACK EarthquakeNotificationsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleEarthquakeNotificationsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleEarthquakeNotificationsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateEarthquakeNotificationsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnEarthquakeNotificationsCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowEarthquakeNotificationsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = EarthquakeNotificationsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kEarthquakeNotificationsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_earthquakeNotificationsWnd || !IsWindow(m_earthquakeNotificationsWnd)) {
            m_earthquakeNotificationsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kEarthquakeNotificationsClassName,
                L"Earthquake Notifications",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                430,
                230,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        SyncEarthquakeNotificationControls();
        ShowWindow(m_earthquakeNotificationsWnd, SW_SHOW);
        SetForegroundWindow(m_earthquakeNotificationsWnd);
    }

    void CreateEarthquakeNotificationsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Earthquake Notifications", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Notify when an earthquake is at or above this magnitude.", 18, 58, nullptr, 360);
        CreateAutoLabel(parent, 0, L"Minimum magnitude", 18, 104);
        m_earthquakeNotificationMagnitudeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 130, 120, 26, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 292, 130, 102, 32, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);
        for (HWND h : { m_earthquakeNotificationMagnitudeEdit, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_earthquakeNotificationMagnitudeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"4.0"));
        SyncEarthquakeNotificationControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncEarthquakeNotificationControls()
    {
        m_syncingControls = true;
        if (m_earthquakeNotificationMagnitudeEdit)
            SetWindowTextSafe(m_earthquakeNotificationMagnitudeEdit, m_earthquakeNotificationMagnitudeText);
        m_syncingControls = false;
    }

    void OnEarthquakeNotificationsCommand(int id, int code)
    {
        if (m_syncingControls)
            return;
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_earthquakeNotificationsWnd, SW_HIDE);
            return;
        }
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_earthquakeNotificationMagnitudeEdit));
            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 0.0) {
                m_earthquakeNotificationMagnitudeText = text;
                m_earthquakeNotificationMagnitude = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_earthquakeNotificationMagnitudeEdit, m_earthquakeNotificationMagnitudeText);
                SetStatusText(L"Earthquake magnitude should be a number such as 4.0.");
            }
        }
    }

    static LRESULT CALLBACK WeatherSystemsListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleWeatherSystemsListMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleWeatherSystemsListMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateWeatherSystemsListControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnWeatherSystemsListCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowWeatherSystemsListWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = WeatherSystemsListWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kWeatherSystemsListClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_weatherSystemsListWnd || !IsWindow(m_weatherSystemsListWnd)) {
            m_weatherSystemsListWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kWeatherSystemsListClassName,
                L"Weather Systems List",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                500,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        RenderWeatherSystemsListRows();
        ShowWindow(m_weatherSystemsListWnd, SW_SHOW);
        SetForegroundWindow(m_weatherSystemsListWnd);
        if (m_allWeatherSystems.empty())
            FetchWeatherSystemsAsync(false);
    }

    void CreateWeatherSystemsListControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Weather Systems List", 18, 18, m_headerFont);
        HWND refreshBtn = CreateWindowExW(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 626, 54, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_REFRESH_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 742, 54, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_weatherSystemsListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 102, 826, 320, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { refreshBtn, closeBtn, m_weatherSystemsListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_weatherSystemsListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

        struct ColumnDef { const wchar_t* text; int width; };
        const ColumnDef columns[] = {
            { L"System", 140 },
            { L"Basin", 120 },
            { L"Lat", 70 },
            { L"Long", 80 },
            { L"Wind", 80 },
            { L"Cat", 70 },
            { L"24h Lat", 80 },
            { L"24h Long", 80 },
            { L"24h Wind", 90 },
            { L"24h Cat", 80 }
        };
        for (int i = 0; i < static_cast<int>(_countof(columns)); ++i) {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<LPWSTR>(columns[i].text);
            col.cx = columns[i].width;
            col.iSubItem = i;
            SendMessageW(m_weatherSystemsListView, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&col));
        }
        RenderWeatherSystemsListRows();
        AutoFitWindowToChildren(parent);
    }

    static std::wstring FormatCoordinateForList(double value, bool lat)
    {
        wchar_t buffer[48]{};
        const wchar_t hemi = lat ? (value < 0.0 ? L'S' : L'N') : (value < 0.0 ? L'W' : L'E');
        swprintf_s(buffer, L"%.1f %c", std::abs(value), hemi);
        return buffer;
    }

    void RenderWeatherSystemsListRows()
    {
        if (!m_weatherSystemsListView)
            return;

        SendMessageW(m_weatherSystemsListView, LVM_DELETEALLITEMS, 0, 0);
        int row = 0;
        for (const WeatherSystemEvent& system : m_filteredWeatherSystems) {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(system.name.c_str());
            int inserted = static_cast<int>(SendMessageW(m_weatherSystemsListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (inserted < 0)
                continue;

            std::wstring lat = system.hasLocation ? FormatCoordinateForList(system.latitude, true) : L"";
            std::wstring lon = system.hasLocation ? FormatCoordinateForList(system.longitude, false) : L"";
            std::wstring forecastLat = system.hasForecastLocation ? FormatCoordinateForList(system.forecastLatitude, true) : L"";
            std::wstring forecastLon = system.hasForecastLocation ? FormatCoordinateForList(system.forecastLongitude, false) : L"";
            const std::wstring values[] = {
                system.basin,
                lat,
                lon,
                system.windText,
                system.category,
                forecastLat,
                forecastLon,
                system.forecastWindText,
                system.forecastCategory
            };
            for (int i = 0; i < static_cast<int>(_countof(values)); ++i) {
                LVITEMW sub{};
                sub.iSubItem = i + 1;
                sub.pszText = const_cast<LPWSTR>(values[i].c_str());
                SendMessageW(m_weatherSystemsListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
            }
            ++row;
        }
    }

    void OnWeatherSystemsListCommand(int id, int code)
    {
        if (id == IDC_WEATHER_SYSTEMS_LIST_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_weatherSystemsListWnd, SW_HIDE);
            return;
        }
        if (id == IDC_WEATHER_SYSTEMS_LIST_REFRESH_BTN && code == BN_CLICKED) {
            FetchWeatherSystemsAsync(false);
            return;
        }
    }

    static LRESULT CALLBACK WeatherSystemNotificationsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleWeatherSystemNotificationsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleWeatherSystemNotificationsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateWeatherSystemNotificationsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnWeatherSystemNotificationsCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowWeatherSystemNotificationsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = WeatherSystemNotificationsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kWeatherSystemNotificationsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_weatherSystemNotificationsWnd || !IsWindow(m_weatherSystemNotificationsWnd)) {
            m_weatherSystemNotificationsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kWeatherSystemNotificationsClassName,
                L"Weather System Notifications",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                470,
                230,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        SyncWeatherSystemNotificationControls();
        ShowWindow(m_weatherSystemNotificationsWnd, SW_SHOW);
        SetForegroundWindow(m_weatherSystemNotificationsWnd);
    }

    void CreateWeatherSystemNotificationsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Weather System Notifications", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Notify when an active weather system is at or above this sustained wind speed.", 18, 58, nullptr, 400);
        CreateAutoLabel(parent, 0, L"Minimum wind (kts)", 18, 104);
        m_weatherSystemNotificationWindEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 130, 120, 26, parent, ControlId(IDC_WEATHER_SYSTEM_NOTIFICATIONS_WIND_EDIT), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 130, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEM_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);
        for (HWND h : { m_weatherSystemNotificationWindEdit, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_weatherSystemNotificationWindEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"34"));
        SyncWeatherSystemNotificationControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncWeatherSystemNotificationControls()
    {
        m_syncingControls = true;
        if (m_weatherSystemNotificationWindEdit)
            SetWindowTextSafe(m_weatherSystemNotificationWindEdit, m_weatherSystemNotificationWindText);
        m_syncingControls = false;
    }

    void OnWeatherSystemNotificationsCommand(int id, int code)
    {
        if (m_syncingControls)
            return;
        if (id == IDC_WEATHER_SYSTEM_NOTIFICATIONS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_weatherSystemNotificationsWnd, SW_HIDE);
            return;
        }
        if (id == IDC_WEATHER_SYSTEM_NOTIFICATIONS_WIND_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_weatherSystemNotificationWindEdit));
            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 0.0) {
                m_weatherSystemNotificationWindText = text;
                m_weatherSystemNotificationWindKnots = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_weatherSystemNotificationWindEdit, m_weatherSystemNotificationWindText);
                SetStatusText(L"Weather system wind should be a number such as 34.");
            }
        }
    }

    void SortAlertsForCurrentOrder()
    {
        std::wstring order = ToLower(Trim(m_alertOrder));
        std::sort(m_allAlerts.begin(), m_allAlerts.end(),
            [&order](const TrafficAlert& a, const TrafficAlert& b)
            {
                if (order == L"updated")
                    return ToLower(Trim(a.updatedText)) > ToLower(Trim(b.updatedText));
                if (order == L"severity") {
                    int sa = (SeverityBucket(a.severity) == L"severe") ? 0 : (SeverityBucket(a.severity) == L"moderate" ? 1 : (SeverityBucket(a.severity) == L"minor" ? 2 : 3));
                    int sb = (SeverityBucket(b.severity) == L"severe") ? 0 : (SeverityBucket(b.severity) == L"moderate" ? 1 : (SeverityBucket(b.severity) == L"minor" ? 2 : 3));
                    if (sa != sb) return sa < sb;
                }
                if (order == L"title")
                    return ToLower(Trim(a.title)) < ToLower(Trim(b.title));

                std::wstring ar = ToLower(Trim(a.road.empty() ? a.region : a.road));
                std::wstring br = ToLower(Trim(b.road.empty() ? b.region : b.road));

                if (ar != br)
                    return ar < br;

                return ToLower(Trim(a.title)) < ToLower(Trim(b.title));
            });
    }

    LRESULT OnDrawItem(DRAWITEMSTRUCT* dis)
    {
        if (!dis || dis->CtlType != ODT_BUTTON)
            return FALSE;

        bool pressed = (dis->itemState & ODS_SELECTED) != 0;
        bool hot = (dis->itemState & ODS_HOTLIGHT) != 0;
        HBRUSH bg = CreateSolidBrush(pressed ? RGB(21, 92, 171) : (hot ? RGB(32, 124, 229) : RGB(0, 103, 192)));
        FillRect(dis->hDC, &dis->rcItem, bg);
        DeleteObject(bg);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(88, 166, 255));
        HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
        HGDIOBJ oldBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 9, 9);
        SelectObject(dis->hDC, oldBrush);
        SelectObject(dis->hDC, oldPen);
        DeleteObject(pen);

        wchar_t text[128]{};
        GetWindowTextW(dis->hwndItem, text, _countof(text));
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, RGB(255, 255, 255));
        HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dis->hDC, m_font));
        RECT textRc = dis->rcItem;
        DrawTextW(dis->hDC, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dis->hDC, oldFont);
        return TRUE;
    }

    static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleSettingsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleSettingsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateSettingsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnSettingsCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowSettingsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = SettingsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kSettingsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_settingsWnd || !IsWindow(m_settingsWnd)) {
            m_settingsWnd = CreateWindowExW(WS_EX_TOOLWINDOW, kSettingsClassName, L"Settings", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT, 470, 690, m_hwnd, nullptr, m_hInst, this);
        }
        SyncSettingsControls();
        ShowWindow(m_settingsWnd, SW_SHOW);
        SetForegroundWindow(m_settingsWnd);
    }

    void CreateSettingsControls(HWND parent)
    {
        m_urlLabel = CreateAutoLabel(parent, IDC_SETTINGS_ENDPOINT_LABEL, L"Alerts endpoint", 18, 18);
        m_urlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 44, 410, 26, parent, ControlId(IDC_URL_EDIT), m_hInst, nullptr);
        m_serverLabel = CreateAutoLabel(parent, IDC_SETTINGS_SERVER_LABEL, L"Collaboration server", 18, 84);
        m_serverEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 110, 410, 26, parent, ControlId(IDC_SERVER_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_REFRESH_LABEL, L"Periodic alert refresh", 18, 150);
        m_settingsRefreshOffRadio = CreateWindowExW(0, L"BUTTON", L"Manual refresh only", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 18, 176, 145, 24, parent, ControlId(IDC_SETTINGS_REFRESH_OFF_RADIO), m_hInst, nullptr);
        m_settingsRefreshOnRadio = CreateWindowExW(0, L"BUTTON", L"Refresh every", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 178, 176, 120, 24, parent, ControlId(IDC_SETTINGS_REFRESH_ON_RADIO), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_REFRESH_INTERVAL_LABEL, L"Interval", 18, 214);
        m_settingsRefreshIntervalEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 178, 208, 120, 26, parent, ControlId(IDC_SETTINGS_REFRESH_INTERVAL_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_WORLD_LABEL, L"Map display", 18, 252);
        m_settingsWorldOffRadio = CreateWindowExW(0, L"BUTTON", L"UK depiction", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 18, 278, 130, 24, parent, ControlId(IDC_SETTINGS_WORLD_OFF_RADIO), m_hInst, nullptr);
        m_settingsWorldOnRadio = CreateWindowExW(0, L"BUTTON", L"Display rest of world", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 178, 278, 190, 24, parent, ControlId(IDC_SETTINGS_WORLD_ON_RADIO), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_SYNC_LABEL, L"Settings source", 18, 316);
        m_settingsSyncLocalRadio = CreateWindowExW(0, L"BUTTON", L"Local settings", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 18, 342, 130, 24, parent, ControlId(IDC_SETTINGS_SYNC_LOCAL_RADIO), m_hInst, nullptr);
        m_settingsSyncServerRadio = CreateWindowExW(0, L"BUTTON", L"Sync Settings", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 178, 342, 150, 24, parent, ControlId(IDC_SETTINGS_SYNC_SERVER_RADIO), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_FILTER_LABEL, L"Traffic England alert filter", 18, 380);
        m_settingsFilterCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 406, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_FILTER), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_ORDER_LABEL, L"Traffic England order", 18, 444);
        m_settingsOrderCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 470, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_ORDER), m_hInst, nullptr);
        HWND boundary = CreateWindowExW(0, L"BUTTON", L"Download / refresh UK boundary", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 516, 260, 32, parent, ControlId(IDC_SETTINGS_BOUNDARY_BTN), m_hInst, nullptr);
        m_settingsWorldBoundaryBtn = CreateWindowExW(0, L"BUTTON", L"Download / refresh World Boundaries", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 556, 310, 32, parent, ControlId(IDC_SETTINGS_WORLD_BOUNDARY_BTN), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 596, 102, 32, parent, ControlId(IDC_SETTINGS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_urlEdit, m_serverEdit, m_settingsRefreshOffRadio, m_settingsRefreshOnRadio, m_settingsRefreshIntervalEdit, m_settingsWorldOffRadio, m_settingsWorldOnRadio, m_settingsSyncLocalRadio, m_settingsSyncServerRadio, m_settingsFilterCombo, m_settingsOrderCombo, boundary, m_settingsWorldBoundaryBtn, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        const int radioY = 176;
        const int radioGap = 12;
        const int offRadioW = PreferredControlWidth(m_settingsRefreshOffRadio, 34, 160);
        const int offRadioH = PreferredControlHeight(m_settingsRefreshOffRadio, 6, 24, offRadioW);
        const int onRadioX = 18 + offRadioW + radioGap;
        const int onRadioW = PreferredControlWidth(m_settingsRefreshOnRadio, 34, 132);
        const int onRadioH = PreferredControlHeight(m_settingsRefreshOnRadio, 6, 24, onRadioW);
        MoveWindow(m_settingsRefreshOffRadio, 18, radioY, offRadioW, offRadioH, TRUE);
        MoveWindow(m_settingsRefreshOnRadio, onRadioX, radioY, onRadioW, onRadioH, TRUE);

        SendMessageW(m_urlEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"https://www.trafficengland.com/traffic-alerts"));
        SendMessageW(m_serverEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"213.254.181.35:8081"));
        SendMessageW(m_settingsRefreshIntervalEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"5s, 3s, 10s"));

        SendMessageW(m_settingsFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unplanned only"));
        SendMessageW(m_settingsFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All alerts"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Road"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Severity"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Updated"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Title"));
        SyncSettingsControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncSettingsControls()
    {
        m_syncingControls = true;
        if (m_urlEdit)
            SetWindowTextSafe(m_urlEdit, m_alertsEndpoint);
        if (m_serverEdit)
            SetWindowTextSafe(m_serverEdit, m_serverBaseUrl);
        if (m_settingsRefreshOffRadio)
            SendMessageW(m_settingsRefreshOffRadio, BM_SETCHECK, m_periodicRefreshEnabled ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_settingsRefreshOnRadio)
            SendMessageW(m_settingsRefreshOnRadio, BM_SETCHECK, m_periodicRefreshEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_settingsRefreshIntervalEdit) {
            SetWindowTextSafe(m_settingsRefreshIntervalEdit, m_refreshIntervalText);
            EnableWindow(m_settingsRefreshIntervalEdit, m_periodicRefreshEnabled);
        }
        if (m_settingsWorldOffRadio)
            SendMessageW(m_settingsWorldOffRadio, BM_SETCHECK, m_displayWorldMap ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_settingsWorldOnRadio)
            SendMessageW(m_settingsWorldOnRadio, BM_SETCHECK, m_displayWorldMap ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_settingsSyncLocalRadio)
            SendMessageW(m_settingsSyncLocalRadio, BM_SETCHECK, m_syncSettingsFromServer ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_settingsSyncServerRadio) {
            SendMessageW(m_settingsSyncServerRadio, BM_SETCHECK, m_syncSettingsFromServer ? BST_CHECKED : BST_UNCHECKED, 0);
            EnableWindow(m_settingsSyncServerRadio, IsOnlineMode());
        }
        if (m_settingsFilterCombo)
            SendMessageW(m_settingsFilterCombo, CB_SETCURSEL, m_alertFilterUnplannedOnly ? 0 : 1, 0);
        if (m_settingsOrderCombo) {
            int idx = 0;
            std::wstring order = ToLower(Trim(m_alertOrder));
            if (order == L"severity") idx = 1;
            else if (order == L"updated") idx = 2;
            else if (order == L"title") idx = 3;
            SendMessageW(m_settingsOrderCombo, CB_SETCURSEL, idx, 0);
        }
        m_syncingControls = false;
    }

    void OnSettingsCommand(int id, int code)
    {
        if (m_syncingControls)
            return;

        if (id == IDC_URL_EDIT && code == EN_CHANGE) {
            m_alertsEndpoint = NormalizeUrl(GetWindowTextString(m_urlEdit));
            SaveSettings();
        }
        else if (id == IDC_SERVER_EDIT && code == EN_CHANGE) {
            m_serverBaseUrl = NormalizeUrl(GetWindowTextString(m_serverEdit));
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_REFRESH_OFF_RADIO && code == BN_CLICKED) {
            m_periodicRefreshEnabled = false;
            ApplyRefreshTimer();
            SyncSettingsControls();
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_REFRESH_ON_RADIO && code == BN_CLICKED) {
            m_periodicRefreshEnabled = true;
            UINT parsedMs = 0;
            if (TryParseRefreshIntervalMilliseconds(GetWindowTextString(m_settingsRefreshIntervalEdit), parsedMs)) {
                m_refreshIntervalText = Trim(GetWindowTextString(m_settingsRefreshIntervalEdit));
                m_refreshIntervalMs = parsedMs;
            }
            ApplyRefreshTimer();
            SyncSettingsControls();
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_REFRESH_INTERVAL_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            UINT parsedMs = 0;
            std::wstring intervalText = Trim(GetWindowTextString(m_settingsRefreshIntervalEdit));
            if (TryParseRefreshIntervalMilliseconds(intervalText, parsedMs)) {
                m_refreshIntervalText = intervalText;
                m_refreshIntervalMs = parsedMs;
                if (m_periodicRefreshEnabled)
                    ApplyRefreshTimer();
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_settingsRefreshIntervalEdit, m_refreshIntervalText);
                SetStatusText(L"Refresh interval must be at least 1 second, e.g. 5s, 3s, or 10s.");
            }
        }
        else if (id == IDC_SETTINGS_WORLD_OFF_RADIO && code == BN_CLICKED) {
            m_displayWorldMap = false;
            m_map.SetDisplayWorldMap(m_displayWorldMap);
            SyncSettingsControls();
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_WORLD_ON_RADIO && code == BN_CLICKED) {
            m_displayWorldMap = true;
            m_map.SetDisplayWorldMap(m_displayWorldMap);
            SyncSettingsControls();
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_SYNC_LOCAL_RADIO && code == BN_CLICKED) {
            m_syncSettingsFromServer = false;
            SyncSettingsControls();
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_SYNC_SERVER_RADIO && code == BN_CLICKED) {
            m_syncSettingsFromServer = true;
            SyncSettingsControls();
            SaveSettings();
            SyncGlobalSettingsFromServerAsync();
        }
        else if (id == IDC_SETTINGS_ALERT_FILTER && code == CBN_SELCHANGE) {
            m_alertFilterUnplannedOnly = SendMessageW(m_settingsFilterCombo, CB_GETCURSEL, 0, 0) == 0;
            SaveSettings();
            RefreshFeedAsync();
        }
        else if (id == IDC_SETTINGS_ALERT_ORDER && code == CBN_SELCHANGE) {
            int idx = static_cast<int>(SendMessageW(m_settingsOrderCombo, CB_GETCURSEL, 0, 0));
            const wchar_t* orders[] = { L"Road", L"Severity", L"Updated", L"Title" };
            m_alertOrder = orders[ClampValue(idx, 0, 3)];
            SortAlertsForCurrentOrder();
            ApplyFilters(true);
            SaveSettings();
        }
        else if (id == IDC_SETTINGS_BOUNDARY_BTN && code == BN_CLICKED) {
            DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind::Uk);
        }
        else if (id == IDC_SETTINGS_WORLD_BOUNDARY_BTN && code == BN_CLICKED) {
            DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind::World);
        }
        else if (id == IDC_SETTINGS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_settingsWnd, SW_HIDE);
        }
    }

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInst = nullptr;
    HFONT m_font = nullptr;
    HFONT m_headerFont = nullptr;

    HWND m_urlLabel = nullptr;
    HWND m_searchLabel = nullptr;
    HWND m_severityLabel = nullptr;
    HWND m_statusBar = nullptr;
    HWND m_serverLabel = nullptr;
    HWND m_panelTabBtn = nullptr;
    HWND m_urlEdit = nullptr;
    HWND m_serverEdit = nullptr;
    HWND m_searchEdit = nullptr;
    HWND m_severityCombo = nullptr;
    HWND m_listView = nullptr;
    HWND m_detailsEdit = nullptr;
    HWND m_settingsWnd = nullptr;
    HWND m_incidentFiltersWnd = nullptr;
    HWND m_incidentNotificationsWnd = nullptr;
    HWND m_notificationRegionsWnd = nullptr;
    HWND m_earthquakeListWnd = nullptr;
    HWND m_earthquakeNotificationsWnd = nullptr;
    HWND m_templatesWizardWnd = nullptr;
    HWND m_templatesEditorWnd = nullptr;
    HWND m_settingsFilterCombo = nullptr;
    HWND m_settingsOrderCombo = nullptr;
    HWND m_settingsRefreshOffRadio = nullptr;
    HWND m_settingsRefreshOnRadio = nullptr;
    HWND m_settingsRefreshIntervalEdit = nullptr;
    HWND m_settingsWorldOffRadio = nullptr;
    HWND m_settingsWorldOnRadio = nullptr;
    HWND m_settingsSyncLocalRadio = nullptr;
    HWND m_settingsSyncServerRadio = nullptr;
    HWND m_settingsWorldBoundaryBtn = nullptr;
    HWND m_incidentSevereCheck = nullptr;
    HWND m_incidentModerateCheck = nullptr;
    HWND m_incidentMinorCheck = nullptr;
    HWND m_incidentUnknownCheck = nullptr;
    HWND m_incidentUnplannedCheck = nullptr;
    HWND m_incidentPlannedCheck = nullptr;
    HWND m_incidentNotifyRoadsEdit = nullptr;
    HWND m_incidentNotifyRoadExclusionsEdit = nullptr;
    HWND m_incidentNotifyLaneThresholdEdit = nullptr;
    HWND m_incidentNotifyDelayThresholdEdit = nullptr;
    HWND m_incidentNotifyAndRadio = nullptr;
    HWND m_incidentNotifyOrRadio = nullptr;
    HWND m_incidentNotifyRegionsBtn = nullptr;
    HWND m_incidentNotifyReasonExclusionsEdit = nullptr;
    HWND m_incidentNotifyLocationExclusionsEdit = nullptr;
    HWND m_notificationRegionsList = nullptr;
    HWND m_earthquakeListMagnitudeEdit = nullptr;
    HWND m_earthquakeListTimeEdit = nullptr;
    HWND m_earthquakeListRegionBtn = nullptr;
    HWND m_earthquakeListClearRegionBtn = nullptr;
    HWND m_earthquakeListView = nullptr;
    HWND m_earthquakeNotificationMagnitudeEdit = nullptr;
    HWND m_weatherSystemsListWnd = nullptr;
    HWND m_weatherSystemNotificationsWnd = nullptr;
    HWND m_weatherSystemsListView = nullptr;
    HWND m_weatherSystemNotificationWindEdit = nullptr;
    HWND m_templateWizardDesc = nullptr;
    HWND m_templateWizardList = nullptr;
    HWND m_templateWizardVariablesEdit = nullptr;
    HWND m_templateWizardPreviewEdit = nullptr;
    HWND m_templateWizardPrevBtn = nullptr;
    HWND m_templateWizardNextBtn = nullptr;
    HWND m_templateWizardCopyBtn = nullptr;
    HWND m_templateWizardCopyLocationBtn = nullptr;
    HWND m_templateEditorList = nullptr;
    HWND m_templateEditorNameEdit = nullptr;
    HWND m_templateEditorBodyEdit = nullptr;

    MapView m_map;
    ClientSession m_session;

    std::vector<TrafficAlert> m_allAlerts;
    std::vector<TrafficAlert> m_filteredAlerts;
    std::vector<ChatMessage> m_chatMessages;
    std::vector<MapNote> m_notes;
    std::unordered_map<std::wstring, MapNote> m_pendingNoteEdits;
    std::vector<AppNotification> m_notificationHistory;
    std::vector<GeoPolygon> m_incidentNotificationRegions;
    std::vector<ReportTemplate> m_reportTemplates;
    std::vector<ReportTemplate> m_earthquakeReportTemplates;
    std::vector<ReportTemplate> m_weatherSystemReportTemplates;
    std::vector<std::pair<std::wstring, std::wstring>> m_templateWizardVariables;
    std::vector<EarthquakeEvent> m_allEarthquakes;
    std::vector<EarthquakeEvent> m_filteredEarthquakes;
    std::vector<WeatherSystemEvent> m_allWeatherSystems;
    std::vector<WeatherSystemEvent> m_filteredWeatherSystems;
    std::vector<GeoPoint> m_earthquakeFilterRegion;
    std::wstring m_selectedId;
    std::wstring m_templateWizardAlertId;
    std::wstring m_templateWizardEarthquakeId;
    std::wstring m_templateWizardWeatherSystemId;
    size_t m_templateWizardStep = 0;
    size_t m_templateWizardTemplateIndex = 0;
    TemplateContext m_templateWizardContext = TemplateContext::Roads;
    TemplateContext m_templateEditorContext = TemplateContext::Roads;
    bool m_programmaticSelection = false;
    bool m_syncingControls = false;
    bool m_isSidePanelVisible = true;
    bool m_showNotificationHistory = false;
    bool m_alertFilterUnplannedOnly = true;
    bool m_incidentFilterSevere = true;
    bool m_incidentFilterModerate = true;
    bool m_incidentFilterMinor = true;
    bool m_incidentFilterUnknown = true;
    bool m_incidentFilterUnplanned = true;
    bool m_incidentFilterPlanned = true;
    std::wstring m_incidentNotifyRoads = L"M*, A*, A1(M), A2, A15, A16, A17, A20, A4, A52";
    std::wstring m_incidentNotifyRoadExclusions;
    std::wstring m_incidentNotifyLaneThresholdText = L"50%";
    double m_incidentNotifyLaneThreshold = 50.0;
    std::wstring m_incidentNotifyDelayThresholdText = L"1 hour";
    double m_incidentNotifyDelayThresholdMinutes = 60.0;
    bool m_incidentNotifyThresholdUseOr = false;
    std::wstring m_incidentNotifyReasonExclusions = L"Road Management";
    std::wstring m_incidentNotifyLocationExclusions = L"entry, exit";
    bool m_showEarthquakes = false;
    bool m_showEarthquakeOverlayLabels = false;
    bool m_showWeatherSystems = false;
    bool m_showWeatherSystemOverlayLabels = false;
    bool m_displayWorldMap = false;
    bool m_syncSettingsFromServer = false;
    std::wstring m_earthquakeListMagnitudeText;
    std::wstring m_earthquakeListTimeText;
    std::wstring m_earthquakeNotificationMagnitudeText = L"4.0";
    double m_earthquakeNotificationMagnitude = 4.0;
    std::wstring m_weatherSystemNotificationWindText = L"34";
    double m_weatherSystemNotificationWindKnots = 34.0;
    std::wstring m_alertOrder = L"Road";
    std::wstring m_alertsEndpoint = L"https://www.trafficengland.com/traffic-alerts";
    std::wstring m_serverBaseUrl = L"http://213.254.181.35:8081";
    bool m_periodicRefreshEnabled = true;
    bool m_hasLoadedAlerts = false;
    std::wstring m_refreshIntervalText = L"300s";
    UINT m_refreshIntervalMs = 5 * 60 * 1000;
    std::atomic_bool m_serverRequestInProgress{ false };
    std::atomic_bool m_earthquakeFetchInProgress{ false };
    bool m_notificationIconAdded = false;
    bool m_haveIncidentNotificationSnapshot = false;
    bool m_haveEarthquakeNotificationSnapshot = false;
    bool m_haveWeatherSystemNotificationSnapshot = false;
    bool m_reportTemplatesConfigured = false;
    bool m_earthquakeReportTemplatesConfigured = false;
    bool m_weatherSystemReportTemplatesConfigured = false;
    std::unordered_map<std::wstring, IncidentNotificationState> m_notifiedIncidentStates;
    std::unordered_map<std::wstring, EarthquakeNotificationState> m_notifiedEarthquakeStates;
    std::unordered_map<std::wstring, WeatherSystemNotificationState> m_notifiedWeatherSystemStates;
    PolygonCaptureTarget m_polygonCaptureTarget = PolygonCaptureTarget::None;
    size_t m_activeIncidentRegionIndex = static_cast<size_t>(-1);
    std::unordered_set<std::wstring> m_deletedNoteIds;
};


int RunMainWindow(HINSTANCE hInstance, int nCmdShow, const ClientSession& session)
{
    MainWindow win(session);
    if (!win.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"ERC Tools", MB_ICONERROR);
        return 0;
    }

    return win.Run(nCmdShow);
}
