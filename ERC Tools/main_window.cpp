// =================================================================================
// FILE: main_window.cpp
// =================================================================================


#include "main_window.h"
#include "app_state.h"
#include "http.h"
#include "map_view.h"
#include "parsing.h"
#include "util.h"


constexpr int IDC_URL_EDIT = 1001;
constexpr int IDC_REFRESH_BTN = 1002;
constexpr int IDC_SEARCH_EDIT = 1003;
constexpr int IDC_SEVERITY_COMBO = 1004;
constexpr int IDC_LISTVIEW = 1005;
constexpr int IDC_DETAILS_EDIT = 1006;
constexpr int IDC_HEADER_LABEL = 1008;
constexpr int IDC_SEARCH_LABEL = 1010;
constexpr int IDC_SEVERITY_LABEL = 1011;
constexpr int IDC_STATUS_BAR = 1012;
constexpr int IDC_SERVER_EDIT = 1013;
constexpr int IDC_CHAT_HISTORY = 1015;
constexpr int IDC_CHAT_EDIT = 1016;
constexpr int IDC_CHAT_SEND_BTN = 1017;
constexpr int IDC_NOTE_EDIT = 1018;
constexpr int IDC_NOTE_BTN = 1019;
constexpr int IDC_NOTE_LABEL = 1020;
constexpr int IDC_PANEL_TAB_BTN = 1021;
constexpr int IDC_INAPP_NOTIFICATION = 1022;
constexpr int IDM_FILE_SETTINGS = 2001;
constexpr int IDM_FILE_EXIT = 2002;
constexpr int IDM_ABOUT = 2003;
constexpr int IDM_ROADS_INCIDENT_FILTERS = 2004;
constexpr int IDM_ROADS_INCIDENT_NOTIFICATIONS = 2005;
constexpr int IDM_EARTHQUAKES_LIST = 2006;
constexpr int IDM_EARTHQUAKE_NOTIFICATIONS = 2007;
constexpr int IDM_SHOW_EARTHQUAKES = 2008;
constexpr int IDM_VIEW_NOTIFICATION_HISTORY = 2009;
constexpr int IDC_NOTIFICATION_HISTORY_OVERLAY = 1023;
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
constexpr int IDC_EARTHQUAKE_LIST_MAG_EDIT = 2501;
constexpr int IDC_EARTHQUAKE_LIST_TIME_EDIT = 2502;
constexpr int IDC_EARTHQUAKE_LIST_REGION_BTN = 2503;
constexpr int IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN = 2504;
constexpr int IDC_EARTHQUAKE_LIST_LISTVIEW = 2505;
constexpr int IDC_EARTHQUAKE_LIST_CLOSE_BTN = 2506;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT = 2521;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN = 2522;
constexpr const wchar_t* kSettingsClassName = L"TrafficEnglandSettingsWindow";
constexpr const wchar_t* kIncidentFiltersClassName = L"TrafficEnglandIncidentFiltersWindow";
constexpr const wchar_t* kIncidentNotificationsClassName = L"TrafficEnglandIncidentNotificationsWindow";
constexpr const wchar_t* kNotificationRegionsClassName = L"TrafficEnglandNotificationRegionsWindow";
constexpr const wchar_t* kNotificationRegionEditorClassName = L"TrafficEnglandNotificationRegionEditorWindow";
constexpr const wchar_t* kEarthquakeListClassName = L"TrafficEnglandEarthquakeListWindow";
constexpr const wchar_t* kEarthquakeNotificationsClassName = L"TrafficEnglandEarthquakeNotificationsWindow";
constexpr UINT WM_APP_NOTIFY_ICON = WM_APP + 20;
constexpr UINT kNotificationIconId = 1;
constexpr UINT_PTR kAlertRefreshTimerId = 1;
constexpr UINT_PTR kServerPollTimerId = 2;
constexpr UINT_PTR kInAppNotificationTimerId = 3;
constexpr UINT_PTR kEarthquakeRefreshTimerId = 4;

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

struct EarthquakeResult
{
    bool ok = false;
    bool notify = false;
    std::wstring error;
    std::vector<EarthquakeEvent> events;
};

struct NotificationHistoryEntry
{
    std::wstring title;
    std::wstring body;
    std::wstring timestamp;
};

enum class ServerAction
{
    Poll,
    SendChat,
    SendNote
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
            if (wParam == kAlertRefreshTimerId)
                RefreshFeedAsync();
            else if (wParam == kServerPollTimerId)
                PollServerAsync();
            else if (wParam == kEarthquakeRefreshTimerId)
                FetchEarthquakesAsync(true);
            else if (wParam == kInAppNotificationTimerId) {
                KillTimer(m_hwnd, kInAppNotificationTimerId);
                if (m_inAppNotification)
                    ShowWindow(m_inAppNotification, SW_HIDE);
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

        case WM_DESTROY:
            g_appQuitting.store(true);
            SaveSettings();
            RemoveNotificationIcon();
            KillTimer(m_hwnd, kAlertRefreshTimerId);
            KillTimer(m_hwnd, kServerPollTimerId);
            KillTimer(m_hwnd, kInAppNotificationTimerId);
            KillTimer(m_hwnd, kEarthquakeRefreshTimerId);
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

        LoadSettings();
        CreateMainMenu();

        m_headerLabel = CreateAutoLabel(m_hwnd, IDC_HEADER_LABEL, L"Traffic England Alerts", 0, 0, m_headerFont);
        m_searchLabel = CreateAutoLabel(m_hwnd, IDC_SEARCH_LABEL, L"Search");
        m_severityLabel = CreateAutoLabel(m_hwnd, IDC_SEVERITY_LABEL, L"Severity");
        m_noteLabel = CreateAutoLabel(m_hwnd, IDC_NOTE_LABEL, L"Map note (double-click map to choose a location)");

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

        m_refreshBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_REFRESH_BTN),
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

        m_chatHistory = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0, m_hwnd, ControlId(IDC_CHAT_HISTORY), m_hInst, nullptr);

        m_chatEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, m_hwnd, ControlId(IDC_CHAT_EDIT), m_hInst, nullptr);

        m_chatSendBtn = CreateWindowExW(
            0, L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
            0, 0, 0, 0, m_hwnd, ControlId(IDC_CHAT_SEND_BTN), m_hInst, nullptr);

        m_noteEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, m_hwnd, ControlId(IDC_NOTE_EDIT), m_hInst, nullptr);

        m_noteBtn = CreateWindowExW(
            0, L"BUTTON", L"Leave note", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            0, 0, 0, 0, m_hwnd, ControlId(IDC_NOTE_BTN), m_hInst, nullptr);

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

        m_inAppNotification = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"STATIC",
            L"",
            WS_CHILD | SS_LEFT | SS_NOPREFIX,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_INAPP_NOTIFICATION),
            m_hInst,
            nullptr);

        m_notificationHistoryOverlay = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            ControlId(IDC_NOTIFICATION_HISTORY_OVERLAY),
            m_hInst,
            nullptr);

        if (!m_headerLabel || !m_searchLabel || !m_severityLabel ||
            !m_refreshBtn || !m_panelTabBtn || !m_searchEdit || !m_severityCombo || !m_listView || !m_detailsEdit || !m_chatHistory || !m_chatEdit || !m_chatSendBtn || !m_noteLabel || !m_noteEdit || !m_noteBtn || !m_statusBar || !m_inAppNotification || !m_notificationHistoryOverlay)
        {
            MessageBoxW(m_hwnd, L"Failed to create one or more child controls.", L"Traffic England Alerts Map", MB_ICONERROR);
            return;
        }

        for (HWND h : { m_panelTabBtn, m_refreshBtn, m_searchEdit, m_severityCombo, m_listView, m_detailsEdit, m_chatHistory, m_chatEdit, m_chatSendBtn, m_noteEdit, m_noteBtn, m_statusBar, m_inAppNotification, m_notificationHistoryOverlay }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        ShowWindow(m_inAppNotification, SW_HIDE);
        ShowWindow(m_notificationHistoryOverlay, m_showNotificationHistory ? SW_SHOW : SW_HIDE);
        RenderNotificationHistory();

        SendMessageW(m_searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter by road, region, or description"));
        SendMessageW(m_chatEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Message local responders..."));
        SendMessageW(m_noteEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Note text for the selected map location"));

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
        m_map.SetNoteLocationCallback([this](double lat, double lon) {
            m_pendingNoteLat = lat;
            m_pendingNoteLon = lon;
            m_hasPendingNoteLocation = true;
            SetStatusText(L"Note location selected: " + std::to_wstring(lat) + L", " + std::to_wstring(lon));
            SetFocus(m_noteEdit);
            });
        m_map.SetPolygonPointCallback([this](double lat, double lon) {
            OnMapPolygonPoint(lat, lon);
            });
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);

        Layout();
        SetStatusText(L"Ready.");
        ApplyRefreshTimer();
        SetTimer(m_hwnd, kServerPollTimerId, 8 * 1000, nullptr);
        SetTimer(m_hwnd, kEarthquakeRefreshTimerId, 10 * 60 * 1000, nullptr);

        RefreshFeedAsync();
        FetchEarthquakesAsync(true);
        PollServerAsync();
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

        const LONG refreshW = 132;
        const int topY = 12;
        const LONG headerMaxW = MaxLong(200L, width - refreshW - static_cast<LONG>(pad * 3));
        const LONG headerW = PreferredControlWidth(m_headerLabel, 8, 200, static_cast<int>(headerMaxW));
        const int headerH = PreferredControlHeight(m_headerLabel, 8, 28, static_cast<int>(headerW));
        MoveWindow(m_headerLabel, pad, topY, headerW, headerH, TRUE);
        MoveWindow(m_refreshBtn, width - refreshW - pad, topY, refreshW, controlH, TRUE);

        const int topBarH = topY + MaxInt(headerH, controlH) + 4;
        int bodyTop = topBarH;
        int leftW = m_isSidePanelVisible ? 440 : 0;
        int detailsH = 185;
        int chatH = 154;

        int leftX = pad;
        int leftY = bodyTop + pad;
        int leftInnerW = MaxInt(10, leftW - pad * 2);

        const int panelShow = m_isSidePanelVisible ? SW_SHOW : SW_HIDE;
        for (HWND h : { m_searchLabel, m_searchEdit, m_severityLabel, m_severityCombo, m_listView, m_detailsEdit, m_chatHistory, m_chatEdit, m_chatSendBtn })
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
            int listHeight = MaxInt(110, bodyHeight - (listTop - leftY) - detailsH - chatH - 58);

            MoveWindow(m_listView, leftX, listTop, leftInnerW, listHeight, TRUE);
            int detailsTop = listTop + listHeight + 10;
            MoveWindow(m_detailsEdit, leftX, detailsTop, leftInnerW, detailsH, TRUE);
            int chatTop = detailsTop + detailsH + 10;
            MoveWindow(m_chatHistory, leftX, chatTop, leftInnerW, chatH - controlH - 8, TRUE);
            MoveWindow(m_chatEdit, leftX, chatTop + chatH - controlH, leftInnerW - 72, controlH, TRUE);
            MoveWindow(m_chatSendBtn, leftX + leftInnerW - 66, chatTop + chatH - controlH, 66, controlH, TRUE);
        }

        int mapX = (m_isSidePanelVisible ? leftW + pad : pad);
        int mapY = bodyTop + pad;
        LONG mapW = MaxLong(100L, width - mapX - pad);
        const int noteLabelW = AutoLabelWidth(m_noteLabel, static_cast<int>(mapW));
        const int noteLabelH = AutoLabelHeight(m_noteLabel, labelH, noteLabelW);
        const int noteAreaH = noteLabelH + 2 + controlH + 8;
        LONG mapH = MaxLong(100L, height - mapY - statusH - pad - noteAreaH);

        MoveWindow(m_map.Hwnd(), mapX, mapY, mapW, mapH, TRUE);
        int historyW = 0;
        if (m_notificationHistoryOverlay) {
            const int notificationMargin = 12;
            historyW = m_showNotificationHistory
                ? MinInt(380, MaxInt(260, static_cast<int>(mapW * 0.30)))
                : 0;
            historyW = MinInt(historyW, MaxInt(160, static_cast<int>(mapW) - notificationMargin * 2));
            if (m_showNotificationHistory) {
                MoveWindow(
                    m_notificationHistoryOverlay,
                    static_cast<int>(mapX + mapW - historyW - notificationMargin),
                    mapY + notificationMargin,
                    historyW,
                    MaxInt(120, static_cast<int>(mapH) - notificationMargin * 2),
                    TRUE);
                ShowWindow(m_notificationHistoryOverlay, SW_SHOW);
                SetWindowPos(m_notificationHistoryOverlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            else {
                ShowWindow(m_notificationHistoryOverlay, SW_HIDE);
            }
        }
        if (m_inAppNotification) {
            const int notificationMargin = 12;
            int notificationW = MaxInt(180, static_cast<int>(mapW) - notificationMargin * 2);
            if (m_showNotificationHistory && historyW > 0)
                notificationW = MaxInt(180, notificationW - historyW - notificationMargin);
            int notificationH = PreferredControlHeight(m_inAppNotification, 14, 44, notificationW);
            notificationH = ClampValue(notificationH, 44, MaxInt(44, static_cast<int>(mapH) / 3));
            MoveWindow(
                m_inAppNotification,
                mapX + notificationMargin,
                mapY + notificationMargin,
                notificationW,
                notificationH,
                TRUE);
        }
        int noteY = mapY + mapH + 8;
        MoveLabelToText(m_noteLabel, mapX, noteY, static_cast<int>(mapW));
        MoveWindow(m_noteEdit, mapX, noteY + noteLabelH + 2, MaxLong(180L, mapW - 132), controlH, TRUE);
        MoveWindow(m_noteBtn, mapX + mapW - 122, noteY + noteLabelH + 2, 122, controlH, TRUE);

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

        case IDM_EARTHQUAKES_LIST:
            ShowEarthquakeListWindow();
            break;

        case IDM_EARTHQUAKE_NOTIFICATIONS:
            ShowEarthquakeNotificationsWindow();
            break;

        case IDM_SHOW_EARTHQUAKES:
            ToggleShowEarthquakes();
            break;

        case IDM_VIEW_NOTIFICATION_HISTORY:
            ToggleNotificationHistory();
            break;

        case IDM_ABOUT:
            ShowAboutDialog();
            break;

        case IDC_CHAT_SEND_BTN:
            if (code == BN_CLICKED)
                SendChatAsync();
            break;

        case IDC_NOTE_BTN:
            if (code == BN_CLICKED)
                SendNoteAsync();
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
            readString("alertOrder", m_alertOrder);
            readBool("alertFilterUnplannedOnly", m_alertFilterUnplannedOnly);
            readBool("periodicRefreshEnabled", m_periodicRefreshEnabled);
            readBool("showNotificationHistory", m_showNotificationHistory);
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
            auto regionsIt = settings->find("incidentNotificationRegions");
            if (regionsIt != settings->end() && regionsIt->is_array()) {
                m_incidentNotificationRegions.clear();
                for (const json& item : *regionsIt) {
                    GeoPolygon polygon = GeoPolygonFromJson(item);
                    if (!polygon.name.empty() || !polygon.points.empty())
                        m_incidentNotificationRegions.push_back(std::move(polygon));
                }
            }

            readBool("showEarthquakes", m_showEarthquakes);
            readString("earthquakeNotificationMagnitudeText", m_earthquakeNotificationMagnitudeText);
            readDouble("earthquakeNotificationMagnitude", m_earthquakeNotificationMagnitude);
            double parsedMag = 0.0;
            if (TryParseDoubleText(m_earthquakeNotificationMagnitudeText, parsedMag))
                m_earthquakeNotificationMagnitude = parsedMag;
        }
        catch (...) {
            OutputDebugStringW(L"Settings file could not be parsed; using defaults.\n");
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
            settings["refreshIntervalText"] = WideToUtf8(m_refreshIntervalText);
            settings["refreshIntervalMs"] = m_refreshIntervalMs;
            settings["incidentFilterSevere"] = m_incidentFilterSevere;
            settings["incidentFilterModerate"] = m_incidentFilterModerate;
            settings["incidentFilterMinor"] = m_incidentFilterMinor;
            settings["incidentFilterUnknown"] = m_incidentFilterUnknown;
            settings["incidentFilterUnplanned"] = m_incidentFilterUnplanned;
            settings["incidentFilterPlanned"] = m_incidentFilterPlanned;
            settings["incidentNotifyRoads"] = WideToUtf8(m_incidentNotifyRoads);
            settings["incidentNotifyLaneThresholdText"] = WideToUtf8(m_incidentNotifyLaneThresholdText);
            settings["incidentNotifyLaneThreshold"] = m_incidentNotifyLaneThreshold;
            settings["incidentNotifyDelayThresholdText"] = WideToUtf8(m_incidentNotifyDelayThresholdText);
            settings["incidentNotifyDelayThresholdMinutes"] = m_incidentNotifyDelayThresholdMinutes;
            settings["incidentNotifyThresholdUseOr"] = m_incidentNotifyThresholdUseOr;
            settings["incidentNotifyReasonExclusions"] = WideToUtf8(m_incidentNotifyReasonExclusions);
            settings["incidentNotificationRegions"] = json::array();
            for (const GeoPolygon& polygon : m_incidentNotificationRegions)
                settings["incidentNotificationRegions"].push_back(GeoPolygonToJson(polygon));
            settings["showEarthquakes"] = m_showEarthquakes;
            settings["earthquakeNotificationMagnitudeText"] = WideToUtf8(m_earthquakeNotificationMagnitudeText);
            settings["earthquakeNotificationMagnitude"] = m_earthquakeNotificationMagnitude;

            std::ofstream out(GetSettingsPath(), std::ios::binary | std::ios::trunc);
            if (out)
                out << root.dump();
        }
        catch (...) {
            OutputDebugStringW(L"Settings file could not be saved.\n");
        }
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

        std::thread([hwnd, url, unplannedOnly, order]() {
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
            }).detach();
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

        size_t visible = ApplyFilters(true);

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
            SeverityAllowedForIncidentNotification(alert) &&
            IncidentTypeAllowedForNotification(alert) &&
            IncidentThresholdsMatchNotification(alert) &&
            !ReasonExcludedFromNotification(alert);
    }

    std::wstring IncidentNotificationKey(const TrafficAlert& alert) const
    {
        std::wstring key = alert.id.empty() ? BuildAlertSummary(alert) : alert.id;
        key += L"|";
        key += alert.updatedText;
        key += L"|";
        key += std::to_wstring(alert.lanesClosed);
        key += L"/";
        key += std::to_wstring(alert.lanesTotal);
        return key;
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

    void NotifyForMatchingIncidents(const std::vector<TrafficAlert>& alerts)
    {
        std::vector<const TrafficAlert*> matches;
        for (const TrafficAlert& alert : alerts) {
            if (!AlertMatchesIncidentNotification(alert))
                continue;

            std::wstring key = IncidentNotificationKey(alert);
            if (m_notifiedIncidentKeys.insert(key).second)
                matches.push_back(&alert);
        }

        if (matches.empty())
            return;

        std::wstring title;
        std::wstring body;
        if (matches.size() == 1) {
            const TrafficAlert& alert = *matches.front();
            title = alert.road.empty() ? L"Incident notification" : alert.road + L" incident notification";
            body = IncidentNotificationLine(alert);
        }
        else {
            title = std::to_wstring(matches.size()) + L" matching incident notifications";
            const size_t displayCount = MinValue<size_t>(matches.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += IncidentNotificationLine(*matches[i]);
            }
            if (matches.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body);
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
        wcsncpy_s(nid.szTip, L"Traffic England Alerts Map", _TRUNCATE);

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
        if (!m_inAppNotification)
            return;

        std::wstring text = title;
        if (!body.empty()) {
            text += L"\r\n";
            text += body;
        }
        SetWindowTextSafe(m_inAppNotification, text);
        Layout();
        ShowWindow(m_inAppNotification, SW_SHOW);
        SetWindowPos(m_inAppNotification, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        KillTimer(m_hwnd, kInAppNotificationTimerId);
        SetTimer(m_hwnd, kInAppNotificationTimerId, 10 * 1000, nullptr);
    }

    void RenderNotificationHistory()
    {
        if (!m_notificationHistoryOverlay)
            return;

        std::wstring text = L"Notification History\r\n\r\n";
        if (m_notificationHistory.empty()) {
            text += L"No notifications yet.";
        }
        else {
            for (const NotificationHistoryEntry& entry : m_notificationHistory) {
                text += L"[";
                text += entry.timestamp;
                text += L"] ";
                text += entry.title;
                if (!entry.body.empty()) {
                    text += L"\r\n";
                    text += entry.body;
                }
                text += L"\r\n\r\n";
            }
        }

        SetWindowTextSafe(m_notificationHistoryOverlay, text);
    }

    void AddNotificationHistory(const std::wstring& title, const std::wstring& body)
    {
        NotificationHistoryEntry entry;
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
        std::thread([hwnd, mapHwnd, urls = std::move(urls)]() {
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
            }).detach();
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
        std::wstring text;
        for (const auto& msg : m_chatMessages) {
            if (!msg.timestamp.empty()) {
                text += L"[" + msg.timestamp + L"] ";
            }
            if (!msg.author.empty())
                text += msg.author + L": ";
            text += msg.text + L"\r\n";
        }
        SetWindowTextSafe(m_chatHistory, text);
    }

    void PollServerAsync()
    {
        if (m_serverRequestInProgress.exchange(true))
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            m_serverRequestInProgress.store(false);
            return;
        }

        HWND hwnd = m_hwnd;
        std::thread([hwnd, server]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::Poll;

            std::string chatBody;
            std::wstring chatError;
            if (HttpGetText(AppendPath(server, L"/api/chat"), chatBody, chatError)) {
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
            if (HttpGetText(AppendPath(server, L"/api/notes"), noteBody, noteError)) {
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
            }).detach();
    }

    void SendChatAsync()
    {
        std::wstring text = Trim(GetWindowTextString(m_chatEdit));
        if (text.empty())
            return;

        SetWindowTextSafe(m_chatEdit, L"");
        ChatMessage local{ L"Me", text, L"pending" };
        m_chatMessages.push_back(local);
        RenderChatHistory();

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        std::thread([hwnd, server, text]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendChat;
            std::string response;
            std::wstring error;
            std::string body = "{\"author\":" + JsonEscape(L"ERCTools") + ",\"text\":" + JsonEscape(text) + "}";
            result->ok = HttpPostJsonText(AppendPath(server, L"/api/chat"), body, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            }).detach();
    }

    void SendNoteAsync()
    {
        std::wstring text = Trim(GetWindowTextString(m_noteEdit));
        if (text.empty()) {
            SetStatusText(L"Type note text first.");
            return;
        }
        if (!m_hasPendingNoteLocation) {
            SetStatusText(L"Double-click the map to choose where the note belongs.");
            return;
        }

        MapNote note;
        note.author = L"Me";
        note.text = text;
        note.timestamp = L"pending";
        note.latitude = m_pendingNoteLat;
        note.longitude = m_pendingNoteLon;
        m_notes.push_back(note);
        m_map.SetNotes(m_notes);
        SetWindowTextSafe(m_noteEdit, L"");

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        double lat = m_pendingNoteLat;
        double lon = m_pendingNoteLon;
        std::thread([hwnd, server, text, lat, lon]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendNote;
            std::string response;
            std::wstring error;
            std::string body = "{\"author\":" + JsonEscape(L"ERCTools") +
                ",\"text\":" + JsonEscape(text) +
                ",\"latitude\":" + std::to_string(lat) +
                ",\"longitude\":" + std::to_string(lon) +
                ",\"lat\":" + std::to_string(lat) +
                ",\"lon\":" + std::to_string(lon) +
                ",\"x\":" + std::to_string(lon) +
                ",\"y\":" + std::to_string(lat) + "}";
            result->ok = HttpPostJsonText(AppendPath(server, L"/api/notes"), body, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            }).detach();
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
                MergePendingLocalNotes(result->notes, m_notes);
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

        delete result;
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

    void CreateMainMenu()
    {
        HMENU menu = CreateMenu();
        HMENU fileMenu = CreatePopupMenu();
        HMENU roadsMenu = CreatePopupMenu();
        HMENU earthquakesMenu = CreatePopupMenu();
        HMENU viewMenu = CreatePopupMenu();
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_SETTINGS, L"Settings...");
        AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXIT, L"Exit");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_FILTERS, L"Incident Filters...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_NOTIFICATIONS, L"Incident Notifications...");
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKES_LIST, L"Earthquakes List...");
        AppendMenuW(earthquakesMenu, MF_STRING, IDM_EARTHQUAKE_NOTIFICATIONS, L"Earthquake Notifications...");
        AppendMenuW(earthquakesMenu, m_showEarthquakes ? MF_CHECKED : MF_UNCHECKED, IDM_SHOW_EARTHQUAKES, L"Show Earthquakes");
        MENUITEMINFOW showEarthquakesInfo{};
        showEarthquakesInfo.cbSize = sizeof(showEarthquakesInfo);
        showEarthquakesInfo.fMask = MIIM_FTYPE;
        showEarthquakesInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(earthquakesMenu, IDM_SHOW_EARTHQUAKES, FALSE, &showEarthquakesInfo);
        AppendMenuW(viewMenu, m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_NOTIFICATION_HISTORY, L"Notification History");
        MENUITEMINFOW historyInfo{};
        historyInfo.cbSize = sizeof(historyInfo);
        historyInfo.fMask = MIIM_FTYPE;
        historyInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(viewMenu, IDM_VIEW_NOTIFICATION_HISTORY, FALSE, &historyInfo);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"File");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(roadsMenu), L"Roads");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(earthquakesMenu), L"Earthquakes");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu), L"View");
        AppendMenuW(menu, MF_STRING, IDM_ABOUT, L"About");
        SetMenu(m_hwnd, menu);
    }

    void ShowAboutDialog()
    {
        MessageBoxW(
            m_hwnd,
            L"Traffic England Alerts Map\n\nView Traffic England alerts on a UK map, collaborate with local responders, and share map notes.",
            L"About Traffic England Alerts Map",
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
                560,
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

        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 392, y + 42, 102, 32, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentNotifyRoadsEdit, m_incidentNotifyLaneThresholdEdit, m_incidentNotifyAndRadio, m_incidentNotifyOrRadio, m_incidentNotifyDelayThresholdEdit, m_incidentNotifyRegionsBtn, m_incidentNotifyReasonExclusionsEdit, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SendMessageW(m_incidentNotifyRoadsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M*, A1(M), A2, A15, A16, A17, A20, A4, A52"));
        SendMessageW(m_incidentNotifyLaneThresholdEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"50%"));
        SendMessageW(m_incidentNotifyDelayThresholdEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1 hour"));
        SendMessageW(m_incidentNotifyReasonExclusionsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Road Management"));
        SyncIncidentNotificationControls();
    }

    void SyncIncidentNotificationControls()
    {
        m_syncingControls = true;
        if (m_incidentNotifyRoadsEdit)
            SetWindowTextSafe(m_incidentNotifyRoadsEdit, m_incidentNotifyRoads);
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
        else if (id == IDC_INCIDENT_NOTIFICATIONS_EXCLUSIONS_EDIT) {
            m_incidentNotifyReasonExclusions = GetWindowTextString(m_incidentNotifyReasonExclusionsEdit);
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
            365,
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
        HWND drawBtn = CreateWindowExW(0, L"BUTTON", L"Draw on map", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 254, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_DRAW_BTN), m_hInst, nullptr);
        HWND clearBtn = CreateWindowExW(0, L"BUTTON", L"Clear points", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 154, 254, 128, 32, parent, ControlId(IDC_NOTIFICATION_REGION_CLEAR_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 292, 254, 102, 32, parent, ControlId(IDC_NOTIFICATION_REGION_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { nameEdit, allRoads, roadsEdit, pointsLabel, drawBtn, clearBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(roadsEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M*, A1(M), A2, A52"));
        UpdateNotificationRegionEditorPointsLabel(parent, index);
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
            m_map.SetDraftPolygon({});
            SetStatusText(L"Click the map to add polygon points for " + (polygon.name.empty() ? L"this region" : polygon.name) + L". Drag to pan.");
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
        return L"https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=43.33464&maxlatitude=61.44499&minlongitude=-27.02637&maxlongitude=26.41113&orderby=time&limit=20000";
    }

    void FetchEarthquakesAsync(bool notify)
    {
        if (m_earthquakeFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        std::wstring url = EarthquakeQueryUrl();
        std::thread([hwnd, url, notify]() {
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
            }).detach();
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
        ApplyEarthquakeVisibility();
        ApplyEarthquakeListFilters();
        if (result->notify)
            NotifyForMatchingEarthquakes(m_allEarthquakes);
        delete result;
    }

    void ApplyEarthquakeVisibility()
    {
        if (m_showEarthquakes)
            m_map.SetEarthquakes(m_allEarthquakes);
        else
            m_map.SetEarthquakes({});
    }

    void UpdateEarthquakeMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (menu)
            CheckMenuItem(menu, IDM_SHOW_EARTHQUAKES, MF_BYCOMMAND | (m_showEarthquakes ? MF_CHECKED : MF_UNCHECKED));
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
        RenderNotificationHistory();
        Layout();
        SaveSettings();
    }

    void ToggleShowEarthquakes()
    {
        m_showEarthquakes = !m_showEarthquakes;
        UpdateEarthquakeMenu();
        ApplyEarthquakeVisibility();
        SaveSettings();
        if (m_showEarthquakes && m_allEarthquakes.empty())
            FetchEarthquakesAsync(false);
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

    void NotifyForMatchingEarthquakes(const std::vector<EarthquakeEvent>& events)
    {
        std::vector<const EarthquakeEvent*> matches;
        for (const EarthquakeEvent& event : events) {
            if (!EarthquakeMatchesNotification(event))
                continue;
            std::wstring key = event.id + L"|" + std::to_wstring(event.timeMs);
            if (m_notifiedEarthquakeKeys.insert(key).second)
                matches.push_back(&event);
        }

        if (matches.empty())
            return;

        std::wstring title;
        std::wstring body;
        if (matches.size() == 1) {
            title = L"Earthquake notification";
            body = EarthquakeNotificationLine(*matches.front());
        }
        else {
            title = std::to_wstring(matches.size()) + L" earthquake notifications";
            const size_t displayCount = MinValue<size_t>(matches.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += EarthquakeNotificationLine(*matches[i]);
            }
            if (matches.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body);
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
    }

    bool EarthquakeMatchesListFilters(const EarthquakeEvent& event) const
    {
        double minMagnitude = 0.0;
        if (m_earthquakeListMagnitudeEdit) {
            std::wstring magText = Trim(GetWindowTextString(m_earthquakeListMagnitudeEdit));
            if (!magText.empty() && TryParseDoubleText(magText, minMagnitude) && event.magnitude + 0.0001 < minMagnitude)
                return false;
        }

        long long afterMs = 0;
        if (m_earthquakeListTimeEdit) {
            std::wstring timeText = Trim(GetWindowTextString(m_earthquakeListTimeEdit));
            if (!timeText.empty() && TryParseDateTimeFilter(timeText, afterMs) && event.timeMs < afterMs)
                return false;
        }

        if (m_earthquakeFilterRegion.size() >= 3) {
            if (!event.hasLocation || !PointInPolygon(event.latitude, event.longitude, m_earthquakeFilterRegion))
                return false;
        }

        return true;
    }

    void ApplyEarthquakeListFilters()
    {
        if (!m_earthquakeListView)
            return;

        SendMessageW(m_earthquakeListView, LVM_DELETEALLITEMS, 0, 0);
        int row = 0;
        for (const EarthquakeEvent& event : m_allEarthquakes) {
            if (!EarthquakeMatchesListFilters(event))
                continue;

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

        if (m_earthquakeListWnd && IsWindowVisible(m_earthquakeListWnd))
            SetStatusText(L"Showing " + std::to_wstring(row) + L" earthquake(s).");
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = kSettingsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_settingsWnd || !IsWindow(m_settingsWnd)) {
            m_settingsWnd = CreateWindowExW(WS_EX_TOOLWINDOW, kSettingsClassName, L"Settings", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT, 470, 465, m_hwnd, nullptr, m_hInst, this);
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
        CreateAutoLabel(parent, IDC_SETTINGS_FILTER_LABEL, L"Traffic England alert filter", 18, 252);
        m_settingsFilterCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 278, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_FILTER), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_ORDER_LABEL, L"Traffic England order", 18, 316);
        m_settingsOrderCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 342, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_ORDER), m_hInst, nullptr);
        HWND boundary = CreateWindowExW(0, L"BUTTON", L"Download / refresh UK boundary", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 388, 260, 32, parent, ControlId(IDC_SETTINGS_BOUNDARY_BTN), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 388, 102, 32, parent, ControlId(IDC_SETTINGS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_urlEdit, m_serverEdit, m_settingsRefreshOffRadio, m_settingsRefreshOnRadio, m_settingsRefreshIntervalEdit, m_settingsFilterCombo, m_settingsOrderCombo, boundary, close }) {
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
        SendMessageW(m_serverEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"http://localhost:8080"));
        SendMessageW(m_settingsRefreshIntervalEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"5s, 3s, 10s"));

        SendMessageW(m_settingsFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unplanned only"));
        SendMessageW(m_settingsFilterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All alerts"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Road"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Severity"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Updated"));
        SendMessageW(m_settingsOrderCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Title"));
        SyncSettingsControls();
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
            DownloadBoundaryFromGitHubAsync();
        }
        else if (id == IDC_SETTINGS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_settingsWnd, SW_HIDE);
        }
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
    HWND m_serverLabel = nullptr;
    HWND m_noteLabel = nullptr;
    HWND m_panelTabBtn = nullptr;
    HWND m_urlEdit = nullptr;
    HWND m_serverEdit = nullptr;
    HWND m_refreshBtn = nullptr;
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
    HWND m_settingsFilterCombo = nullptr;
    HWND m_settingsOrderCombo = nullptr;
    HWND m_settingsRefreshOffRadio = nullptr;
    HWND m_settingsRefreshOnRadio = nullptr;
    HWND m_settingsRefreshIntervalEdit = nullptr;
    HWND m_incidentSevereCheck = nullptr;
    HWND m_incidentModerateCheck = nullptr;
    HWND m_incidentMinorCheck = nullptr;
    HWND m_incidentUnknownCheck = nullptr;
    HWND m_incidentUnplannedCheck = nullptr;
    HWND m_incidentPlannedCheck = nullptr;
    HWND m_incidentNotifyRoadsEdit = nullptr;
    HWND m_incidentNotifyLaneThresholdEdit = nullptr;
    HWND m_incidentNotifyDelayThresholdEdit = nullptr;
    HWND m_incidentNotifyAndRadio = nullptr;
    HWND m_incidentNotifyOrRadio = nullptr;
    HWND m_incidentNotifyRegionsBtn = nullptr;
    HWND m_incidentNotifyReasonExclusionsEdit = nullptr;
    HWND m_notificationRegionsList = nullptr;
    HWND m_earthquakeListMagnitudeEdit = nullptr;
    HWND m_earthquakeListTimeEdit = nullptr;
    HWND m_earthquakeListRegionBtn = nullptr;
    HWND m_earthquakeListClearRegionBtn = nullptr;
    HWND m_earthquakeListView = nullptr;
    HWND m_earthquakeNotificationMagnitudeEdit = nullptr;
    HWND m_chatHistory = nullptr;
    HWND m_chatEdit = nullptr;
    HWND m_chatSendBtn = nullptr;
    HWND m_noteEdit = nullptr;
    HWND m_noteBtn = nullptr;
    HWND m_inAppNotification = nullptr;
    HWND m_notificationHistoryOverlay = nullptr;

    MapView m_map;

    std::vector<TrafficAlert> m_allAlerts;
    std::vector<TrafficAlert> m_filteredAlerts;
    std::vector<ChatMessage> m_chatMessages;
    std::vector<MapNote> m_notes;
    std::vector<NotificationHistoryEntry> m_notificationHistory;
    std::vector<GeoPolygon> m_incidentNotificationRegions;
    std::vector<EarthquakeEvent> m_allEarthquakes;
    std::vector<GeoPoint> m_earthquakeFilterRegion;
    std::wstring m_selectedId;
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
    std::wstring m_incidentNotifyRoads = L"M*, A1(M), A2, A15, A16, A17, A20, A4, A52";
    std::wstring m_incidentNotifyLaneThresholdText = L"50%";
    double m_incidentNotifyLaneThreshold = 50.0;
    std::wstring m_incidentNotifyDelayThresholdText = L"1 hour";
    double m_incidentNotifyDelayThresholdMinutes = 60.0;
    bool m_incidentNotifyThresholdUseOr = false;
    std::wstring m_incidentNotifyReasonExclusions = L"Road Management";
    bool m_showEarthquakes = false;
    std::wstring m_earthquakeNotificationMagnitudeText = L"4.0";
    double m_earthquakeNotificationMagnitude = 4.0;
    std::wstring m_alertOrder = L"Road";
    std::wstring m_alertsEndpoint = L"https://www.trafficengland.com/traffic-alerts";
    std::wstring m_serverBaseUrl = L"http://localhost:8080";
    bool m_periodicRefreshEnabled = true;
    std::wstring m_refreshIntervalText = L"300s";
    UINT m_refreshIntervalMs = 5 * 60 * 1000;
    std::atomic_bool m_serverRequestInProgress{ false };
    std::atomic_bool m_earthquakeFetchInProgress{ false };
    bool m_notificationIconAdded = false;
    std::unordered_set<std::wstring> m_notifiedIncidentKeys;
    std::unordered_set<std::wstring> m_notifiedEarthquakeKeys;
    PolygonCaptureTarget m_polygonCaptureTarget = PolygonCaptureTarget::None;
    size_t m_activeIncidentRegionIndex = static_cast<size_t>(-1);
    bool m_hasPendingNoteLocation = false;
    double m_pendingNoteLat = 0.0;
    double m_pendingNoteLon = 0.0;
};


int RunMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    MainWindow win;
    if (!win.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"Traffic England Alerts Map", MB_ICONERROR);
        return 0;
    }

    return win.Run(nCmdShow);
}
