// =================================================================================
// FILE: main_window.cpp
// =================================================================================


#include "ui/windows/main_window.h"
#include "app/app_state.h"
#include "net/binary_protocol.h"
#include "net/client_update.h"
#include "data/parsers/earthquake_data.h"
#include "data/parsers/ntis_traffic.h"
#include "net/http.h"
#include "map/map_view.h"
#include "data/parsers/parsing.h"
#include "data/places/populated_places.h"
#include "audio/sound_cues.h"
#include "data/time_filters.h"
#include "data/parsers/traffic_scotland.h"
#include "core/util.h"
#include "data/parsers/weather_data.h"
#include "data/weather/weather_intensity.h"

#include <iomanip>
#include <richedit.h>

#ifndef MSFTEDIT_CLASS
#define MSFTEDIT_CLASS L"RICHEDIT50W"
#endif
#ifndef EM_GETLANGOPTIONS
#define EM_GETLANGOPTIONS (WM_USER + 121)
#endif
#ifndef EM_SETLANGOPTIONS
#define EM_SETLANGOPTIONS (WM_USER + 120)
#endif
#ifndef IMF_SPELLCHECKING
#define IMF_SPELLCHECKING 0x0800
#endif


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
constexpr int IDM_FILE_LOGOUT = 2023;
constexpr int IDM_ROADS_INCIDENTS_LIST = 2024;
constexpr int IDM_FILE_USERS = 2025;
constexpr int IDM_FILE_ACCOUNT_CREATOR = 2026;
constexpr int IDM_WEATHER_WARNINGS_LIST = 2027;
constexpr int IDM_FLOODS_LIST = 2028;
constexpr int IDM_SHOW_WEATHER_WARNINGS = 2029;
constexpr int IDM_WEATHER_WARNING_OVERLAY_NONE = 2030;
constexpr int IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA = 2031;
constexpr int IDM_SHOW_FLOODS = 2032;
constexpr int IDM_FLOOD_OVERLAY_NONE = 2033;
constexpr int IDM_FLOOD_OVERLAY_SEVERITY_AREA = 2034;
constexpr int IDM_VIEW_AREA_LABELS = 2035;
constexpr int IDM_VIEW_ROAD_DEPICTIONS = 2036;
constexpr int IDM_ROADS_SHOW_INCIDENTS = 2037;
constexpr int IDM_INCIDENT_OVERLAY_NONE = 2038;
constexpr int IDM_INCIDENT_OVERLAY_SUMMARY = 2039;
constexpr int IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY = 2040;
constexpr int IDM_WEATHER_WARNING_POLYGONS = 2041;
constexpr int IDM_WEATHER_SYSTEM_FORECASTS = 2042;
constexpr int IDM_VIEW_FPS_COUNTER = 2043;
constexpr int IDM_VIEW_MAP_CONTROLS = 2044;
constexpr int IDM_FILE_ADMIN_LOG = 2045;
constexpr int IDM_ABOUT_APP = 2046;
constexpr int IDM_ABOUT_LEGEND = 2047;
constexpr int IDM_VIEW_ROAD_DEPICTIONS_SHOW = 2048;
constexpr int IDM_VIEW_ROAD_DEPICTIONS_LIST = 2049;
constexpr int IDM_FILE_CACHE_MANAGER = 2050;
constexpr int IDM_VIEW_NOTIFICATION_REGION_POLYGONS = 2051;
constexpr int IDM_ROADS_TRAFFIC_SCOTLAND = 2052;
constexpr int IDM_ROADS_INCIDENT_EXCLUSIONS = 2053;
constexpr int IDM_VIEW_SOUND_CUES = 2054;
constexpr int IDM_VIEW_COUNTDOWN_TIMER = 2055;
constexpr int IDM_SETTINGS_GENERAL = 2056;
constexpr int IDM_SETTINGS_SOUNDS = 2057;
constexpr int IDM_VIEW_COMMS_INDICATOR = 2058;
constexpr int IDC_SETTINGS_ALERT_FILTER = 2101;
constexpr int IDC_SETTINGS_ALERT_ORDER = 2102;
constexpr int IDC_SETTINGS_BOUNDARY_BTN = 2103;
constexpr int IDC_SETTINGS_CLOSE_BTN = 2104;
constexpr int IDC_SETTINGS_FILTER_LABEL = 2105;
constexpr int IDC_SETTINGS_ORDER_LABEL = 2106;
constexpr int IDC_SETTINGS_SERVER_LABEL = 2108;
constexpr int IDC_SETTINGS_WORLD_LABEL = 2114;
constexpr int IDC_SETTINGS_WORLD_OFF_RADIO = 2115;
constexpr int IDC_SETTINGS_WORLD_ON_RADIO = 2116;
constexpr int IDC_SETTINGS_SYNC_LABEL = 2117;
constexpr int IDC_SETTINGS_SYNC_LOCAL_RADIO = 2118;
constexpr int IDC_SETTINGS_SYNC_SERVER_RADIO = 2119;
constexpr int IDC_SETTINGS_WORLD_BOUNDARY_BTN = 2120;
constexpr int IDC_SETTINGS_PUSH_REMOTE_BTN = 2121;
constexpr int IDC_SETTINGS_ROADS_BTN = 2122;
constexpr int IDC_SETTINGS_EARTHQUAKE_RADIUS_RATIO_EDIT = 2123;
constexpr int IDC_SETTINGS_NOTIFICATION_AVOIDANCE_CHECK = 2124;
constexpr int IDC_SOUNDS_DEVICE_COMBO = 2130;
constexpr int IDC_SOUNDS_MASTER_CHECK = 2131;
constexpr int IDC_SOUNDS_MESSAGE_CHECK = 2132;
constexpr int IDC_SOUNDS_PRIVATE_MESSAGE_CHECK = 2133;
constexpr int IDC_SOUNDS_NOTIFICATION_CHECK = 2134;
constexpr int IDC_SOUNDS_TIMER_START_CHECK = 2135;
constexpr int IDC_SOUNDS_TIMER_WARNING_CHECK = 2136;
constexpr int IDC_SOUNDS_TIMER_COMPLETE_CHECK = 2137;
constexpr int IDC_SOUNDS_TEST_BTN = 2138;
constexpr int IDC_SOUNDS_CLOSE_BTN = 2139;
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
constexpr int IDC_INCIDENT_FILTERS_SIDE_PANEL_LIST_ONLY_CHECK = 2212;
constexpr int IDC_INCIDENT_FILTERS_SHOW_UNRESOLVED_CHECK = 2213;
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
constexpr int IDC_INCIDENT_NOTIFICATIONS_UPDATE_LABEL = 2320;
constexpr int IDC_INCIDENT_NOTIFICATIONS_UPDATE_NOTIFY_RADIO = 2321;
constexpr int IDC_INCIDENT_NOTIFICATIONS_UPDATE_IGNORE_RADIO = 2322;
constexpr int IDC_NOTIFICATION_REGIONS_LIST = 2401;
constexpr int IDC_NOTIFICATION_REGIONS_NEW_BTN = 2402;
constexpr int IDC_NOTIFICATION_REGIONS_EDIT_BTN = 2403;
constexpr int IDC_NOTIFICATION_REGIONS_DELETE_BTN = 2404;
constexpr int IDC_NOTIFICATION_REGIONS_CLOSE_BTN = 2405;
constexpr int IDC_NOTIFICATION_REGIONS_SHOW_POLYGONS_CHECK = 2406;
constexpr int IDC_NOTIFICATION_REGION_NAME_EDIT = 2411;
constexpr int IDC_NOTIFICATION_REGION_ROADS_EDIT = 2412;
constexpr int IDC_NOTIFICATION_REGION_ALL_ROADS_CHECK = 2413;
constexpr int IDC_NOTIFICATION_REGION_DRAW_BTN = 2414;
constexpr int IDC_NOTIFICATION_REGION_CLEAR_BTN = 2415;
constexpr int IDC_NOTIFICATION_REGION_CLOSE_BTN = 2416;
constexpr int IDC_NOTIFICATION_REGION_POINTS_LABEL = 2417;
constexpr int IDC_NOTIFICATION_REGION_UNDO_BTN = 2418;
constexpr int IDC_NOTIFICATION_REGION_FINISH_BTN = 2419;
constexpr int IDC_INCIDENTS_LIST_SEARCH_EDIT = 2451;
constexpr int IDC_INCIDENTS_LIST_SEVERITY_COMBO = 2452;
constexpr int IDC_INCIDENTS_LIST_LISTVIEW = 2453;
constexpr int IDC_INCIDENTS_LIST_CLOSE_BTN = 2454;
constexpr int IDC_EARTHQUAKE_LIST_MAG_EDIT = 2501;
constexpr int IDC_EARTHQUAKE_LIST_TIME_EDIT = 2502;
constexpr int IDC_EARTHQUAKE_LIST_REGION_BTN = 2503;
constexpr int IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN = 2504;
constexpr int IDC_EARTHQUAKE_LIST_LISTVIEW = 2505;
constexpr int IDC_EARTHQUAKE_LIST_CLOSE_BTN = 2506;
constexpr int IDC_EARTHQUAKE_LIST_PERIOD_COMBO = 2507;
constexpr int IDC_EARTHQUAKE_LIST_DATE_RADIO = 2508;
constexpr int IDC_EARTHQUAKE_LIST_PERIOD_RADIO = 2509;
constexpr int IDC_EARTHQUAKE_LIST_POPULATED_RADIUS_EDIT = 2510;
constexpr int IDC_EARTHQUAKE_LIST_MINIMUM_POPULATION_EDIT = 2511;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT = 2521;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN = 2522;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_TIME_EDIT = 2523;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_COMBO = 2524;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_DATE_RADIO = 2525;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_RADIO = 2526;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_POPULATED_RADIUS_EDIT = 2527;
constexpr int IDC_EARTHQUAKE_NOTIFICATIONS_MINIMUM_POPULATION_EDIT = 2528;
constexpr int IDC_WEATHER_SYSTEMS_LIST_LISTVIEW = 2541;
constexpr int IDC_WEATHER_SYSTEMS_LIST_REFRESH_BTN = 2542;
constexpr int IDC_WEATHER_SYSTEMS_LIST_CLOSE_BTN = 2543;
constexpr int IDC_WEATHER_SYSTEMS_LIST_FORECAST_COMBO = 2544;
constexpr int IDC_WEATHER_SYSTEM_NOTIFICATIONS_WIND_EDIT = 2551;
constexpr int IDC_WEATHER_SYSTEM_NOTIFICATIONS_CLOSE_BTN = 2552;
constexpr int IDC_WEATHER_WARNINGS_LIST_LISTVIEW = 2561;
constexpr int IDC_WEATHER_WARNINGS_LIST_REFRESH_BTN = 2562;
constexpr int IDC_WEATHER_WARNINGS_LIST_CLOSE_BTN = 2563;
constexpr int IDC_WEATHER_WARNINGS_LIST_PERIOD_COMBO = 2564;
constexpr int IDC_FLOODS_LIST_LISTVIEW = 2571;
constexpr int IDC_FLOODS_LIST_REFRESH_BTN = 2572;
constexpr int IDC_FLOODS_LIST_CLOSE_BTN = 2573;
constexpr int IDC_FLOODS_LIST_PERIOD_COMBO = 2574;
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
constexpr int IDC_TEMPLATES_WIZARD_TITLE_PREVIEW = 2611;
constexpr int IDC_TEMPLATES_WIZARD_COPY_TITLE = 2612;
constexpr int IDC_TEMPLATES_EDITOR_LIST = 2621;
constexpr int IDC_TEMPLATES_EDITOR_NAME = 2622;
constexpr int IDC_TEMPLATES_EDITOR_BODY = 2623;
constexpr int IDC_TEMPLATES_EDITOR_NEW = 2624;
constexpr int IDC_TEMPLATES_EDITOR_SAVE = 2625;
constexpr int IDC_TEMPLATES_EDITOR_DELETE = 2626;
constexpr int IDC_TEMPLATES_EDITOR_CLOSE = 2627;
constexpr int IDC_TEMPLATES_EDITOR_TITLE = 2628;
constexpr int IDC_TEMPLATES_EDITOR_WEATHER_TYPE = 2629;
constexpr int IDC_ACCOUNT_CREATOR_USERNAME = 2641;
constexpr int IDC_ACCOUNT_CREATOR_DISPLAY_NAME = 2642;
constexpr int IDC_ACCOUNT_CREATOR_PASSWORD = 2643;
constexpr int IDC_ACCOUNT_CREATOR_POSITION = 2644;
constexpr int IDC_ACCOUNT_CREATOR_ACTIVE = 2645;
constexpr int IDC_ACCOUNT_CREATOR_CREATE = 2646;
constexpr int IDC_ACCOUNT_CREATOR_CLOSE = 2647;
constexpr int IDC_ACCOUNT_CREATOR_STATUS = 2648;
constexpr int IDC_ADMIN_LOG_TYPE_COMBO = 2661;
constexpr int IDC_ADMIN_LOG_LIST = 2662;
constexpr int IDC_ADMIN_LOG_REFRESH_BTN = 2663;
constexpr int IDC_ADMIN_LOG_CLOSE_BTN = 2664;
constexpr int IDC_ADMIN_LOG_CLEAR_BTN = 2665;
constexpr int IDC_ROAD_DEPICTIONS_LIST = 2681;
constexpr int IDC_ROAD_DEPICTIONS_ALL_BTN = 2682;
constexpr int IDC_ROAD_DEPICTIONS_NONE_BTN = 2683;
constexpr int IDC_ROAD_DEPICTIONS_CLOSE_BTN = 2684;
constexpr int IDC_ROAD_DEPICTIONS_ROAD_EDIT = 2685;
constexpr int IDC_ROAD_DEPICTIONS_ADD_BTN = 2686;
constexpr int IDC_ROAD_DEPICTIONS_REMOVE_BTN = 2687;
constexpr int IDC_CACHE_UK_BOUNDARY_BTN = 2701;
constexpr int IDC_CACHE_WORLD_BOUNDARY_BTN = 2702;
constexpr int IDC_CACHE_ROADS_BTN = 2703;
constexpr int IDC_CACHE_POPULATED_PLACES_BTN = 2704;
constexpr int IDC_CACHE_CLOSE_BTN = 2705;
constexpr int IDC_CACHE_STATUS_LABEL = 2706;
constexpr int IDC_CACHE_PROGRESS = 2707;
constexpr int IDC_TRAFFIC_SCOTLAND_ENABLED = 2721;
constexpr int IDC_TRAFFIC_SCOTLAND_URL = 2722;
constexpr int IDC_TRAFFIC_SCOTLAND_REFRESH = 2723;
constexpr int IDC_TRAFFIC_SCOTLAND_CLOSE = 2724;
constexpr int IDC_TRAFFIC_SCOTLAND_STATUS = 2725;
constexpr int IDC_INCIDENT_EXCLUSIONS_LIST = 2731;
constexpr int IDC_INCIDENT_EXCLUSIONS_REMOVE = 2732;
constexpr int IDC_INCIDENT_EXCLUSIONS_CLOSE = 2733;
constexpr const wchar_t* kSettingsClassName = L"TrafficEnglandSettingsWindow";
constexpr const wchar_t* kSoundsClassName = L"ERCToolsSoundsWindow";
constexpr const wchar_t* kIncidentFiltersClassName = L"TrafficEnglandIncidentFiltersWindow";
constexpr const wchar_t* kIncidentNotificationsClassName = L"TrafficEnglandIncidentNotificationsWindow";
constexpr const wchar_t* kIncidentsListClassName = L"TrafficEnglandIncidentsListWindow";
constexpr const wchar_t* kNotificationRegionsClassName = L"TrafficEnglandNotificationRegionsWindow";
constexpr const wchar_t* kNotificationRegionEditorClassName = L"TrafficEnglandNotificationRegionEditorWindow";
constexpr const wchar_t* kEarthquakeListClassName = L"TrafficEnglandEarthquakeListWindow";
constexpr const wchar_t* kEarthquakeNotificationsClassName = L"TrafficEnglandEarthquakeNotificationsWindow";
constexpr const wchar_t* kWeatherSystemsListClassName = L"TrafficEnglandWeatherSystemsListWindow";
constexpr const wchar_t* kWeatherSystemNotificationsClassName = L"TrafficEnglandWeatherSystemNotificationsWindow";
constexpr const wchar_t* kWeatherWarningsListClassName = L"ERCToolsWeatherWarningsListWindow";
constexpr const wchar_t* kFloodsListClassName = L"ERCToolsFloodsListWindow";
constexpr const wchar_t* kTemplatesWizardClassName = L"TrafficEnglandTemplatesWizardWindow";
constexpr const wchar_t* kTemplatesEditorClassName = L"TrafficEnglandTemplatesEditorWindow";
constexpr const wchar_t* kAccountCreatorClassName = L"ERCToolsAccountCreatorWindow";
constexpr const wchar_t* kAdminLogClassName = L"ERCToolsAdminLogWindow";
constexpr const wchar_t* kRoadDepictionsClassName = L"ERCToolsRoadDepictionsWindow";
constexpr const wchar_t* kLegendClassName = L"ERCToolsLegendWindow";
constexpr const wchar_t* kCacheManagerClassName = L"ERCToolsCacheManagerWindow";
constexpr const wchar_t* kTrafficScotlandClassName = L"ERCToolsTrafficScotlandWindow";
constexpr const wchar_t* kIncidentExclusionsClassName = L"ERCToolsIncidentExclusionsWindow";
constexpr UINT WM_APP_NOTIFY_ICON = WM_APP + 20;
constexpr UINT WM_APP_UPDATE_READY = WM_APP + 21;
constexpr UINT WM_APP_SETTINGS_SYNC_READY = WM_APP + 22;
constexpr UINT WM_APP_WEATHER_WARNINGS_READY = WM_APP + 23;
constexpr UINT WM_APP_FLOODS_READY = WM_APP + 24;
constexpr UINT WM_APP_ADMIN_LOG_READY = WM_APP + 25;
constexpr UINT WM_APP_ROAD_DEPICTIONS_READY = WM_APP + 26;
constexpr UINT WM_APP_POPULATED_PLACES_READY = WM_APP + 27;
constexpr UINT WM_APP_SERVER_SOURCE_SIGNAL_READY = WM_APP + 28;
constexpr UINT kNotificationIconId = 1;
constexpr UINT_PTR kAlertRefreshTimerId = 1;
constexpr UINT_PTR kServerPollTimerId = 2;
constexpr UINT_PTR kInAppNotificationTimerId = 3;
constexpr UINT_PTR kEarthquakeRefreshTimerId = 4;
constexpr UINT_PTR kWeatherSystemsRefreshTimerId = 5;
constexpr UINT kServerPollIntervalMs = 2000;
constexpr const wchar_t* kWeatherSystemsSourceUrl = L"https://www.tropicalstormrisk.com/tracker/dynamic/main.html";
constexpr const wchar_t* kWeatherWarningsSourceUrl = L"https://weather.metoffice.gov.uk/warnings-and-advice/uk-warnings";
constexpr const wchar_t* kFloodsSourceUrl = L"https://environment.data.gov.uk/flood-monitoring/id/floods?_view=full";
constexpr const wchar_t* kOpenRoadsFeatureQueryUrl = L"https://services.arcgis.com/qHLhLQrcvEnxjtPr/arcgis/rest/services/OS_OpenRoads/FeatureServer/1/query";
constexpr const wchar_t* kPopulatedPlacesSourceUrl = L"https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_10m_populated_places_simple.geojson";
constexpr const wchar_t* kSupplementalPopulatedPlacesSourceUrl = L"https://raw.githubusercontent.com/lmfmaier/cities-json/master/cities500.json";
static std::atomic_bool g_roadDepictionsDownloadInProgress{ false };
static std::atomic_bool g_populatedPlacesDownloadInProgress{ false };

enum class ServerSourceKind : uint32_t
{
    None = 0,
    Roads,
    Earthquakes,
    WeatherSystems,
    WeatherWarnings,
    Floods
};

struct ServerSourceSignalResult
{
    ServerSourceKind source = ServerSourceKind::None;
    uint32_t epoch = 0;
    bool ok = false;
    bool changed = false;
    uint32_t generation = 0;
    std::wstring error;
};

struct FeedResult
{
    bool ok = false;
    bool serverSourced = false;
    ServerSourceKind serverSource = ServerSourceKind::None;
    uint32_t serverGeneration = 0;
    uint32_t serverSyncEpoch = 0;
    std::wstring error;
    std::vector<TrafficAlert> alerts;
};

enum class BoundaryDownloadKind
{
    Uk,
    World
};

enum class CacheActivity
{
    Boundary,
    RoadDepictions,
    PopulatedPlaces
};

struct BoundaryDownloadResult
{
    BoundaryDownloadKind kind = BoundaryDownloadKind::Uk;
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
};

struct RoadDepictionsDownloadResult
{
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
    size_t featureCount = 0;
};

struct PopulatedPlacesDownloadResult
{
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
    std::vector<PopulatedPlace> places;
};

struct EarthquakeResult
{
    bool ok = false;
    bool notify = false;
    bool serverSourced = false;
    ServerSourceKind serverSource = ServerSourceKind::None;
    uint32_t serverGeneration = 0;
    uint32_t serverSyncEpoch = 0;
    std::wstring error;
    std::wstring statusText;
    std::vector<EarthquakeEvent> events;
};

struct WeatherSystemsResult
{
    bool ok = false;
    bool notify = false;
    bool serverSourced = false;
    ServerSourceKind serverSource = ServerSourceKind::None;
    uint32_t serverGeneration = 0;
    uint32_t serverSyncEpoch = 0;
    std::wstring error;
    std::wstring statusText;
    std::vector<WeatherSystemEvent> systems;
};

struct WeatherWarningsResult
{
    bool ok = false;
    bool notify = false;
    bool serverSourced = false;
    ServerSourceKind serverSource = ServerSourceKind::None;
    uint32_t serverGeneration = 0;
    uint32_t serverSyncEpoch = 0;
    std::wstring error;
    std::wstring statusText;
    std::vector<WeatherWarningEvent> warnings;
};

struct FloodsResult
{
    bool ok = false;
    bool notify = false;
    bool serverSourced = false;
    ServerSourceKind serverSource = ServerSourceKind::None;
    uint32_t serverGeneration = 0;
    uint32_t serverSyncEpoch = 0;
    std::wstring error;
    std::wstring statusText;
    std::vector<FloodEvent> floods;
};

enum class ServerAction
{
    Poll,
    SendChat,
    SendNote,
    UpdateNote,
    DeleteNote,
    ClearChat,
    DeleteChatMessage,
    KickUser,
    MuteUser,
    SendPrivateMessage,
    CreateAccount,
    AddIncidentExclusion,
    RemoveIncidentExclusion,
    ClearAdminLog
};

struct ServerResult
{
    ServerAction action = ServerAction::Poll;
    bool ok = false;
    bool chatOk = false;
    bool notesOk = false;
    bool usersOk = false;
    bool privateMessagesOk = false;
    bool incidentExclusionsOk = false;
    uint32_t collaborationVersion = 0;
    std::wstring error;
    std::vector<ChatMessage> chat;
    std::vector<PrivateMessage> privateMessages;
    std::vector<MapNote> notes;
    std::vector<OnlineUser> users;
    std::vector<IncidentExclusion> incidentExclusions;
};

struct GlobalSettingsResult
{
    bool ok = false;
    bool push = false;
    std::wstring error;
    json settings;
};

struct AdminLogEntry
{
    std::wstring event;
    std::wstring username;
    std::wstring displayName;
    std::wstring position;
    std::wstring pod;
    std::wstring timestamp;
    std::wstring actor;
    std::wstring details;
};

struct AdminLogResult
{
    bool ok = false;
    std::wstring error;
    std::vector<AdminLogEntry> entries;
};

struct ReportTemplate
{
    std::wstring name;
    std::wstring title;
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
    TrafficAlert alert;
};

struct NotificationBatchItem
{
    std::wstring line;
    std::wstring sourceType;
    std::wstring sourceId;
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

struct WeatherWarningNotificationState
{
    std::wstring signature;
    std::wstring line;
};

struct FloodNotificationState
{
    std::wstring signature;
    std::wstring line;
};

enum class TemplateContext
{
    Roads,
    Earthquakes,
    WeatherSystems,
    WeatherWarnings,
    Floods
};

struct TemplateEditableRange
{
    LONG start = 0;
    LONG end = 0;
};

struct TemplateRenderedText
{
    std::wstring text;
    std::vector<TemplateEditableRange> editableRanges;
};

enum class PolygonCaptureTarget
{
    None,
    IncidentRegion,
    EarthquakeRegion
};

class MainWindow;

static bool MergeGeoJsonFeatureCollections(
    const std::vector<BYTE>& firstBytes,
    const std::vector<BYTE>& secondBytes,
    std::vector<BYTE>& mergedBytes,
    std::wstring& errorOut)
{
    try {
        const std::string firstText(firstBytes.begin(), firstBytes.end());
        const std::string secondText(secondBytes.begin(), secondBytes.end());
        json first = json::parse(firstText);
        json second = json::parse(secondText);
        if (!first.is_object() || !second.is_object() ||
            !first.contains("features") || !first["features"].is_array() ||
            !second.contains("features") || !second["features"].is_array()) {
            errorOut = L"Downloaded boundary data is not a GeoJSON FeatureCollection.";
            return false;
        }

        json merged = first;
        for (const auto& feature : second["features"])
            merged["features"].push_back(feature);

        const std::string mergedText = merged.dump();
        mergedBytes.assign(mergedText.begin(), mergedText.end());
        return true;
    }
    catch (const std::exception& e) {
        errorOut = L"Could not merge UK and Ireland boundaries: " + Utf8ToWide(e.what());
        return false;
    }
}

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

static bool IsTabStopCandidate(HWND hwnd)
{
    if (!hwnd || !IsWindowEnabled(hwnd))
        return false;

    wchar_t className[64]{};
    if (!GetClassNameW(hwnd, className, static_cast<int>(_countof(className))))
        return false;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (lstrcmpiW(className, L"Button") == 0)
        return (style & BS_TYPEMASK) != BS_GROUPBOX;
    if (lstrcmpiW(className, L"Edit") == 0)
        return true;
    if (lstrcmpiW(className, L"ComboBox") == 0)
        return true;
    if (lstrcmpiW(className, L"ListBox") == 0)
        return true;
    if (lstrcmpiW(className, L"SysListView32") == 0)
        return true;
    return false;
}

static BOOL CALLBACK EnableChildTabStopsProc(HWND child, LPARAM)
{
    if (IsTabStopCandidate(child)) {
        LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
        if ((style & WS_TABSTOP) == 0)
            SetWindowLongPtrW(child, GWL_STYLE, style | WS_TABSTOP);
    }
    return TRUE;
}

static void EnableChildTabStops(HWND parent)
{
    if (parent)
        EnumChildWindows(parent, EnableChildTabStopsProc, 0);
}

static constexpr COLORREF kUiBackground = RGB(246, 248, 251);
static constexpr COLORREF kUiSurface = RGB(255, 255, 255);
static constexpr COLORREF kUiText = RGB(22, 34, 49);
static constexpr COLORREF kUiMutedText = RGB(86, 99, 115);
static constexpr COLORREF kUiSelection = RGB(226, 240, 255);
static constexpr const wchar_t* kAutoLabelMaxWidthProp = L"ERC_AUTO_LABEL_MAX_WIDTH";

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

static bool EnsureRichEditLoaded()
{
    static HMODULE module = LoadLibraryW(L"Msftedit.dll");
    return module != nullptr;
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

static bool IsClassName(HWND hwnd, const wchar_t* expected)
{
    wchar_t className[64]{};
    if (!GetClassNameW(hwnd, className, static_cast<int>(_countof(className))))
        return false;
    return _wcsicmp(className, expected) == 0;
}

static int MaxAutoLayoutClientWidth(HWND hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{ sizeof(mi) };
    if (GetMonitorInfoW(monitor, &mi))
        return MaxInt(320, (mi.rcWork.right - mi.rcWork.left) - 96);
    return 900;
}

static bool IsTextButtonStyle(DWORD style)
{
    switch (style & BS_TYPEMASK) {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
    case BS_CHECKBOX:
    case BS_AUTOCHECKBOX:
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
    case BS_OWNERDRAW:
        return true;
    default:
        return false;
    }
}

static bool IsPushButtonStyle(DWORD style)
{
    switch (style & BS_TYPEMASK) {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
    case BS_OWNERDRAW:
        return true;
    default:
        return false;
    }
}

static bool IsTextStaticStyle(DWORD style)
{
    switch (style & SS_TYPEMASK) {
    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_SIMPLE:
    case SS_LEFTNOWORDWRAP:
        return true;
    default:
        return false;
    }
}

static BOOL CALLBACK AutoSizeTextChildEnumProc(HWND child, LPARAM param)
{
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0)
        return TRUE;

    int textLength = GetWindowTextLengthW(child);
    if (textLength <= 0)
        return TRUE;

    HWND dialog = reinterpret_cast<HWND>(param);
    HWND parent = GetParent(child);
    if (!dialog || !parent)
        return TRUE;

    RECT screenRect{};
    if (!GetWindowRect(child, &screenRect))
        return TRUE;

    POINT topLeft{ screenRect.left, screenRect.top };
    ScreenToClient(dialog, &topLeft);

    RECT parentRect = screenRect;
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT*>(&parentRect), 2);
    const int currentW = parentRect.right - parentRect.left;
    const int currentH = parentRect.bottom - parentRect.top;

    const int maxRight = MaxAutoLayoutClientWidth(dialog) - 28;
    int maxWidth = MaxInt(80, maxRight - topLeft.x);

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
    if (IsClassName(child, L"BUTTON") && IsTextButtonStyle(style)) {
        const bool checkLike = (style & BS_TYPEMASK) == BS_CHECKBOX ||
            (style & BS_TYPEMASK) == BS_AUTOCHECKBOX ||
            (style & BS_TYPEMASK) == BS_RADIOBUTTON ||
            (style & BS_TYPEMASK) == BS_AUTORADIOBUTTON;
        const int horizontalPadding = checkLike ? 36 : 34;
        const int desiredW = PreferredControlWidth(child, horizontalPadding, currentW, maxWidth);
        const int desiredH = PreferredControlHeight(child, 10, currentH, desiredW);
        if (desiredW != currentW || desiredH != currentH) {
            SetWindowPos(child, nullptr, 0, 0, desiredW, desiredH,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return TRUE;
    }

    if (IsClassName(child, L"STATIC") && IsTextStaticStyle(style)) {
        const int labelMaxWidth = static_cast<int>(reinterpret_cast<INT_PTR>(GetPropW(child, kAutoLabelMaxWidthProp)));
        if (labelMaxWidth > 0)
            maxWidth = MinInt(maxWidth, labelMaxWidth);
        const int desiredW = PreferredControlWidth(child, 6, currentW, maxWidth);
        const int desiredH = PreferredControlHeight(child, 8, currentH, desiredW);
        if (desiredW != currentW || desiredH != currentH) {
            SetWindowPos(child, nullptr, 0, 0, desiredW, desiredH,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    return TRUE;
}

static void AutoSizeTextControls(HWND hwnd)
{
    if (!hwnd)
        return;
    EnumChildWindows(hwnd, AutoSizeTextChildEnumProc, reinterpret_cast<LPARAM>(hwnd));
}

struct AutoLayoutButtonInfo
{
    HWND hwnd = nullptr;
    RECT rect{};
};

struct AutoLayoutButtonCollectState
{
    HWND parent = nullptr;
    std::vector<AutoLayoutButtonInfo> buttons;
};

static BOOL CALLBACK AutoLayoutButtonEnumProc(HWND child, LPARAM param)
{
    auto* state = reinterpret_cast<AutoLayoutButtonCollectState*>(param);
    if (!state || GetParent(child) != state->parent)
        return TRUE;
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0)
        return TRUE;
    if (!IsClassName(child, L"BUTTON"))
        return TRUE;

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
    if (!IsPushButtonStyle(style))
        return TRUE;

    RECT rect{};
    if (!GetWindowRect(child, &rect))
        return TRUE;
    MapWindowPoints(HWND_DESKTOP, state->parent, reinterpret_cast<POINT*>(&rect), 2);
    state->buttons.push_back({ child, rect });
    return TRUE;
}

static void AutoLayoutButtonRows(HWND hwnd)
{
    if (!hwnd)
        return;

    AutoLayoutButtonCollectState state;
    state.parent = hwnd;
    EnumChildWindows(hwnd, AutoLayoutButtonEnumProc, reinterpret_cast<LPARAM>(&state));
    if (state.buttons.empty())
        return;

    std::sort(state.buttons.begin(), state.buttons.end(), [](const AutoLayoutButtonInfo& a, const AutoLayoutButtonInfo& b) {
        if (std::abs(a.rect.top - b.rect.top) > 8)
            return a.rect.top < b.rect.top;
        return a.rect.left < b.rect.left;
        });

    std::vector<std::vector<AutoLayoutButtonInfo>> rows;
    for (const AutoLayoutButtonInfo& button : state.buttons) {
        if (rows.empty() || std::abs(rows.back().front().rect.top - button.rect.top) > 8)
            rows.push_back({});
        rows.back().push_back(button);
    }

    const int gap = 8;
    const int leftPadding = 18;
    const int rightLimit = MaxAutoLayoutClientWidth(hwnd) - 28;
    for (std::vector<AutoLayoutButtonInfo>& row : rows) {
        if (row.empty())
            continue;

        std::sort(row.begin(), row.end(), [](const AutoLayoutButtonInfo& a, const AutoLayoutButtonInfo& b) {
            return a.rect.left < b.rect.left;
            });

        int rowLeft = row.front().rect.left;
        int rowTop = row.front().rect.top;
        int x = rowLeft;
        int y = rowTop;
        int rowHeight = 0;
        for (const AutoLayoutButtonInfo& button : row) {
            const int width = MaxInt(1, button.rect.right - button.rect.left);
            const int height = MaxInt(1, button.rect.bottom - button.rect.top);
            if (row.size() > 1 && x > rowLeft && x + width > rightLimit) {
                x = MaxInt(leftPadding, rowLeft);
                y += rowHeight + gap;
                rowHeight = 0;
            }

            SetWindowPos(button.hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
            x += width + gap;
            rowHeight = MaxInt(rowHeight, height);
        }
    }
}

static BOOL CALLBACK AutoFitChildEnumProc(HWND child, LPARAM param)
{
    RECT childRect{};
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0 || !GetWindowRect(child, &childRect))
        return TRUE;

    HWND parent = GetParent(child);
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT*>(&childRect), 2);

    RECT* bounds = reinterpret_cast<RECT*>(param);
    bounds->right = MaxLong(bounds->right, childRect.right);
    bounds->bottom = MaxLong(bounds->bottom, childRect.bottom);
    return TRUE;
}

static void AutoFitWindowToChildren(HWND hwnd, int padding = 28)
{
    if (!hwnd)
        return;

    AutoSizeTextControls(hwnd);
    AutoLayoutButtonRows(hwnd);

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

static std::wstring UrlEncodeUtf8(const std::wstring& value)
{
    const std::string utf8 = WideToUtf8(value);
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (unsigned char ch : utf8) {
        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.' || ch == '~')
        {
            encoded << static_cast<char>(ch);
        }
        else {
            encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
    }
    return Utf8ToWide(encoded.str());
}

static std::wstring NormalizeRoadDepictionRoadLabel(std::wstring label)
{
    label = Trim(label);
    std::wstring normalised;
    normalised.reserve(label.size());
    bool pendingSpace = false;
    for (wchar_t ch : label) {
        if (iswspace(ch)) {
            pendingSpace = !normalised.empty();
            continue;
        }
        if (pendingSpace && !normalised.empty())
            normalised.push_back(L' ');
        pendingSpace = false;
        normalised.push_back(static_cast<wchar_t>(towupper(ch)));
    }
    return Trim(normalised);
}

static std::vector<std::wstring> DefaultRoadDepictionRoadLabels()
{
    return {
        L"M1", L"M4", L"M5", L"M6", L"M25", L"M62",
        L"A1(M)", L"A14", L"A27"
    };
}

static std::vector<std::wstring> NormaliseRoadDepictionRoadLabels(std::vector<std::wstring> labels)
{
    std::vector<std::wstring> out;
    std::unordered_set<std::wstring> seen;
    out.reserve(labels.size());
    for (std::wstring& label : labels) {
        label = NormalizeRoadDepictionRoadLabel(std::move(label));
        if (label.empty())
            continue;
        if (seen.insert(ToLower(label)).second)
            out.push_back(std::move(label));
    }
    std::sort(out.begin(), out.end(), [](const std::wstring& a, const std::wstring& b) {
        return ToLower(a) < ToLower(b);
        });
    return out;
}

static std::unordered_set<std::wstring> RoadDepictionRoadSet(const std::vector<std::wstring>& labels)
{
    std::unordered_set<std::wstring> out;
    for (const std::wstring& label : labels) {
        std::wstring normalised = NormalizeRoadDepictionRoadLabel(label);
        if (!normalised.empty())
            out.insert(normalised);
    }
    return out;
}

static std::wstring BuildOpenRoadsWhereClause(const std::vector<std::wstring>& roads)
{
    std::wstring where;
    for (const std::wstring& road : roads) {
        std::wstring safeRoad = NormalizeRoadDepictionRoadLabel(road);
        if (safeRoad.empty())
            continue;
        size_t quote = 0;
        while ((quote = safeRoad.find(L'\'', quote)) != std::wstring::npos) {
            safeRoad.insert(quote, 1, L'\'');
            quote += 2;
        }
        if (!where.empty())
            where += L" OR ";
        where += L"roadNumber='";
        where += safeRoad;
        where += L"'";
    }
    return where.empty() ? L"1=0" : where;
}

static std::wstring BuildOpenRoadsDownloadPageUrl(const std::vector<std::wstring>& roads, size_t offset, size_t pageSize)
{
    return std::wstring(kOpenRoadsFeatureQueryUrl) +
        L"?where=" + UrlEncodeUtf8(BuildOpenRoadsWhereClause(roads)) +
        L"&outFields=roadNumber"
        L"&returnGeometry=true"
        L"&outSR=4326"
        L"&f=geojson"
        L"&geometryPrecision=5"
        L"&orderByFields=OBJECTID"
        L"&resultRecordCount=" + std::to_wstring(pageSize) +
        L"&resultOffset=" + std::to_wstring(offset);
}

static bool DownloadOpenRoadsGeoJsonToFile(
    const std::filesystem::path& cachePath,
    const std::vector<std::wstring>& roads,
    size_t& featureCountOut,
    std::wstring& errorOut)
{
    featureCountOut = 0;
    if (roads.empty()) {
        errorOut = L"Add at least one road in the Road Depictions window before downloading.";
        return false;
    }

    const size_t pageSize = 2000;
    std::filesystem::path tempPath = cachePath;
    tempPath += L".tmp";

    std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        errorOut = L"Could not open the road depictions cache for writing.";
        return false;
    }

    out << "{\"type\":\"FeatureCollection\",\"name\":\"OS Open Roads selected roads\",\"features\":[";

    bool firstFeature = true;
    const size_t roadsPerQuery = 24;
    for (size_t start = 0; start < roads.size(); start += roadsPerQuery) {
        const size_t end = std::min(start + roadsPerQuery, roads.size());
        std::vector<std::wstring> roadChunk(roads.begin() + static_cast<std::ptrdiff_t>(start), roads.begin() + static_cast<std::ptrdiff_t>(end));

        for (size_t offset = 0; ; ) {
            std::string body;
            std::wstring pageError;
            if (!HttpGetText(BuildOpenRoadsDownloadPageUrl(roadChunk, offset, pageSize), body, pageError)) {
                errorOut = L"OS Open Roads download failed at offset " + std::to_wstring(offset) + L": " + pageError;
                out.close();
                std::error_code ec;
                std::filesystem::remove(tempPath, ec);
                return false;
            }

            json root;
            try {
                root = json::parse(body.empty() ? "{}" : body);
            }
            catch (const std::exception& e) {
                errorOut = L"OS Open Roads response could not be parsed: " + Utf8ToWide(e.what());
                out.close();
                std::error_code ec;
                std::filesystem::remove(tempPath, ec);
                return false;
            }

            auto featuresIt = root.find("features");
            if (featuresIt == root.end() || !featuresIt->is_array()) {
                errorOut = L"OS Open Roads response did not contain a GeoJSON feature list.";
                out.close();
                std::error_code ec;
                std::filesystem::remove(tempPath, ec);
                return false;
            }

            const size_t pageCount = featuresIt->size();
            if (pageCount == 0)
                break;

            for (const json& feature : *featuresIt) {
                if (!firstFeature)
                    out << ',';
                out << feature.dump();
                firstFeature = false;
                ++featureCountOut;
            }

            if (!out.good()) {
                errorOut = L"Could not write all road depictions to the local cache.";
                out.close();
                std::error_code ec;
                std::filesystem::remove(tempPath, ec);
                return false;
            }

            offset += pageCount;
            if (pageCount < pageSize)
                break;
        }
    }

    out << "]}";
    out.close();
    if (!out.good()) {
        errorOut = L"Could not finalise the road depictions cache.";
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(cachePath, ec);
    ec.clear();
    std::filesystem::rename(tempPath, cachePath, ec);
    if (ec) {
        errorOut = L"Could not replace the road depictions cache: " + Utf8ToWide(ec.message());
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    return true;
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

static void EnableNativeSpellCheck(HWND edit)
{
    if (!edit)
        return;

    LRESULT options = SendMessageW(edit, EM_GETLANGOPTIONS, 0, 0);
    SendMessageW(edit, EM_SETLANGOPTIONS, 0, static_cast<LPARAM>(options | IMF_SPELLCHECKING));
}

static std::wstring ResolveRelativeUrl(std::wstring baseUrl, std::wstring href)
{
    href = Trim(href);
    if (href.empty())
        return L"";
    std::wstring lower = ToLower(href);
    if (lower.rfind(L"http://", 0) == 0 || lower.rfind(L"https://", 0) == 0)
        return href;

    size_t query = baseUrl.find(L'?');
    if (query != std::wstring::npos)
        baseUrl = baseUrl.substr(0, query);
    size_t slash = baseUrl.find_last_of(L"/\\");
    std::wstring root = slash == std::wstring::npos ? baseUrl : baseUrl.substr(0, slash + 1);
    while (href.rfind(L"./", 0) == 0)
        href.erase(0, 2);
    if (!href.empty() && href.front() == L'/') {
        std::wsmatch m;
        if (std::regex_search(baseUrl, m, std::wregex(LR"(^((?:https?:)?//[^/]+))", std::regex_constants::icase)))
            return m[1].str() + href;
    }
    return root + href;
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

    static const wchar_t* kKnownLabels =
        L"From Location|To Location|Location|Reason|Status|Time To Clear|Return To Normal|Lanes Closed|Delay";
    std::wstring inlinePattern = LR"((?:^|\s))";
    inlinePattern += label;
    inlinePattern += LR"(\s*:\s*(.*?)(?=\s+(?:)";
    inlinePattern += kKnownLabels;
    inlinePattern += LR"()\s*:|$))";

    std::wregex inlineRe(inlinePattern, std::regex_constants::icase);
    if (std::regex_search(description, m, inlineRe) && m.size() > 1)
        return Trim(m[1].str());

    return L"";
}

static std::wstring FormatKnotsAsMph(double knots)
{
    const int mph = static_cast<int>(std::round(knots * 1.150779448));
    return std::to_wstring(mph) + L" mph";
}

static double KnotsToMph(double knots)
{
    return knots * 1.150779448;
}

static double MilesToKm(double miles)
{
    return miles / 0.621371192237334;
}

static double KmToMiles(double km)
{
    return km * 0.621371192237334;
}

static std::wstring FormatMilesSettingText(double miles)
{
    wchar_t buffer[32]{};
    const double rounded = std::round(miles);
    if (std::fabs(miles - rounded) < 0.05)
        swprintf_s(buffer, L"%.0f", rounded);
    else
        swprintf_s(buffer, L"%.1f", miles);
    return buffer;
}

static bool TryExtractAlertDelayMinutes(const TrafficAlert& alert, double& minutesOut)
{
    std::wstring delayText = ExtractLabeledNotificationField(alert.description, L"Delay");
    if (!delayText.empty() && TryParseDurationMinutes(delayText, minutesOut))
        return true;

    std::wsmatch m;
    std::wregex re(LR"(\bDelay\s*:\s*(.*?)(?=\s+(?:From Location|To Location|Location|Reason|Status|Time To Clear|Return To Normal|Lanes Closed)\s*:|$))", std::regex_constants::icase);
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
        msg.id = PickString(item, { "id", "messageId" });
        msg.author = PickString(item, { "author", "user", "name" });
        msg.username = PickString(item, { "username", "login" });
        msg.position = PickString(item, { "position", "role" });
        msg.text = PickString(item, { "text", "message", "body" });
        msg.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        if (!msg.text.empty())
            out.push_back(std::move(msg));
    }

    return out;
}

static std::vector<OnlineUser> ParseOnlineUsers(const json& root)
{
    std::vector<OnlineUser> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("users") && root["users"].is_array())
        arr = &root["users"];
    else if (root.contains("onlineUsers") && root["onlineUsers"].is_array())
        arr = &root["onlineUsers"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        OnlineUser user;
        user.id = PickString(item, { "id", "userId" });
        user.displayName = PickString(item, { "displayName", "display_name", "name" });
        user.username = PickString(item, { "username", "user" });
        user.position = PickString(item, { "position", "role" });
        user.pod = PickString(item, { "pod" });
        user.lastSeen = PickString(item, { "lastSeen", "last_seen", "timestamp" });
        if (!user.username.empty() || !user.displayName.empty())
            out.push_back(std::move(user));
    }

    return out;
}

static std::vector<PrivateMessage> ParsePrivateMessages(const json& root)
{
    std::vector<PrivateMessage> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("privateMessages") && root["privateMessages"].is_array())
        arr = &root["privateMessages"];
    else if (root.contains("messages") && root["messages"].is_array())
        arr = &root["messages"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        PrivateMessage msg;
        msg.id = PickString(item, { "id", "messageId" });
        msg.senderUsername = PickString(item, { "senderUsername", "sender_username" });
        msg.senderDisplayName = PickString(item, { "senderDisplayName", "sender_display_name" });
        msg.senderPosition = PickString(item, { "senderPosition", "sender_position" });
        msg.recipientUsername = PickString(item, { "recipientUsername", "recipient_username" });
        msg.recipientDisplayName = PickString(item, { "recipientDisplayName", "recipient_display_name" });
        msg.recipientPosition = PickString(item, { "recipientPosition", "recipient_position" });
        msg.text = PickString(item, { "text", "message", "body" });
        msg.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        if (!msg.text.empty())
            out.push_back(std::move(msg));
    }

    return out;
}

static std::vector<IncidentExclusion> ParseIncidentExclusions(const json& root)
{
    std::vector<IncidentExclusion> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("items") && root["items"].is_array())
        arr = &root["items"];
    else if (root.contains("exclusions") && root["exclusions"].is_array())
        arr = &root["exclusions"];
    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;
        IncidentExclusion exclusion;
        exclusion.key = PickString(item, { "key", "incidentKey" });
        exclusion.sourceId = PickString(item, { "sourceId", "source_id" });
        exclusion.source = PickString(item, { "source", "sourceName" });
        exclusion.road = PickString(item, { "road" });
        exclusion.summary = PickString(item, { "summary", "title" });
        exclusion.addedBy = PickString(item, { "addedBy", "added_by" });
        exclusion.addedAt = PickString(item, { "addedAt", "added_at" });
        if (!exclusion.key.empty())
            out.push_back(std::move(exclusion));
    }
    return out;
}

static std::vector<AdminLogEntry> ParseAdminLogEntries(const json& root)
{
    std::vector<AdminLogEntry> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("items") && root["items"].is_array())
        arr = &root["items"];
    else if (root.contains("logs") && root["logs"].is_array())
        arr = &root["logs"];
    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;
        AdminLogEntry entry;
        entry.event = PickString(item, { "event", "eventType", "type" });
        entry.username = PickString(item, { "username", "user" });
        entry.displayName = PickString(item, { "displayName", "display_name", "name" });
        entry.position = PickString(item, { "position", "role" });
        entry.pod = PickString(item, { "pod" });
        entry.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        entry.actor = PickString(item, { "actor", "actorUsername" });
        entry.details = PickString(item, { "details", "message" });
        out.push_back(std::move(entry));
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
                std::abs(serverNote.latitude - note.latitude) <= 1e-6 &&
                std::abs(serverNote.longitude - note.longitude) <= 1e-6;
            });

        if (!alreadyPresent)
            serverNotes.push_back(note);
    }
}

static const BinarySourceBlob* FindSourceBlobByName(const std::vector<BinarySourceBlob>& blobs, const std::wstring& name)
{
    auto found = std::find_if(blobs.begin(), blobs.end(), [&](const BinarySourceBlob& blob) {
        return blob.name == name;
        });
    return found == blobs.end() ? nullptr : &*found;
}

static const BinarySourceBlob* FindSourceBlobByUrl(const std::vector<BinarySourceBlob>& blobs, const std::wstring& url)
{
    auto found = std::find_if(blobs.begin(), blobs.end(), [&](const BinarySourceBlob& blob) {
        return ToLower(Trim(blob.url)) == ToLower(Trim(url));
        });
    return found == blobs.end() ? nullptr : &*found;
}

static bool StartsWithWide(const std::wstring& value, const std::wstring& prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static void AppendFetchIssue(std::wstring& error, const std::wstring& issue)
{
    if (issue.empty())
        return;
    if (!error.empty())
        error += L" ";
    error += issue;
}

static bool ParseRoadAlertsFromSourceBundle(
    const BinarySourceBundleResult& bundle,
    bool scotlandEnabled,
    bool unplannedOnly,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    alertsOut.clear();
    statusOut.clear();
    errorOut.clear();

    bool parsedNtis = false;
    bool sawNtis = false;
    std::string scotlandListBody;
    std::unordered_map<std::wstring, std::string> scotlandDetails;

    for (const BinarySourceBlob& blob : bundle.blobs) {
        if (blob.name == L"ntis_events") {
            sawNtis = true;
            if (!blob.ok) {
                AppendFetchIssue(errorOut, L"National Highways NTIS: " + blob.error);
                continue;
            }
            std::vector<TrafficAlert> parsed;
            std::wstring ntisStatus;
            std::wstring ntisError;
            parsedNtis = ParseNtisTrafficSnapshot(
                blob.body,
                !unplannedOnly,
                parsed,
                ntisStatus,
                ntisError);
            if (!parsedNtis) {
                AppendFetchIssue(errorOut, L"National Highways NTIS: " + ntisError);
                continue;
            }
            alertsOut.insert(
                alertsOut.end(),
                std::make_move_iterator(parsed.begin()),
                std::make_move_iterator(parsed.end()));
            statusOut = ntisStatus;
        }
        else if (blob.name == L"traffic_scotland_list") {
            if (blob.ok)
                scotlandListBody = blob.body;
            else if (scotlandEnabled)
                AppendFetchIssue(errorOut, L"Traffic Scotland: " + blob.error);
        }
        else if (StartsWithWide(blob.name, L"traffic_scotland_detail:") && blob.ok) {
            std::wstring sid = blob.name.substr(std::wstring(L"traffic_scotland_detail:").size());
            if (!sid.empty())
                scotlandDetails[sid] = blob.body;
        }
    }

    bool scotlandOk = !scotlandEnabled;
    if (scotlandEnabled && !scotlandListBody.empty()) {
        std::vector<TrafficAlert> scotlandAlerts;
        std::wstring scotlandStatus;
        std::wstring scotlandError;
        scotlandOk = ParseTrafficScotlandAlertsFromBodies(
            scotlandListBody,
            scotlandDetails,
            scotlandAlerts,
            scotlandStatus,
            scotlandError);
        if (scotlandOk) {
            if (!scotlandStatus.empty()) {
                if (!statusOut.empty())
                    statusOut += L" ";
                statusOut += scotlandStatus;
            }
            alertsOut.insert(alertsOut.end(), std::make_move_iterator(scotlandAlerts.begin()), std::make_move_iterator(scotlandAlerts.end()));
        }
        else {
            AppendFetchIssue(errorOut, L"Traffic Scotland: " + scotlandError);
        }
    }

    if (!sawNtis && !scotlandEnabled)
        AppendFetchIssue(errorOut, L"Server source bundle did not include road incident data.");
    if (bundle.fromCache)
        statusOut += L" Server cache age: " + std::to_wstring(bundle.ageMs / 1000) + L"s.";

    return parsedNtis || (scotlandEnabled && scotlandOk && !alertsOut.empty());
}

static bool ParseEarthquakesFromSourceBundle(
    const BinarySourceBundleResult& bundle,
    std::vector<EarthquakeEvent>& eventsOut,
    std::wstring& errorOut)
{
    eventsOut.clear();
    errorOut.clear();

    const BinarySourceBlob* blob = FindSourceBlobByName(bundle.blobs, L"earthquakes");
    if (!blob) {
        errorOut = L"Server source bundle did not include earthquake data.";
        return false;
    }
    if (!blob->ok) {
        errorOut = blob->error;
        return false;
    }
    eventsOut = ParseEarthquakeEvents(blob->body);
    return true;
}

static std::wstring WeatherSystemDetailKey(std::wstring value)
{
    value = Trim(value);
    std::replace(value.begin(), value.end(), L'\\', L'/');
    const size_t suffix = value.find_first_of(L"?#");
    if (suffix != std::wstring::npos)
        value.resize(suffix);
    const size_t slash = value.find_last_of(L'/');
    if (slash != std::wstring::npos)
        value = value.substr(slash + 1);
    return ToLower(Trim(value));
}

static const BinarySourceBlob* FindWeatherSystemDetailBlob(
    const std::vector<BinarySourceBlob>& blobs,
    const std::wstring& detailPath,
    const std::wstring& detailUrl)
{
    if (const BinarySourceBlob* byUrl = FindSourceBlobByUrl(blobs, detailUrl))
        return byUrl;

    const std::wstring wantedKey = WeatherSystemDetailKey(detailPath.empty() ? detailUrl : detailPath);
    if (wantedKey.empty())
        return nullptr;

    for (const BinarySourceBlob& blob : blobs) {
        std::wstring name = blob.name;
        const std::wstring prefix = L"weather_system_detail:";
        if (name.rfind(prefix, 0) == 0)
            name = name.substr(prefix.size());

        if (WeatherSystemDetailKey(name) == wantedKey ||
            WeatherSystemDetailKey(blob.url) == wantedKey)
        {
            return &blob;
        }
    }
    return nullptr;
}

static bool ParseWeatherSystemsFromSourceBundle(
    const BinarySourceBundleResult& bundle,
    std::vector<WeatherSystemEvent>& systemsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    systemsOut.clear();
    statusOut.clear();
    errorOut.clear();

    const BinarySourceBlob* main = FindSourceBlobByName(bundle.blobs, L"weather_systems_main");
    if (!main) {
        errorOut = L"Server source bundle did not include weather systems data.";
        return false;
    }
    if (!main->ok) {
        errorOut = main->error;
        return false;
    }

    systemsOut = ParseWeatherSystemEvents(main->body, statusOut);
    for (WeatherSystemEvent& system : systemsOut) {
        if (system.detailPath.empty())
            continue;
        const std::wstring trackUrl = ResolveRelativeUrl(kWeatherSystemsSourceUrl, system.detailPath);
        const BinarySourceBlob* detail = FindWeatherSystemDetailBlob(bundle.blobs, system.detailPath, trackUrl);
        if (!detail || !detail->ok)
            continue;
        std::vector<WeatherForecastPoint> track = ParseWeatherSystemForecastTrack(detail->body);
        if (!track.empty())
            system.forecastTrack = std::move(track);
    }
    if (bundle.fromCache)
        statusOut += L" Server cache age: " + std::to_wstring(bundle.ageMs / 1000) + L"s.";
    return true;
}

static bool ParseWeatherWarningsFromSourceBundle(
    const BinarySourceBundleResult& bundle,
    std::vector<WeatherWarningEvent>& warningsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    warningsOut.clear();
    statusOut.clear();
    errorOut.clear();

    const BinarySourceBlob* blob = FindSourceBlobByName(bundle.blobs, L"weather_warnings");
    if (!blob) {
        errorOut = L"Server source bundle did not include weather warning data.";
        return false;
    }
    if (!blob->ok) {
        errorOut = blob->error;
        return false;
    }
    warningsOut = ParseWeatherWarningEvents(blob->body, statusOut);
    if (bundle.fromCache)
        statusOut += L" Server cache age: " + std::to_wstring(bundle.ageMs / 1000) + L"s.";
    return true;
}

static bool ParseFloodsFromSourceBundle(
    const BinarySourceBundleResult& bundle,
    std::vector<FloodEvent>& floodsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    floodsOut.clear();
    statusOut.clear();
    errorOut.clear();

    const BinarySourceBlob* blob = FindSourceBlobByName(bundle.blobs, L"floods");
    if (!blob) {
        errorOut = L"Server source bundle did not include flood data.";
        return false;
    }
    if (!blob->ok) {
        errorOut = blob->error;
        return false;
    }
    floodsOut = ParseFloodEvents(blob->body, statusOut);
    if (bundle.fromCache)
        statusOut += L" Server cache age: " + std::to_wstring(bundle.ageMs / 1000) + L"s.";
    return true;
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
            if (RouteOwnedDialogMessage(msg))
                continue;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

private:
    static constexpr const wchar_t* kBaseTitle = L"ERC Tools";

    bool RouteOwnedDialogMessage(MSG& msg)
    {
        if (!m_hwnd || !msg.hwnd)
            return false;

        HWND root = GetAncestor(msg.hwnd, GA_ROOT);
        if (!root || !IsWindow(root) || !IsWindowVisible(root))
            return false;

        if (root == m_hwnd) {
            HWND map = m_map.Hwnd();
            if (map && (msg.hwnd == map || IsChild(map, msg.hwnd)))
                return false;
            EnableChildTabStops(m_hwnd);
            return IsDialogMessageW(m_hwnd, &msg) != FALSE;
        }

        if (GetWindow(root, GW_OWNER) != m_hwnd)
            return false;

        EnableChildTabStops(root);
        return IsDialogMessageW(root, &msg) != FALSE;
    }

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
        if (maximumWidth > 0)
            SetPropW(label, kAutoLabelMaxWidthProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(maximumWidth)));
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
            if (wParam == kAlertRefreshTimerId) {
                if (!ShouldUseServerToFetchData())
                    RefreshFeedAsync();
            }
            else if (wParam == kServerPollTimerId)
                PollServerAsync();
            else if (wParam == kEarthquakeRefreshTimerId && !ShouldUseServerToFetchData())
                FetchEarthquakesAsync(true);
            else if (wParam == kWeatherSystemsRefreshTimerId && !ShouldUseServerToFetchData()) {
                FetchWeatherSystemsAsync(true);
                FetchWeatherWarningsAsync(true);
                FetchFloodsAsync(true);
            }
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

        case WM_APP_WEATHER_WARNINGS_READY:
            OnWeatherWarningsReady(reinterpret_cast<WeatherWarningsResult*>(lParam));
            return 0;

        case WM_APP_FLOODS_READY:
            OnFloodsReady(reinterpret_cast<FloodsResult*>(lParam));
            return 0;

        case WM_APP_UPDATE_READY:
            OnClientUpdateReady(reinterpret_cast<ClientUpdateResult*>(lParam));
            return 0;

        case WM_APP_SETTINGS_SYNC_READY:
            OnGlobalSettingsSyncReady(reinterpret_cast<GlobalSettingsResult*>(lParam));
            return 0;

        case WM_APP_ADMIN_LOG_READY:
            OnAdminLogReady(reinterpret_cast<AdminLogResult*>(lParam));
            return 0;

        case WM_APP_ROAD_DEPICTIONS_READY:
            OnRoadDepictionsReady(reinterpret_cast<RoadDepictionsDownloadResult*>(lParam));
            return 0;

        case WM_APP_POPULATED_PLACES_READY:
            OnPopulatedPlacesReady(reinterpret_cast<PopulatedPlacesDownloadResult*>(lParam));
            return 0;

        case WM_APP_SERVER_SOURCE_SIGNAL_READY:
            OnServerSourceSignalReady(reinterpret_cast<ServerSourceSignalResult*>(lParam));
            return 0;

        case WM_CLOSE:
            LogoutOnlineSession(L"client_closed");
            DestroyWindow(m_hwnd);
            return 0;

        case WM_QUERYENDSESSION:
            LogoutOnlineSession(L"windows_shutdown");
            return TRUE;

        case WM_ENDSESSION:
            if (wParam)
                LogoutOnlineSession(L"windows_session_ended");
            return 0;

        case WM_DESTROY:
            LogoutOnlineSession(L"client_destroyed");
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
            if (m_boldFont) {
                DeleteObject(m_boldFont);
                m_boldFont = nullptr;
            }
            if (m_headerFont) {
                DeleteObject(m_headerFont);
                m_headerFont = nullptr;
            }
            PostQuitMessage(m_logoutRequested ? kMainWindowLogoutExitCode : 0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void OnCreate()
    {
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icc);

        EnableModernWindowFrame(m_hwnd);

        m_font = CreateUiFont(11);
        m_boldFont = CreateUiFont(11, FW_SEMIBOLD);
        m_headerFont = CreateUiFont(18, FW_SEMIBOLD);

        LoadSettings();
        EnsureDefaultReportTemplates();
        EnsureDefaultEarthquakeReportTemplates();
        EnsureDefaultWeatherSystemReportTemplates();
        EnsureDefaultWeatherWarningReportTemplates();
        EnsureDefaultFloodReportTemplates();
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
        std::wstring roadDepictionsError;
        std::unordered_set<std::wstring> allowedRoadDepictions = RoadDepictionRoadSet(m_roadDepictionRoadLabels);
        if (!m_map.LoadRoadDepictionsFromFile(GetRoadDepictionsCachePath(), &roadDepictionsError, &allowedRoadDepictions)) {
            OutputDebugStringW((L"Road depictions cache load: " + roadDepictionsError + L"\n").c_str());
        }
        LoadPopulatedPlacesCache(false);
        if (!PopulatedPlacesSupportPopulationFilter(m_populatedPlaces))
            EnsurePopulatedPlacesAvailableAsync(false);

        m_map.SetSelectCallback([this](const std::wstring& id) {
            SelectAlertById(id, true);
            });
        m_map.SetEventSelectCallback([this](const std::wstring& sourceType, const std::wstring& id) {
            SelectMapEvent(sourceType, id);
            });
        m_map.SetEventActionCallback([this](
            const std::wstring& sourceType,
            const std::wstring& id,
            const std::wstring& action) {
                HandleMapEventExclusionAction(sourceType, id, action);
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
        m_map.SetNotificationHistoryActivateCallback([this](const AppNotification& notification) {
            ActivateNotificationHistoryEntry(notification);
            });
        m_map.SetNotificationHistoryDeleteCallback([this](size_t index) {
            DeleteNotificationHistoryEntry(index);
            });
        m_map.SetNotificationHistoryActionCallback([this](const AppNotification& notification, const std::wstring& action) {
            HandleNotificationHistoryExclusionAction(notification, action);
            });
        m_map.SetChatSendCallback([this](const std::wstring& text) {
            SendChatTextAsync(text);
            });
        m_map.SetPrivateChatSendCallback([this](const std::wstring& recipientUsername, const std::wstring& text) {
            SendPrivateMessageAsync(recipientUsername, text);
            });
        m_map.SetPrivateChatStateCallback([this](const std::wstring& recipientUsername, bool open) {
            const std::wstring username = ToLower(Trim(recipientUsername));
            if (open) {
                m_activePrivateChatUsername = username;
                if (!username.empty())
                    m_privateMessageUnreadCounts.erase(username);
                RenderPrivateMessageUnreadCounts();
            }
            else if (m_activePrivateChatUsername == username) {
                m_activePrivateChatUsername.clear();
            }
            });
        m_map.SetCountdownPresetsChangedCallback([this](const std::array<std::wstring, 3>& presets) {
            m_countdownPresets = presets;
            SaveSettings();
            });
        m_map.SetChatClearCallback([this]() {
            ClearResponderChatAsync();
            });
        m_map.SetChatMessageActionCallback([this](const ChatMessage& message, const std::wstring& action) {
            HandleResponderChatMessageAction(message, action);
            });
        m_map.SetUserActionCallback([this](const OnlineUser& user, const std::wstring& action) {
            HandleOnlineUserAction(user, action);
            });
        m_map.SetPanelCloseCallback([this](const std::wstring& panelName) {
            HandleMapOverlayPanelClose(panelName);
            });
        m_map.SetMapDisplayModeCallback([this](bool displayWorldMap) {
            SetMapDisplayMode(displayWorldMap);
            });
        m_map.SetIrelandVisibilityCallback([this](bool visible) {
            m_showIreland = visible;
            m_map.SetIrelandVisible(visible);
            SaveSettings();
            SetStatusText(visible ? L"Ireland depiction enabled." : L"Ireland depiction hidden.");
            });
        m_map.SetChatClearEnabled(CanClearResponderChat());
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        m_map.SetNotificationPolygonsVisible(m_showIncidentNotificationRegionPolygons);
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        m_map.SetUsersVisible(m_showUsersOverlay);
        m_map.SetIrelandVisible(m_showIreland);
        m_map.SetIncidentOverlayVisible(m_showIncidents && m_showIncidentOverlayLabels);
        m_map.SetEarthquakeOverlayVisible(m_showEarthquakes && m_showEarthquakeOverlayLabels);
        m_map.SetWeatherSystemOverlayVisible(m_showWeatherSystems && m_showWeatherSystemOverlayLabels);
        m_map.SetWeatherWarningOverlayVisible(m_showWeatherWarnings && m_showWeatherWarningOverlayLabels);
        m_map.SetWeatherWarningPolygonsVisible(m_showWeatherWarnings && m_showWeatherWarningPolygons);
        m_map.SetFloodOverlayVisible(m_showFloods && m_showFloodOverlayLabels);
        m_map.SetAreaLabelsVisible(m_showAreaLabels);
        m_map.SetRoadDepictionsVisible(m_showRoadDepictions);
        m_map.SetHiddenRoadDepictions(m_hiddenRoadDepictionIds);
        m_map.SetDisplayWorldMap(m_displayWorldMap);
        m_map.SetFpsCounterVisible(m_showFpsCounter);
        m_map.SetToolbarVisible(m_showMapControls);
        m_map.SetCountdownPresets(m_countdownPresets);
        m_map.SetCountdownVisible(m_showCountdownTimer);
        m_map.SetCommsIndicatorVisible(m_showCommsIndicator);
        m_map.SetNotificationAvoidanceEnabled(m_avoidOverlaysForNotifications);
        ApplySoundSettings();
        RenderChatHistory();
        RenderPrivateMessages();
        RenderOnlineUsers();
        RenderNotificationHistory();

        Layout();
        SetStatusText(L"Ready.");
        ApplyRefreshTimer();
        if (IsOnlineMode())
            SetTimer(m_hwnd, kServerPollTimerId, kServerPollIntervalMs, nullptr);
        SetTimer(m_hwnd, kEarthquakeRefreshTimerId, 10 * 60 * 1000, nullptr);
        SetTimer(m_hwnd, kWeatherSystemsRefreshTimerId, 10 * 60 * 1000, nullptr);

        RefreshFeedAsync();
        FetchEarthquakesAsync(true);
        FetchWeatherSystemsAsync(true);
        FetchWeatherWarningsAsync(true);
        FetchFloodsAsync(true);
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
        case IDM_SETTINGS_GENERAL:
            ShowSettingsWindow();
            break;

        case IDM_SETTINGS_SOUNDS:
            ShowSoundsWindow();
            break;

        case IDM_FILE_CACHE_MANAGER:
            ShowCacheManagerWindow();
            break;

        case IDM_FILE_USERS:
            ToggleUsersOverlay();
            break;

        case IDM_FILE_ACCOUNT_CREATOR:
            if (CanManageAccounts())
                ShowAccountCreatorWindow();
            break;

        case IDM_FILE_ADMIN_LOG:
            if (CanViewAdministratorLog())
                ShowAdminLogWindow();
            break;

        case IDM_FILE_LOGOUT:
            LogoutOnlineSession(L"user_logout");
            m_logoutRequested = true;
            DestroyWindow(m_hwnd);
            break;

        case IDM_FILE_EXIT:
            LogoutOnlineSession(L"client_closed");
            DestroyWindow(m_hwnd);
            break;

        case IDM_ROADS_INCIDENT_FILTERS:
            ShowIncidentFiltersWindow();
            break;

        case IDM_ROADS_INCIDENT_NOTIFICATIONS:
            ShowIncidentNotificationsWindow();
            break;

        case IDM_ROADS_INCIDENTS_LIST:
            ShowIncidentsListWindow();
            break;

        case IDM_ROADS_TRAFFIC_SCOTLAND:
            ShowTrafficScotlandWindow();
            break;

        case IDM_ROADS_INCIDENT_EXCLUSIONS:
            ShowIncidentExclusionsWindow();
            break;

        case IDM_ROADS_SHOW_INCIDENTS:
            ToggleShowIncidents();
            break;

        case IDM_INCIDENT_OVERLAY_NONE:
            SetIncidentOverlayLabels(false);
            break;

        case IDM_INCIDENT_OVERLAY_SUMMARY:
            SetIncidentOverlayLabels(true);
            break;

        case IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY:
            ToggleIncidentOverlayNotifiedOnly();
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

        case IDM_WEATHER_WARNINGS_LIST:
            ShowWeatherWarningsListWindow();
            break;

        case IDM_FLOODS_LIST:
            ShowFloodsListWindow();
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

        case IDM_WEATHER_SYSTEM_FORECASTS:
            ToggleWeatherSystemForecasts();
            break;

        case IDM_SHOW_WEATHER_WARNINGS:
            ToggleShowWeatherWarnings();
            break;

        case IDM_WEATHER_WARNING_OVERLAY_NONE:
            SetWeatherWarningOverlayLabels(false);
            break;

        case IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA:
            SetWeatherWarningOverlayLabels(true);
            break;

        case IDM_WEATHER_WARNING_POLYGONS:
            ToggleWeatherWarningPolygons();
            break;

        case IDM_SHOW_FLOODS:
            ToggleShowFloods();
            break;

        case IDM_FLOOD_OVERLAY_NONE:
            SetFloodOverlayLabels(false);
            break;

        case IDM_FLOOD_OVERLAY_SEVERITY_AREA:
            SetFloodOverlayLabels(true);
            break;

        case IDM_VIEW_NOTIFICATION_HISTORY:
            ToggleNotificationHistory();
            break;

        case IDM_VIEW_AREA_LABELS:
            ToggleAreaLabels();
            break;

        case IDM_VIEW_NOTIFICATION_REGION_POLYGONS:
            ToggleIncidentNotificationRegionPolygons();
            break;

        case IDM_VIEW_ROAD_DEPICTIONS:
            ToggleRoadDepictions();
            break;

        case IDM_VIEW_ROAD_DEPICTIONS_LIST:
            ShowRoadDepictionsWindow();
            break;

        case IDM_VIEW_FPS_COUNTER:
            ToggleFpsCounter();
            break;

        case IDM_VIEW_MAP_CONTROLS:
            ToggleMapControls();
            break;

        case IDM_VIEW_COUNTDOWN_TIMER:
            ToggleCountdownTimer();
            break;

        case IDM_VIEW_COMMS_INDICATOR:
            ToggleCommsIndicator();
            break;

        case IDM_VIEW_SOUND_CUES:
            ToggleSoundCues();
            break;

        case IDM_ABOUT:
        case IDM_ABOUT_APP:
            ShowAboutDialog();
            break;

        case IDM_ABOUT_LEGEND:
            ShowLegendWindow();
            break;
        }
    }

    LRESULT OnNotify(NMHDR* nmh)
    {
        if (!nmh)
            return 0;

        if (nmh->hwndFrom == m_listView && nmh->code == NM_CUSTOMDRAW)
            return OnAlertListCustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(nmh));

        if (nmh->hwndFrom == m_incidentsListView && nmh->code == NM_CUSTOMDRAW)
            return OnIncidentsListCustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(nmh));

        if (nmh->code == NM_RCLICK && nmh->hwndFrom == m_listView) {
            int selected = static_cast<int>(SendMessageW(m_listView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredAlerts.size()))
                ShowIncidentExclusionContextMenu(m_filteredAlerts[static_cast<size_t>(selected)], m_listView);
            return 0;
        }

        if (nmh->code == NM_RCLICK && nmh->hwndFrom == m_incidentsListView) {
            int selected = static_cast<int>(SendMessageW(m_incidentsListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_incidentsListKeys.size())) {
                auto found = m_notifiedIncidentStates.find(m_incidentsListKeys[static_cast<size_t>(selected)]);
                if (found != m_notifiedIncidentStates.end())
                    ShowIncidentExclusionContextMenu(found->second.alert, m_incidentsListView);
            }
            return 0;
        }

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

        if (nmh->hwndFrom == m_incidentsListView && nmh->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
            if ((lv->uChanged & LVIF_STATE) && (lv->uNewState & LVIS_SELECTED)) {
                int selected = static_cast<int>(SendMessageW(m_incidentsListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (selected >= 0 && selected < static_cast<int>(m_incidentsListKeys.size())) {
                    const auto it = m_notifiedIncidentStates.find(m_incidentsListKeys[static_cast<size_t>(selected)]);
                    if (it != m_notifiedIncidentStates.end())
                        ShowAlertDetailsById(m_incidentsListKeys[static_cast<size_t>(selected)], true);
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
            if (row >= 0 && row < static_cast<int>(m_filteredAlerts.size()) &&
                m_filteredAlerts[static_cast<size_t>(row)].excluded) {
                cd->clrText = RGB(158, 82, 210);
                if (m_boldFont)
                    SelectObject(cd->nmcd.hdc, m_boldFont);
            }
            else if (row >= 0 && row < static_cast<int>(m_filteredAlerts.size()) && cd->iSubItem == 0) {
                cd->clrText = SeverityLabelColor(m_filteredAlerts[static_cast<size_t>(row)].severity);
                if (m_font)
                    SelectObject(cd->nmcd.hdc, m_font);
            }
            else {
                cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
                if (m_font)
                    SelectObject(cd->nmcd.hdc, m_font);
            }
            cd->clrTextBk = selected ? kUiSelection : kUiSurface;
            return CDRF_NEWFONT;
        }
        }

        return CDRF_DODEFAULT;
    }

    LRESULT OnIncidentsListCustomDraw(NMLVCUSTOMDRAW* cd)
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
            if (row >= 0 && row < static_cast<int>(m_incidentsListKeys.size())) {
                auto it = m_notifiedIncidentStates.find(m_incidentsListKeys[static_cast<size_t>(row)]);
                if (it != m_notifiedIncidentStates.end() && it->second.alert.excluded) {
                    cd->clrText = RGB(158, 82, 210);
                    if (m_boldFont)
                        SelectObject(cd->nmcd.hdc, m_boldFont);
                }
                else if (cd->iSubItem == 0) {
                    cd->clrText = it == m_notifiedIncidentStates.end()
                        ? kUiText
                        : SeverityLabelColor(it->second.alert.severity);
                    if (m_font)
                        SelectObject(cd->nmcd.hdc, m_font);
                }
                else {
                    cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
                    if (m_font)
                        SelectObject(cd->nmcd.hdc, m_font);
                }
            }
            else {
                cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
                if (m_font)
                    SelectObject(cd->nmcd.hdc, m_font);
            }
            cd->clrTextBk = selected ? kUiSelection : kUiSurface;
            return CDRF_NEWFONT;
        }
        }

        return CDRF_DODEFAULT;
    }

    void ShowIncidentExclusionContextMenu(const TrafficAlert& alert, HWND owner)
    {
        const std::wstring key = IncidentNotificationStableKey(alert);
        const bool excluded = IsIncidentExcluded(alert);
        ShowExclusionContextMenu(key, excluded, owner, [this, alert]() {
            AddIncidentExclusionAsync(alert);
            });
    }

    void ShowExclusionContextMenu(
        const std::wstring& key,
        bool excluded,
        HWND owner,
        std::function<void()> addAction)
    {
        HMENU menu = CreatePopupMenu();
        if (!menu)
            return;
        AppendMenuW(menu, MF_STRING | (excluded ? MF_GRAYED : MF_ENABLED), 1, L"Add to exclusion");
        AppendMenuW(menu, MF_STRING | (excluded ? MF_ENABLED : MF_GRAYED), 2, L"Remove from exclusion");
        POINT cursor{};
        GetCursorPos(&cursor);
        const UINT command = TrackPopupMenu(
            menu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
            cursor.x,
            cursor.y,
            0,
            owner,
            nullptr);
        DestroyMenu(menu);
        if (command == 1 && addAction)
            addAction();
        else if (command == 2)
            RemoveIncidentExclusionAsync(key);
    }

    template <typename TEvent>
    LRESULT OnExcludedEventListCustomDraw(
        NMLVCUSTOMDRAW* cd,
        const std::vector<TEvent>& events)
    {
        if (!cd)
            return CDRF_DODEFAULT;
        if (cd->nmcd.dwDrawStage == CDDS_PREPAINT)
            return CDRF_NOTIFYITEMDRAW;
        if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
            return CDRF_NOTIFYSUBITEMDRAW;
        if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
            const size_t row = static_cast<size_t>(cd->nmcd.dwItemSpec);
            const bool selected = (cd->nmcd.uItemState & CDIS_SELECTED) != 0;
            if (row < events.size() && events[row].excluded) {
                cd->clrText = RGB(158, 82, 210);
                if (m_boldFont)
                    SelectObject(cd->nmcd.hdc, m_boldFont);
            }
            else {
                cd->clrText = selected ? RGB(12, 75, 142) : kUiText;
                if (m_font)
                    SelectObject(cd->nmcd.hdc, m_font);
            }
            cd->clrTextBk = selected ? kUiSelection : kUiSurface;
            return CDRF_NEWFONT;
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

    bool ShouldUseServerToFetchData() const
    {
        return IsOnlineMode() &&
            !ServerBaseUrl().empty() &&
            !m_session.token.empty();
    }

    bool CanClearResponderChat() const
    {
        std::wstring role = ToLower(Trim(m_session.position));
        return role == L"administrator" || role == L"admin" || role == L"supervisor" || role == L"sup";
    }

    int CurrentPositionRank() const
    {
        std::wstring role = ToLower(Trim(m_session.position));
        if (role == L"administrator" || role == L"admin")
            return 4;
        if (role == L"supervisor" || role == L"sup")
            return 3;
        if (role == L"manager" || role == L"mgr")
            return 2;
        if (role == L"erc")
            return 1;
        return 0;
    }

    bool CanManageAccounts() const
    {
        return CurrentPositionRank() >= 3;
    }

    bool CanViewAdministratorLog() const
    {
        return CurrentPositionRank() >= 4;
    }

    int PositionRankText(const std::wstring& position) const
    {
        std::wstring role = ToLower(Trim(position));
        if (role == L"administrator" || role == L"admin")
            return 4;
        if (role == L"supervisor" || role == L"sup")
            return 3;
        if (role == L"manager" || role == L"mgr")
            return 2;
        if (role == L"erc")
            return 1;
        return 0;
    }

    bool CanModerateUser(const OnlineUser& user) const
    {
        const int current = CurrentPositionRank();
        const int target = PositionRankText(user.position);
        return current >= 2 && target > 0 && current > target;
    }

    void LogoutOnlineSession(const std::wstring& reason = L"client_closed")
    {
        if (m_logoutSent || !IsOnlineMode())
            return;

        m_logoutSent = true;
        BinaryCallResult binary;
        if (BinaryLogout(ServerBaseUrl(), m_session, reason, binary) || binary.protocolAvailable)
            return;

        std::string response;
        std::wstring error;
        std::string body = "{\"reason\":" + JsonEscape(reason) + "}";
        HttpPostJsonTextWithHeaders(AppendPath(ServerBaseUrl(), L"/api/auth/logout"), body, BearerAuthHeader(m_session), response, error);
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

            const int settingsVersion = root.value("version", 0);

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

            m_alertsEndpoint.clear();
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
            readBool("trafficScotlandEnabled", m_trafficScotlandEnabled);
            readString("trafficScotlandIncidentsUrl", m_trafficScotlandIncidentsUrl);
            m_periodicRefreshEnabled = false;
            readBool("showNotificationHistory", m_showNotificationHistory);
            readBool("showFpsCounter", m_showFpsCounter);
            readBool("showMapControls", m_showMapControls);
            readBool("showCountdownTimer", m_showCountdownTimer);
            readBool("showCommsIndicator", m_showCommsIndicator);
            {
                auto it = settings->find("countdownPresets");
                if (it != settings->end() && it->is_array()) {
                    const size_t count = MinValue<size_t>(m_countdownPresets.size(), it->size());
                    for (size_t i = 0; i < count; ++i) {
                        if ((*it)[i].is_string())
                            m_countdownPresets[i] = Utf8ToWide((*it)[i].get<std::string>());
                    }
                }
            }
            readBool("avoidOverlaysForNotifications", m_avoidOverlaysForNotifications);
            readBool("soundCuesEnabled", m_soundCuesEnabled);
            readBool("soundMessageEnabled", m_soundMessageEnabled);
            readBool("soundPrivateMessageEnabled", m_soundPrivateMessageEnabled);
            readBool("soundNotificationEnabled", m_soundNotificationEnabled);
            readBool("soundTimerStartEnabled", m_soundTimerStartEnabled);
            readBool("soundTimerWarningEnabled", m_soundTimerWarningEnabled);
            readBool("soundTimerCompleteEnabled", m_soundTimerCompleteEnabled);
            {
                auto it = settings->find("soundOutputDeviceId");
                if (it != settings->end()) {
                    if (it->is_number_unsigned()) {
                        m_soundOutputDeviceId = static_cast<unsigned int>(it->get<unsigned int>());
                    }
                    else if (it->is_number_integer()) {
                        const int parsed = it->get<int>();
                        if (parsed >= 0)
                            m_soundOutputDeviceId = static_cast<unsigned int>(parsed);
                    }
                    else if (it->is_string()) {
                        const std::string value = it->get<std::string>();
                        if (value == "default") {
                            m_soundOutputDeviceId = kDefaultSoundOutputDeviceId;
                        }
                        else {
                            try {
                                m_soundOutputDeviceId = static_cast<unsigned int>(std::stoul(value));
                            }
                            catch (...) {
                                m_soundOutputDeviceId = kDefaultSoundOutputDeviceId;
                            }
                        }
                    }
                }
            }
            readBool("displayWorldMap", m_displayWorldMap);
            readBool("showIreland", m_showIreland);
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
            readBool("incidentSidePanelListOnly", m_incidentSidePanelListOnly);
            readBool("showUnresolvedIncidents", m_showUnresolvedIncidents);
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
            readBool("incidentIgnoreUpdates", m_incidentIgnoreUpdates);
            readBool("showIncidentNotificationRegionPolygons", m_showIncidentNotificationRegionPolygons);
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
                    auto titleIt = item.find("title");
                    if (titleIt != item.end())
                        reportTemplate.title = JsonValueToText(*titleIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.title.empty() || !reportTemplate.body.empty())
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
                    auto titleIt = item.find("title");
                    if (titleIt != item.end())
                        reportTemplate.title = JsonValueToText(*titleIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.title.empty() || !reportTemplate.body.empty())
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
                    auto titleIt = item.find("title");
                    if (titleIt != item.end())
                        reportTemplate.title = JsonValueToText(*titleIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.title.empty() || !reportTemplate.body.empty())
                        m_weatherSystemReportTemplates.push_back(std::move(reportTemplate));
                }
            }
            auto weatherWarningTemplatesIt = settings->find("weatherWarningReportTemplates");
            if (weatherWarningTemplatesIt != settings->end() && weatherWarningTemplatesIt->is_array()) {
                m_weatherWarningReportTemplatesConfigured = true;
                m_weatherWarningReportTemplates.clear();
                for (const json& item : *weatherWarningTemplatesIt) {
                    if (!item.is_object())
                        continue;
                    ReportTemplate reportTemplate;
                    auto nameIt = item.find("name");
                    if (nameIt != item.end())
                        reportTemplate.name = JsonValueToText(*nameIt);
                    auto titleIt = item.find("title");
                    if (titleIt != item.end())
                        reportTemplate.title = JsonValueToText(*titleIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.title.empty() || !reportTemplate.body.empty())
                        m_weatherWarningReportTemplates.push_back(std::move(reportTemplate));
                }
            }
            auto floodTemplatesIt = settings->find("floodReportTemplates");
            if (floodTemplatesIt != settings->end() && floodTemplatesIt->is_array()) {
                m_floodReportTemplatesConfigured = true;
                m_floodReportTemplates.clear();
                for (const json& item : *floodTemplatesIt) {
                    if (!item.is_object())
                        continue;
                    ReportTemplate reportTemplate;
                    auto nameIt = item.find("name");
                    if (nameIt != item.end())
                        reportTemplate.name = JsonValueToText(*nameIt);
                    auto titleIt = item.find("title");
                    if (titleIt != item.end())
                        reportTemplate.title = JsonValueToText(*titleIt);
                    auto bodyIt = item.find("body");
                    if (bodyIt != item.end())
                        reportTemplate.body = JsonValueToText(*bodyIt);
                    if (!reportTemplate.name.empty() || !reportTemplate.title.empty() || !reportTemplate.body.empty())
                        m_floodReportTemplates.push_back(std::move(reportTemplate));
                }
            }

            readBool("showEarthquakes", m_showEarthquakes);
            readBool("showEarthquakeOverlayLabels", m_showEarthquakeOverlayLabels);
            readBool("showIncidents", m_showIncidents);
            readBool("showIncidentOverlayLabels", m_showIncidentOverlayLabels);
            readBool("incidentOverlayNotifiedOnly", m_incidentOverlayNotifiedOnly);
            readBool("showWeatherSystems", m_showWeatherSystems);
            readBool("showWeatherSystemOverlayLabels", m_showWeatherSystemOverlayLabels);
            readBool("showWeatherSystemForecasts", m_showWeatherSystemForecasts);
            readBool("showWeatherWarnings", m_showWeatherWarnings);
            readBool("showWeatherWarningOverlayLabels", m_showWeatherWarningOverlayLabels);
            readBool("showWeatherWarningPolygons", m_showWeatherWarningPolygons);
            readBool("showFloods", m_showFloods);
            readBool("showFloodOverlayLabels", m_showFloodOverlayLabels);
            readBool("showAreaLabels", m_showAreaLabels);
            readBool("showRoadDepictions", m_showRoadDepictions);
            auto hiddenRoadDepictionsIt = settings->find("hiddenRoadDepictions");
            if (hiddenRoadDepictionsIt != settings->end() && hiddenRoadDepictionsIt->is_array()) {
                m_hiddenRoadDepictionIds.clear();
                for (const json& item : *hiddenRoadDepictionsIt) {
                    if (item.is_string())
                        m_hiddenRoadDepictionIds.insert(NormalizeRoadDepictionRoadLabel(Utf8ToWide(item.get<std::string>())));
                }
            }
            auto roadDepictionRoadsIt = settings->find("roadDepictionRoads");
            if (roadDepictionRoadsIt != settings->end() && roadDepictionRoadsIt->is_array()) {
                std::vector<std::wstring> labels;
                for (const json& item : *roadDepictionRoadsIt) {
                    if (item.is_string())
                        labels.push_back(Utf8ToWide(item.get<std::string>()));
                }
                labels = NormaliseRoadDepictionRoadLabels(std::move(labels));
                if (!labels.empty())
                    m_roadDepictionRoadLabels = std::move(labels);
            }
            readString("earthquakeListMagnitudeText", m_earthquakeListMagnitudeText);
            readString("earthquakeListTimeText", m_earthquakeListTimeText);
            readBool("earthquakeListUseDateFilter", m_earthquakeListUseDateFilter);
            readString("earthquakeListPeriodText", m_earthquakeListPeriodText);
            readString("earthquakeListPopulatedRadiusText", m_earthquakeListPopulatedRadiusText);
            readDouble("earthquakeListPopulatedRadiusMiles", m_earthquakeListPopulatedRadiusMiles);
            readString("earthquakeListMinimumPopulationText", m_earthquakeListMinimumPopulationText);
            readDouble("earthquakeListMinimumPopulation", m_earthquakeListMinimumPopulation);
            double parsedListRadius = 0.0;
            if (TryParseDoubleText(m_earthquakeListPopulatedRadiusText, parsedListRadius))
                m_earthquakeListPopulatedRadiusMiles = MaxValue(0.0, parsedListRadius);
            else if (m_earthquakeListPopulatedRadiusText.empty() && m_earthquakeListPopulatedRadiusMiles > 0.0)
                m_earthquakeListPopulatedRadiusText = FormatMilesSettingText(m_earthquakeListPopulatedRadiusMiles);
            double parsedListPopulation = 0.0;
            if (TryParseDoubleText(m_earthquakeListMinimumPopulationText, parsedListPopulation))
                m_earthquakeListMinimumPopulation = MaxValue(1.0, parsedListPopulation);
            readString("earthquakePopulatedRadiusMilesPerMagnitudeTenthText", m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText);
            readDouble("earthquakePopulatedRadiusMilesPerMagnitudeTenth", m_earthquakePopulatedRadiusMilesPerMagnitudeTenth);
            double parsedRadiusRatio = 0.0;
            if (TryParseDoubleText(m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText, parsedRadiusRatio))
                m_earthquakePopulatedRadiusMilesPerMagnitudeTenth = MaxValue(0.0, parsedRadiusRatio);
            else if (m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText.empty())
                m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText = FormatMilesSettingText(m_earthquakePopulatedRadiusMilesPerMagnitudeTenth);
            readString("weatherSystemsListForecastText", m_weatherSystemsListForecastText);
            readString("weatherWarningsListPeriodText", m_weatherWarningsListPeriodText);
            readString("floodsListPeriodText", m_floodsListPeriodText);
            auto hiddenWeatherSystemForecastsIt = settings->find("hiddenWeatherSystemForecasts");
            if (hiddenWeatherSystemForecastsIt != settings->end() && hiddenWeatherSystemForecastsIt->is_array()) {
                m_hiddenWeatherSystemForecastIds.clear();
                for (const json& item : *hiddenWeatherSystemForecastsIt) {
                    if (item.is_string())
                        m_hiddenWeatherSystemForecastIds.insert(Utf8ToWide(item.get<std::string>()));
                }
            }
            auto hiddenWeatherWarningPolygonsIt = settings->find("hiddenWeatherWarningPolygons");
            if (hiddenWeatherWarningPolygonsIt != settings->end() && hiddenWeatherWarningPolygonsIt->is_array()) {
                m_hiddenWeatherWarningPolygonIds.clear();
                for (const json& item : *hiddenWeatherWarningPolygonsIt) {
                    if (item.is_string())
                        m_hiddenWeatherWarningPolygonIds.insert(Utf8ToWide(item.get<std::string>()));
                }
            }
            readString("earthquakeNotificationMagnitudeText", m_earthquakeNotificationMagnitudeText);
            readDouble("earthquakeNotificationMagnitude", m_earthquakeNotificationMagnitude);
            readString("earthquakeNotificationTimeText", m_earthquakeNotificationTimeText);
            readBool("earthquakeNotificationUseDateFilter", m_earthquakeNotificationUseDateFilter);
            readString("earthquakeNotificationPeriodText", m_earthquakeNotificationPeriodText);
            readString("earthquakeNotificationPopulatedRadiusText", m_earthquakeNotificationPopulatedRadiusText);
            readString("earthquakeNotificationMinimumPopulationText", m_earthquakeNotificationMinimumPopulationText);
            readDouble("earthquakeNotificationMinimumPopulation", m_earthquakeNotificationMinimumPopulation);
            const bool hasNotificationRadiusMiles = settings->find("earthquakeNotificationPopulatedRadiusMiles") != settings->end();
            if (hasNotificationRadiusMiles) {
                readDouble("earthquakeNotificationPopulatedRadiusMiles", m_earthquakeNotificationPopulatedRadiusMiles);
            }
            else {
                double legacyRadiusKm = 0.0;
                readDouble("earthquakeNotificationPopulatedRadiusKm", legacyRadiusKm);
                if (legacyRadiusKm > 0.0) {
                    m_earthquakeNotificationPopulatedRadiusMiles = KmToMiles(legacyRadiusKm);
                    m_earthquakeNotificationPopulatedRadiusText = FormatMilesSettingText(m_earthquakeNotificationPopulatedRadiusMiles);
                }
            }
            double parsedMag = 0.0;
            if (TryParseDoubleText(m_earthquakeNotificationMagnitudeText, parsedMag))
                m_earthquakeNotificationMagnitude = parsedMag;
            double parsedRadius = 0.0;
            if (TryParseDoubleText(m_earthquakeNotificationPopulatedRadiusText, parsedRadius))
                m_earthquakeNotificationPopulatedRadiusMiles = MaxValue(0.0, parsedRadius);
            else if (m_earthquakeNotificationPopulatedRadiusText.empty() && m_earthquakeNotificationPopulatedRadiusMiles > 0.0)
                m_earthquakeNotificationPopulatedRadiusText = FormatMilesSettingText(m_earthquakeNotificationPopulatedRadiusMiles);
            double parsedNotificationPopulation = 0.0;
            if (TryParseDoubleText(m_earthquakeNotificationMinimumPopulationText, parsedNotificationPopulation))
                m_earthquakeNotificationMinimumPopulation = MaxValue(1.0, parsedNotificationPopulation);
            readString("weatherSystemNotificationWindText", m_weatherSystemNotificationWindText);
            const bool hasWindMphSetting = settings->find("weatherSystemNotificationWindMph") != settings->end();
            const bool hasLegacyWindKnotsSetting = settings->find("weatherSystemNotificationWindKnots") != settings->end();
            if (!hasWindMphSetting && hasLegacyWindKnotsSetting) {
                double legacyKnots = 0.0;
                readDouble("weatherSystemNotificationWindKnots", legacyKnots);
                if (legacyKnots > 0.0) {
                    m_weatherSystemNotificationWindMph = KnotsToMph(legacyKnots);
                    m_weatherSystemNotificationWindText = std::to_wstring(static_cast<int>(std::round(m_weatherSystemNotificationWindMph)));
                }
            }
            else {
                readDouble("weatherSystemNotificationWindMph", m_weatherSystemNotificationWindMph);
                double parsedWind = 0.0;
                if (TryParseDoubleText(m_weatherSystemNotificationWindText, parsedWind))
                    m_weatherSystemNotificationWindMph = parsedWind;
            }

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
        reportTemplate.name = L"England road incident";
        reportTemplate.title = L"UK - ENGLAND - $TITLE_SHORT - $ROAD $DIRECTION %JUNCTION_RANGE.";
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
        reportTemplate.title = L"EARTHQUAKE - M$MAGNITUDE - $PLACE.";
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
        reportTemplate.title = L"WEATHER SYSTEM - $SYSTEM - $CATEGORY - $BASIN.";
        reportTemplate.body = L"$DATE: WEATHER SYSTEM REPORTS: $SYSTEM is active in the $BASIN basin as a $CATEGORY with winds of $WIND. Current position: %LATITUDE, %LONGITUDE. 24-hour projection: %FORECAST_LATITUDE, %FORECAST_LONGITUDE with $FORECAST_WIND winds.";
        m_weatherSystemReportTemplates.push_back(std::move(reportTemplate));
        m_weatherSystemReportTemplatesConfigured = true;
    }

    void EnsureDefaultWeatherWarningReportTemplates()
    {
        if (!m_weatherWarningReportTemplates.empty() || m_weatherWarningReportTemplatesConfigured)
            return;

        ReportTemplate reportTemplate;
        reportTemplate.name = L"Weather warning report";
        reportTemplate.title = L"WEATHER WARNING - $COLOUR $TYPE - $AREA.";
        reportTemplate.body = L"$DATE: WEATHER WARNING REPORTS: A $COLOUR $TYPE warning is in force for $AREA from $FROM to $TO. Headline: $HEADLINE. Details: $DETAIL. Coordinates: %LATITUDE, %LONGITUDE.";
        m_weatherWarningReportTemplates.push_back(std::move(reportTemplate));
        m_weatherWarningReportTemplatesConfigured = true;
    }

    void EnsureDefaultFloodReportTemplates()
    {
        if (!m_floodReportTemplates.empty() || m_floodReportTemplatesConfigured)
            return;

        ReportTemplate reportTemplate;
        reportTemplate.name = L"Flood report";
        reportTemplate.title = L"FLOOD - $SEVERITY - $AREA.";
        reportTemplate.body = L"$DATE: FLOOD REPORTS: $SEVERITY for $AREA in $REGION. Source: $RIVER_OR_SEA. Message: $MESSAGE. Updated: $UPDATED. Coordinates: %LATITUDE, %LONGITUDE.";
        m_floodReportTemplates.push_back(std::move(reportTemplate));
        m_floodReportTemplatesConfigured = true;
    }

    std::vector<ReportTemplate>& TemplatesForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplates;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplates;
        if (context == TemplateContext::WeatherWarnings)
            return m_weatherWarningReportTemplates;
        if (context == TemplateContext::Floods)
            return m_floodReportTemplates;
        return m_reportTemplates;
    }

    const std::vector<ReportTemplate>& TemplatesForContext(TemplateContext context) const
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplates;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplates;
        if (context == TemplateContext::WeatherWarnings)
            return m_weatherWarningReportTemplates;
        if (context == TemplateContext::Floods)
            return m_floodReportTemplates;
        return m_reportTemplates;
    }

    bool& TemplatesConfiguredForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            return m_earthquakeReportTemplatesConfigured;
        if (context == TemplateContext::WeatherSystems)
            return m_weatherSystemReportTemplatesConfigured;
        if (context == TemplateContext::WeatherWarnings)
            return m_weatherWarningReportTemplatesConfigured;
        if (context == TemplateContext::Floods)
            return m_floodReportTemplatesConfigured;
        return m_reportTemplatesConfigured;
    }

    std::wstring DefaultTemplateBodyForContext(TemplateContext context) const
    {
        if (context == TemplateContext::Earthquakes)
            return L"$DATE: EARTHQUAKE REPORTS: An earthquake of magnitude $MAGNITUDE was recorded near $PLACE at $TIME. Coordinates: %LATITUDE, %LONGITUDE. Depth: %DEPTH km.";
        if (context == TemplateContext::WeatherSystems)
            return L"$DATE: WEATHER SYSTEM REPORTS: $SYSTEM is active in the $BASIN basin as a $CATEGORY with winds of $WIND. Current position: %LATITUDE, %LONGITUDE.";
        if (context == TemplateContext::WeatherWarnings)
            return L"$DATE: WEATHER WARNING REPORTS: A $COLOUR $TYPE warning is in force for $AREA from $FROM to $TO. Headline: $HEADLINE. Details: $DETAIL. Coordinates: %LATITUDE, %LONGITUDE.";
        if (context == TemplateContext::Floods)
            return L"$DATE: FLOOD REPORTS: $SEVERITY for $AREA in $REGION. Source: $RIVER_OR_SEA. Message: $MESSAGE. Updated: $UPDATED. Coordinates: %LATITUDE, %LONGITUDE.";
        return L"$DATE: NATIONAL HIGHWAYS REPORTS: A $TITLE on the $ROAD $DIRECTION %JUNCTIONS_WITH_DATA with %LANECLOSURES closed. Allow extra time for your journey.";
    }

    std::wstring DefaultTemplateTitleForContext(TemplateContext context) const
    {
        if (context == TemplateContext::Earthquakes)
            return L"EARTHQUAKE - M$MAGNITUDE - $PLACE.";
        if (context == TemplateContext::WeatherSystems)
            return L"WEATHER SYSTEM - $SYSTEM - $CATEGORY - $BASIN.";
        if (context == TemplateContext::WeatherWarnings)
            return L"WEATHER WARNING - $COLOUR $TYPE - $AREA.";
        if (context == TemplateContext::Floods)
            return L"FLOOD - $SEVERITY - $AREA.";
        return L"UK - ENGLAND - $TITLE_SHORT - $ROAD $DIRECTION %JUNCTION_RANGE.";
    }

    void EnsureDefaultTemplatesForContext(TemplateContext context)
    {
        if (context == TemplateContext::Earthquakes)
            EnsureDefaultEarthquakeReportTemplates();
        else if (context == TemplateContext::WeatherSystems)
            EnsureDefaultWeatherSystemReportTemplates();
        else if (context == TemplateContext::WeatherWarnings)
            EnsureDefaultWeatherWarningReportTemplates();
        else if (context == TemplateContext::Floods)
            EnsureDefaultFloodReportTemplates();
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
            if (Trim(reportTemplate.title).empty())
                reportTemplate.title = DefaultTemplateTitleForContext(TemplateContext::Roads);
        }
        for (ReportTemplate& reportTemplate : m_earthquakeReportTemplates) {
            if (Trim(reportTemplate.title).empty())
                reportTemplate.title = DefaultTemplateTitleForContext(TemplateContext::Earthquakes);
        }
        for (ReportTemplate& reportTemplate : m_weatherSystemReportTemplates) {
            if (Trim(reportTemplate.title).empty())
                reportTemplate.title = DefaultTemplateTitleForContext(TemplateContext::WeatherSystems);
        }
        for (ReportTemplate& reportTemplate : m_weatherWarningReportTemplates) {
            if (Trim(reportTemplate.title).empty())
                reportTemplate.title = DefaultTemplateTitleForContext(TemplateContext::WeatherWarnings);
        }
        for (ReportTemplate& reportTemplate : m_floodReportTemplates) {
            if (Trim(reportTemplate.title).empty())
                reportTemplate.title = DefaultTemplateTitleForContext(TemplateContext::Floods);
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

            root["version"] = 2;
            json& settings = root["settings"];
            if (!settings.is_object())
                settings = json::object();

            for (auto it = settings.begin(); it != settings.end();) {
                std::string key = it.key();
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                    });
                if (key.find("apikeyprotected") != std::string::npos)
                    it = settings.erase(it);
                else
                    ++it;
            }
            settings.erase("alertsEndpoint");
            settings.erase("useServerToFetchData");
            settings.erase("periodicRefreshEnabled");

            settings["serverBaseUrl"] = WideToUtf8(m_serverBaseUrl);
            settings["alertOrder"] = WideToUtf8(m_alertOrder);
            settings["alertFilterUnplannedOnly"] = m_alertFilterUnplannedOnly;
            settings["trafficScotlandEnabled"] = m_trafficScotlandEnabled;
            settings["trafficScotlandIncidentsUrl"] = WideToUtf8(m_trafficScotlandIncidentsUrl);
            settings["showNotificationHistory"] = m_showNotificationHistory;
            settings["showFpsCounter"] = m_showFpsCounter;
            settings["showMapControls"] = m_showMapControls;
            settings["showCountdownTimer"] = m_showCountdownTimer;
            settings["showCommsIndicator"] = m_showCommsIndicator;
            settings["countdownPresets"] = json::array({
                WideToUtf8(m_countdownPresets[0]),
                WideToUtf8(m_countdownPresets[1]),
                WideToUtf8(m_countdownPresets[2])
                });
            settings["avoidOverlaysForNotifications"] = m_avoidOverlaysForNotifications;
            settings["soundCuesEnabled"] = m_soundCuesEnabled;
            settings["soundMessageEnabled"] = m_soundMessageEnabled;
            settings["soundPrivateMessageEnabled"] = m_soundPrivateMessageEnabled;
            settings["soundNotificationEnabled"] = m_soundNotificationEnabled;
            settings["soundTimerStartEnabled"] = m_soundTimerStartEnabled;
            settings["soundTimerWarningEnabled"] = m_soundTimerWarningEnabled;
            settings["soundTimerCompleteEnabled"] = m_soundTimerCompleteEnabled;
            settings["soundOutputDeviceId"] = m_soundOutputDeviceId == kDefaultSoundOutputDeviceId
                ? json("default")
                : json(m_soundOutputDeviceId);
            settings["displayWorldMap"] = m_displayWorldMap;
            settings["showIreland"] = m_showIreland;
            settings["syncSettingsFromServer"] = m_syncSettingsFromServer;
            settings["refreshIntervalText"] = WideToUtf8(m_refreshIntervalText);
            settings["refreshIntervalMs"] = m_refreshIntervalMs;
            settings["incidentFilterSevere"] = m_incidentFilterSevere;
            settings["incidentFilterModerate"] = m_incidentFilterModerate;
            settings["incidentFilterMinor"] = m_incidentFilterMinor;
            settings["incidentFilterUnknown"] = m_incidentFilterUnknown;
            settings["incidentFilterUnplanned"] = m_incidentFilterUnplanned;
            settings["incidentFilterPlanned"] = m_incidentFilterPlanned;
            settings["incidentSidePanelListOnly"] = m_incidentSidePanelListOnly;
            settings["showUnresolvedIncidents"] = m_showUnresolvedIncidents;
            settings["incidentNotifyRoads"] = WideToUtf8(m_incidentNotifyRoads);
            settings["incidentNotifyRoadExclusions"] = WideToUtf8(m_incidentNotifyRoadExclusions);
            settings["incidentNotifyLaneThresholdText"] = WideToUtf8(m_incidentNotifyLaneThresholdText);
            settings["incidentNotifyLaneThreshold"] = m_incidentNotifyLaneThreshold;
            settings["incidentNotifyDelayThresholdText"] = WideToUtf8(m_incidentNotifyDelayThresholdText);
            settings["incidentNotifyDelayThresholdMinutes"] = m_incidentNotifyDelayThresholdMinutes;
            settings["incidentNotifyThresholdUseOr"] = m_incidentNotifyThresholdUseOr;
            settings["incidentIgnoreUpdates"] = m_incidentIgnoreUpdates;
            settings["showIncidentNotificationRegionPolygons"] = m_showIncidentNotificationRegionPolygons;
            settings["incidentNotifyReasonExclusions"] = WideToUtf8(m_incidentNotifyReasonExclusions);
            settings["incidentNotifyLocationExclusions"] = WideToUtf8(m_incidentNotifyLocationExclusions);
            settings["incidentNotificationRegions"] = json::array();
            for (const GeoPolygon& polygon : m_incidentNotificationRegions)
                settings["incidentNotificationRegions"].push_back(GeoPolygonToJson(polygon));
            settings["roadReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_reportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["title"] = WideToUtf8(reportTemplate.title);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["roadReportTemplates"].push_back(std::move(item));
            }
            settings["earthquakeReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_earthquakeReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["title"] = WideToUtf8(reportTemplate.title);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["earthquakeReportTemplates"].push_back(std::move(item));
            }
            settings["weatherSystemReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_weatherSystemReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["title"] = WideToUtf8(reportTemplate.title);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["weatherSystemReportTemplates"].push_back(std::move(item));
            }
            settings["weatherWarningReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_weatherWarningReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["title"] = WideToUtf8(reportTemplate.title);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["weatherWarningReportTemplates"].push_back(std::move(item));
            }
            settings["floodReportTemplates"] = json::array();
            for (const ReportTemplate& reportTemplate : m_floodReportTemplates) {
                json item = json::object();
                item["name"] = WideToUtf8(reportTemplate.name);
                item["title"] = WideToUtf8(reportTemplate.title);
                item["body"] = WideToUtf8(reportTemplate.body);
                settings["floodReportTemplates"].push_back(std::move(item));
            }
            settings["showEarthquakes"] = m_showEarthquakes;
            settings["showEarthquakeOverlayLabels"] = m_showEarthquakeOverlayLabels;
            settings["showIncidents"] = m_showIncidents;
            settings["showIncidentOverlayLabels"] = m_showIncidentOverlayLabels;
            settings["incidentOverlayNotifiedOnly"] = m_incidentOverlayNotifiedOnly;
            settings["showWeatherSystems"] = m_showWeatherSystems;
            settings["showWeatherSystemOverlayLabels"] = m_showWeatherSystemOverlayLabels;
            settings["showWeatherSystemForecasts"] = m_showWeatherSystemForecasts;
            settings["showWeatherWarnings"] = m_showWeatherWarnings;
            settings["showWeatherWarningOverlayLabels"] = m_showWeatherWarningOverlayLabels;
            settings["showWeatherWarningPolygons"] = m_showWeatherWarningPolygons;
            settings["showFloods"] = m_showFloods;
            settings["showFloodOverlayLabels"] = m_showFloodOverlayLabels;
            settings["showAreaLabels"] = m_showAreaLabels;
            settings["showRoadDepictions"] = m_showRoadDepictions;
            settings["roadDepictionRoads"] = json::array();
            for (const std::wstring& id : m_roadDepictionRoadLabels)
                settings["roadDepictionRoads"].push_back(WideToUtf8(id));
            settings["hiddenRoadDepictions"] = json::array();
            for (const std::wstring& id : m_hiddenRoadDepictionIds)
                settings["hiddenRoadDepictions"].push_back(WideToUtf8(id));
            settings["earthquakeListMagnitudeText"] = WideToUtf8(m_earthquakeListMagnitudeText);
            settings["earthquakeListTimeText"] = WideToUtf8(m_earthquakeListTimeText);
            settings["earthquakeListUseDateFilter"] = m_earthquakeListUseDateFilter;
            settings["earthquakeListPeriodText"] = WideToUtf8(m_earthquakeListPeriodText);
            settings["earthquakeListPopulatedRadiusText"] = WideToUtf8(m_earthquakeListPopulatedRadiusText);
            settings["earthquakeListPopulatedRadiusMiles"] = m_earthquakeListPopulatedRadiusMiles;
            settings["earthquakeListMinimumPopulationText"] = WideToUtf8(m_earthquakeListMinimumPopulationText);
            settings["earthquakeListMinimumPopulation"] = m_earthquakeListMinimumPopulation;
            settings["earthquakePopulatedRadiusMilesPerMagnitudeTenthText"] = WideToUtf8(m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText);
            settings["earthquakePopulatedRadiusMilesPerMagnitudeTenth"] = m_earthquakePopulatedRadiusMilesPerMagnitudeTenth;
            settings["weatherSystemsListForecastText"] = WideToUtf8(m_weatherSystemsListForecastText);
            settings["weatherWarningsListPeriodText"] = WideToUtf8(m_weatherWarningsListPeriodText);
            settings["floodsListPeriodText"] = WideToUtf8(m_floodsListPeriodText);
            settings["hiddenWeatherSystemForecasts"] = json::array();
            for (const std::wstring& id : m_hiddenWeatherSystemForecastIds)
                settings["hiddenWeatherSystemForecasts"].push_back(WideToUtf8(id));
            settings["hiddenWeatherWarningPolygons"] = json::array();
            for (const std::wstring& id : m_hiddenWeatherWarningPolygonIds)
                settings["hiddenWeatherWarningPolygons"].push_back(WideToUtf8(id));
            settings["earthquakeFilterRegion"] = json::array();
            for (const GeoPoint& point : m_earthquakeFilterRegion) {
                settings["earthquakeFilterRegion"].push_back({
                    { "lat", point.lat },
                    { "lon", point.lon }
                    });
            }
            settings["earthquakeNotificationMagnitudeText"] = WideToUtf8(m_earthquakeNotificationMagnitudeText);
            settings["earthquakeNotificationMagnitude"] = m_earthquakeNotificationMagnitude;
            settings["earthquakeNotificationTimeText"] = WideToUtf8(m_earthquakeNotificationTimeText);
            settings["earthquakeNotificationUseDateFilter"] = m_earthquakeNotificationUseDateFilter;
            settings["earthquakeNotificationPeriodText"] = WideToUtf8(m_earthquakeNotificationPeriodText);
            settings["earthquakeNotificationPopulatedRadiusText"] = WideToUtf8(m_earthquakeNotificationPopulatedRadiusText);
            settings["earthquakeNotificationPopulatedRadiusMiles"] = m_earthquakeNotificationPopulatedRadiusMiles;
            settings["earthquakeNotificationPopulatedRadiusKm"] = MilesToKm(m_earthquakeNotificationPopulatedRadiusMiles);
            settings["earthquakeNotificationMinimumPopulationText"] = WideToUtf8(m_earthquakeNotificationMinimumPopulationText);
            settings["earthquakeNotificationMinimumPopulation"] = m_earthquakeNotificationMinimumPopulation;
            settings["weatherSystemNotificationWindText"] = WideToUtf8(m_weatherSystemNotificationWindText);
            settings["weatherSystemNotificationWindMph"] = m_weatherSystemNotificationWindMph;

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

    bool ReadLocalSettingsForRemotePush(json& settingsOut, std::wstring& errorOut)
    {
        SaveSettings();

        try {
            std::ifstream in(GetSettingsPath(), std::ios::binary);
            if (!in) {
                errorOut = L"Could not open the local settings file.";
                return false;
            }

            json root = json::parse(in);
            if (root.is_object()) {
                auto settingsIt = root.find("settings");
                if (settingsIt != root.end() && settingsIt->is_object()) {
                    settingsOut = *settingsIt;
                    return true;
                }
            }

            errorOut = L"Local settings file does not contain a settings object.";
            return false;
        }
        catch (const std::exception& e) {
            errorOut = L"Could not read local settings: " + Utf8ToWide(e.what());
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
        EnsureDefaultWeatherWarningReportTemplates();
        EnsureDefaultFloodReportTemplates();
        ModernizeReportTemplates();

        ApplyRefreshTimer();
        UpdateRoadsMenu();
        UpdateNotificationHistoryMenu();
        UpdateEarthquakeMenu();
        UpdateWeatherSystemsMenu();
        UpdateWeatherWarningMenu();
        UpdateFloodMenu();
        UpdateViewMenu();
        m_map.SetNotificationPolygons(m_incidentNotificationRegions);
        m_map.SetNotificationPolygonsVisible(m_showIncidentNotificationRegionPolygons);
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        m_map.SetDisplayWorldMap(m_displayWorldMap);
        m_map.SetNotificationAvoidanceEnabled(m_avoidOverlaysForNotifications);
        m_map.SetAreaLabelsVisible(m_showAreaLabels);
        m_map.SetRoadDepictionsVisible(m_showRoadDepictions);

        SortAlertsForCurrentOrder();
        if (m_listView)
            ApplyFilters(false);

        RebuildFilteredEarthquakes();
        RenderEarthquakeListRows();
        ApplyEarthquakeVisibility();
        ApplyWeatherSystemsListFilter(false);
        ApplyWeatherWarningsListFilter(false);
        ApplyFloodsListFilter(false);
        RenderNotificationHistory();
        SyncSettingsControls();
    }

    void SyncGlobalSettingsFromServerAsync()
    {
        if (!IsOnlineMode()) {
            SetStatusText(L"Remote Settings needs an online session.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            SetStatusText(L"Set the collaboration server before pulling remote settings.");
            return;
        }

        SetStatusText(L"Pulling remote settings...");
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session]() {
            auto* result = new GlobalSettingsResult{};
            BinaryCallResult binary;
            json binarySettings;
            if (BinaryGetGlobalSettings(server, session, binarySettings, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->settings = std::move(binarySettings);
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SETTINGS_SYNC_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

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

    void PushGlobalSettingsToServerAsync()
    {
        if (!IsOnlineMode()) {
            SetStatusText(L"Remote Settings needs an online session.");
            return;
        }
        if (!CanManageAccounts()) {
            SetStatusText(L"Only Administrators and Supervisors can push remote settings.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            SetStatusText(L"Set the collaboration server before pushing remote settings.");
            return;
        }

        json settings;
        std::wstring readError;
        if (!ReadLocalSettingsForRemotePush(settings, readError)) {
            SetStatusText(readError);
            return;
        }

        SetStatusText(L"Pushing settings remotely...");
        EnableWindow(m_settingsPushRemoteBtn, FALSE);
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, settings = std::move(settings)]() {
            auto* result = new GlobalSettingsResult{};
            result->push = true;

            BinaryCallResult binary;
            if (BinarySetGlobalSettings(server, session, settings, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->settings = settings;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SETTINGS_SYNC_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            json body = json::object({ { "settings", settings } });
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/settings/global"), body.dump(), authHeaders, response, error);
            result->settings = settings;
            result->error = error;
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

        if (result->push) {
            EnableWindow(m_settingsPushRemoteBtn, IsOnlineMode() && CanManageAccounts());
            SetStatusText(result->ok
                ? L"Settings pushed remotely."
                : (result->error.empty() ? L"Remote settings push failed." : L"Remote settings push failed: " + result->error));
            return;
        }

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
        SetStatusText(L"Remote settings applied.");
    }

    void ApplyRefreshTimer()
    {
        KillTimer(m_hwnd, kAlertRefreshTimerId);
        if (ShouldUseServerToFetchData()) {
            StartServerSourceSyncEpoch();
            return;
        }

        CancelServerSourceSync();
        if (m_periodicRefreshEnabled)
            SetTimer(m_hwnd, kAlertRefreshTimerId, m_refreshIntervalMs, nullptr);
    }

    void StartServerSourceSyncEpoch()
    {
        m_serverSourceSyncEpoch.fetch_add(1, std::memory_order_acq_rel);
        m_roadsSourceGeneration = 0;
        m_earthquakeSourceGeneration = 0;
        m_weatherSystemsSourceGeneration = 0;
        m_weatherWarningsSourceGeneration = 0;
        m_floodsSourceGeneration = 0;
        m_roadsSourceWaitInProgress.store(false);
        m_earthquakeSourceWaitInProgress.store(false);
        m_weatherSystemsSourceWaitInProgress.store(false);
        m_weatherWarningsSourceWaitInProgress.store(false);
        m_floodsSourceWaitInProgress.store(false);
    }

    void CancelServerSourceSync()
    {
        m_serverSourceSyncEpoch.fetch_add(1, std::memory_order_acq_rel);
        m_roadsSourceWaitInProgress.store(false);
        m_earthquakeSourceWaitInProgress.store(false);
        m_weatherSystemsSourceWaitInProgress.store(false);
        m_weatherWarningsSourceWaitInProgress.store(false);
        m_floodsSourceWaitInProgress.store(false);
    }

    std::atomic_bool* ServerSourceWaitFlag(ServerSourceKind source)
    {
        switch (source) {
        case ServerSourceKind::Roads:
            return &m_roadsSourceWaitInProgress;
        case ServerSourceKind::Earthquakes:
            return &m_earthquakeSourceWaitInProgress;
        case ServerSourceKind::WeatherSystems:
            return &m_weatherSystemsSourceWaitInProgress;
        case ServerSourceKind::WeatherWarnings:
            return &m_weatherWarningsSourceWaitInProgress;
        case ServerSourceKind::Floods:
            return &m_floodsSourceWaitInProgress;
        default:
            return nullptr;
        }
    }

    void ResetServerSourceWaitFlag(ServerSourceKind source)
    {
        if (std::atomic_bool* flag = ServerSourceWaitFlag(source))
            flag->store(false);
    }

    uint32_t ServerSourceGeneration(ServerSourceKind source) const
    {
        switch (source) {
        case ServerSourceKind::Roads:
            return m_roadsSourceGeneration;
        case ServerSourceKind::Earthquakes:
            return m_earthquakeSourceGeneration;
        case ServerSourceKind::WeatherSystems:
            return m_weatherSystemsSourceGeneration;
        case ServerSourceKind::WeatherWarnings:
            return m_weatherWarningsSourceGeneration;
        case ServerSourceKind::Floods:
            return m_floodsSourceGeneration;
        default:
            return 0;
        }
    }

    void StoreServerSourceGeneration(ServerSourceKind source, uint32_t generation)
    {
        if (generation == 0)
            return;

        switch (source) {
        case ServerSourceKind::Roads:
            m_roadsSourceGeneration = generation;
            break;
        case ServerSourceKind::Earthquakes:
            m_earthquakeSourceGeneration = generation;
            break;
        case ServerSourceKind::WeatherSystems:
            m_weatherSystemsSourceGeneration = generation;
            break;
        case ServerSourceKind::WeatherWarnings:
            m_weatherWarningsSourceGeneration = generation;
            break;
        case ServerSourceKind::Floods:
            m_floodsSourceGeneration = generation;
            break;
        default:
            break;
        }
    }

    std::wstring ServerSourceType(ServerSourceKind source) const
    {
        switch (source) {
        case ServerSourceKind::Roads:
            return L"roads";
        case ServerSourceKind::Earthquakes:
            return L"earthquakes";
        case ServerSourceKind::WeatherSystems:
            return L"weather_systems";
        case ServerSourceKind::WeatherWarnings:
            return L"weather_warnings";
        case ServerSourceKind::Floods:
            return L"floods";
        default:
            return L"";
        }
    }

    json ServerSourceOptions(ServerSourceKind source) const
    {
        switch (source) {
        case ServerSourceKind::Roads: {
            return {
                { "unplannedOnly", m_alertFilterUnplannedOnly },
                { "trafficScotlandEnabled", m_trafficScotlandEnabled },
                { "trafficScotlandIncidentsUrl", WideToUtf8(m_trafficScotlandIncidentsUrl) }
            };
        }
        case ServerSourceKind::Earthquakes:
            return { { "url", WideToUtf8(EarthquakeQueryUrl()) } };
        case ServerSourceKind::WeatherSystems:
            return { { "url", WideToUtf8(std::wstring(kWeatherSystemsSourceUrl)) } };
        case ServerSourceKind::WeatherWarnings:
            return { { "url", WideToUtf8(WeatherWarningsQueryUrl()) } };
        case ServerSourceKind::Floods:
            return { { "url", WideToUtf8(std::wstring(kFloodsSourceUrl)) } };
        default:
            return json::object();
        }
    }

    bool IsCurrentServerSourceEpoch(uint32_t epoch) const
    {
        return ShouldUseServerToFetchData() &&
            epoch == m_serverSourceSyncEpoch.load(std::memory_order_acquire);
    }

    static void PostServerSourceSignal(
        HWND hwnd,
        ServerSourceKind source,
        uint32_t epoch,
        bool ok,
        bool changed,
        uint32_t generation,
        const std::wstring& error)
    {
        auto* signal = new ServerSourceSignalResult{};
        signal->source = source;
        signal->epoch = epoch;
        signal->ok = ok;
        signal->changed = changed;
        signal->generation = generation;
        signal->error = error;
        if (!PostMessageW(hwnd, WM_APP_SERVER_SOURCE_SIGNAL_READY, 0, reinterpret_cast<LPARAM>(signal)))
            delete signal;
    }

    void ScheduleServerSourceWait(ServerSourceKind source)
    {
        if (!ShouldUseServerToFetchData() || g_appQuitting.load() || !IsWindow(m_hwnd))
            return;

        std::atomic_bool* flag = ServerSourceWaitFlag(source);
        if (!flag || flag->exchange(true))
            return;

        const HWND hwnd = m_hwnd;
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t epoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        const uint32_t knownGeneration = ServerSourceGeneration(source);
        const std::wstring sourceType = ServerSourceType(source);
        const json sourceOptions = ServerSourceOptions(source);
        const TrafficScotlandOptions scotlandOptions{
            m_trafficScotlandEnabled,
            m_trafficScotlandIncidentsUrl
        };
        const bool unplannedOnly = m_alertFilterUnplannedOnly;

        ScheduleBackgroundTask([hwnd, server, session, source, epoch, knownGeneration, sourceType, sourceOptions, scotlandOptions, unplannedOnly]() {
            BinarySourceBundleResult bundle;
            if (!BinaryWaitSourceBundle(server, session, sourceType, knownGeneration, 65000u, sourceOptions, bundle)) {
                Sleep(30000);
                PostServerSourceSignal(
                    hwnd,
                    source,
                    epoch,
                    false,
                    false,
                    bundle.generation,
                    bundle.error.empty() ? L"Server source wait failed." : bundle.error);
                return;
            }

            if (!bundle.changed) {
                PostServerSourceSignal(hwnd, source, epoch, true, false, bundle.generation, L"");
                return;
            }

            std::wstring error;
            switch (source) {
            case ServerSourceKind::Roads: {
                auto* result = new FeedResult{};
                result->serverSourced = true;
                result->serverSource = source;
                result->serverGeneration = bundle.generation;
                result->serverSyncEpoch = epoch;
                std::wstring status;
                if (ParseRoadAlertsFromSourceBundle(bundle, scotlandOptions.enabled, unplannedOnly, result->alerts, status, error)) {
                    result->ok = true;
                    result->error = status;
                    if (!PostMessageW(hwnd, WM_APP_FEED_READY, 0, reinterpret_cast<LPARAM>(result)))
                        delete result;
                    return;
                }
                delete result;
                break;
            }
            case ServerSourceKind::Earthquakes: {
                auto* result = new EarthquakeResult{};
                result->notify = true;
                result->serverSourced = true;
                result->serverSource = source;
                result->serverGeneration = bundle.generation;
                result->serverSyncEpoch = epoch;
                try {
                    result->ok = ParseEarthquakesFromSourceBundle(bundle, result->events, error);
                }
                catch (const std::exception& e) {
                    error = L"Server earthquake parse failed: " + Utf8ToWide(e.what());
                }
                if (result->ok) {
                    if (!PostMessageW(hwnd, WM_APP_EARTHQUAKE_READY, 0, reinterpret_cast<LPARAM>(result)))
                        delete result;
                    return;
                }
                delete result;
                break;
            }
            case ServerSourceKind::WeatherSystems: {
                auto* result = new WeatherSystemsResult{};
                result->notify = true;
                result->serverSourced = true;
                result->serverSource = source;
                result->serverGeneration = bundle.generation;
                result->serverSyncEpoch = epoch;
                try {
                    result->ok = ParseWeatherSystemsFromSourceBundle(bundle, result->systems, result->statusText, error);
                }
                catch (const std::exception& e) {
                    error = L"Server weather systems parse failed: " + Utf8ToWide(e.what());
                }
                if (result->ok) {
                    if (!PostMessageW(hwnd, WM_APP_WEATHER_READY, 0, reinterpret_cast<LPARAM>(result)))
                        delete result;
                    return;
                }
                delete result;
                break;
            }
            case ServerSourceKind::WeatherWarnings: {
                auto* result = new WeatherWarningsResult{};
                result->notify = true;
                result->serverSourced = true;
                result->serverSource = source;
                result->serverGeneration = bundle.generation;
                result->serverSyncEpoch = epoch;
                try {
                    result->ok = ParseWeatherWarningsFromSourceBundle(bundle, result->warnings, result->statusText, error);
                }
                catch (const std::exception& e) {
                    error = L"Server weather warnings parse failed: " + Utf8ToWide(e.what());
                }
                if (result->ok) {
                    if (!PostMessageW(hwnd, WM_APP_WEATHER_WARNINGS_READY, 0, reinterpret_cast<LPARAM>(result)))
                        delete result;
                    return;
                }
                delete result;
                break;
            }
            case ServerSourceKind::Floods: {
                auto* result = new FloodsResult{};
                result->notify = true;
                result->serverSourced = true;
                result->serverSource = source;
                result->serverGeneration = bundle.generation;
                result->serverSyncEpoch = epoch;
                try {
                    result->ok = ParseFloodsFromSourceBundle(bundle, result->floods, result->statusText, error);
                }
                catch (const std::exception& e) {
                    error = L"Server floods parse failed: " + Utf8ToWide(e.what());
                }
                if (result->ok) {
                    if (!PostMessageW(hwnd, WM_APP_FLOODS_READY, 0, reinterpret_cast<LPARAM>(result)))
                        delete result;
                    return;
                }
                delete result;
                break;
            }
            default:
                error = L"Unknown server source signal.";
                break;
            }

            Sleep(30000);
            PostServerSourceSignal(
                hwnd,
                source,
                epoch,
                false,
                true,
                bundle.generation,
                error.empty() ? L"Server source bundle could not be parsed." : error);
            });
    }

    void OnServerSourceSignalReady(ServerSourceSignalResult* result)
    {
        std::unique_ptr<ServerSourceSignalResult> signal(result);
        if (!signal)
            return;

        ResetServerSourceWaitFlag(signal->source);
        if (!IsCurrentServerSourceEpoch(signal->epoch))
            return;

        StoreServerSourceGeneration(signal->source, signal->generation);
        if (!signal->ok && !signal->error.empty())
            SetStatusText(L"Server data sync waiting: " + signal->error);

        ScheduleServerSourceWait(signal->source);
    }

    void RefreshFeedAsync()
    {
        if (g_fetchInProgress.exchange(true)) {
            SetStatusText(L"Already fetching alerts...");
            return;
        }

        std::wstring url = NormalizeUrl(m_alertsEndpoint);
        const bool useServerFetch = ShouldUseServerToFetchData();
        const TrafficScotlandOptions scotlandOptions{
            m_trafficScotlandEnabled,
            m_trafficScotlandIncidentsUrl
        };
        if (!useServerFetch && url.empty() && !scotlandOptions.enabled) {
            g_fetchInProgress.store(false);
            SetStatusText(L"Enable server fetching for National Highways NTIS data or configure a direct feed.");
            return;
        }

        SetStatusText(L"Fetching alerts...");

        HWND hwnd = m_hwnd;
        const bool unplannedOnly = m_alertFilterUnplannedOnly;
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t serverSyncEpoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        const uint32_t requestedIntervalMs = useServerFetch ? 0u : (m_periodicRefreshEnabled ? m_refreshIntervalMs : 30000u);
        json sourceOptions = {
            { "unplannedOnly", unplannedOnly },
            { "trafficScotlandEnabled", scotlandOptions.enabled },
            { "trafficScotlandIncidentsUrl", WideToUtf8(scotlandOptions.incidentsUrl) }
        };

        ScheduleBackgroundTask([hwnd, url, unplannedOnly, scotlandOptions, useServerFetch, server, session, serverSyncEpoch, requestedIntervalMs, sourceOptions]() {
            auto* result = new FeedResult{};
            if (useServerFetch) {
                result->serverSourced = true;
                result->serverSource = ServerSourceKind::Roads;
                result->serverSyncEpoch = serverSyncEpoch;
            }
            std::wstring brokerError;

            if (useServerFetch) {
                BinarySourceBundleResult bundle;
                result->serverGeneration = bundle.generation;
                if (BinaryFetchSourceBundle(server, session, L"roads", requestedIntervalMs, sourceOptions, bundle)) {
                    result->serverGeneration = bundle.generation;
                    std::wstring brokerStatus;
                    std::wstring brokerParseError;
                    if (ParseRoadAlertsFromSourceBundle(
                        bundle,
                        scotlandOptions.enabled,
                        unplannedOnly,
                        result->alerts,
                        brokerStatus,
                        brokerParseError))
                    {
                        result->ok = true;
                        result->error = brokerStatus;
                    }
                    else {
                        brokerError = brokerParseError.empty() ? L"Server road data could not be parsed." : brokerParseError;
                    }
                }
                else {
                    brokerError = bundle.error.empty() ? L"Server road data fetch failed." : bundle.error;
                }
            }

            if (!result->ok && !useServerFetch) {
                std::string body;
                std::wstring primaryError;
                bool primaryOk = false;

                if (!url.empty() && HttpGetText(url, body, primaryError)) {
                    std::wstring parseError;
                    std::vector<TrafficAlert> alerts = ParseTrafficAlerts(body, parseError);

                    if (!alerts.empty()) {
                        primaryOk = true;
                        result->alerts = std::move(alerts);
                    }
                    else
                        primaryError = parseError.empty() ? L"Feed could not be parsed." : parseError;
                }

                std::vector<TrafficAlert> scotlandAlerts;
                std::wstring scotlandStatus;
                std::wstring scotlandError;
                const bool scotlandOk = FetchTrafficScotlandAlerts(
                    scotlandOptions,
                    scotlandAlerts,
                    scotlandStatus,
                    scotlandError);
                if (scotlandOk && scotlandOptions.enabled) {
                    result->alerts.insert(
                        result->alerts.end(),
                        std::make_move_iterator(scotlandAlerts.begin()),
                        std::make_move_iterator(scotlandAlerts.end()));
                }

                result->ok = primaryOk || (scotlandOk && scotlandOptions.enabled);
                if (!primaryOk && !primaryError.empty())
                    result->error = L"Direct road feed: " + primaryError;
                if (scotlandOptions.enabled && !scotlandOk) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Traffic Scotland: " + scotlandError;
                }
            }

            if (!result->ok && useServerFetch)
                result->error = brokerError.empty() ? L"National Highways NTIS data is unavailable." : brokerError;
            if (!result->ok && !useServerFetch) {
                result->alerts = SampleAlerts();
                if (!result->error.empty())
                    result->error += L" ";
                result->error += L"Showing sample data.";
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

        const bool serverSourced = result->serverSourced;
        const ServerSourceKind serverSource = result->serverSource;
        const uint32_t serverSyncEpoch = result->serverSyncEpoch;
        if (serverSourced) {
            ResetServerSourceWaitFlag(serverSource);
            if (!IsCurrentServerSourceEpoch(serverSyncEpoch)) {
                delete result;
                return;
            }
            StoreServerSourceGeneration(serverSource, result->serverGeneration);
        }

        const bool feedOk = result->ok;
        const std::wstring feedWarning = result->error;
        const size_t scotlandCount = static_cast<size_t>(std::count_if(
            result->alerts.begin(),
            result->alerts.end(),
            [](const TrafficAlert& alert) {
                return alert.id.rfind(L"traffic-scotland:", 0) == 0;
            }));
        m_allAlerts = result->alerts;
        DownloadMissingLaneImagesAsync(m_allAlerts);
        SortAlertsForCurrentOrder();
        for (TrafficAlert& alert : m_allAlerts)
            alert.excluded = IsIncidentExcluded(alert);
        delete result;

        const bool fitMap = !m_hasLoadedAlerts;
        size_t visible = ApplyFilters(fitMap);
        m_hasLoadedAlerts = true;

        if (m_allAlerts.empty()) {
            SetStatusText(L"No alerts available.");
        }
        else if (!feedWarning.empty()) {
            SetStatusText(
                L"Loaded " + std::to_wstring(visible) + L" alert(s). " + feedWarning);
        }
        else {
            SetStatusText(L"Loaded " + std::to_wstring(visible) + L" alert(s).");
        }

        if (feedOk)
            NotifyForMatchingIncidents(m_allAlerts);
        if (m_trafficScotlandStatusLabel) {
            SetWindowTextSafe(
                m_trafficScotlandStatusLabel,
                m_trafficScotlandEnabled
                    ? (L"Loaded " + std::to_wstring(scotlandCount) + L" Traffic Scotland incident(s).")
                    : L"Traffic Scotland incidents are disabled.");
        }
        if (serverSourced)
            ScheduleServerSourceWait(serverSource);
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
        std::wstring fromLocation = ExtractLabeledNotificationField(alert.description, L"From Location");
        std::wstring toLocation = ExtractLabeledNotificationField(alert.description, L"To Location");
        if (!fromLocation.empty() && !toLocation.empty())
            return L"from " + fromLocation + L" to " + toLocation;
        if (!fromLocation.empty())
            return fromLocation;
        if (!toLocation.empty())
            return toLocation;

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
        return alert.trafficEnglandVisible &&
            RoadMatchesIncidentNotification(alert) &&
            !RoadExcludedFromIncidentNotification(alert) &&
            SeverityAllowedForIncidentNotification(alert) &&
            IncidentTypeAllowedForNotification(alert) &&
            IncidentThresholdsMatchNotification(alert) &&
            !ReasonExcludedFromNotification(alert) &&
            !LocationExcludedFromNotification(alert);
    }

    std::wstring IncidentNotificationStableKey(const TrafficAlert& alert) const
    {
        auto generatedId = [](const std::wstring& id) {
            std::wstring value = ToLower(Trim(id));
            const std::wstring alertPrefix = L"alert-";
            const std::wstring htmlPrefix = L"html-";
            size_t prefixLen = 0;
            if (value.rfind(alertPrefix, 0) == 0)
                prefixLen = alertPrefix.size();
            else if (value.rfind(htmlPrefix, 0) == 0)
                prefixLen = htmlPrefix.size();
            else
                return false;
            if (prefixLen >= value.size())
                return false;
            for (size_t i = prefixLen; i < value.size(); ++i) {
                if (!iswdigit(value[i]))
                    return false;
            }
            return true;
            };

        if (!alert.id.empty() && !generatedId(alert.id))
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

    bool IsIncidentExcluded(const TrafficAlert& alert) const
    {
        return m_incidentExclusionKeys.find(IncidentNotificationStableKey(alert)) !=
            m_incidentExclusionKeys.end();
    }

    static std::wstring EventExclusionKey(
        const std::wstring& sourceType,
        const std::wstring& sourceId)
    {
        const std::wstring type = ToLower(Trim(sourceType));
        const std::wstring id = Trim(sourceId);
        if (id.empty())
            return L"";
        if (type.empty() || type == L"incident")
            return id;
        return type + L":" + id;
    }

    bool IsEventExcluded(const std::wstring& sourceType, const std::wstring& sourceId) const
    {
        const std::wstring key = EventExclusionKey(sourceType, sourceId);
        return !key.empty() && m_incidentExclusionKeys.find(key) != m_incidentExclusionKeys.end();
    }

    void ApplyIncidentExclusions()
    {
        m_incidentExclusionKeys.clear();
        for (const IncidentExclusion& exclusion : m_incidentExclusions) {
            if (!exclusion.key.empty())
                m_incidentExclusionKeys.insert(exclusion.key);
        }
        for (TrafficAlert& alert : m_allAlerts)
            alert.excluded = IsIncidentExcluded(alert);
        for (auto& [key, state] : m_notifiedIncidentStates)
            state.alert.excluded = m_incidentExclusionKeys.find(key) != m_incidentExclusionKeys.end();
        for (EarthquakeEvent& event : m_allEarthquakes)
            event.excluded = IsEventExcluded(L"earthquake", EarthquakeStableKey(event));
        for (EarthquakeEvent& event : m_filteredEarthquakes)
            event.excluded = IsEventExcluded(L"earthquake", EarthquakeStableKey(event));
        for (WeatherSystemEvent& system : m_allWeatherSystems)
            system.excluded = IsEventExcluded(L"weather_system", WeatherSystemStableKey(system));
        for (WeatherSystemEvent& system : m_filteredWeatherSystems)
            system.excluded = IsEventExcluded(L"weather_system", WeatherSystemStableKey(system));
        for (WeatherWarningEvent& warning : m_allWeatherWarnings)
            warning.excluded = IsEventExcluded(L"weather_warning", WeatherWarningStableKey(warning));
        for (WeatherWarningEvent& warning : m_filteredWeatherWarnings)
            warning.excluded = IsEventExcluded(L"weather_warning", WeatherWarningStableKey(warning));
        for (FloodEvent& flood : m_allFloods)
            flood.excluded = IsEventExcluded(L"flood", FloodStableKey(flood));
        for (FloodEvent& flood : m_filteredFloods)
            flood.excluded = IsEventExcluded(L"flood", FloodStableKey(flood));

        ApplyFilters(false);
        RenderEarthquakeListRows();
        RenderWeatherSystemsListRows();
        RenderWeatherWarningsListRows();
        RenderFloodsListRows();
        ApplyEarthquakeVisibility();
        ApplyWeatherSystemVisibility();
        ApplyWeatherWarningVisibility();
        ApplyFloodVisibility();
        RenderNotificationHistory();
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
        std::vector<JunctionTemplateData> junctions = ExtractJunctionsFromLocation(AlertLocationForNotification(alert));
        if (!junctions.empty()) {
            std::vector<std::wstring> labels;
            for (const JunctionTemplateData& junction : junctions) {
                std::wstring label = junction.number.empty() ? junction.display : L"J" + junction.number;
                PushUniqueText(labels, label);
            }
            if (!labels.empty())
                road += L" (" + JoinTemplateItems(labels, L" ") + L")";
        }
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
        const std::vector<NotificationBatchItem>& items,
        const std::wstring& singleTitleSuffix,
        const std::wstring& pluralTitle)
    {
        if (items.empty())
            return;

        std::wstring title;
        std::wstring body;
        std::wstring sourceType;
        std::wstring sourceId;
        std::vector<AppNotificationLink> links;
        if (items.size() == 1) {
            title = singleTitleSuffix;
            body = items.front().line;
            sourceType = items.front().sourceType;
            sourceId = items.front().sourceId;
            links.push_back({ items.front().line, items.front().sourceType, items.front().sourceId });
        }
        else {
            title = std::to_wstring(items.size()) + L" " + pluralTitle;
            const size_t displayCount = MinValue<size_t>(items.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += items[i].line;
                links.push_back({ items[i].line, items[i].sourceType, items[i].sourceId });
            }
            if (items.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body, sourceType, sourceId, links);
    }

    void NotifyForMatchingIncidents(const std::vector<TrafficAlert>& alerts)
    {
        std::unordered_set<std::wstring> currentIncidentKeys;
        std::vector<NotificationBatchItem> newLines;
        std::vector<NotificationBatchItem> updateLines;
        std::vector<NotificationBatchItem> removedLines;

        for (const TrafficAlert& alert : alerts) {
            std::wstring stableKey = IncidentNotificationStableKey(alert);
            currentIncidentKeys.insert(stableKey);

            if (!AlertMatchesIncidentNotification(alert)) {
                auto existing = m_notifiedIncidentStates.find(stableKey);
                if (m_haveIncidentNotificationSnapshot && existing != m_notifiedIncidentStates.end()) {
                    removedLines.push_back({ existing->second.line, L"incident", stableKey });
                    m_notifiedIncidentStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = IncidentNotificationSignature(alert);
            std::wstring line = IncidentNotificationLine(alert);
            auto existing = m_notifiedIncidentStates.find(stableKey);
            if (existing == m_notifiedIncidentStates.end()) {
                newLines.push_back({ line, L"incident", stableKey });
                m_notifiedIncidentStates[stableKey] = IncidentNotificationState{ signature, line, alert };
            }
            else if (existing->second.signature != signature) {
                if (!m_incidentIgnoreUpdates)
                    updateLines.push_back({ line, L"incident", stableKey });
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
                existing->second.alert = alert;
            }
            else {
                existing->second.alert = alert;
            }
        }

        if (m_haveIncidentNotificationSnapshot) {
            for (auto it = m_notifiedIncidentStates.begin(); it != m_notifiedIncidentStates.end();) {
                if (currentIncidentKeys.find(it->first) == currentIncidentKeys.end()) {
                    removedLines.push_back({ it->second.line, L"incident", it->first });
                    it = m_notifiedIncidentStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveIncidentNotificationSnapshot = true;

        PublishIncidentNotificationBatch(newLines, L"Incident added", L"incident additions");
        PublishIncidentNotificationBatch(updateLines, L"Incident update", L"incident updates");
        PublishIncidentNotificationBatch(removedLines, L"Incident removed", L"incident removals");
        RefreshIncidentsListRows();
        if (m_incidentSidePanelListOnly)
            ApplyFilters(false);
        else
            ApplyIncidentMapVisibility();
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

    void ShowInAppIncidentNotification(
        const std::wstring& title,
        const std::wstring& body,
        const std::wstring& sourceType = L"",
        const std::wstring& sourceId = L"",
        const std::vector<AppNotificationLink>& links = {})
    {
        AppNotification notification;
        notification.title = title;
        notification.body = body;
        notification.timestamp = TimeTToText(std::time(nullptr));
        notification.sourceType = sourceType;
        notification.sourceId = sourceId;
        notification.links = links;
        m_map.SetActiveNotification(notification);
        KillTimer(m_hwnd, kInAppNotificationTimerId);
        SetTimer(m_hwnd, kInAppNotificationTimerId, 10 * 1000, nullptr);
    }

    void RenderNotificationHistory()
    {
        std::vector<AppNotification> rendered = m_notificationHistory;
        for (AppNotification& notification : rendered) {
            notification.excluded = IsEventExcluded(notification.sourceType, notification.sourceId);
            for (AppNotificationLink& link : notification.links) {
                link.excluded = IsEventExcluded(link.sourceType, link.sourceId);
                notification.excluded = notification.excluded || link.excluded;
            }
        }
        m_map.SetNotificationHistory(rendered);
    }

    void ClearNotificationHistory()
    {
        m_notificationHistory.clear();
        RenderNotificationHistory();
        SetStatusText(L"Notification history cleared.");
    }

    void DeleteNotificationHistoryEntry(size_t index)
    {
        if (index >= m_notificationHistory.size())
            return;

        m_notificationHistory.erase(m_notificationHistory.begin() + static_cast<std::ptrdiff_t>(index));
        RenderNotificationHistory();
        SetStatusText(L"Notification deleted.");
    }

    void HandleNotificationHistoryExclusionAction(
        const AppNotification& notification,
        const std::wstring& action)
    {
        const std::wstring sourceType = ToLower(Trim(notification.sourceType));
        if (sourceType.empty() || notification.sourceId.empty())
            return;

        const std::wstring exclusionKey = EventExclusionKey(sourceType, notification.sourceId);
        const std::wstring cleanAction = ToLower(Trim(action));
        if (cleanAction == L"unexclude") {
            RemoveIncidentExclusionAsync(exclusionKey);
            return;
        }
        if (cleanAction != L"exclude")
            return;

        if (sourceType == L"incident") {
            if (const TrafficAlert* alert = FindAnyAlertById(notification.sourceId)) {
                AddIncidentExclusionAsync(*alert);
                return;
            }
        }

        IncidentExclusion exclusion;
        exclusion.key = exclusionKey;
        exclusion.sourceId = notification.sourceId;
        exclusion.summary = notification.body.empty() ? notification.title : notification.body;
        if (sourceType == L"earthquake")
            exclusion.source = L"Earthquake";
        else if (sourceType == L"weather_system")
            exclusion.source = L"Weather system";
        else if (sourceType == L"weather_warning")
            exclusion.source = L"Weather warning";
        else if (sourceType == L"flood")
            exclusion.source = L"Flood";
        else if (sourceType == L"incident")
            exclusion.source = L"Road incident notification";
        else
            return;

        AddIncidentExclusionAsync(exclusion);
    }

    void HandleMapEventExclusionAction(
        const std::wstring& sourceType,
        const std::wstring& id,
        const std::wstring& action)
    {
        const std::wstring type = ToLower(Trim(sourceType));
        const std::wstring cleanAction = ToLower(Trim(action));
        if (id.empty() || (cleanAction != L"exclude" && cleanAction != L"unexclude"))
            return;

        if (type == L"incident") {
            const TrafficAlert* alert = FindAnyAlertById(id);
            if (!alert)
                return;
            if (cleanAction == L"unexclude")
                RemoveIncidentExclusionAsync(IncidentNotificationStableKey(*alert));
            else
                AddIncidentExclusionAsync(*alert);
            return;
        }

        if (type == L"earthquake") {
            const int index = FindEventIndexByKey(m_allEarthquakes, id, [this](const EarthquakeEvent& event) {
                return MapSelectionIdForEarthquake(event);
                });
            if (index < 0)
                return;
            const EarthquakeEvent& event = m_allEarthquakes[static_cast<size_t>(index)];
            if (cleanAction == L"unexclude")
                RemoveIncidentExclusionAsync(EventExclusionKey(type, EarthquakeStableKey(event)));
            else
                AddEventExclusionAsync(event);
            return;
        }

        if (type == L"weather_system") {
            const int index = FindEventIndexByKey(m_allWeatherSystems, id, [this](const WeatherSystemEvent& system) {
                return MapSelectionIdForWeatherSystem(system);
                });
            if (index < 0)
                return;
            const WeatherSystemEvent& system = m_allWeatherSystems[static_cast<size_t>(index)];
            if (cleanAction == L"unexclude")
                RemoveIncidentExclusionAsync(EventExclusionKey(type, WeatherSystemStableKey(system)));
            else
                AddEventExclusionAsync(system);
            return;
        }

        if (type == L"weather_warning") {
            const int index = FindEventIndexByKey(m_allWeatherWarnings, id, [this](const WeatherWarningEvent& warning) {
                return MapSelectionIdForWeatherWarning(warning);
                });
            if (index < 0)
                return;
            const WeatherWarningEvent& warning = m_allWeatherWarnings[static_cast<size_t>(index)];
            if (cleanAction == L"unexclude")
                RemoveIncidentExclusionAsync(EventExclusionKey(type, WeatherWarningStableKey(warning)));
            else
                AddEventExclusionAsync(warning);
            return;
        }

        if (type == L"flood") {
            const int index = FindEventIndexByKey(m_allFloods, id, [this](const FloodEvent& flood) {
                return MapSelectionIdForFlood(flood);
                });
            if (index < 0)
                return;
            const FloodEvent& flood = m_allFloods[static_cast<size_t>(index)];
            if (cleanAction == L"unexclude")
                RemoveIncidentExclusionAsync(EventExclusionKey(type, FloodStableKey(flood)));
            else
                AddEventExclusionAsync(flood);
        }
    }

    void AddEventExclusionAsync(
        const std::wstring& sourceType,
        const std::wstring& sourceId,
        const std::wstring& source,
        const std::wstring& summary)
    {
        IncidentExclusion exclusion;
        exclusion.key = EventExclusionKey(sourceType, sourceId);
        exclusion.sourceId = sourceId;
        exclusion.source = source;
        exclusion.summary = summary;
        AddIncidentExclusionAsync(exclusion);
    }

    void AddEventExclusionAsync(const EarthquakeEvent& event)
    {
        AddEventExclusionAsync(
            L"earthquake",
            EarthquakeStableKey(event),
            L"Earthquake",
            EarthquakeNotificationLine(event));
    }

    void AddEventExclusionAsync(const WeatherSystemEvent& system)
    {
        AddEventExclusionAsync(
            L"weather_system",
            WeatherSystemStableKey(system),
            L"Weather system",
            WeatherSystemNotificationLine(system));
    }

    void AddEventExclusionAsync(const WeatherWarningEvent& warning)
    {
        AddEventExclusionAsync(
            L"weather_warning",
            WeatherWarningStableKey(warning),
            L"Weather warning",
            WeatherWarningNotificationLine(warning));
    }

    void AddEventExclusionAsync(const FloodEvent& flood)
    {
        AddEventExclusionAsync(
            L"flood",
            FloodStableKey(flood),
            L"Flood",
            FloodNotificationLine(flood));
    }

    void AddNotificationHistory(
        const std::wstring& title,
        const std::wstring& body,
        const std::wstring& sourceType = L"",
        const std::wstring& sourceId = L"",
        const std::vector<AppNotificationLink>& links = {})
    {
        AppNotification entry;
        entry.title = title;
        entry.body = body;
        entry.timestamp = TimeTToText(std::time(nullptr));
        entry.sourceType = sourceType;
        entry.sourceId = sourceId;
        entry.links = links;
        m_notificationHistory.insert(m_notificationHistory.begin(), std::move(entry));
        if (m_notificationHistory.size() > 100)
            m_notificationHistory.resize(100);
        RenderNotificationHistory();
    }

    void PublishNotification(
        const std::wstring& title,
        const std::wstring& body,
        const std::wstring& sourceType = L"",
        const std::wstring& sourceId = L"",
        const std::vector<AppNotificationLink>& links = {})
    {
        PlaySoundCue(SoundCue::Notification);
        AddNotificationHistory(title, body, sourceType, sourceId, links);
        ShowWindowsIncidentNotification(title, body);
        ShowInAppIncidentNotification(title, body, sourceType, sourceId, links);
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

    bool AlertOnIncidentsList(const TrafficAlert& alert) const
    {
        return m_notifiedIncidentStates.find(IncidentNotificationStableKey(alert)) != m_notifiedIncidentStates.end();
    }

    void ApplyIncidentMapVisibility()
    {
        m_map.SetIncidentOverlayVisible(m_showIncidents && m_showIncidentOverlayLabels);
        if (!m_showIncidents) {
            m_map.SetAlerts({});
            return;
        }

        if (!m_incidentOverlayNotifiedOnly) {
            m_map.SetAlerts(m_filteredAlerts);
            return;
        }

        std::vector<TrafficAlert> mapAlerts;
        for (const TrafficAlert& alert : m_filteredAlerts) {
            if (AlertOnIncidentsList(alert))
                mapAlerts.push_back(alert);
        }
        m_map.SetAlerts(mapAlerts);
    }

    size_t ApplyFilters(bool fitMap)
    {
        std::wstring previousSelected = m_selectedId;

        m_filteredAlerts.clear();
        for (size_t i = 0; i < m_allAlerts.size(); ++i) {
            const bool resolutionVisible = m_allAlerts[i].trafficEnglandVisible ||
                (m_showUnresolvedIncidents &&
                    m_allAlerts[i].trafficEnglandEligible &&
                    m_allAlerts[i].unresolved &&
                    m_allAlerts[i].networkResolved);
            if (resolutionVisible && TextFilterMatches(m_allAlerts[i]) && SeverityFilterMatches(m_allAlerts[i]) &&
                (!m_incidentSidePanelListOnly || AlertOnIncidentsList(m_allAlerts[i])))
            {
                m_filteredAlerts.push_back(m_allAlerts[i]);
            }
        }

        m_programmaticSelection = true;
        SendMessageW(m_listView, LVM_DELETEALLITEMS, 0, 0);
        SendMessageW(m_listView, LVM_REMOVEALLGROUPS, 0, 0);

        LVGROUP englandGroup{};
        englandGroup.cbSize = sizeof(englandGroup);
        englandGroup.mask = LVGF_GROUPID | LVGF_HEADER;
        englandGroup.iGroupId = 1;
        englandGroup.pszHeader = const_cast<LPWSTR>(L"England");
        SendMessageW(m_listView, LVM_INSERTGROUP, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&englandGroup));

        LVGROUP scotlandGroup{};
        scotlandGroup.cbSize = sizeof(scotlandGroup);
        scotlandGroup.mask = LVGF_GROUPID | LVGF_HEADER;
        scotlandGroup.iGroupId = 2;
        scotlandGroup.pszHeader = const_cast<LPWSTR>(L"Scotland");
        SendMessageW(m_listView, LVM_INSERTGROUP, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&scotlandGroup));
        SendMessageW(m_listView, LVM_ENABLEGROUPVIEW, TRUE, 0);

        for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
            const TrafficAlert& a = m_filteredAlerts[i];

            std::wstring sev = BuildSeverityDisplay(a.severity);
            std::wstring summary = BuildAlertSummary(a);
            std::wstring updated = a.updatedText.empty() ? L"" : a.updatedText;

            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_GROUPID;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(sev.c_str());
            item.iGroupId =
                a.id.rfind(L"traffic-scotland:", 0) == 0 ||
                ToLower(a.region).find(L"scotland") != std::wstring::npos
                ? 2
                : 1;

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

        ApplyIncidentMapVisibility();

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
            if (m_filteredAlerts[i].id == id || IncidentNotificationStableKey(m_filteredAlerts[i]) == id)
                return static_cast<int>(i);
        }

        return -1;
    }

    const TrafficAlert* FindAnyAlertById(const std::wstring& id) const
    {
        for (const TrafficAlert& alert : m_allAlerts) {
            if (alert.id == id || IncidentNotificationStableKey(alert) == id)
                return &alert;
        }
        return nullptr;
    }

    void SelectAlertById(const std::wstring& id, bool centerMap)
    {
        int idx = FindAlertIndexById(id);
        if (idx < 0)
            return;

        const TrafficAlert& a = m_filteredAlerts[static_cast<size_t>(idx)];
        m_selectedId = a.id;

        SetWindowTextSafe(m_detailsEdit, BuildAlertDetails(a));
        m_map.SetSelectedId(a.id);

        if (centerMap)
            m_map.CenterOnAlert(a.id);

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

    void ShowAlertDetailsById(const std::wstring& id, bool centerMap)
    {
        int filteredIndex = FindAlertIndexById(id);
        if (filteredIndex >= 0) {
            SelectAlertById(id, centerMap);
            return;
        }

        const TrafficAlert* alert = FindAnyAlertById(id);
        if (!alert) {
            SetStatusText(L"Notification source incident is no longer available.");
            return;
        }

        m_selectedId = alert->id;
        SetWindowTextSafe(m_detailsEdit, BuildAlertDetails(*alert));
        m_map.SetSelectedId(L"");

        if (m_listView) {
            LVITEMW state{};
            state.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            state.state = 0;
            SendMessageW(m_listView, LVM_SETITEMSTATE, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&state));
        }

        SetStatusText(L"Showing notification incident details.");
    }

    static void SelectListViewRow(HWND listView, int row)
    {
        if (!listView || row < 0)
            return;

        LVITEMW clear{};
        clear.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        clear.state = 0;
        SendMessageW(listView, LVM_SETITEMSTATE, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&clear));

        LVITEMW select{};
        select.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        select.state = LVIS_SELECTED | LVIS_FOCUSED;
        SendMessageW(listView, LVM_SETITEMSTATE, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&select));
        SendMessageW(listView, LVM_ENSUREVISIBLE, static_cast<WPARAM>(row), FALSE);
        SetFocus(listView);
    }

    template <typename TEvent, typename TKeyFn>
    static int FindEventIndexByKey(const std::vector<TEvent>& events, const std::wstring& sourceId, TKeyFn keyFn)
    {
        for (size_t i = 0; i < events.size(); ++i) {
            if (keyFn(events[i]) == sourceId)
                return static_cast<int>(i);
        }
        return -1;
    }

    void FocusEarthquakeOnMap(const EarthquakeEvent& event)
    {
        if (event.hasLocation)
            m_map.FitToPoints({ { event.latitude, event.longitude } }, 6);
    }

    void FocusWeatherSystemOnMap(const WeatherSystemEvent& system)
    {
        std::vector<GeoPoint> points;
        if (system.hasLocation)
            points.push_back({ system.latitude, system.longitude });
        if (system.hasForecastLocation)
            points.push_back({ system.forecastLatitude, system.forecastLongitude });
        for (const WeatherForecastPoint& point : system.forecastTrack) {
            if (point.hasLocation)
                points.push_back({ point.latitude, point.longitude });
        }
        m_map.FitToPoints(points, 5);
    }

    void FocusWeatherWarningOnMap(const WeatherWarningEvent& warning)
    {
        std::vector<GeoPoint> points = warning.polygon;
        if (points.empty() && warning.hasLocation)
            points.push_back({ warning.latitude, warning.longitude });
        m_map.FitToPoints(points, 7);
    }

    void FocusFloodOnMap(const FloodEvent& flood)
    {
        if (flood.hasLocation)
            m_map.FitToPoints({ { flood.latitude, flood.longitude } }, 8);
    }

    std::wstring MapSelectionIdForEarthquake(const EarthquakeEvent& event) const
    {
        return event.id.empty() ? EarthquakeStableKey(event) : event.id;
    }

    std::wstring MapSelectionIdForWeatherSystem(const WeatherSystemEvent& system) const
    {
        return system.id.empty() ? WeatherSystemStableKey(system) : system.id;
    }

    std::wstring MapSelectionIdForWeatherWarning(const WeatherWarningEvent& warning) const
    {
        return warning.id.empty() ? WeatherWarningStableKey(warning) : warning.id;
    }

    std::wstring MapSelectionIdForFlood(const FloodEvent& flood) const
    {
        return flood.id.empty() ? FloodStableKey(flood) : flood.id;
    }

    void SelectEarthquakeEventFromList(size_t index, bool centerMap)
    {
        if (index >= m_filteredEarthquakes.size())
            return;

        const EarthquakeEvent& event = m_filteredEarthquakes[index];
        m_selectedId = MapSelectionIdForEarthquake(event);
        m_map.SetSelectedId(m_selectedId);
        if (centerMap)
            FocusEarthquakeOnMap(event);
    }

    void SelectWeatherSystemEventFromList(size_t index, bool centerMap)
    {
        if (index >= m_filteredWeatherSystems.size())
            return;

        const WeatherSystemEvent& system = m_filteredWeatherSystems[index];
        m_selectedId = MapSelectionIdForWeatherSystem(system);
        m_map.SetSelectedId(m_selectedId);
        if (centerMap)
            FocusWeatherSystemOnMap(system);
    }

    void SelectWeatherWarningEventFromList(size_t index, bool centerMap)
    {
        if (index >= m_filteredWeatherWarnings.size())
            return;

        const WeatherWarningEvent& warning = m_filteredWeatherWarnings[index];
        m_selectedId = MapSelectionIdForWeatherWarning(warning);
        m_map.SetSelectedId(m_selectedId);
        if (centerMap)
            FocusWeatherWarningOnMap(warning);
    }

    void SelectFloodEventFromList(size_t index, bool centerMap)
    {
        if (index >= m_filteredFloods.size())
            return;

        const FloodEvent& flood = m_filteredFloods[index];
        m_selectedId = MapSelectionIdForFlood(flood);
        m_map.SetSelectedId(m_selectedId);
        if (centerMap)
            FocusFloodOnMap(flood);
    }

    void SelectMapEvent(const std::wstring& sourceType, const std::wstring& id)
    {
        const std::wstring type = ToLower(Trim(sourceType));
        auto selectVisibleRow = [this](HWND listView, int row) {
            if (!listView || row < 0)
                return;
            m_syncingControls = true;
            SelectListViewRow(listView, row);
            m_syncingControls = false;
            };

        if (type == L"earthquake") {
            const int row = FindEventIndexByKey(m_filteredEarthquakes, id, [this](const EarthquakeEvent& event) {
                return MapSelectionIdForEarthquake(event);
                });
            if (row >= 0) {
                SelectEarthquakeEventFromList(static_cast<size_t>(row), false);
                selectVisibleRow(m_earthquakeListView, row);
                SetStatusText(L"Earthquake selected.");
            }
            return;
        }
        if (type == L"weather_system") {
            int row = FindEventIndexByKey(m_filteredWeatherSystems, id, [this](const WeatherSystemEvent& system) {
                return MapSelectionIdForWeatherSystem(system);
                });
            if (row >= 0) {
                SelectWeatherSystemEventFromList(static_cast<size_t>(row), false);
                selectVisibleRow(m_weatherSystemsListView, row);
                SetStatusText(L"Weather system selected.");
            }
            else {
                const int allRow = FindEventIndexByKey(m_allWeatherSystems, id, [this](const WeatherSystemEvent& system) {
                    return MapSelectionIdForWeatherSystem(system);
                    });
                if (allRow >= 0) {
                    m_selectedId = MapSelectionIdForWeatherSystem(m_allWeatherSystems[static_cast<size_t>(allRow)]);
                    m_map.SetSelectedId(m_selectedId);
                    SetStatusText(L"Weather system selected.");
                }
            }
            return;
        }
        if (type == L"weather_warning") {
            const int row = FindEventIndexByKey(m_filteredWeatherWarnings, id, [this](const WeatherWarningEvent& warning) {
                return MapSelectionIdForWeatherWarning(warning);
                });
            if (row >= 0) {
                SelectWeatherWarningEventFromList(static_cast<size_t>(row), false);
                selectVisibleRow(m_weatherWarningsListView, row);
                SetStatusText(L"Weather warning selected.");
            }
            return;
        }
        if (type == L"flood") {
            const int row = FindEventIndexByKey(m_filteredFloods, id, [this](const FloodEvent& flood) {
                return MapSelectionIdForFlood(flood);
                });
            if (row >= 0) {
                SelectFloodEventFromList(static_cast<size_t>(row), false);
                selectVisibleRow(m_floodsListView, row);
                SetStatusText(L"Flood event selected.");
            }
        }
    }

    bool SelectEarthquakeNotificationSource(const std::wstring& sourceId)
    {
        ShowEarthquakeListWindow();
        int row = FindEventIndexByKey(m_filteredEarthquakes, sourceId, [this](const EarthquakeEvent& event) {
            return EarthquakeStableKey(event);
            });
        if (row < 0 && FindEventIndexByKey(m_allEarthquakes, sourceId, [this](const EarthquakeEvent& event) {
            return EarthquakeStableKey(event);
            }) >= 0)
        {
            m_earthquakeListMagnitudeText.clear();
            m_earthquakeListTimeText.clear();
            m_earthquakeListUseDateFilter = false;
            m_earthquakeListPeriodText = L"All";
            m_earthquakeListPopulatedRadiusText.clear();
            m_earthquakeListPopulatedRadiusMiles = 0.0;
            m_earthquakeFilterRegion.clear();
            if (m_earthquakeListMagnitudeEdit)
                SetWindowTextSafe(m_earthquakeListMagnitudeEdit, m_earthquakeListMagnitudeText);
            if (m_earthquakeListTimeEdit)
                SetWindowTextSafe(m_earthquakeListTimeEdit, m_earthquakeListTimeText);
            if (m_earthquakeListPeriodCombo)
                PopulatePeriodCombo(m_earthquakeListPeriodCombo, m_earthquakeListPeriodText);
            if (m_earthquakeListPopulatedRadiusEdit)
                SetWindowTextSafe(m_earthquakeListPopulatedRadiusEdit, m_earthquakeListPopulatedRadiusText);
            SyncEarthquakeListDateModeControls();
            ApplyEarthquakeListFilters();
            row = FindEventIndexByKey(m_filteredEarthquakes, sourceId, [this](const EarthquakeEvent& event) {
                return EarthquakeStableKey(event);
                });
        }
        if (row < 0)
            return false;
        SelectEarthquakeEventFromList(static_cast<size_t>(row), true);
        SelectListViewRow(m_earthquakeListView, row);
        return true;
    }

    bool SelectWeatherSystemNotificationSource(const std::wstring& sourceId)
    {
        ShowWeatherSystemsListWindow();
        int row = FindEventIndexByKey(m_filteredWeatherSystems, sourceId, [this](const WeatherSystemEvent& system) {
            return WeatherSystemStableKey(system);
            });
        if (row < 0 && FindEventIndexByKey(m_allWeatherSystems, sourceId, [this](const WeatherSystemEvent& system) {
            return WeatherSystemStableKey(system);
            }) >= 0)
        {
            m_weatherSystemsListForecastText = L"All";
            if (m_weatherSystemsListForecastCombo)
                PopulateForecastMinimumCombo(m_weatherSystemsListForecastCombo, m_weatherSystemsListForecastText);
            ApplyWeatherSystemsListFilter(false);
            row = FindEventIndexByKey(m_filteredWeatherSystems, sourceId, [this](const WeatherSystemEvent& system) {
                return WeatherSystemStableKey(system);
                });
        }
        if (row < 0)
            return false;
        SelectWeatherSystemEventFromList(static_cast<size_t>(row), true);
        SelectListViewRow(m_weatherSystemsListView, row);
        return true;
    }

    bool SelectWeatherWarningNotificationSource(const std::wstring& sourceId)
    {
        ShowWeatherWarningsListWindow();
        int row = FindEventIndexByKey(m_filteredWeatherWarnings, sourceId, [this](const WeatherWarningEvent& warning) {
            return WeatherWarningStableKey(warning);
            });
        if (row < 0 && FindEventIndexByKey(m_allWeatherWarnings, sourceId, [this](const WeatherWarningEvent& warning) {
            return WeatherWarningStableKey(warning);
            }) >= 0)
        {
            m_weatherWarningsListPeriodText = L"All";
            if (m_weatherWarningsListPeriodCombo)
                PopulatePeriodCombo(m_weatherWarningsListPeriodCombo, m_weatherWarningsListPeriodText);
            ApplyWeatherWarningsListFilter(false);
            row = FindEventIndexByKey(m_filteredWeatherWarnings, sourceId, [this](const WeatherWarningEvent& warning) {
                return WeatherWarningStableKey(warning);
                });
        }
        if (row < 0)
            return false;
        SelectWeatherWarningEventFromList(static_cast<size_t>(row), true);
        SelectListViewRow(m_weatherWarningsListView, row);
        return true;
    }

    bool SelectFloodNotificationSource(const std::wstring& sourceId)
    {
        ShowFloodsListWindow();
        int row = FindEventIndexByKey(m_filteredFloods, sourceId, [this](const FloodEvent& flood) {
            return FloodStableKey(flood);
            });
        if (row < 0 && FindEventIndexByKey(m_allFloods, sourceId, [this](const FloodEvent& flood) {
            return FloodStableKey(flood);
            }) >= 0)
        {
            m_floodsListPeriodText = L"All";
            if (m_floodsListPeriodCombo)
                PopulatePeriodCombo(m_floodsListPeriodCombo, m_floodsListPeriodText);
            ApplyFloodsListFilter(false);
            row = FindEventIndexByKey(m_filteredFloods, sourceId, [this](const FloodEvent& flood) {
                return FloodStableKey(flood);
                });
        }
        if (row < 0)
            return false;
        SelectFloodEventFromList(static_cast<size_t>(row), true);
        SelectListViewRow(m_floodsListView, row);
        return true;
    }

    void ActivateNotificationHistoryEntry(const AppNotification& notification)
    {
        std::wstring sourceType = ToLower(Trim(notification.sourceType));
        if (sourceType.empty() || notification.sourceId.empty()) {
            SetStatusText(L"This notification is not linked to a selectable event.");
            return;
        }

        if (sourceType == L"incident") {
            ShowAlertDetailsById(notification.sourceId, true);
            return;
        }
        if (sourceType == L"earthquake") {
            SetStatusText(SelectEarthquakeNotificationSource(notification.sourceId)
                ? L"Showing notification earthquake."
                : L"Notification source earthquake is no longer available.");
            return;
        }
        if (sourceType == L"weather_system") {
            SetStatusText(SelectWeatherSystemNotificationSource(notification.sourceId)
                ? L"Showing notification weather system."
                : L"Notification source weather system is no longer available.");
            return;
        }
        if (sourceType == L"weather_warning") {
            SetStatusText(SelectWeatherWarningNotificationSource(notification.sourceId)
                ? L"Showing notification weather warning."
                : L"Notification source weather warning is no longer available.");
            return;
        }
        if (sourceType == L"flood") {
            SetStatusText(SelectFloodNotificationSource(notification.sourceId)
                ? L"Showing notification flood item."
                : L"Notification source flood item is no longer available.");
            return;
        }

        SetStatusText(L"This notification is not linked to a selectable event.");
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

    void RenderPrivateMessages()
    {
        if (m_map.Hwnd())
            m_map.SetPrivateMessages(m_privateMessages);
    }

    void RenderPrivateMessageUnreadCounts()
    {
        if (m_map.Hwnd())
            m_map.SetPrivateMessageUnreadCounts(m_privateMessageUnreadCounts);
    }

    void RenderOnlineUsers()
    {
        if (m_map.Hwnd())
            m_map.SetOnlineUsers(m_onlineUsers);
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
        ClientSession session = m_session;
        uint32_t knownVersion = m_collaborationVersion;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, knownVersion]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::Poll;

            BinaryPollResult binary;
            if (BinaryPollCollaboration(server, session, knownVersion, binary)) {
                result->chat = std::move(binary.chat);
                result->notes = std::move(binary.notes);
                result->users = std::move(binary.users);
                result->privateMessages = std::move(binary.privateMessages);
                result->incidentExclusions = std::move(binary.incidentExclusions);
                result->collaborationVersion = binary.version;
                result->chatOk = true;
                result->notesOk = true;
                result->usersOk = true;
                result->privateMessagesOk = true;
                result->incidentExclusionsOk = true;
                result->ok = true;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }
            if (binary.protocolAvailable) {
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

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

            std::string usersBody;
            std::wstring usersError;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/users/online"), authHeaders, usersBody, usersError)) {
                try {
                    result->users = ParseOnlineUsers(json::parse(usersBody));
                    result->usersOk = true;
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Online users parse failed: " + Utf8ToWide(e.what());
                }
            }
            else if (result->error.empty()) {
                result->error = L"Online users poll failed: " + usersError;
            }

            std::string privateBody;
            std::wstring privateError;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/private-messages"), authHeaders, privateBody, privateError)) {
                try {
                    result->privateMessages = ParsePrivateMessages(json::parse(privateBody));
                    result->privateMessagesOk = true;
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Private message parse failed: " + Utf8ToWide(e.what());
                }
            }
            else if (result->error.empty()) {
                result->error = L"Private message poll failed: " + privateError;
            }

            std::string exclusionsBody;
            std::wstring exclusionsError;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/incidents/exclusions"), authHeaders, exclusionsBody, exclusionsError)) {
                try {
                    result->incidentExclusions = ParseIncidentExclusions(json::parse(exclusionsBody));
                    result->incidentExclusionsOk = true;
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Incident exclusions parse failed: " + Utf8ToWide(e.what());
                }
            }
            else if (result->error.empty()) {
                result->error = L"Incident exclusions poll failed: " + exclusionsError;
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
        ChatMessage local{ L"", author, m_session.username, m_session.position, text, L"pending" };
        m_chatMessages.push_back(local);
        RenderChatHistory();

        if (!IsOnlineMode()) {
            SetStatusText(L"Offline mode: chat kept locally.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, text, author, authHeaders, session]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendChat;
            BinaryCallResult binary;
            if (BinarySendChat(server, session, text, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

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

    void SendPrivateMessageAsync(const std::wstring& recipientUsername, const std::wstring& inputText)
    {
        std::wstring recipient = Trim(recipientUsername);
        std::wstring text = Trim(inputText);
        if (recipient.empty() || text.empty())
            return;

        auto peer = std::find_if(m_onlineUsers.begin(), m_onlineUsers.end(), [&](const OnlineUser& user) {
            return ToLower(Trim(user.username)) == ToLower(recipient);
            });

        PrivateMessage local;
        local.id = L"local-private-" + std::to_wstring(GetTickCount64());
        local.senderUsername = m_session.username;
        local.senderDisplayName = SessionDisplayName();
        local.senderPosition = m_session.position;
        local.recipientUsername = recipient;
        if (peer != m_onlineUsers.end()) {
            local.recipientDisplayName = peer->displayName;
            local.recipientPosition = peer->position;
        }
        local.text = text;
        local.timestamp = IsOnlineMode() ? L"pending" : L"";
        m_privateMessages.push_back(local);
        RenderPrivateMessages();

        if (!IsOnlineMode()) {
            SetStatusText(L"Offline mode: private message kept locally.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, recipient, text]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendPrivateMessage;
            BinaryCallResult binary;
            if (BinarySendPrivateMessage(server, session, recipient, text, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            std::string body = "{\"recipient\":" + JsonEscape(recipient) + ",\"text\":" + JsonEscape(text) + "}";
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/private-messages"), body, authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void ClearResponderChatAsync()
    {
        if (!CanClearResponderChat()) {
            SetStatusText(L"Only Administrators and Supervisors can clear responder chat.");
            return;
        }

        if (m_chatMessages.empty())
            return;

        if (!IsOnlineMode()) {
            m_chatMessages.clear();
            RenderChatHistory();
            SetStatusText(L"Responder chat cleared locally.");
            return;
        }

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            SetStatusText(L"Responder chat clear failed: no collaboration server configured.");
            return;
        }

        HWND hwnd = m_hwnd;
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::ClearChat;
            BinaryCallResult binary;
            if (BinaryClearChat(server, session, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            result->ok = HttpDeleteTextWithHeaders(AppendPath(server, L"/api/chat"), authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void AddIncidentExclusionAsync(const TrafficAlert& alert)
    {
        IncidentExclusion exclusion;
        exclusion.key = IncidentNotificationStableKey(alert);
        exclusion.sourceId = alert.id;
        if (alert.id.rfind(L"traffic-scotland:", 0) == 0)
            exclusion.source = L"Traffic Scotland";
        else if (alert.id.rfind(L"ntis:", 0) == 0)
            exclusion.source = L"National Highways NTIS";
        else
            exclusion.source = L"Road incidents";
        exclusion.road = alert.road;
        exclusion.summary = BuildAlertSummary(alert);
        AddIncidentExclusionAsync(exclusion);
    }

    void AddIncidentExclusionAsync(const IncidentExclusion& exclusion)
    {
        if (!IsOnlineMode()) {
            SetStatusText(L"Exclusions are controlled by the collaboration server and require online mode.");
            return;
        }
        if (exclusion.key.empty())
            return;
        if (m_incidentExclusionKeys.find(exclusion.key) != m_incidentExclusionKeys.end()) {
            SetStatusText(L"That event is already on the shared exclusion list.");
            return;
        }

        HWND hwnd = m_hwnd;
        const std::wstring server = ServerBaseUrl();
        const std::wstring authHeaders = BearerAuthHeader(m_session);
        const ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, exclusion]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::AddIncidentExclusion;
            BinaryCallResult binary;
            if (BinaryAddIncidentExclusion(server, session, exclusion, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
            }
            else {
                std::string body = "{";
                body += "\"key\":" + JsonEscape(exclusion.key);
                body += ",\"sourceId\":" + JsonEscape(exclusion.sourceId);
                body += ",\"source\":" + JsonEscape(exclusion.source);
                body += ",\"road\":" + JsonEscape(exclusion.road);
                body += ",\"summary\":" + JsonEscape(exclusion.summary);
                body += "}";
                std::string response;
                std::wstring error;
                result->ok = HttpPostJsonTextWithHeaders(
                    AppendPath(server, L"/api/incidents/exclusions"),
                    body,
                    authHeaders,
                    response,
                    error);
                result->error = error;
            }

            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void RemoveIncidentExclusionAsync(const std::wstring& key)
    {
        if (!IsOnlineMode()) {
            SetStatusText(L"Exclusions are controlled by the collaboration server and require online mode.");
            return;
        }
        if (key.empty())
            return;
        if (m_incidentExclusionKeys.find(key) == m_incidentExclusionKeys.end()) {
            SetStatusText(L"That event is not on the shared exclusion list.");
            return;
        }

        HWND hwnd = m_hwnd;
        const std::wstring server = ServerBaseUrl();
        const std::wstring authHeaders = BearerAuthHeader(m_session);
        const ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, key]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::RemoveIncidentExclusion;
            BinaryCallResult binary;
            if (BinaryRemoveIncidentExclusion(server, session, key, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
            }
            else {
                const std::string body = "{\"key\":" + JsonEscape(key) + "}";
                std::string response;
                std::wstring error;
                result->ok = HttpPostJsonTextWithHeaders(
                    AppendPath(server, L"/api/incidents/exclusions/remove"),
                    body,
                    authHeaders,
                    response,
                    error);
                result->error = error;
            }

            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void HandleResponderChatMessageAction(const ChatMessage& message, const std::wstring& action)
    {
        if (ToLower(Trim(action)) != L"delete")
            return;

        if (CurrentPositionRank() < 2) {
            SetStatusText(L"Only Managers, Supervisors and Administrators can delete responder chat messages.");
            return;
        }
        if (message.id.empty()) {
            SetStatusText(L"That responder message is local or pending and cannot be deleted remotely yet.");
            return;
        }
        const int targetRank = PositionRankText(message.position);
        if (targetRank > 0 && CurrentPositionRank() <= targetRank) {
            SetStatusText(L"You can only delete responder messages from users below your current position.");
            return;
        }

        DeleteResponderChatMessageAsync(message.id);
    }

    void HandleOnlineUserAction(const OnlineUser& user, const std::wstring& action)
    {
        std::wstring cleanAction = ToLower(Trim(action));
        if (cleanAction == L"private") {
            m_map.OpenPrivateChat(user);
            SetStatusText(L"Private chat opened with " + (user.displayName.empty() ? user.username : user.displayName) + L".");
            return;
        }

        if (!CanModerateUser(user)) {
            SetStatusText(L"You can only mute or kick users below your current position.");
            return;
        }

        if (cleanAction == L"mute") {
            MuteOnlineUserAsync(user.username, 15);
            return;
        }
        if (cleanAction == L"kick") {
            KickOnlineUserAsync(user.username);
            return;
        }
    }

    void HandleMapOverlayPanelClose(const std::wstring& panelName)
    {
        std::wstring panel = ToLower(Trim(panelName));
        if (panel == L"users") {
            m_showUsersOverlay = false;
            m_map.SetUsersVisible(false);
            UpdateNotificationHistoryMenu();
            SaveSettings();
        }
        else if (panel == L"notification_history") {
            m_showNotificationHistory = false;
            m_map.SetNotificationHistoryVisible(false);
            UpdateNotificationHistoryMenu();
            SaveSettings();
        }
        else if (panel == L"map_controls") {
            m_showMapControls = false;
            m_map.SetToolbarVisible(false);
            UpdateViewMenu();
            SaveSettings();
        }
        else if (panel == L"countdown_timer") {
            m_showCountdownTimer = false;
            m_map.SetCountdownVisible(false);
            UpdateViewMenu();
            SaveSettings();
        }
    }

    void DeleteResponderChatMessageAsync(const std::wstring& messageId)
    {
        if (!IsOnlineMode()) {
            m_chatMessages.erase(std::remove_if(m_chatMessages.begin(), m_chatMessages.end(), [&](const ChatMessage& msg) {
                return msg.id == messageId;
                }), m_chatMessages.end());
            RenderChatHistory();
            SetStatusText(L"Responder chat message deleted locally.");
            return;
        }

        HWND hwnd = m_hwnd;
        std::wstring server = ServerBaseUrl();
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, messageId]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::DeleteChatMessage;
            BinaryCallResult binary;
            if (BinaryDeleteChatMessage(server, session, messageId, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            result->ok = HttpDeleteTextWithHeaders(AppendPath(server, L"/api/chat/") + UrlEncodePathSegment(messageId), authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void KickOnlineUserAsync(const std::wstring& username)
    {
        if (username.empty())
            return;
        HWND hwnd = m_hwnd;
        std::wstring server = ServerBaseUrl();
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, username]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::KickUser;
            BinaryCallResult binary;
            if (BinaryKickUser(server, session, username, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/users/") + UrlEncodePathSegment(username) + L"/kick", "{}", authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void MuteOnlineUserAsync(const std::wstring& username, uint32_t minutes)
    {
        if (username.empty())
            return;
        HWND hwnd = m_hwnd;
        std::wstring server = ServerBaseUrl();
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, username, minutes]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::MuteUser;
            BinaryCallResult binary;
            if (BinaryMuteUser(server, session, username, minutes, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            std::string body = "{\"minutes\":" + std::to_string(minutes) + "}";
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/users/") + UrlEncodePathSegment(username) + L"/mute", body, authHeaders, response, error);
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
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, body, authHeaders, note, session]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendNote;
            BinaryCallResult binary;
            MapNote serverNote;
            if (BinaryCreateNote(server, session, note, serverNote, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (binary.ok) {
                    result->notes.push_back(std::move(serverNote));
                    result->notesOk = true;
                }
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/notes"), body, authHeaders, response, error);
            result->error = error;
            if (result->ok) {
                try {
                    json root = json::parse(response);
                    auto noteIt = root.find("note");
                    if (noteIt != root.end()) {
                        std::vector<MapNote> parsed = ParseMapNotes(json::array({ *noteIt }));
                        if (!parsed.empty()) {
                            result->notes.push_back(std::move(parsed.front()));
                            result->notesOk = true;
                        }
                    }
                }
                catch (...) {
                }
            }
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
        MapNote noteForSend = note;
        std::string body = BuildNoteJsonBody(note);
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, noteId, noteForSend, body, authHeaders, session]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::UpdateNote;
            BinaryCallResult binary;
            if (BinaryUpdateNote(server, session, noteForSend, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

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
        ClientSession session = m_session;
        ScheduleBackgroundTask([hwnd, server, noteId, authHeaders, session]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::DeleteNote;
            BinaryCallResult binary;
            if (BinaryDeleteNote(server, session, noteId, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

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

    bool HasNewIncomingChatMessage(const std::vector<ChatMessage>& incoming) const
    {
        std::unordered_set<std::wstring> existing;
        existing.reserve(m_chatMessages.size());
        for (const ChatMessage& message : m_chatMessages) {
            const std::wstring key = !message.id.empty()
                ? message.id
                : message.username + L"|" + message.timestamp + L"|" + message.text;
            existing.insert(key);
        }

        const std::wstring currentUsername = ToLower(Trim(m_session.username));
        const std::wstring currentDisplayName = ToLower(Trim(SessionDisplayName()));
        for (const ChatMessage& message : incoming) {
            if ((!currentUsername.empty() && ToLower(Trim(message.username)) == currentUsername) ||
                (message.username.empty() && ToLower(Trim(message.author)) == currentDisplayName))
            {
                continue;
            }
            const std::wstring key = !message.id.empty()
                ? message.id
                : message.username + L"|" + message.timestamp + L"|" + message.text;
            if (existing.find(key) == existing.end())
                return true;
        }
        return false;
    }

    std::unordered_map<std::wstring, size_t> NewIncomingPrivateMessageCounts(
        const std::vector<PrivateMessage>& incoming) const
    {
        std::unordered_set<std::wstring> existing;
        existing.reserve(m_privateMessages.size());
        for (const PrivateMessage& message : m_privateMessages) {
            const std::wstring key = !message.id.empty()
                ? message.id
                : message.senderUsername + L"|" + message.timestamp + L"|" + message.text;
            existing.insert(key);
        }

        const std::wstring currentUsername = ToLower(Trim(m_session.username));
        std::unordered_map<std::wstring, size_t> counts;
        for (const PrivateMessage& message : incoming) {
            const std::wstring sender = ToLower(Trim(message.senderUsername));
            if (sender.empty() || (!currentUsername.empty() && sender == currentUsername))
                continue;
            const std::wstring key = !message.id.empty()
                ? message.id
                : message.senderUsername + L"|" + message.timestamp + L"|" + message.text;
            if (existing.find(key) == existing.end())
                ++counts[sender];
        }
        return counts;
    }

    void OnServerReady(ServerResult* result)
    {
        if (!result)
            return;
        if (result->action == ServerAction::Poll)
            m_serverRequestInProgress.store(false);

        if (result->action == ServerAction::Poll && result->ok) {
            if (result->collaborationVersion > 0)
                m_collaborationVersion = result->collaborationVersion;

            if (result->chatOk) {
                const bool playMessageSound =
                    m_chatSoundBaselineReady &&
                    HasNewIncomingChatMessage(result->chat);
                m_chatMessages = std::move(result->chat);
                RenderChatHistory();
                m_chatSoundBaselineReady = true;
                if (playMessageSound)
                    PlaySoundCue(SoundCue::Message);
            }

            if (result->privateMessagesOk) {
                std::unordered_map<std::wstring, size_t> newMessages;
                if (m_privateMessageSoundBaselineReady)
                    newMessages = NewIncomingPrivateMessageCounts(result->privateMessages);
                for (const auto& [sender, count] : newMessages) {
                    if (sender != m_activePrivateChatUsername)
                        m_privateMessageUnreadCounts[sender] += count;
                }
                m_privateMessages = std::move(result->privateMessages);
                RenderPrivateMessages();
                RenderPrivateMessageUnreadCounts();
                m_privateMessageSoundBaselineReady = true;
                if (!newMessages.empty())
                    PlaySoundCue(SoundCue::PrivateMessage);
            }

            if (result->notesOk) {
                ReconcilePendingNoteEdits(result->notes);
                ApplyLocalNoteOverrides(result->notes);
                if (!MapNotesEqual(m_notes, result->notes)) {
                    m_notes = std::move(result->notes);
                    m_map.SetNotes(m_notes);
                }
            }

            if (result->usersOk) {
                m_onlineUsers = std::move(result->users);
                RenderOnlineUsers();
            }

            if (result->incidentExclusionsOk) {
                m_incidentExclusions = std::move(result->incidentExclusions);
                ApplyIncidentExclusions();
                RefreshIncidentsListRows();
                RefreshIncidentExclusionsRows();
            }

            if (result->collaborationVersion > 0)
                PollServerAsync();
        }
        else if (result->action == ServerAction::SendChat) {
            SetStatusText(result->ok ? L"Chat message sent." : L"Chat send failed; kept locally.");
            PollServerAsync();
        }
        else if (result->action == ServerAction::SendPrivateMessage) {
            SetStatusText(result->ok
                ? L"Private message sent."
                : (result->error.empty() ? L"Private message send failed; kept locally." : L"Private message send failed: " + result->error));
            PollServerAsync();
        }
        else if (result->action == ServerAction::ClearChat) {
            if (result->ok) {
                m_chatMessages.clear();
                RenderChatHistory();
                SetStatusText(L"Responder chat cleared.");
            }
            else {
                SetStatusText(result->error.empty() ? L"Responder chat clear failed." : L"Responder chat clear failed: " + result->error);
            }
            PollServerAsync();
        }
        else if (result->action == ServerAction::DeleteChatMessage) {
            SetStatusText(result->ok
                ? L"Responder chat message deleted."
                : (result->error.empty() ? L"Responder chat message delete failed." : L"Responder chat message delete failed: " + result->error));
            PollServerAsync();
        }
        else if (result->action == ServerAction::KickUser) {
            SetStatusText(result->ok
                ? L"User kicked from the collaboration server."
                : (result->error.empty() ? L"Kick failed." : L"Kick failed: " + result->error));
            PollServerAsync();
        }
        else if (result->action == ServerAction::MuteUser) {
            SetStatusText(result->ok
                ? L"User muted from responder chat."
                : (result->error.empty() ? L"Mute failed." : L"Mute failed: " + result->error));
            PollServerAsync();
        }
        else if (result->action == ServerAction::SendNote) {
            if (result->ok && result->notesOk && !result->notes.empty()) {
                const MapNote& serverNote = result->notes.front();
                auto pending = std::find_if(m_notes.begin(), m_notes.end(), [&](const MapNote& note) {
                    return note.timestamp == L"pending" &&
                        note.text == serverNote.text &&
                        std::abs(note.latitude - serverNote.latitude) <= 1e-6 &&
                        std::abs(note.longitude - serverNote.longitude) <= 1e-6;
                    });
                if (pending != m_notes.end()) {
                    *pending = serverNote;
                    m_map.SetNotes(m_notes);
                }
            }
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
        else if (result->action == ServerAction::CreateAccount) {
            EnableWindow(m_accountCreatorCreateBtn, TRUE);
            if (result->ok) {
                SetWindowTextSafe(m_accountCreatorPasswordEdit, L"");
                SetWindowTextSafe(m_accountCreatorStatusLabel, L"Account created or updated.");
                SetStatusText(L"Account created or updated.");
            }
            else {
                std::wstring error = result->error.empty() ? L"Account creation failed." : L"Account creation failed: " + result->error;
                SetWindowTextSafe(m_accountCreatorStatusLabel, error);
                SetStatusText(error);
            }
        }
        else if (result->action == ServerAction::AddIncidentExclusion ||
            result->action == ServerAction::RemoveIncidentExclusion)
        {
            SetStatusText(result->ok
                ? (result->action == ServerAction::AddIncidentExclusion
                    ? L"Event added to the shared exclusion list."
                    : L"Event removed from the shared exclusion list.")
                : (result->error.empty() ? L"Exclusion update failed." : result->error));
            PollServerAsync();
        }
        else if (result->action == ServerAction::ClearAdminLog) {
            SetStatusText(result->ok
                ? L"Selected Administrator Log cleared."
                : (result->error.empty() ? L"Administrator Log clear failed." : result->error));
            if (result->ok)
                FetchAdminLogAsync();
        }
        delete result;
    }

    void DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind kind = BoundaryDownloadKind::Uk)
    {
        if (g_boundaryDownloadInProgress.exchange(true)) {
            SetStatusText(L"Boundary download already in progress...");
            SetCacheManagerStatus(L"Boundary download already in progress...");
            return;
        }

        SetStatusText(kind == BoundaryDownloadKind::World
            ? L"Downloading world boundaries from geoBoundaries..."
            : L"Downloading UK and Ireland boundaries from geoBoundaries...");
        SetCacheManagerActivity(CacheActivity::Boundary, true);

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
            else if (kind == BoundaryDownloadKind::Uk) {
                std::vector<BYTE> irelandBytes;
                std::vector<BYTE> mergedBytes;
                if (!HttpGetBinary(kIrelandBoundarySourceUrl, irelandBytes, error)) {
                    result->ok = false;
                    result->error = L"Ireland boundary download failed: " + error;
                }
                else if (!MergeGeoJsonFeatureCollections(bytes, irelandBytes, mergedBytes, error)) {
                    result->ok = false;
                    result->error = error;
                }
                else if (!SaveBinaryToFile(cachePath, mergedBytes)) {
                    result->ok = false;
                    result->error = L"UK and Ireland boundaries downloaded but could not be saved locally.";
                }
                else {
                    result->ok = true;
                    result->filePath = cachePath;
                }
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
        SetCacheManagerActivity(CacheActivity::Boundary, false);

        if (!result)
            return;

        if (!result->ok) {
            SetStatusText(result->error);
            SetCacheManagerStatus(result->error);
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
                : L"UK and Ireland boundaries downloaded and loaded.");
            SetCacheManagerStatus(result->kind == BoundaryDownloadKind::World
                ? L"World boundaries refreshed."
                : L"UK and Ireland boundaries refreshed.");
        }
        else {
            SetStatusText(L"Boundary downloaded, but could not load it.");
            SetCacheManagerStatus(L"Boundary downloaded, but could not load.");
            OutputDebugStringW((L"Boundary load failed: " + loadError + L"\n").c_str());
        }

        delete result;
    }

    bool LoadPopulatedPlacesCache(bool reportStatus)
    {
        m_populatedPlacesLoadAttempted = true;

        const std::filesystem::path cachePath = GetPopulatedPlacesCachePath();
        if (!std::filesystem::exists(cachePath)) {
            if (reportStatus)
                SetStatusText(L"Populated places cache is not available yet.");
            return false;
        }

        std::vector<PopulatedPlace> places;
        std::wstring error;
        if (!LoadPopulatedPlacesFromFile(cachePath, places, &error)) {
            m_populatedPlacesLoaded = false;
            OutputDebugStringW((L"Populated places cache load: " + error + L"\n").c_str());
            if (reportStatus)
                SetStatusText(error.empty() ? L"Populated places cache could not be loaded." : error);
            return false;
        }

        m_populatedPlaces = std::move(places);
        m_populatedPlacesLoaded = !m_populatedPlaces.empty();
        if (reportStatus) {
            SetStatusText(L"Loaded " + std::to_wstring(m_populatedPlaces.size()) + L" populated place(s).");
        }
        return m_populatedPlacesLoaded;
    }

    void EnsurePopulatedPlacesAvailableAsync(bool reportStatus, bool forceRefresh = false)
    {
        if (!forceRefresh &&
            m_populatedPlacesLoaded &&
            PopulatedPlacesSupportPopulationFilter(m_populatedPlaces))
        {
            return;
        }

        if (!forceRefresh && !m_populatedPlacesLoadAttempted && LoadPopulatedPlacesCache(reportStatus)) {
            // Natural Earth alone contains about 7,000 records. A merged v2
            // cache contains well over 100,000, so transparently upgrade older
            // caches the first time the radius feature is used.
            if (PopulatedPlacesSupportPopulationFilter(m_populatedPlaces))
                return;
        }

        if (!forceRefresh && m_populatedPlacesDownloadAttempted && !reportStatus)
            return;

        if (g_populatedPlacesDownloadInProgress.exchange(true)) {
            if (reportStatus)
                SetStatusText(L"Populated places download already in progress...");
            if (reportStatus)
                SetCacheManagerStatus(L"Populated places download already in progress...");
            return;
        }

        m_populatedPlacesDownloadAttempted = true;
        if (reportStatus) {
            SetStatusText(L"Downloading populated places for earthquake radius checks...");
            SetCacheManagerActivity(CacheActivity::PopulatedPlaces, true);
        }

        HWND hwnd = m_hwnd;
        const std::filesystem::path cachePath = GetPopulatedPlacesCachePath();
        ScheduleBackgroundTask([hwnd, cachePath]() {
            auto* result = new PopulatedPlacesDownloadResult{};
            result->filePath = cachePath;

            std::string body;
            std::string supplementalBody;
            std::wstring error;
            constexpr DWORD kPopulatedPlacesDownloadTimeoutMs = 60000;
            if (!HttpGetTextWithTimeout(
                kPopulatedPlacesSourceUrl,
                body,
                error,
                kPopulatedPlacesDownloadTimeoutMs))
            {
                result->ok = false;
                result->error = L"Populated places download failed: " + error;
            }
            else if (!HttpGetTextWithTimeout(
                kSupplementalPopulatedPlacesSourceUrl,
                supplementalBody,
                error,
                kPopulatedPlacesDownloadTimeoutMs))
            {
                result->ok = false;
                result->error = L"Supplemental populated places download failed: " + error;
            }
            else {
                std::wstring parseError;
                std::vector<PopulatedPlace> places = ParsePopulatedPlacesGeoJson(body, &parseError);
                if (places.empty()) {
                    result->ok = false;
                    result->error = parseError.empty()
                        ? L"Populated places download did not contain usable places."
                        : parseError;
                }
                else {
                    std::vector<PopulatedPlace> supplementalPlaces =
                        ParseSupplementalPopulatedPlacesJson(supplementalBody, &parseError);
                    if (supplementalPlaces.empty()) {
                        result->ok = false;
                        result->error = parseError.empty()
                            ? L"Supplemental populated places did not contain usable places."
                            : parseError;
                    }
                    else {
                        MergePopulatedPlaces(places, std::move(supplementalPlaces));
                        std::wstring saveError;
                        const std::string cacheBody = SerializePopulatedPlacesCache(places, &saveError);
                        if (cacheBody.empty() ||
                            !SavePopulatedPlacesToFile(cachePath, cacheBody, &saveError))
                        {
                            result->ok = false;
                            result->error = saveError.empty()
                                ? L"Populated places downloaded but could not be saved locally."
                                : saveError;
                        }
                        else {
                            result->ok = true;
                            result->places = std::move(places);
                        }
                    }
                }
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_populatedPlacesDownloadInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_POPULATED_PLACES_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_populatedPlacesDownloadInProgress.store(false);
            }
            });
    }

    void OnPopulatedPlacesReady(PopulatedPlacesDownloadResult* result)
    {
        g_populatedPlacesDownloadInProgress.store(false);
        SetCacheManagerActivity(CacheActivity::PopulatedPlaces, false);

        std::unique_ptr<PopulatedPlacesDownloadResult> owned(result);
        if (!result)
            return;

        if (!result->ok) {
            m_populatedPlacesLoaded = false;
            OutputDebugStringW((result->error + L"\n").c_str());
            SetCacheManagerStatus(result->error.empty() ? L"Populated places refresh failed." : result->error);
            if (m_earthquakeListPopulatedRadiusMiles > 0.0 ||
                m_earthquakeNotificationPopulatedRadiusMiles > 0.0)
            {
                SetStatusText(result->error);
            }
            return;
        }

        m_populatedPlaces = std::move(result->places);
        if (m_populatedPlaces.empty())
            LoadPopulatedPlacesCache(false);
        else
            m_populatedPlacesLoaded = true;

        SetStatusText(L"Loaded " + std::to_wstring(m_populatedPlaces.size()) + L" populated place(s) for earthquake radius checks.");
        SetCacheManagerStatus(L"Populated places refreshed: " + std::to_wstring(m_populatedPlaces.size()) + L" place(s).");
        ApplyEarthquakeListFilters();
        if (!m_allEarthquakes.empty())
            NotifyForMatchingEarthquakes(m_allEarthquakes);
    }

    void DownloadRoadDepictionsAsync()
    {
        if (g_roadDepictionsDownloadInProgress.exchange(true)) {
            SetStatusText(L"OS Open Roads download already in progress...");
            SetCacheManagerStatus(L"OS Open Roads download already in progress...");
            return;
        }

        SyncRoadDepictionRoadLabelsFromList();
        std::vector<std::wstring> roads = NormaliseRoadDepictionRoadLabels(m_roadDepictionRoadLabels);
        if (roads.empty()) {
            g_roadDepictionsDownloadInProgress.store(false);
            SetStatusText(L"Add roads in Road Depictions before downloading OS Open Roads.");
            MessageBoxW(m_hwnd, L"Add at least one road in the Road Depictions window before downloading OS Open Roads.", L"Road Depictions", MB_OK | MB_ICONINFORMATION);
            return;
        }

        SetStatusText(L"Downloading OS Open Roads for selected road depictions...");
        SetCacheManagerActivity(CacheActivity::RoadDepictions, true);

        HWND hwnd = m_hwnd;
        const std::filesystem::path cachePath = GetRoadDepictionsCachePath();

        ScheduleBackgroundTask([hwnd, cachePath, roads = std::move(roads)]() {
            auto* result = new RoadDepictionsDownloadResult{};
            result->filePath = cachePath;

            size_t featureCount = 0;
            std::wstring error;
            if (DownloadOpenRoadsGeoJsonToFile(cachePath, roads, featureCount, error)) {
                result->ok = true;
                result->featureCount = featureCount;
            }
            else {
                result->ok = false;
                result->error = error.empty() ? L"OS Open Roads download failed." : error;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_roadDepictionsDownloadInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_ROAD_DEPICTIONS_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_roadDepictionsDownloadInProgress.store(false);
            }
            });
    }

    void OnRoadDepictionsReady(RoadDepictionsDownloadResult* result)
    {
        g_roadDepictionsDownloadInProgress.store(false);
        SetCacheManagerActivity(CacheActivity::RoadDepictions, false);

        std::unique_ptr<RoadDepictionsDownloadResult> holder(result);
        if (!result)
            return;

        if (!result->ok) {
            SetStatusText(result->error.empty() ? L"OS Open Roads download failed." : result->error);
            SetCacheManagerStatus(result->error.empty() ? L"OS Open Roads refresh failed." : result->error);
            return;
        }

        std::wstring loadError;
        std::unordered_set<std::wstring> allowedRoadDepictions = RoadDepictionRoadSet(m_roadDepictionRoadLabels);
        if (!m_map.LoadRoadDepictionsFromFile(result->filePath, &loadError, &allowedRoadDepictions)) {
            SetStatusText(L"OS Open Roads downloaded, but could not be loaded.");
            SetCacheManagerStatus(L"OS Open Roads downloaded, but could not load.");
            OutputDebugStringW((L"OS Open Roads load failed: " + loadError + L"\n").c_str());
            return;
        }

        RenderRoadDepictionsList();
        SetStatusText(L"OS Open Roads selected roads downloaded and loaded (" + std::to_wstring(result->featureCount) + L" features).");
        SetCacheManagerStatus(L"OS Open Roads refreshed: " + std::to_wstring(result->featureCount) + L" feature(s).");
    }

    static LRESULT CALLBACK CacheManagerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleCacheManagerMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleCacheManagerMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateCacheManagerControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnCacheManagerCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowCacheManagerWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = CacheManagerWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kCacheManagerClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_cacheManagerWnd || !IsWindow(m_cacheManagerWnd)) {
            m_cacheManagerWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kCacheManagerClassName,
                L"Data Caches",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                430,
                390,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        ShowWindow(m_cacheManagerWnd, SW_SHOW);
        SetForegroundWindow(m_cacheManagerWnd);
    }

    void CreateCacheManagerControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Data Caches", 18, 20, m_headerFont);
        CreateAutoLabel(parent, 0, L"Refresh the local datasets used by the map and earthquake populated-area checks.", 18, 58, nullptr, 360);

        HWND ukBoundary = CreateWindowExW(0, L"BUTTON", L"Download / refresh UK and Ireland", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 112, 310, 32, parent, ControlId(IDC_CACHE_UK_BOUNDARY_BTN), m_hInst, nullptr);
        HWND worldBoundary = CreateWindowExW(0, L"BUTTON", L"Download / refresh world boundaries", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 152, 310, 32, parent, ControlId(IDC_CACHE_WORLD_BOUNDARY_BTN), m_hInst, nullptr);
        HWND roads = CreateWindowExW(0, L"BUTTON", L"Download / refresh OS Open Roads", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 192, 310, 32, parent, ControlId(IDC_CACHE_ROADS_BTN), m_hInst, nullptr);
        HWND populatedPlaces = CreateWindowExW(0, L"BUTTON", L"Download / refresh populated places", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 232, 310, 32, parent, ControlId(IDC_CACHE_POPULATED_PLACES_BTN), m_hInst, nullptr);
        m_cacheProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | PBS_MARQUEE, 18, 278, 372, 16, parent, ControlId(IDC_CACHE_PROGRESS), m_hInst, nullptr);
        m_cacheStatusLabel = CreateAutoLabel(parent, IDC_CACHE_STATUS_LABEL, L"Ready.", 18, 306, nullptr, 256);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 288, 318, 102, 32, parent, ControlId(IDC_CACHE_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { ukBoundary, worldBoundary, roads, populatedPlaces, m_cacheStatusLabel, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        if (m_cacheProgressBar)
            ApplyExplorerTheme(m_cacheProgressBar);
        UpdateCacheManagerProgressVisibility();
        AutoFitWindowToChildren(parent);
    }

    void SetCacheManagerStatus(const std::wstring& text)
    {
        if (m_cacheStatusLabel)
            SetWindowTextSafe(m_cacheStatusLabel, text);
    }

    void SetCacheManagerActivity(CacheActivity activity, bool active)
    {
        switch (activity) {
        case CacheActivity::Boundary:
            m_cacheBoundaryBusy = active;
            break;
        case CacheActivity::RoadDepictions:
            m_cacheRoadDepictionsBusy = active;
            break;
        case CacheActivity::PopulatedPlaces:
            m_cachePopulatedPlacesBusy = active;
            break;
        }

        UpdateCacheManagerProgressVisibility();
    }

    bool HasActiveCacheManagerWork() const
    {
        return m_cacheBoundaryBusy || m_cacheRoadDepictionsBusy || m_cachePopulatedPlacesBusy;
    }

    void UpdateCacheManagerProgressVisibility()
    {
        if (!m_cacheProgressBar)
            return;

        const bool active = HasActiveCacheManagerWork();
        ShowWindow(m_cacheProgressBar, active ? SW_SHOW : SW_HIDE);
        SendMessageW(m_cacheProgressBar, PBM_SETMARQUEE, active ? TRUE : FALSE, active ? 24 : 0);
        if (!active)
            SendMessageW(m_cacheProgressBar, PBM_SETPOS, 0, 0);
    }

    void OnCacheManagerCommand(int id, int code)
    {
        if (code != BN_CLICKED)
            return;

        switch (id) {
        case IDC_CACHE_UK_BOUNDARY_BTN:
            SetCacheManagerStatus(L"Refreshing UK and Ireland boundaries...");
            DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind::Uk);
            break;
        case IDC_CACHE_WORLD_BOUNDARY_BTN:
            SetCacheManagerStatus(L"Refreshing world boundaries...");
            DownloadBoundaryFromGitHubAsync(BoundaryDownloadKind::World);
            break;
        case IDC_CACHE_ROADS_BTN:
            SetCacheManagerStatus(L"Refreshing OS Open Roads...");
            DownloadRoadDepictionsAsync();
            break;
        case IDC_CACHE_POPULATED_PLACES_BTN:
            SetCacheManagerStatus(L"Refreshing populated places...");
            EnsurePopulatedPlacesAvailableAsync(true, true);
            break;
        case IDC_CACHE_CLOSE_BTN:
            ShowWindow(m_cacheManagerWnd, SW_HIDE);
            break;
        }
    }

    void CreateMainMenu()
    {
        HMENU menu = CreateMenu();
        HMENU fileMenu = CreatePopupMenu();
        HMENU settingsMenu = CreatePopupMenu();
        HMENU roadsMenu = CreatePopupMenu();
        HMENU earthquakesMenu = CreatePopupMenu();
        HMENU weatherMenu = CreatePopupMenu();
        HMENU viewMenu = CreatePopupMenu();
        HMENU aboutMenu = CreatePopupMenu();
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_CACHE_MANAGER, L"Data Caches...");
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_USERS, L"Users");
        if (CanManageAccounts())
            AppendMenuW(fileMenu, MF_STRING, IDM_FILE_ACCOUNT_CREATOR, L"Account Creator...");
        if (CanViewAdministratorLog())
            AppendMenuW(fileMenu, MF_STRING, IDM_FILE_ADMIN_LOG, L"Administrator Log...");
        AppendMenuW(fileMenu, MF_STRING, IDM_ROADS_INCIDENT_EXCLUSIONS, L"Exclusions...");
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_LOGOUT, L"Logout");
        AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXIT, L"Exit");
        AppendMenuW(settingsMenu, MF_STRING, IDM_SETTINGS_GENERAL, L"General...");
        AppendMenuW(settingsMenu, MF_STRING, IDM_SETTINGS_SOUNDS, L"Sounds...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENTS_LIST, L"Incidents List...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_FILTERS, L"Incident Filters...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_INCIDENT_NOTIFICATIONS, L"Incident Notifications...");
        AppendMenuW(roadsMenu, MF_STRING, IDM_ROADS_TRAFFIC_SCOTLAND, L"Traffic Scotland...");
        AppendMenuW(roadsMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(roadsMenu, m_showIncidents ? MF_CHECKED : MF_UNCHECKED, IDM_ROADS_SHOW_INCIDENTS, L"Show Incidents");
        HMENU incidentOverlayMenu = CreatePopupMenu();
        const UINT incidentOverlayEnabled = m_showIncidents ? MF_ENABLED : MF_GRAYED;
        AppendMenuW(incidentOverlayMenu, incidentOverlayEnabled | (m_showIncidentOverlayLabels ? MF_UNCHECKED : MF_CHECKED), IDM_INCIDENT_OVERLAY_NONE, L"None");
        AppendMenuW(incidentOverlayMenu, incidentOverlayEnabled | (m_showIncidentOverlayLabels ? MF_CHECKED : MF_UNCHECKED), IDM_INCIDENT_OVERLAY_SUMMARY, L"Summary");
        AppendMenuW(incidentOverlayMenu, incidentOverlayEnabled | (m_incidentOverlayNotifiedOnly ? MF_CHECKED : MF_UNCHECKED), IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY, L"Only Incidents List");
        AppendMenuW(roadsMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(incidentOverlayMenu), L"Incident Overlays");
        MENUITEMINFOW roadRadioInfo{};
        roadRadioInfo.cbSize = sizeof(roadRadioInfo);
        roadRadioInfo.fMask = MIIM_FTYPE;
        roadRadioInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(roadsMenu, IDM_ROADS_SHOW_INCIDENTS, FALSE, &roadRadioInfo);
        SetMenuItemInfoW(incidentOverlayMenu, IDM_INCIDENT_OVERLAY_NONE, FALSE, &roadRadioInfo);
        SetMenuItemInfoW(incidentOverlayMenu, IDM_INCIDENT_OVERLAY_SUMMARY, FALSE, &roadRadioInfo);
        SetMenuItemInfoW(incidentOverlayMenu, IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY, FALSE, &roadRadioInfo);
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
        AppendMenuW(weatherMenu, weatherOverlayEnabled | (m_showWeatherSystemForecasts ? MF_CHECKED : MF_UNCHECKED), IDM_WEATHER_SYSTEM_FORECASTS, L"Weather System Forecasts");
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
        AppendMenuW(weatherMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(weatherMenu, MF_STRING, IDM_WEATHER_WARNINGS_LIST, L"Weather Warnings...");
        AppendMenuW(weatherMenu, m_showWeatherWarnings ? MF_CHECKED : MF_UNCHECKED, IDM_SHOW_WEATHER_WARNINGS, L"Show Weather Warnings");
        HMENU weatherWarningOverlayMenu = CreatePopupMenu();
        const UINT weatherWarningOverlayEnabled = m_showWeatherWarnings ? MF_ENABLED : MF_GRAYED;
        AppendMenuW(weatherWarningOverlayMenu, weatherWarningOverlayEnabled | (m_showWeatherWarningOverlayLabels ? MF_UNCHECKED : MF_CHECKED), IDM_WEATHER_WARNING_OVERLAY_NONE, L"None");
        AppendMenuW(weatherWarningOverlayMenu, weatherWarningOverlayEnabled | (m_showWeatherWarningOverlayLabels ? MF_CHECKED : MF_UNCHECKED), IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA, L"Type and Area");
        AppendMenuW(weatherMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(weatherWarningOverlayMenu), L"Weather Warning Overlays");
        AppendMenuW(weatherMenu, weatherWarningOverlayEnabled | (m_showWeatherWarningPolygons ? MF_CHECKED : MF_UNCHECKED), IDM_WEATHER_WARNING_POLYGONS, L"Weather Warning Polygons");
        SetMenuItemInfoW(weatherMenu, IDM_SHOW_WEATHER_WARNINGS, FALSE, &showWeatherInfo);
        SetMenuItemInfoW(weatherWarningOverlayMenu, IDM_WEATHER_WARNING_OVERLAY_NONE, FALSE, &weatherOverlayInfo);
        SetMenuItemInfoW(weatherWarningOverlayMenu, IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA, FALSE, &weatherOverlayInfo);
        AppendMenuW(weatherMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(weatherMenu, MF_STRING, IDM_FLOODS_LIST, L"Floods...");
        AppendMenuW(weatherMenu, m_showFloods ? MF_CHECKED : MF_UNCHECKED, IDM_SHOW_FLOODS, L"Show Floods");
        HMENU floodOverlayMenu = CreatePopupMenu();
        const UINT floodOverlayEnabled = m_showFloods ? MF_ENABLED : MF_GRAYED;
        AppendMenuW(floodOverlayMenu, floodOverlayEnabled | (m_showFloodOverlayLabels ? MF_UNCHECKED : MF_CHECKED), IDM_FLOOD_OVERLAY_NONE, L"None");
        AppendMenuW(floodOverlayMenu, floodOverlayEnabled | (m_showFloodOverlayLabels ? MF_CHECKED : MF_UNCHECKED), IDM_FLOOD_OVERLAY_SEVERITY_AREA, L"Severity and Area");
        AppendMenuW(weatherMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(floodOverlayMenu), L"Flood Overlays");
        SetMenuItemInfoW(weatherMenu, IDM_SHOW_FLOODS, FALSE, &showWeatherInfo);
        SetMenuItemInfoW(floodOverlayMenu, IDM_FLOOD_OVERLAY_NONE, FALSE, &weatherOverlayInfo);
        SetMenuItemInfoW(floodOverlayMenu, IDM_FLOOD_OVERLAY_SEVERITY_AREA, FALSE, &weatherOverlayInfo);
        AppendMenuW(viewMenu, m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_NOTIFICATION_HISTORY, L"Notification History");
        AppendMenuW(viewMenu, m_showAreaLabels ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_AREA_LABELS, L"Area Labels");
        AppendMenuW(viewMenu, m_showIncidentNotificationRegionPolygons ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_NOTIFICATION_REGION_POLYGONS, L"Notification Region Polygons");
        HMENU roadDepictionsMenu = CreatePopupMenu();
        AppendMenuW(roadDepictionsMenu, m_showRoadDepictions ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_ROAD_DEPICTIONS, L"Show Road Depictions");
        AppendMenuW(roadDepictionsMenu, MF_STRING, IDM_VIEW_ROAD_DEPICTIONS_LIST, L"Choose Roads...");
        AppendMenuW(viewMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(roadDepictionsMenu), L"Road Depictions");
        AppendMenuW(viewMenu, m_showFpsCounter ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_FPS_COUNTER, L"FPS Counter");
        AppendMenuW(viewMenu, m_showMapControls ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_MAP_CONTROLS, L"Map Controls");
        AppendMenuW(viewMenu, m_showCountdownTimer ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_COUNTDOWN_TIMER, L"Countdown Timer");
        AppendMenuW(viewMenu, m_showCommsIndicator ? MF_CHECKED : MF_UNCHECKED, IDM_VIEW_COMMS_INDICATOR, L"Communications Indicator");
        MENUITEMINFOW historyInfo{};
        historyInfo.cbSize = sizeof(historyInfo);
        historyInfo.fMask = MIIM_FTYPE;
        historyInfo.fType = MFT_RADIOCHECK;
        SetMenuItemInfoW(viewMenu, IDM_VIEW_NOTIFICATION_HISTORY, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_AREA_LABELS, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_NOTIFICATION_REGION_POLYGONS, FALSE, &historyInfo);
        SetMenuItemInfoW(roadDepictionsMenu, IDM_VIEW_ROAD_DEPICTIONS, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_FPS_COUNTER, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_MAP_CONTROLS, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_COUNTDOWN_TIMER, FALSE, &historyInfo);
        SetMenuItemInfoW(viewMenu, IDM_VIEW_COMMS_INDICATOR, FALSE, &historyInfo);
        AppendMenuW(aboutMenu, MF_STRING, IDM_ABOUT_APP, L"About ERC Tools...");
        AppendMenuW(aboutMenu, MF_STRING, IDM_ABOUT_LEGEND, L"Legend...");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"File");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(settingsMenu), L"Settings");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu), L"View");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(roadsMenu), L"Roads");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(earthquakesMenu), L"Earthquakes");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(weatherMenu), L"Weather");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(aboutMenu), L"About");
        SetMenu(m_hwnd, menu);
    }

    void ShowAboutDialog()
    {
        std::wstring message = L"ERC Tools\nVersion ";
        message += kClientVersion;
        message += L"\n\nCreated by Samuel Mason.\n\nView live alerts on a UK map, collaborate with local responders, and share map notes.\n\nSettlement data: Natural Earth and GeoNames cities500 via cities-json (CC BY 4.0).";
        MessageBoxW(
            m_hwnd,
            message.c_str(),
            L"About ERC Tools",
            MB_OK | MB_ICONINFORMATION);
    }

    static LRESULT CALLBACK LegendWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleLegendMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleLegendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_PAINT:
            PaintLegend(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowLegendWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = LegendWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kLegendClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_legendWnd || !IsWindow(m_legendWnd)) {
            m_legendWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kLegendClassName,
                L"ERC Tools Legend",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                560,
                560,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        ShowWindow(m_legendWnd, SW_SHOW);
        SetForegroundWindow(m_legendWnd);
        InvalidateRect(m_legendWnd, nullptr, TRUE);
    }

    void PaintLegend(HWND hwnd)
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        if (!hdc)
            return;

        RECT client{};
        GetClientRect(hwnd, &client);
        HBRUSH background = CreateSolidBrush(kUiBackground);
        FillRect(hdc, &client, background);
        DeleteObject(background);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kUiText);

        HGDIOBJ oldFont = SelectObject(hdc, m_headerFont ? m_headerFont : GetStockObject(DEFAULT_GUI_FONT));
        RECT titleRect{ 24, 20, client.right - 24, 56 };
        DrawTextW(hdc, L"Map Legend", -1, &titleRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        SelectObject(hdc, m_font ? m_font : GetStockObject(DEFAULT_GUI_FONT));

        auto makeBrush = [](COLORREF color) { return CreateSolidBrush(color); };
        auto makePen = [](COLORREF color, int width = 1) { return CreatePen(PS_SOLID, width, color); };

        auto scopedSelect = [](HDC dc, HGDIOBJ obj, HGDIOBJ& oldObj) {
            oldObj = SelectObject(dc, obj);
            };

        auto drawCircle = [&](int x, int y, int radius, COLORREF fill, COLORREF outline, int outlineWidth = 1) {
            HBRUSH brush = makeBrush(fill);
            HPEN pen = makePen(outline, outlineWidth);
            HGDIOBJ oldBrush = nullptr;
            HGDIOBJ oldPen = nullptr;
            scopedSelect(hdc, brush, oldBrush);
            scopedSelect(hdc, pen, oldPen);
            Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            };

        auto drawLine = [&](int x1, int y1, int x2, int y2, COLORREF color, int width = 1, int style = PS_SOLID) {
            HPEN pen = CreatePen(style, width, color);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            MoveToEx(hdc, x1, y1, nullptr);
            LineTo(hdc, x2, y2);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
            };

        auto drawPin = [&](int x, int y, COLORREF color) {
            POINT triangle[3] = { { x - 7, y + 4 }, { x, y + 23 }, { x + 7, y + 4 } };
            HBRUSH brush = makeBrush(color);
            HPEN pen = makePen(RGB(42, 56, 70), 1);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            Polygon(hdc, triangle, 3);
            Ellipse(hdc, x - 12, y - 12, x + 12, y + 12);
            HBRUSH inner = makeBrush(RGB(255, 255, 255));
            SelectObject(hdc, inner);
            Ellipse(hdc, x - 6, y - 6, x + 6, y + 6);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(inner);
            DeleteObject(brush);
            DeleteObject(pen);
            };

        auto drawWarningTriangle = [&](int x, int y) {
            POINT tri[3] = { { x, y - 15 }, { x + 16, y + 13 }, { x - 16, y + 13 } };
            HBRUSH brush = makeBrush(RGB(247, 199, 53));
            HPEN pen = makePen(RGB(151, 112, 4), 2);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            Polygon(hdc, tri, 3);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            SetTextColor(hdc, RGB(35, 44, 51));
            RECT mark{ x - 4, y - 8, x + 4, y + 12 };
            DrawTextW(hdc, L"!", -1, &mark, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SetTextColor(hdc, kUiText);
            };

        auto drawFlood = [&](int x, int y) {
            RECT diamond{ x - 13, y - 13, x + 13, y + 13 };
            HBRUSH brush = makeBrush(RGB(44, 143, 204));
            HPEN pen = makePen(RGB(24, 67, 104), 2);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, diamond.left, diamond.top, diamond.right, diamond.bottom, 7, 7);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            drawLine(x - 8, y + 2, x - 3, y - 2, RGB(255, 255, 255), 2);
            drawLine(x - 3, y - 2, x + 3, y + 2, RGB(255, 255, 255), 2);
            drawLine(x + 3, y + 2, x + 8, y - 1, RGB(255, 255, 255), 2);
            };

        auto drawWeatherSystem = [&](int x, int y) {
            drawLine(x - 20, y + 18, x + 22, y - 18, RGB(41, 169, 135), 3);
            drawLine(x + 22, y - 18, x + 8, y - 15, RGB(12, 113, 89), 3);
            drawLine(x + 22, y - 18, x + 16, y - 6, RGB(12, 113, 89), 3);
            drawCircle(x - 20, y + 18, 5, RGB(238, 219, 75), RGB(80, 84, 42), 1);
            HPEN dotted = CreatePen(PS_DASH, 2, RGB(10, 117, 105));
            HGDIOBJ oldPen = SelectObject(hdc, dotted);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            Ellipse(hdc, x - 36, y + 2, x - 4, y + 34);
            Ellipse(hdc, x + 4, y - 36, x + 50, y + 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(dotted);
            drawCircle(x - 2, y - 2, 6, RGB(72, 198, 165), RGB(24, 96, 85), 1);
            };

        auto drawNote = [&](int x, int y) {
            HBRUSH brush = makeBrush(RGB(63, 76, 93));
            HPEN pen = makePen(RGB(111, 74, 235), 2);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, x - 22, y - 14, x + 26, y + 12, 10, 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            drawLine(x - 22, y + 12, x - 30, y + 20, RGB(111, 74, 235), 2);
            drawCircle(x - 31, y + 21, 5, RGB(111, 74, 235), RGB(63, 76, 93), 1);
            };

        auto drawRegion = [&](int x, int y) {
            HPEN pen = makePen(RGB(18, 108, 199), 3);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            Rectangle(hdc, x - 24, y - 14, x + 24, y + 18);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
            drawCircle(x - 24, y - 14, 3, RGB(18, 108, 199), RGB(18, 108, 199), 1);
            drawCircle(x + 24, y + 18, 3, RGB(18, 108, 199), RGB(18, 108, 199), 1);
            };

        auto drawRoad = [&](int x, int y) {
            drawLine(x - 28, y, x + 30, y - 10, RGB(47, 72, 55), 7);
            drawLine(x - 28, y, x + 30, y - 10, RGB(232, 222, 165), 4);
            RECT shield{ x - 8, y - 23, x + 24, y - 4 };
            HBRUSH brush = makeBrush(RGB(255, 255, 255));
            HPEN pen = makePen(RGB(47, 72, 55), 1);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, shield.left, shield.top, shield.right, shield.bottom, 6, 6);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            DrawTextW(hdc, L"A1", -1, &shield, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            };

        auto drawPanel = [&](int x, int y) {
            HBRUSH brush = makeBrush(RGB(47, 61, 72));
            HPEN pen = makePen(RGB(83, 103, 121), 1);
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, x - 26, y - 15, x + 28, y + 17, 10, 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            drawLine(x - 16, y - 4, x + 16, y - 4, RGB(126, 148, 166), 1);
            drawLine(x - 16, y + 5, x + 16, y + 5, RGB(126, 148, 166), 1);
            };

        int y = 74;
        auto row = [&](const wchar_t* text, const std::function<void(int, int)>& drawIcon) {
            drawIcon(52, y + 17);
            RECT textRect{ 96, y, client.right - 26, y + 36 };
            DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            y += 43;
            };

        row(L"Road incidents: severity pins", [&](int x, int y0) {
            drawPin(x - 18, y0 - 2, RGB(211, 44, 44));
            drawPin(x + 8, y0 - 2, RGB(231, 146, 42));
            drawPin(x + 34, y0 - 2, RGB(39, 126, 213));
            });
        row(L"Earthquakes: magnitude blips", [&](int x, int y0) {
            drawCircle(x, y0, 13, RGB(187, 45, 58), RGB(117, 23, 35), 2);
            });
        row(L"Weather systems: centre, track, forecast rings and arrow", drawWeatherSystem);
        row(L"Weather warnings: warning marker and optional polygon", drawWarningTriangle);
        row(L"Floods: flood alert marker", drawFlood);
        row(L"Notes: map note bubble and anchor", drawNote);
        row(L"Notification regions: editable region outline", drawRegion);
        row(L"Road depictions: OS Open Roads line and label", drawRoad);
        row(L"Map overlay panels: draggable tool surfaces", drawPanel);

        SelectObject(hdc, oldFont);
        EndPaint(hwnd, &ps);
    }

    static LRESULT CALLBACK AdminLogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleAdminLogMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleAdminLogMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateAdminLogControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnAdminLogCommand(LOWORD(wParam), HIWORD(wParam));
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

    void ShowAdminLogWindow()
    {
        if (!CanViewAdministratorLog()) {
            SetStatusText(L"Only Administrators can view the Administrator Log.");
            MessageBoxW(m_hwnd, L"Only Administrators can view the Administrator Log.", L"Administrator Log", MB_OK | MB_ICONINFORMATION);
            return;
        }
        if (!IsOnlineMode()) {
            SetStatusText(L"Administrator Log needs an online session.");
            MessageBoxW(m_hwnd, L"The Administrator Log is tracked by the server, so it is only available in online mode.", L"Administrator Log", MB_OK | MB_ICONINFORMATION);
            return;
        }

        static bool registered = false;
        if (!registered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc = AdminLogWndProc;
            wc.hInstance = m_hInst;
            wc.lpszClassName = kAdminLogClassName;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            ::RegisterClassW(&wc);
            registered = true;
        }

        if (!m_adminLogWnd) {
            RECT rc{ 0, 0, 820, 520 };
            AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
            m_adminLogWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kAdminLogClassName,
                L"Administrator Log",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT,
                rc.right - rc.left, rc.bottom - rc.top,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
            EnableModernWindowFrame(m_adminLogWnd);
        }

        ShowWindow(m_adminLogWnd, SW_SHOW);
        SetForegroundWindow(m_adminLogWnd);
        FetchAdminLogAsync();
    }

    void CreateAdminLogControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Administrator Log", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Log type", 18, 64);
        m_adminLogTypeCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            18, 90, 240, 120,
            parent,
            ControlId(IDC_ADMIN_LOG_TYPE_COMBO),
            m_hInst,
            nullptr);
        HWND refreshBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON,
            458, 88, 102, 32,
            parent,
            ControlId(IDC_ADMIN_LOG_REFRESH_BTN),
            m_hInst,
            nullptr);
        HWND clearBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Clear selected log",
            WS_CHILD | (CurrentPositionRank() >= 4 ? WS_VISIBLE : 0) | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON,
            570, 88, 132, 32,
            parent,
            ControlId(IDC_ADMIN_LOG_CLEAR_BTN),
            m_hInst,
            nullptr);
        HWND closeBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Close",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON,
            712, 88, 78, 32,
            parent,
            ControlId(IDC_ADMIN_LOG_CLOSE_BTN),
            m_hInst,
            nullptr);
        m_adminLogListView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            18, 138, 772, 340,
            parent,
            ControlId(IDC_ADMIN_LOG_LIST),
            m_hInst,
            nullptr);

        for (HWND h : { m_adminLogTypeCombo, refreshBtn, clearBtn, closeBtn, m_adminLogListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SendMessageW(m_adminLogTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"User Login Times"));
        SendMessageW(m_adminLogTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Exclusions"));
        SendMessageW(m_adminLogTypeCombo, CB_SETCURSEL, 0, 0);

        SendMessageW(m_adminLogListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);
        ListView_SetBkColor(m_adminLogListView, kUiSurface);
        ListView_SetTextBkColor(m_adminLogListView, CLR_NONE);
        ListView_SetTextColor(m_adminLogListView, kUiText);

        struct Column { const wchar_t* title; int width; };
        const Column columns[] = {
            { L"Time", 142 },
            { L"Event", 92 },
            { L"User", 120 },
            { L"Position", 104 },
            { L"Pod", 76 },
            { L"Actor", 112 },
            { L"Details", 210 }
        };
        for (int i = 0; i < static_cast<int>(sizeof(columns) / sizeof(columns[0])); ++i) {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<LPWSTR>(columns[i].title);
            col.cx = columns[i].width;
            col.iSubItem = i;
            SendMessageW(m_adminLogListView, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&col));
        }
        RenderAdminLogEntries();
    }

    void OnAdminLogCommand(int id, int code)
    {
        if (id == IDC_ADMIN_LOG_CLOSE_BTN && code == BN_CLICKED) {
            if (m_adminLogWnd)
                ShowWindow(m_adminLogWnd, SW_HIDE);
            return;
        }
        if (id == IDC_ADMIN_LOG_CLEAR_BTN && code == BN_CLICKED) {
            ClearSelectedAdminLogAsync();
            return;
        }
        if ((id == IDC_ADMIN_LOG_REFRESH_BTN && code == BN_CLICKED) ||
            (id == IDC_ADMIN_LOG_TYPE_COMBO && code == CBN_SELCHANGE))
        {
            FetchAdminLogAsync();
        }
    }

    void ClearSelectedAdminLogAsync()
    {
        if (CurrentPositionRank() < 4 || !IsOnlineMode())
            return;

        const int selectedType = m_adminLogTypeCombo
            ? static_cast<int>(SendMessageW(m_adminLogTypeCombo, CB_GETCURSEL, 0, 0))
            : 0;
        const std::wstring label = selectedType == 1 ? L"Exclusions" : L"User Login Times";
        if (MessageBoxW(
            m_adminLogWnd,
            (L"Clear all entries in '" + label + L"'?").c_str(),
            L"Administrator Log",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        {
            return;
        }

        const std::wstring category = selectedType == 1 ? L"exclusions" : L"user_login_times";
        const std::wstring server = ServerBaseUrl();
        const std::wstring authHeaders = BearerAuthHeader(m_session);
        HWND hwnd = m_hwnd;
        ScheduleBackgroundTask([hwnd, server, authHeaders, category]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::ClearAdminLog;
            std::string response;
            std::wstring error;
            result->ok = HttpDeleteTextWithHeaders(
                AppendPath(server, (L"/api/admin/logs/" + category).c_str()),
                authHeaders,
                response,
                error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
    }

    void FetchAdminLogAsync()
    {
        if (!m_adminLogWnd || !IsOnlineMode() || !CanViewAdministratorLog())
            return;

        std::wstring server = ServerBaseUrl();
        if (Trim(server).empty()) {
            SetStatusText(L"Set the collaboration server before loading the Administrator Log.");
            return;
        }

        SetStatusText(L"Loading Administrator Log...");
        HWND hwnd = m_hwnd;
        ClientSession session = m_session;
        std::wstring authHeaders = BearerAuthHeader(session);
        std::thread([hwnd, server, authHeaders]() {
            auto* result = new AdminLogResult();
            std::string response;
            std::wstring error;
            if (HttpGetTextWithHeaders(AppendPath(server, L"/api/admin/logs"), authHeaders, response, error)) {
                try {
                    result->entries = ParseAdminLogEntries(json::parse(response));
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    result->ok = false;
                    result->error = L"Administrator Log parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->ok = false;
                result->error = error.empty() ? L"Administrator Log fetch failed." : error;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            if (!PostMessageW(hwnd, WM_APP_ADMIN_LOG_READY, 0, reinterpret_cast<LPARAM>(result)))
                delete result;
            }).detach();
    }

    void OnAdminLogReady(AdminLogResult* result)
    {
        if (!result)
            return;
        if (!result->ok) {
            SetStatusText(result->error.empty() ? L"Administrator Log failed to load." : result->error);
            delete result;
            return;
        }

        m_adminLogEntries = std::move(result->entries);
        RenderAdminLogEntries();
        SetStatusText(L"Loaded " + std::to_wstring(m_adminLogEntries.size()) + L" Administrator Log item(s).");
        delete result;
    }

    void RenderAdminLogEntries()
    {
        if (!m_adminLogListView)
            return;

        SendMessageW(m_adminLogListView, LVM_DELETEALLITEMS, 0, 0);
        const int selectedType = m_adminLogTypeCombo
            ? static_cast<int>(SendMessageW(m_adminLogTypeCombo, CB_GETCURSEL, 0, 0))
            : 0;
        int row = 0;
        for (const AdminLogEntry& entry : m_adminLogEntries) {
            const std::wstring eventType = ToLower(entry.event);
            const bool exclusionEvent =
                eventType.rfind(L"incident_exclusion_", 0) == 0 ||
                eventType.rfind(L"exclusion_", 0) == 0;
            const bool loginEvent = eventType == L"login" || eventType == L"logout";
            if ((selectedType == 1 && !exclusionEvent) ||
                (selectedType == 0 && !loginEvent))
                continue;
            std::wstring user = entry.displayName.empty() ? entry.username : entry.displayName;
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(entry.timestamp.c_str());
            int inserted = static_cast<int>(SendMessageW(m_adminLogListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (inserted < 0)
                continue;

            auto setSub = [&](int subItem, const std::wstring& text) {
                LVITEMW sub{};
                sub.iSubItem = subItem;
                sub.pszText = const_cast<LPWSTR>(text.c_str());
                SendMessageW(m_adminLogListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
                };
            setSub(1, entry.event);
            setSub(2, user);
            setSub(3, entry.position);
            setSub(4, entry.pod);
            setSub(5, entry.actor);
            setSub(6, entry.details);
            ++row;
        }
    }

    static LRESULT CALLBACK RoadDepictionsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleRoadDepictionsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleRoadDepictionsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateRoadDepictionsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnRoadDepictionsCommand(LOWORD(wParam), HIWORD(wParam));
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

    void ShowRoadDepictionsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSW wc{};
            wc.lpfnWndProc = RoadDepictionsWndProc;
            wc.hInstance = m_hInst;
            wc.lpszClassName = kRoadDepictionsClassName;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            ::RegisterClassW(&wc);
            registered = true;
        }

        if (!m_roadDepictionsWnd) {
            RECT rc{ 0, 0, 460, 505 };
            AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
            m_roadDepictionsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kRoadDepictionsClassName,
                L"Road Depictions",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT,
                rc.right - rc.left, rc.bottom - rc.top,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
            EnableModernWindowFrame(m_roadDepictionsWnd);
        }

        ShowWindow(m_roadDepictionsWnd, SW_SHOW);
        SetForegroundWindow(m_roadDepictionsWnd);
        RenderRoadDepictionsList();
    }

    void CreateRoadDepictionsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Road Depictions", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Add roads to download; select roads to show on the map.", 18, 56, nullptr, 400);
        m_roadDepictionsList = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_MULTIPLESEL | LBS_NOTIFY | WS_VSCROLL,
            18, 92, 406, 230,
            parent,
            ControlId(IDC_ROAD_DEPICTIONS_LIST),
            m_hInst,
            nullptr);
        CreateAutoLabel(parent, 0, L"Road", 18, 340);
        m_roadDepictionsRoadEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            72, 334, 132, 26,
            parent,
            ControlId(IDC_ROAD_DEPICTIONS_ROAD_EDIT),
            m_hInst,
            nullptr);
        HWND addBtn = CreateWindowExW(0, L"BUTTON", L"Add Road", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 214, 330, 102, 32, parent, ControlId(IDC_ROAD_DEPICTIONS_ADD_BTN), m_hInst, nullptr);
        HWND removeBtn = CreateWindowExW(0, L"BUTTON", L"Remove Road", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 322, 330, 122, 32, parent, ControlId(IDC_ROAD_DEPICTIONS_REMOVE_BTN), m_hInst, nullptr);
        HWND allBtn = CreateWindowExW(0, L"BUTTON", L"Show All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 386, 102, 32, parent, ControlId(IDC_ROAD_DEPICTIONS_ALL_BTN), m_hInst, nullptr);
        HWND noneBtn = CreateWindowExW(0, L"BUTTON", L"Hide All", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 128, 386, 102, 32, parent, ControlId(IDC_ROAD_DEPICTIONS_NONE_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 322, 386, 102, 32, parent, ControlId(IDC_ROAD_DEPICTIONS_CLOSE_BTN), m_hInst, nullptr);
        for (HWND h : { m_roadDepictionsList, m_roadDepictionsRoadEdit, addBtn, removeBtn, allBtn, noneBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_roadDepictionsRoadEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"M25, A1(M), A14..."));
        RenderRoadDepictionsList();
    }

    void RenderRoadDepictionsList()
    {
        if (!m_roadDepictionsList)
            return;

        SendMessageW(m_roadDepictionsList, LB_RESETCONTENT, 0, 0);
        std::vector<std::wstring> labels = NormaliseRoadDepictionRoadLabels(m_roadDepictionRoadLabels);
        m_roadDepictionRoadLabels = labels;
        for (const std::wstring& label : labels) {
            int idx = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            if (idx >= 0)
                SendMessageW(m_roadDepictionsList, LB_SETSEL, m_hiddenRoadDepictionIds.find(label) == m_hiddenRoadDepictionIds.end(), idx);
        }
    }

    void SyncRoadDepictionRoadLabelsFromList()
    {
        if (!m_roadDepictionsList)
            return;

        std::vector<std::wstring> labels;
        const int count = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETCOUNT, 0, 0));
        labels.reserve(count > 0 ? static_cast<size_t>(count) : 0);
        for (int i = 0; i < count; ++i) {
            const int textLen = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETTEXTLEN, i, 0));
            if (textLen < 0)
                continue;
            std::wstring text(static_cast<size_t>(textLen) + 1, L'\0');
            SendMessageW(m_roadDepictionsList, LB_GETTEXT, i, reinterpret_cast<LPARAM>(text.data()));
            text.resize(wcsnlen_s(text.c_str(), text.size()));
            labels.push_back(std::move(text));
        }

        m_roadDepictionRoadLabels = NormaliseRoadDepictionRoadLabels(std::move(labels));
    }

    void ApplyRoadDepictionSelectionsFromList()
    {
        if (!m_roadDepictionsList)
            return;

        std::unordered_set<std::wstring> hidden;
        const int count = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETCOUNT, 0, 0));
        for (int i = 0; i < count; ++i) {
            const LRESULT selected = SendMessageW(m_roadDepictionsList, LB_GETSEL, i, 0);
            if (selected > 0)
                continue;

            const int textLen = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETTEXTLEN, i, 0));
            if (textLen < 0)
                continue;
            std::wstring text(static_cast<size_t>(textLen) + 1, L'\0');
            SendMessageW(m_roadDepictionsList, LB_GETTEXT, i, reinterpret_cast<LPARAM>(text.data()));
            text.resize(wcsnlen_s(text.c_str(), text.size()));
            hidden.insert(NormalizeRoadDepictionRoadLabel(text));
        }

        m_hiddenRoadDepictionIds = std::move(hidden);
        SyncRoadDepictionRoadLabelsFromList();
        m_map.SetHiddenRoadDepictions(m_hiddenRoadDepictionIds);
        SaveSettings();
    }

    void AddRoadDepictionRoadFromEdit()
    {
        std::wstring text = GetWindowTextString(m_roadDepictionsRoadEdit);
        std::replace(text.begin(), text.end(), L';', L',');
        std::replace(text.begin(), text.end(), L'\n', L',');
        std::replace(text.begin(), text.end(), L'\r', L',');

        std::vector<std::wstring> labels = m_roadDepictionRoadLabels;
        std::wstringstream parts(text);
        std::wstring part;
        size_t added = 0;
        while (std::getline(parts, part, L',')) {
            std::wstring label = NormalizeRoadDepictionRoadLabel(part);
            if (label.empty())
                continue;
            labels.push_back(label);
            m_hiddenRoadDepictionIds.erase(label);
            ++added;
        }

        if (added == 0) {
            SetStatusText(L"Enter a road number to add.");
            return;
        }

        m_roadDepictionRoadLabels = NormaliseRoadDepictionRoadLabels(std::move(labels));
        SetWindowTextSafe(m_roadDepictionsRoadEdit, L"");
        RenderRoadDepictionsList();
        SaveSettings();
        SetStatusText(added == 1 ? L"Road depiction added." : std::to_wstring(added) + L" road depictions added.");
    }

    void RemoveFocusedRoadDepictionRoad()
    {
        if (!m_roadDepictionsList)
            return;

        const int count = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETCOUNT, 0, 0));
        if (count <= 0)
            return;

        int index = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETCARETINDEX, 0, 0));
        if (index < 0 || index >= count) {
            index = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETCURSEL, 0, 0));
        }
        if (index < 0 || index >= count) {
            SetStatusText(L"Choose a road to remove.");
            return;
        }

        const int textLen = static_cast<int>(SendMessageW(m_roadDepictionsList, LB_GETTEXTLEN, index, 0));
        if (textLen < 0)
            return;
        std::wstring label(static_cast<size_t>(textLen) + 1, L'\0');
        SendMessageW(m_roadDepictionsList, LB_GETTEXT, index, reinterpret_cast<LPARAM>(label.data()));
        label.resize(wcsnlen_s(label.c_str(), label.size()));
        label = NormalizeRoadDepictionRoadLabel(label);
        if (label.empty())
            return;

        m_roadDepictionRoadLabels.erase(
            std::remove_if(
                m_roadDepictionRoadLabels.begin(),
                m_roadDepictionRoadLabels.end(),
                [&](const std::wstring& item) { return ToLower(NormalizeRoadDepictionRoadLabel(item)) == ToLower(label); }),
            m_roadDepictionRoadLabels.end());
        m_hiddenRoadDepictionIds.erase(label);
        RenderRoadDepictionsList();
        SaveSettings();
        SetStatusText(L"Road depiction removed: " + label);
    }

    void OnRoadDepictionsCommand(int id, int code)
    {
        if (id == IDC_ROAD_DEPICTIONS_ROAD_EDIT && code == EN_CHANGE)
            return;
        if (id == IDC_ROAD_DEPICTIONS_CLOSE_BTN && code == BN_CLICKED) {
            if (m_roadDepictionsWnd)
                ShowWindow(m_roadDepictionsWnd, SW_HIDE);
            return;
        }
        if (id == IDC_ROAD_DEPICTIONS_ADD_BTN && code == BN_CLICKED) {
            AddRoadDepictionRoadFromEdit();
            return;
        }
        if (id == IDC_ROAD_DEPICTIONS_REMOVE_BTN && code == BN_CLICKED) {
            RemoveFocusedRoadDepictionRoad();
            return;
        }
        if (id == IDC_ROAD_DEPICTIONS_ALL_BTN && code == BN_CLICKED) {
            if (m_roadDepictionsList)
                SendMessageW(m_roadDepictionsList, LB_SETSEL, TRUE, -1);
            ApplyRoadDepictionSelectionsFromList();
            return;
        }
        if (id == IDC_ROAD_DEPICTIONS_NONE_BTN && code == BN_CLICKED) {
            if (m_roadDepictionsList)
                SendMessageW(m_roadDepictionsList, LB_SETSEL, FALSE, -1);
            ApplyRoadDepictionSelectionsFromList();
            return;
        }
        if (id == IDC_ROAD_DEPICTIONS_LIST && code == LBN_SELCHANGE)
            ApplyRoadDepictionSelectionsFromList();
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
                500,
                380,
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
        constexpr int contentW = 420;
        HWND descLabel = CreateAutoLabel(
            parent,
            IDC_INCIDENT_FILTERS_DESC_LABEL,
            L"Choose which road incident categories should be available for filtering. These controls are ready for the next filtering step.",
            18,
            54,
            nullptr,
            contentW);
        const int descH = AutoLabelHeight(descLabel, 44, contentW);

        CreateAutoLabel(parent, IDC_INCIDENT_FILTERS_SEVERITY_LABEL, L"Severity", 18, 54 + descH + 18);
        const int severityY = 54 + descH + 46;
        m_incidentSevereCheck = CreateWindowExW(0, L"BUTTON", L"Severe", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, severityY, 120, 24, parent, ControlId(IDC_INCIDENT_FILTERS_SEVERE_CHECK), m_hInst, nullptr);
        m_incidentModerateCheck = CreateWindowExW(0, L"BUTTON", L"Moderate", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 148, severityY, 130, 24, parent, ControlId(IDC_INCIDENT_FILTERS_MODERATE_CHECK), m_hInst, nullptr);
        m_incidentMinorCheck = CreateWindowExW(0, L"BUTTON", L"Minor", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 288, severityY, 110, 24, parent, ControlId(IDC_INCIDENT_FILTERS_MINOR_CHECK), m_hInst, nullptr);
        m_incidentUnknownCheck = CreateWindowExW(0, L"BUTTON", L"Unknown", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, severityY + 32, 130, 24, parent, ControlId(IDC_INCIDENT_FILTERS_UNKNOWN_CHECK), m_hInst, nullptr);

        CreateAutoLabel(parent, IDC_INCIDENT_FILTERS_TYPE_LABEL, L"Incident type", 18, severityY + 84);
        const int typeY = severityY + 112;
        m_incidentUnplannedCheck = CreateWindowExW(0, L"BUTTON", L"Unplanned incidents", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, typeY, 180, 24, parent, ControlId(IDC_INCIDENT_FILTERS_UNPLANNED_CHECK), m_hInst, nullptr);
        m_incidentPlannedCheck = CreateWindowExW(0, L"BUTTON", L"Planned roadworks", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 236, typeY, 170, 24, parent, ControlId(IDC_INCIDENT_FILTERS_PLANNED_CHECK), m_hInst, nullptr);
        m_incidentSidePanelListOnlyCheck = CreateWindowExW(0, L"BUTTON", L"Side panel: only Incidents List", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, typeY + 38, 270, 24, parent, ControlId(IDC_INCIDENT_FILTERS_SIDE_PANEL_LIST_ONLY_CHECK), m_hInst, nullptr);
        m_incidentShowUnresolvedCheck = CreateWindowExW(0, L"BUTTON", L"Show unresolved incidents", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, typeY + 70, 270, 24, parent, ControlId(IDC_INCIDENT_FILTERS_SHOW_UNRESOLVED_CHECK), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 336, typeY + 108, 102, 32, parent, ControlId(IDC_INCIDENT_FILTERS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentSevereCheck, m_incidentModerateCheck, m_incidentMinorCheck, m_incidentUnknownCheck, m_incidentUnplannedCheck, m_incidentPlannedCheck, m_incidentSidePanelListOnlyCheck, m_incidentShowUnresolvedCheck, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SizeControlToText(m_incidentSevereCheck, 34, 6, 120, 0, 24);
        SizeControlToText(m_incidentModerateCheck, 34, 6, 130, 0, 24);
        SizeControlToText(m_incidentMinorCheck, 34, 6, 110, 0, 24);
        SizeControlToText(m_incidentUnknownCheck, 34, 6, 130, 0, 24);
        SizeControlToText(m_incidentUnplannedCheck, 34, 6, 180, 0, 24);
        SizeControlToText(m_incidentPlannedCheck, 34, 6, 170, 0, 24);
        SizeControlToText(m_incidentSidePanelListOnlyCheck, 34, 6, 270, 0, 24);
        SizeControlToText(m_incidentShowUnresolvedCheck, 34, 6, 270, 0, 24);
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
        if (m_incidentSidePanelListOnlyCheck)
            SendMessageW(m_incidentSidePanelListOnlyCheck, BM_SETCHECK, m_incidentSidePanelListOnly ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_incidentShowUnresolvedCheck)
            SendMessageW(m_incidentShowUnresolvedCheck, BM_SETCHECK, m_showUnresolvedIncidents ? BST_CHECKED : BST_UNCHECKED, 0);
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
            else if (id == IDC_INCIDENT_FILTERS_SIDE_PANEL_LIST_ONLY_CHECK) {
                m_incidentSidePanelListOnly = SendMessageW(m_incidentSidePanelListOnlyCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
                ApplyFilters(false);
            }
            else if (id == IDC_INCIDENT_FILTERS_SHOW_UNRESOLVED_CHECK) {
                m_showUnresolvedIncidents = SendMessageW(m_incidentShowUnresolvedCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
                ApplyFilters(false);
            }

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
                500,
                720,
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
        constexpr int contentW = 430;
        HWND descLabel = CreateAutoLabel(
            parent,
            IDC_INCIDENT_NOTIFICATIONS_DESC_LABEL,
            L"Define which incidents should trigger a notification. Roads and exclusions are comma separated; lane threshold accepts percentages such as 50%.",
            18,
            54,
            nullptr,
            contentW);
        const int descH = AutoLabelHeight(descLabel, 44, contentW);

        const int left = 18;
        const int editX = 18;
        const int editW = contentW;
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

        CreateAutoLabel(parent, IDC_INCIDENT_NOTIFICATIONS_UPDATE_LABEL, L"Incident updates", left, y);
        y += 26;
        m_incidentNotifyUpdatesRadio = CreateWindowExW(0, L"BUTTON", L"Notify incident updates", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, editX, y, 190, 24, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_UPDATE_NOTIFY_RADIO), m_hInst, nullptr);
        m_incidentIgnoreUpdatesRadio = CreateWindowExW(0, L"BUTTON", L"Ignore incident updates", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, editX + 210, y, 190, 24, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_UPDATE_IGNORE_RADIO), m_hInst, nullptr);
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

        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, editX + editW - 102, y + 42, 102, 32, parent, ControlId(IDC_INCIDENT_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentNotifyRoadsEdit, m_incidentNotifyRoadExclusionsEdit, m_incidentNotifyLaneThresholdEdit, m_incidentNotifyAndRadio, m_incidentNotifyOrRadio, m_incidentNotifyDelayThresholdEdit, m_incidentNotifyUpdatesRadio, m_incidentIgnoreUpdatesRadio, m_incidentNotifyRegionsBtn, m_incidentNotifyReasonExclusionsEdit, m_incidentNotifyLocationExclusionsEdit, close }) {
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
        if (m_incidentNotifyUpdatesRadio)
            SendMessageW(m_incidentNotifyUpdatesRadio, BM_SETCHECK, m_incidentIgnoreUpdates ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_incidentIgnoreUpdatesRadio)
            SendMessageW(m_incidentIgnoreUpdatesRadio, BM_SETCHECK, m_incidentIgnoreUpdates ? BST_CHECKED : BST_UNCHECKED, 0);
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
        if ((id == IDC_INCIDENT_NOTIFICATIONS_UPDATE_NOTIFY_RADIO || id == IDC_INCIDENT_NOTIFICATIONS_UPDATE_IGNORE_RADIO) && code == BN_CLICKED) {
            m_incidentIgnoreUpdates = (id == IDC_INCIDENT_NOTIFICATIONS_UPDATE_IGNORE_RADIO);
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

    static LRESULT CALLBACK TrafficScotlandWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleTrafficScotlandMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleTrafficScotlandMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateTrafficScotlandControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnTrafficScotlandCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowTrafficScotlandWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = TrafficScotlandWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kTrafficScotlandClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_trafficScotlandWnd || !IsWindow(m_trafficScotlandWnd)) {
            m_trafficScotlandWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kTrafficScotlandClassName,
                L"Traffic Scotland",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                560,
                270,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        SyncTrafficScotlandControls();
        ShowWindow(m_trafficScotlandWnd, SW_SHOW);
        SetForegroundWindow(m_trafficScotlandWnd);
    }

    void CreateTrafficScotlandControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Traffic Scotland", 18, 18, m_headerFont);
        CreateAutoLabel(
            parent,
            0,
            L"Adds incidents from Scotland's official live traffic service to the normal incident feed.",
            18,
            54,
            nullptr,
            500);
        m_trafficScotlandEnabledCheck = CreateWindowExW(
            0,
            L"BUTTON",
            L"Include Traffic Scotland incidents",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            18,
            100,
            300,
            24,
            parent,
            ControlId(IDC_TRAFFIC_SCOTLAND_ENABLED),
            m_hInst,
            nullptr);
        CreateAutoLabel(parent, 0, L"Incidents feed", 18, 132);
        m_trafficScotlandUrlEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            18,
            158,
            500,
            26,
            parent,
            ControlId(IDC_TRAFFIC_SCOTLAND_URL),
            m_hInst,
            nullptr);
        m_trafficScotlandStatusLabel = CreateAutoLabel(
            parent,
            IDC_TRAFFIC_SCOTLAND_STATUS,
            L"Official public incidents feed. Coordinates are resolved from each incident detail page.",
            18,
            194,
            nullptr,
            306);
        HWND refresh = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            344,
            198,
            82,
            32,
            parent,
            ControlId(IDC_TRAFFIC_SCOTLAND_REFRESH),
            m_hInst,
            nullptr);
        HWND close = CreateWindowExW(
            0,
            L"BUTTON",
            L"Close",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            436,
            198,
            82,
            32,
            parent,
            ControlId(IDC_TRAFFIC_SCOTLAND_CLOSE),
            m_hInst,
            nullptr);

        for (HWND control : { m_trafficScotlandEnabledCheck, m_trafficScotlandUrlEdit, refresh, close }) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(control);
        }
        SyncTrafficScotlandControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncTrafficScotlandControls()
    {
        if (m_trafficScotlandEnabledCheck)
            SendMessageW(m_trafficScotlandEnabledCheck, BM_SETCHECK, m_trafficScotlandEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_trafficScotlandUrlEdit)
            SetWindowTextSafe(m_trafficScotlandUrlEdit, m_trafficScotlandIncidentsUrl);
    }

    void OnTrafficScotlandCommand(int id, int code)
    {
        if (id == IDC_TRAFFIC_SCOTLAND_CLOSE && code == BN_CLICKED) {
            ShowWindow(m_trafficScotlandWnd, SW_HIDE);
            return;
        }
        if (id == IDC_TRAFFIC_SCOTLAND_REFRESH && code == BN_CLICKED) {
            m_trafficScotlandIncidentsUrl = NormalizeUrl(GetWindowTextString(m_trafficScotlandUrlEdit));
            SaveSettings();
            SetWindowTextSafe(m_trafficScotlandStatusLabel, L"Refreshing Traffic Scotland incidents...");
            RefreshFeedAsync();
            return;
        }
        if (id == IDC_TRAFFIC_SCOTLAND_ENABLED && code == BN_CLICKED) {
            m_trafficScotlandEnabled =
                SendMessageW(m_trafficScotlandEnabledCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SaveSettings();
            RefreshFeedAsync();
            return;
        }
        if (id == IDC_TRAFFIC_SCOTLAND_URL && code == EN_KILLFOCUS) {
            std::wstring value = NormalizeUrl(GetWindowTextString(m_trafficScotlandUrlEdit));
            if (!value.empty())
                m_trafficScotlandIncidentsUrl = std::move(value);
            SyncTrafficScotlandControls();
            SaveSettings();
        }
    }

    static LRESULT CALLBACK IncidentExclusionsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleIncidentExclusionsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleIncidentExclusionsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateIncidentExclusionsControls(hwnd);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_INCIDENT_EXCLUSIONS_CLOSE && HIWORD(wParam) == BN_CLICKED) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            if (LOWORD(wParam) == IDC_INCIDENT_EXCLUSIONS_REMOVE && HIWORD(wParam) == BN_CLICKED) {
                RemoveSelectedIncidentExclusion();
                return 0;
            }
            break;
        case WM_NOTIFY:
            if (reinterpret_cast<NMHDR*>(lParam)->hwndFrom == m_incidentExclusionsList &&
                reinterpret_cast<NMHDR*>(lParam)->code == NM_DBLCLK)
            {
                RemoveSelectedIncidentExclusion();
                return 0;
            }
            break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            return HandleModernCtlColor(msg, wParam);
        case WM_DRAWITEM:
            return OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void ShowIncidentExclusionsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = IncidentExclusionsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kIncidentExclusionsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }
        if (!m_incidentExclusionsWnd || !IsWindow(m_incidentExclusionsWnd)) {
            m_incidentExclusionsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kIncidentExclusionsClassName,
                L"Exclusions",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                520,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        RefreshIncidentExclusionsRows();
        ShowWindow(m_incidentExclusionsWnd, SW_SHOW);
        SetForegroundWindow(m_incidentExclusionsWnd);
    }

    void CreateIncidentExclusionsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Exclusions", 18, 18, m_headerFont);
        CreateAutoLabel(
            parent,
            0,
            L"Shared exclusions classify and highlight events for all online users; they do not affect notifications.",
            18,
            54,
            nullptr,
            820);
        m_incidentExclusionsList = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            18,
            88,
            846,
            334,
            parent,
            ControlId(IDC_INCIDENT_EXCLUSIONS_LIST),
            m_hInst,
            nullptr);
        ListView_SetExtendedListViewStyle(
            m_incidentExclusionsList,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
        const wchar_t* headings[] = { L"Scope", L"Event", L"Source", L"Added by", L"Added" };
        const int widths[] = { 90, 350, 125, 115, 145 };
        for (int i = 0; i < 5; ++i) {
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            column.pszText = const_cast<LPWSTR>(headings[i]);
            column.cx = widths[i];
            column.iSubItem = i;
            ListView_InsertColumn(m_incidentExclusionsList, i, &column);
        }
        HWND remove = CreateWindowExW(
            0,
            L"BUTTON",
            L"Remove selected",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            632,
            438,
            132,
            32,
            parent,
            ControlId(IDC_INCIDENT_EXCLUSIONS_REMOVE),
            m_hInst,
            nullptr);
        HWND close = CreateWindowExW(
            0,
            L"BUTTON",
            L"Close",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON,
            774,
            438,
            90,
            32,
            parent,
            ControlId(IDC_INCIDENT_EXCLUSIONS_CLOSE),
            m_hInst,
            nullptr);
        for (HWND control : { m_incidentExclusionsList, remove, close }) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(control);
        }
        ListView_SetTextColor(m_incidentExclusionsList, kUiText);
    }

    void RefreshIncidentExclusionsRows()
    {
        if (!m_incidentExclusionsList || !IsWindow(m_incidentExclusionsList))
            return;
        ListView_DeleteAllItems(m_incidentExclusionsList);
        for (size_t i = 0; i < m_incidentExclusions.size(); ++i) {
            const IncidentExclusion& exclusion = m_incidentExclusions[i];
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            const std::wstring scope = exclusion.road.empty() ? exclusion.source : exclusion.road;
            item.pszText = const_cast<LPWSTR>(scope.c_str());
            item.lParam = static_cast<LPARAM>(i);
            const int row = ListView_InsertItem(m_incidentExclusionsList, &item);
            ListView_SetItemText(m_incidentExclusionsList, row, 1, const_cast<LPWSTR>(exclusion.summary.c_str()));
            ListView_SetItemText(m_incidentExclusionsList, row, 2, const_cast<LPWSTR>(exclusion.source.c_str()));
            ListView_SetItemText(m_incidentExclusionsList, row, 3, const_cast<LPWSTR>(exclusion.addedBy.c_str()));
            ListView_SetItemText(m_incidentExclusionsList, row, 4, const_cast<LPWSTR>(exclusion.addedAt.c_str()));
        }
    }

    void RemoveSelectedIncidentExclusion()
    {
        if (!m_incidentExclusionsList)
            return;
        const int selected = ListView_GetNextItem(m_incidentExclusionsList, -1, LVNI_SELECTED);
        if (selected < 0)
            return;
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = selected;
        if (!ListView_GetItem(m_incidentExclusionsList, &item))
            return;
        const size_t index = static_cast<size_t>(item.lParam);
        if (index < m_incidentExclusions.size())
            RemoveIncidentExclusionAsync(m_incidentExclusions[index].key);
    }

    static LRESULT CALLBACK IncidentsListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleIncidentsListMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleIncidentsListMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateIncidentsListControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnIncidentsListCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_NOTIFY:
            return OnNotify(reinterpret_cast<NMHDR*>(lParam));
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

    void ShowIncidentsListWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = IncidentsListWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kIncidentsListClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_incidentsListWnd || !IsWindow(m_incidentsListWnd)) {
            m_incidentsListWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kIncidentsListClassName,
                L"Incidents List",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                820,
                520,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        RefreshIncidentsListRows();
        ShowWindow(m_incidentsListWnd, SW_SHOW);
        SetForegroundWindow(m_incidentsListWnd);
    }

    void CreateIncidentsListControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Incidents List", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Search", 18, 64);
        m_incidentsListSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 90, 360, 26, parent, ControlId(IDC_INCIDENTS_LIST_SEARCH_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Severity", 396, 64);
        m_incidentsListSeverityCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 396, 90, 160, 160, parent, ControlId(IDC_INCIDENTS_LIST_SEVERITY_COMBO), m_hInst, nullptr);
        m_incidentsListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 132, 756, 300, parent, ControlId(IDC_INCIDENTS_LIST_LISTVIEW), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 672, 446, 102, 32, parent, ControlId(IDC_INCIDENTS_LIST_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_incidentsListSearchEdit, m_incidentsListSeverityCombo, m_incidentsListView, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SendMessageW(m_incidentsListSearchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter by road, location, title, or update text"));
        for (const wchar_t* item : { L"All", L"Severe", L"Moderate", L"Minor", L"Unknown" })
            SendMessageW(m_incidentsListSeverityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        SendMessageW(m_incidentsListSeverityCombo, CB_SETCURSEL, 0, 0);

        SendMessageW(m_incidentsListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);
        ListView_SetBkColor(m_incidentsListView, kUiSurface);
        ListView_SetTextBkColor(m_incidentsListView, CLR_NONE);
        ListView_SetTextColor(m_incidentsListView, kUiText);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        std::wstring c0 = L"Severity";
        col.pszText = const_cast<LPWSTR>(c0.c_str());
        col.cx = 90;
        SendMessageW(m_incidentsListView, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&col));
        std::wstring c1 = L"Road";
        col.pszText = const_cast<LPWSTR>(c1.c_str());
        col.cx = 90;
        SendMessageW(m_incidentsListView, LVM_INSERTCOLUMNW, 1, reinterpret_cast<LPARAM>(&col));
        std::wstring c2 = L"Incident";
        col.pszText = const_cast<LPWSTR>(c2.c_str());
        col.cx = 360;
        SendMessageW(m_incidentsListView, LVM_INSERTCOLUMNW, 2, reinterpret_cast<LPARAM>(&col));
        std::wstring c3 = L"Updated";
        col.pszText = const_cast<LPWSTR>(c3.c_str());
        col.cx = 170;
        SendMessageW(m_incidentsListView, LVM_INSERTCOLUMNW, 3, reinterpret_cast<LPARAM>(&col));

        ApplyModernEditChrome(m_incidentsListSearchEdit);
        RefreshIncidentsListRows();
        AutoFitWindowToChildren(parent);
    }

    std::wstring IncidentsListSeverityFilter() const
    {
        int idx = m_incidentsListSeverityCombo
            ? static_cast<int>(SendMessageW(m_incidentsListSeverityCombo, CB_GETCURSEL, 0, 0))
            : 0;
        switch (idx) {
        case 1: return L"severe";
        case 2: return L"moderate";
        case 3: return L"minor";
        case 4: return L"unknown";
        default: return L"";
        }
    }

    bool IncidentStateMatchesListFilters(const IncidentNotificationState& state) const
    {
        std::wstring severityFilter = IncidentsListSeverityFilter();
        if (!severityFilter.empty() && SeverityBucket(state.alert.severity) != severityFilter)
            return false;

        std::wstring search = m_incidentsListSearchEdit ? ToLower(Trim(GetWindowTextString(m_incidentsListSearchEdit))) : L"";
        if (search.empty())
            return true;

        std::wstring haystack = ToLower(
            state.line + L" " +
            state.alert.road + L" " +
            state.alert.region + L" " +
            state.alert.title + L" " +
            state.alert.description + L" " +
            state.alert.updatedText + L" " +
            BuildSeverityDisplay(state.alert.severity));
        return haystack.find(search) != std::wstring::npos;
    }

    void RefreshIncidentsListRows()
    {
        if (!m_incidentsListView)
            return;

        std::vector<std::wstring> keys;
        keys.reserve(m_notifiedIncidentStates.size());
        for (const auto& item : m_notifiedIncidentStates) {
            if (IncidentStateMatchesListFilters(item.second))
                keys.push_back(item.first);
        }
        std::sort(keys.begin(), keys.end(), [this](const std::wstring& a, const std::wstring& b) {
            const auto ia = m_notifiedIncidentStates.find(a);
            const auto ib = m_notifiedIncidentStates.find(b);
            if (ia == m_notifiedIncidentStates.end() || ib == m_notifiedIncidentStates.end())
                return a < b;
            int sa = (SeverityBucket(ia->second.alert.severity) == L"severe") ? 0 : (SeverityBucket(ia->second.alert.severity) == L"moderate" ? 1 : (SeverityBucket(ia->second.alert.severity) == L"minor" ? 2 : 3));
            int sb = (SeverityBucket(ib->second.alert.severity) == L"severe") ? 0 : (SeverityBucket(ib->second.alert.severity) == L"moderate" ? 1 : (SeverityBucket(ib->second.alert.severity) == L"minor" ? 2 : 3));
            if (sa != sb)
                return sa < sb;
            if (ia->second.alert.road != ib->second.alert.road)
                return ia->second.alert.road < ib->second.alert.road;
            return ia->second.line < ib->second.line;
            });

        m_incidentsListKeys = keys;
        SendMessageW(m_incidentsListView, LVM_DELETEALLITEMS, 0, 0);
        for (size_t i = 0; i < m_incidentsListKeys.size(); ++i) {
            const IncidentNotificationState& state = m_notifiedIncidentStates[m_incidentsListKeys[i]];
            std::wstring sev = BuildSeverityDisplay(state.alert.severity);
            std::wstring road = state.alert.road.empty() ? state.alert.region : state.alert.road;
            std::wstring incident = state.line.empty() ? BuildAlertSummary(state.alert) : state.line;
            std::wstring updated = state.alert.updatedText;

            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(sev.c_str());
            int row = static_cast<int>(SendMessageW(m_incidentsListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (row >= 0) {
                LVITEMW sub{};
                sub.iSubItem = 1;
                sub.pszText = const_cast<LPWSTR>(road.c_str());
                SendMessageW(m_incidentsListView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));
                sub.iSubItem = 2;
                sub.pszText = const_cast<LPWSTR>(incident.c_str());
                SendMessageW(m_incidentsListView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));
                sub.iSubItem = 3;
                sub.pszText = const_cast<LPWSTR>(updated.c_str());
                SendMessageW(m_incidentsListView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));
            }
        }

        if (m_incidentsListWnd && IsWindowVisible(m_incidentsListWnd)) {
            SetStatusText(L"Showing " + std::to_wstring(m_incidentsListKeys.size()) + L" notified incident(s).");
        }
    }

    void OnIncidentsListCommand(int id, int code)
    {
        if (id == IDC_INCIDENTS_LIST_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_incidentsListWnd, SW_HIDE);
            return;
        }
        if ((id == IDC_INCIDENTS_LIST_SEARCH_EDIT && code == EN_CHANGE) ||
            (id == IDC_INCIDENTS_LIST_SEVERITY_COMBO && code == CBN_SELCHANGE))
        {
            RefreshIncidentsListRows();
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
        m_notificationRegionsShowPolygonsCheck = CreateWindowExW(0, L"BUTTON", L"Show region polygons on map", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, 100, 260, 24, parent, ControlId(IDC_NOTIFICATION_REGIONS_SHOW_POLYGONS_CHECK), m_hInst, nullptr);
        SendMessageW(m_notificationRegionsShowPolygonsCheck, BM_SETCHECK, m_showIncidentNotificationRegionPolygons ? BST_CHECKED : BST_UNCHECKED, 0);
        m_notificationRegionsList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 134, 376, 150, parent, ControlId(IDC_NOTIFICATION_REGIONS_LIST), m_hInst, nullptr);
        HWND addBtn = CreateWindowExW(0, L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 302, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_NEW_BTN), m_hInst, nullptr);
        HWND editBtn = CreateWindowExW(0, L"BUTTON", L"Edit", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 108, 302, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_EDIT_BTN), m_hInst, nullptr);
        HWND deleteBtn = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 198, 302, 82, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_DELETE_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 292, 302, 102, 32, parent, ControlId(IDC_NOTIFICATION_REGIONS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_notificationRegionsShowPolygonsCheck, m_notificationRegionsList, addBtn, editBtn, deleteBtn, closeBtn }) {
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
        if (id == IDC_NOTIFICATION_REGIONS_SHOW_POLYGONS_CHECK && code == BN_CLICKED) {
            m_showIncidentNotificationRegionPolygons =
                SendMessageW(m_notificationRegionsShowPolygonsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_map.SetNotificationPolygonsVisible(m_showIncidentNotificationRegionPolygons);
            UpdateViewMenu();
            SaveSettings();
            return;
        }
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

    const WeatherWarningEvent* FindSelectedWeatherWarning() const
    {
        if (m_weatherWarningsListView) {
            int selected = static_cast<int>(SendMessageW(m_weatherWarningsListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredWeatherWarnings.size()))
                return &m_filteredWeatherWarnings[static_cast<size_t>(selected)];
        }

        if (!m_filteredWeatherWarnings.empty())
            return &m_filteredWeatherWarnings.front();
        if (!m_allWeatherWarnings.empty())
            return &m_allWeatherWarnings.front();
        return nullptr;
    }

    const FloodEvent* FindSelectedFlood() const
    {
        if (m_floodsListView) {
            int selected = static_cast<int>(SendMessageW(m_floodsListView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
            if (selected >= 0 && selected < static_cast<int>(m_filteredFloods.size()))
                return &m_filteredFloods[static_cast<size_t>(selected)];
        }

        if (!m_filteredFloods.empty())
            return &m_filteredFloods.front();
        if (!m_allFloods.empty())
            return &m_allFloods.front();
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

    static std::wstring ShortTrafficTitle(const std::wstring& title)
    {
        std::wstring key = ToLower(CompactTemplateWhitespace(title));
        if (key.find(L"road traffic collision") != std::wstring::npos)
            return L"RTC";
        if (key.find(L"road traffic incident") != std::wstring::npos)
            return L"RTI";
        return title;
    }

    std::vector<std::pair<std::wstring, std::wstring>> BuildTemplateVariables(const TrafficAlert& alert) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        std::wstring road = alert.road.empty() ? alert.region : alert.road;
        std::wstring location = AlertLocationForNotification(alert);
        std::wstring direction = ExtractDirectionFromLocation(location);
        std::vector<JunctionTemplateData> junctions = ExtractJunctionsFromLocation(location);

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        std::wstring title = alert.title.empty() ? AlertReasonForNotification(alert) : alert.title;
        SetTemplateVariable(variables, L"$TITLE", title);
        SetTemplateVariable(variables, L"$TITLE_SHORT", ShortTrafficTitle(title));
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
        std::wstring junctionRangeText = JoinTemplateItemsAsPhrase(junctionNames);
        std::wstring junctionsWithDataText = JoinTemplateItemsAsPhrase(junctionsWithData);
        if (!junctionsWithDataText.empty()) {
            std::wstring lowerLocation = ToLower(location);
            if (lowerLocation.find(L"between") != std::wstring::npos) {
                junctionsWithDataText = L"between " + junctionsWithDataText;
                if (!junctionRangeText.empty())
                    junctionRangeText = L"between " + junctionRangeText;
            }
            else if (lowerLocation.find(L" at ") != std::wstring::npos || StartsWithNoCase(lowerLocation, L"at ")) {
                junctionsWithDataText = L"at " + junctionsWithDataText;
                if (!junctionRangeText.empty())
                    junctionRangeText = L"at " + junctionRangeText;
            }
        }
        SetTemplateVariable(variables, L"%JUNCTION_RANGE", junctionRangeText);
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

    static std::wstring ExpandCompassDirectionToken(const std::wstring& token)
    {
        static const std::unordered_map<std::wstring, std::wstring> names = {
            { L"N", L"North" },
            { L"NNE", L"North-North-East" },
            { L"NE", L"North-East" },
            { L"ENE", L"East-North-East" },
            { L"E", L"East" },
            { L"ESE", L"East-South-East" },
            { L"SE", L"South-East" },
            { L"SSE", L"South-South-East" },
            { L"S", L"South" },
            { L"SSW", L"South-South-West" },
            { L"SW", L"South-West" },
            { L"WSW", L"West-South-West" },
            { L"W", L"West" },
            { L"WNW", L"West-North-West" },
            { L"NW", L"North-West" },
            { L"NNW", L"North-North-West" }
        };
        auto it = names.find(token);
        return it == names.end() ? token : it->second;
    }

    static std::wstring ExpandCompassDirectionsInText(const std::wstring& text)
    {
        static const std::wregex compassRe(LR"(\b(N|NNE|NE|ENE|E|ESE|SE|SSE|S|SSW|SW|WSW|W|WNW|NW|NNW)\b)");
        std::wstring output;
        size_t cursor = 0;
        for (std::wsregex_iterator it(text.begin(), text.end(), compassRe), end; it != end; ++it) {
            const auto& match = *it;
            const size_t pos = static_cast<size_t>(match.position(0));
            output.append(text, cursor, pos - cursor);
            output += ExpandCompassDirectionToken(match[1].str());
            cursor = pos + static_cast<size_t>(match.length(0));
        }
        output.append(text, cursor, std::wstring::npos);
        return output;
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
        SetTemplateVariable(variables, L"$PLACE", event.place.empty() ? L"unknown region" : ExpandCompassDirectionsInText(event.place));
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

    std::vector<std::pair<std::wstring, std::wstring>> BuildWeatherWarningTemplateVariables(const WeatherWarningEvent& warning) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        wchar_t latitude[48]{};
        wchar_t longitude[48]{};
        if (warning.hasLocation) {
            swprintf_s(latitude, L"%.5f", warning.latitude);
            swprintf_s(longitude, L"%.5f", warning.longitude);
        }

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        SetTemplateVariable(variables, L"$COLOUR", warning.colour.empty() ? L"Weather" : warning.colour);
        SetTemplateVariable(variables, L"$TYPE", warning.type.empty() ? L"warning" : warning.type);
        SetTemplateVariable(variables, L"$AREA", warning.area.empty() ? L"affected area" : warning.area);
        SetTemplateVariable(variables, L"$FROM", warning.validFrom.empty() ? L"unknown" : warning.validFrom);
        SetTemplateVariable(variables, L"$TO", warning.validTo.empty() ? L"unknown" : warning.validTo);
        SetTemplateVariable(variables, L"$ISSUED", warning.issuedText);
        SetTemplateVariable(variables, L"$HEADLINE", warning.headline);
        SetTemplateVariable(variables, L"$DETAIL", CompactTemplateWhitespace(warning.detail));
        SetTemplateVariable(variables, L"%LATITUDE", latitude);
        SetTemplateVariable(variables, L"%LONGITUDE", longitude);
        return variables;
    }

    std::vector<std::pair<std::wstring, std::wstring>> BuildFloodTemplateVariables(const FloodEvent& flood) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables;
        wchar_t latitude[48]{};
        wchar_t longitude[48]{};
        if (flood.hasLocation) {
            swprintf_s(latitude, L"%.5f", flood.latitude);
            swprintf_s(longitude, L"%.5f", flood.longitude);
        }

        SetTemplateVariable(variables, L"$DATE", CurrentDateText());
        SetTemplateVariable(variables, L"$SEVERITY", flood.severity.empty() ? L"Flood alert" : flood.severity);
        SetTemplateVariable(variables, L"$AREA", flood.area.empty() ? L"affected area" : flood.area);
        SetTemplateVariable(variables, L"$REGION", flood.region);
        SetTemplateVariable(variables, L"$RIVER_OR_SEA", flood.riverOrSea);
        SetTemplateVariable(variables, L"$MESSAGE", CompactTemplateWhitespace(flood.message));
        SetTemplateVariable(variables, L"$RAISED", flood.timeRaised);
        SetTemplateVariable(variables, L"$UPDATED", flood.timeChanged);
        SetTemplateVariable(variables, L"%LATITUDE", latitude);
        SetTemplateVariable(variables, L"%LONGITUDE", longitude);
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

    static std::wstring ToUpperText(std::wstring text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch) {
            return static_cast<wchar_t>(towupper(ch));
            });
        return text;
    }

    std::wstring RenderTemplateText(std::wstring output, bool fixArticles, bool upperCase) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables = m_templateWizardVariables;
        std::sort(variables.begin(), variables.end(), [](const auto& a, const auto& b) {
            return a.first.size() > b.first.size();
            });
        for (const auto& item : variables)
            ReplaceAllText(output, item.first, item.second);
        if (upperCase)
            output = CompactTemplateWhitespace(output);
        if (fixArticles)
            output = FixIndefiniteArticles(output);
        return upperCase ? ToUpperText(output) : output;
    }

    std::vector<std::wstring> TemplateVariableValuesInOrder(const std::wstring& templateText, bool upperCase) const
    {
        std::vector<std::pair<std::wstring, std::wstring>> variables = m_templateWizardVariables;
        std::sort(variables.begin(), variables.end(), [](const auto& a, const auto& b) {
            return a.first.size() > b.first.size();
            });

        std::vector<std::wstring> values;
        size_t pos = 0;
        while (pos < templateText.size()) {
            const auto it = std::find_if(variables.begin(), variables.end(), [&](const auto& item) {
                return !item.first.empty() &&
                    pos + item.first.size() <= templateText.size() &&
                    templateText.compare(pos, item.first.size(), item.first) == 0;
                });
            if (it == variables.end()) {
                ++pos;
                continue;
            }

            std::wstring value = upperCase ? ToUpperText(CompactTemplateWhitespace(it->second)) : it->second;
            if (!value.empty())
                values.push_back(std::move(value));
            pos += it->first.size();
        }
        return values;
    }

    TemplateRenderedText RenderTemplateTextWithEditableValues(const std::wstring& templateText, bool fixArticles, bool upperCase) const
    {
        TemplateRenderedText rendered;
        rendered.text = RenderTemplateText(templateText, fixArticles, upperCase);

        size_t cursor = 0;
        for (const std::wstring& value : TemplateVariableValuesInOrder(templateText, upperCase)) {
            size_t pos = rendered.text.find(value, cursor);
            if (pos == std::wstring::npos)
                pos = rendered.text.find(value);
            if (pos == std::wstring::npos)
                continue;

            rendered.editableRanges.push_back({
                static_cast<LONG>(pos),
                static_cast<LONG>(pos + value.size())
                });
            cursor = pos + value.size();
        }
        return rendered;
    }

    std::wstring RenderReportTemplateTitle(const ReportTemplate& reportTemplate) const
    {
        std::wstring titleTemplate = Trim(reportTemplate.title).empty()
            ? DefaultTemplateTitleForContext(m_templateWizardContext)
            : reportTemplate.title;
        return RenderTemplateText(titleTemplate, false, true);
    }

    TemplateRenderedText RenderReportTemplateTitleWithEditableValues(const ReportTemplate& reportTemplate) const
    {
        std::wstring titleTemplate = Trim(reportTemplate.title).empty()
            ? DefaultTemplateTitleForContext(m_templateWizardContext)
            : reportTemplate.title;
        return RenderTemplateTextWithEditableValues(titleTemplate, false, true);
    }

    std::wstring RenderReportTemplate(const ReportTemplate& reportTemplate) const
    {
        return RenderTemplateText(reportTemplate.body, true, false);
    }

    TemplateRenderedText RenderReportTemplateWithEditableValues(const ReportTemplate& reportTemplate) const
    {
        return RenderTemplateTextWithEditableValues(reportTemplate.body, true, false);
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
        case WM_NOTIFY:
            return OnTemplatesWizardNotify(reinterpret_cast<NMHDR*>(lParam));
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

    LRESULT OnTemplatesWizardNotify(NMHDR* nmh)
    {
        if (!nmh)
            return 0;
        if ((nmh->hwndFrom == m_templateWizardTitlePreviewEdit ||
            nmh->hwndFrom == m_templateWizardPreviewEdit) &&
            nmh->code == EN_PROTECTED)
        {
            const ENPROTECTED* info = reinterpret_cast<const ENPROTECTED*>(nmh);
            const std::vector<TemplateEditableRange>& ranges =
                nmh->hwndFrom == m_templateWizardTitlePreviewEdit
                ? m_templateWizardTitleEditableRanges
                : m_templateWizardBodyEditableRanges;
            LONG start = info->chrg.cpMin;
            LONG end = info->chrg.cpMax;
            if (end < start)
                std::swap(start, end);

            for (const TemplateEditableRange& range : ranges) {
                const bool insertion = start == end;
                if ((insertion && start >= range.start && start <= range.end) ||
                    (!insertion && start >= range.start && end <= range.end))
                {
                    return 0;
                }
            }
            return 1;
        }
        return 0;
    }

    void ClearTemplateWizardSourceIds()
    {
        m_templateWizardAlertId.clear();
        m_templateWizardEarthquakeId.clear();
        m_templateWizardWeatherSystemId.clear();
        m_templateWizardWeatherWarningId.clear();
        m_templateWizardFloodId.clear();
    }

    bool TemplateWizardWeatherKindStep() const
    {
        return m_templateWizardWeatherChooser && m_templateWizardStep == 0;
    }

    size_t TemplateWizardTemplateChoiceStepIndex() const
    {
        return m_templateWizardWeatherChooser ? 1 : 0;
    }

    size_t TemplateWizardVariablesStepIndex() const
    {
        return m_templateWizardWeatherChooser ? 2 : 1;
    }

    size_t TemplateWizardReviewStepIndex() const
    {
        return m_templateWizardWeatherChooser ? 3 : 2;
    }

    bool TemplateWizardTemplateChoiceStep() const
    {
        return m_templateWizardStep == TemplateWizardTemplateChoiceStepIndex();
    }

    bool TemplateWizardVariablesStep() const
    {
        return m_templateWizardStep == TemplateWizardVariablesStepIndex();
    }

    bool TemplateWizardReviewStep() const
    {
        return m_templateWizardStep == TemplateWizardReviewStepIndex();
    }

    static TemplateContext WeatherTemplateContextFromIndex(int index)
    {
        if (index == 1)
            return TemplateContext::WeatherWarnings;
        if (index == 2)
            return TemplateContext::Floods;
        return TemplateContext::WeatherSystems;
    }

    bool PrepareWeatherTemplateWizardForContext(TemplateContext context)
    {
        m_templateWizardContext = context;
        EnsureDefaultTemplatesForContext(context);
        if (TemplatesForContext(context).empty()) {
            MessageBoxW(m_hwnd, L"No templates are configured. Open Weather > Edit Templates to add one.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
            return false;
        }

        ClearTemplateWizardSourceIds();
        if (context == TemplateContext::WeatherSystems) {
            const WeatherSystemEvent* system = FindSelectedWeatherSystem();
            if (!system) {
                MessageBoxW(m_hwnd, L"Select a weather system in the Weather Systems List first, then continue the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
                return false;
            }
            m_templateWizardWeatherSystemId = WeatherSystemStableKey(*system);
            m_templateWizardVariables = BuildWeatherSystemTemplateVariables(*system);
        }
        else if (context == TemplateContext::WeatherWarnings) {
            const WeatherWarningEvent* warning = FindSelectedWeatherWarning();
            if (!warning) {
                MessageBoxW(m_hwnd, L"Select a weather warning in the Weather Warnings list first, then continue the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
                return false;
            }
            m_templateWizardWeatherWarningId = WeatherWarningStableKey(*warning);
            m_templateWizardVariables = BuildWeatherWarningTemplateVariables(*warning);
        }
        else if (context == TemplateContext::Floods) {
            const FloodEvent* flood = FindSelectedFlood();
            if (!flood) {
                MessageBoxW(m_hwnd, L"Select a flood in the Floods list first, then continue the Templates Wizard.", L"Templates Wizard", MB_OK | MB_ICONINFORMATION);
                return false;
            }
            m_templateWizardFloodId = FloodStableKey(*flood);
            m_templateWizardVariables = BuildFloodTemplateVariables(*flood);
        }
        else {
            return false;
        }

        m_templateWizardTemplateIndex = 0;
        return true;
    }

    void ShowTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::Roads;
        m_templateWizardWeatherChooser = false;
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
        m_templateWizardWeatherWarningId.clear();
        m_templateWizardFloodId.clear();
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables = BuildTemplateVariables(*alert);

        ShowTemplatesWizardWindowShell(L"Road Templates Wizard");
    }

    void ShowEarthquakeTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::Earthquakes;
        m_templateWizardWeatherChooser = false;
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
        m_templateWizardWeatherWarningId.clear();
        m_templateWizardFloodId.clear();
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables = BuildEarthquakeTemplateVariables(*event);

        ShowTemplatesWizardWindowShell(L"Earthquake Templates Wizard");
    }

    void ShowWeatherSystemsTemplatesWizardWindow()
    {
        m_templateWizardContext = TemplateContext::WeatherSystems;
        m_templateWizardWeatherChooser = true;
        ClearTemplateWizardSourceIds();
        m_templateWizardStep = 0;
        m_templateWizardTemplateIndex = 0;
        m_templateWizardVariables.clear();

        ShowTemplatesWizardWindowShell(L"Weather Templates Wizard");
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
                820,
                560,
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
        m_templateWizardTitleEditableRanges.clear();
        m_templateWizardBodyEditableRanges.clear();
        SetProtectedTemplateText(m_templateWizardTitlePreviewEdit, {});
        SetProtectedTemplateText(m_templateWizardPreviewEdit, {});
        RenderTemplatesWizardStep();
        ShowWindow(m_templatesWizardWnd, SW_SHOW);
        SetForegroundWindow(m_templatesWizardWnd);
    }

    void CreateTemplatesWizardControls(HWND parent)
    {
        const wchar_t* previewClass = EnsureRichEditLoaded() ? MSFTEDIT_CLASS : L"EDIT";
        CreateAutoLabel(parent, IDC_TEMPLATES_WIZARD_TITLE, L"Templates Wizard", 18, 18, m_headerFont);
        m_templateWizardDesc = CreateAutoLabel(parent, IDC_TEMPLATES_WIZARD_DESC, L"", 18, 54, nullptr, 684);
        m_templateWizardList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 92, 684, 300, parent, ControlId(IDC_TEMPLATES_WIZARD_LIST), m_hInst, nullptr);
        m_templateWizardVariablesEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 18, 92, 684, 300, parent, ControlId(IDC_TEMPLATES_WIZARD_VARIABLES), m_hInst, nullptr);
        m_templateWizardTitlePreviewLabel = CreateAutoLabel(parent, 0, L"Title", 18, 92);
        m_templateWizardTitlePreviewEdit = CreateWindowExW(WS_EX_CLIENTEDGE, previewClass, L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NOHIDESEL, 18, 118, 684, 26, parent, ControlId(IDC_TEMPLATES_WIZARD_TITLE_PREVIEW), m_hInst, nullptr);
        m_templateWizardBodyPreviewLabel = CreateAutoLabel(parent, 0, L"Template", 18, 158);
        m_templateWizardPreviewEdit = CreateWindowExW(WS_EX_CLIENTEDGE, previewClass, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL | WS_VSCROLL, 18, 184, 684, 208, parent, ControlId(IDC_TEMPLATES_WIZARD_PREVIEW), m_hInst, nullptr);
        m_templateWizardPrevBtn = CreateWindowExW(0, L"BUTTON", L"Previous", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 122, 432, 102, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_PREV), m_hInst, nullptr);
        m_templateWizardNextBtn = CreateWindowExW(0, L"BUTTON", L"Next", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 232, 432, 102, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_NEXT), m_hInst, nullptr);
        m_templateWizardCopyTitleBtn = CreateWindowExW(0, L"BUTTON", L"Copy Title", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 342, 432, 104, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_COPY_TITLE), m_hInst, nullptr);
        m_templateWizardCopyBtn = CreateWindowExW(0, L"BUTTON", L"Copy Template", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 454, 432, 124, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_COPY), m_hInst, nullptr);
        m_templateWizardCopyLocationBtn = CreateWindowExW(0, L"BUTTON", L"Copy Coords", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 586, 432, 118, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_COPY_LOCATION), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 712, 432, 68, 32, parent, ControlId(IDC_TEMPLATES_WIZARD_CLOSE), m_hInst, nullptr);

        for (HWND h : { m_templateWizardDesc, m_templateWizardList, m_templateWizardVariablesEdit, m_templateWizardTitlePreviewLabel, m_templateWizardTitlePreviewEdit, m_templateWizardBodyPreviewLabel, m_templateWizardPreviewEdit, m_templateWizardPrevBtn, m_templateWizardNextBtn, m_templateWizardCopyTitleBtn, m_templateWizardCopyBtn, m_templateWizardCopyLocationBtn, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        EnableNativeSpellCheck(m_templateWizardTitlePreviewEdit);
        EnableNativeSpellCheck(m_templateWizardPreviewEdit);

        PopulateTemplatesWizardList();
        RenderTemplatesWizardStep();
        AutoFitWindowToChildren(parent);
    }

    void PopulateTemplatesWizardList()
    {
        if (!m_templateWizardList)
            return;
        SendMessageW(m_templateWizardList, LB_RESETCONTENT, 0, 0);
        if (TemplateWizardWeatherKindStep()) {
            SendMessageW(m_templateWizardList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weather Systems"));
            SendMessageW(m_templateWizardList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weather Warnings"));
            SendMessageW(m_templateWizardList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Floods"));
            SendMessageW(m_templateWizardList, LB_SETCURSEL, 0, 0);
            return;
        }

        const auto& templates = TemplatesForContext(m_templateWizardContext);
        for (const ReportTemplate& reportTemplate : templates)
            SendMessageW(m_templateWizardList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(reportTemplate.name.c_str()));
        if (!templates.empty()) {
            m_templateWizardTemplateIndex = MinValue<size_t>(m_templateWizardTemplateIndex, templates.size() - 1);
            SendMessageW(m_templateWizardList, LB_SETCURSEL, static_cast<WPARAM>(m_templateWizardTemplateIndex), 0);
        }
    }

    void RefreshTemplatesWizardPreview()
    {
        const auto& templates = TemplatesForContext(m_templateWizardContext);
        if (templates.empty())
            return;

        m_templateWizardTemplateIndex = MinValue<size_t>(m_templateWizardTemplateIndex, templates.size() - 1);
        SetProtectedTemplateText(m_templateWizardTitlePreviewEdit, RenderReportTemplateTitleWithEditableValues(templates[m_templateWizardTemplateIndex]));
        SetProtectedTemplateText(m_templateWizardPreviewEdit, RenderReportTemplateWithEditableValues(templates[m_templateWizardTemplateIndex]));
    }

    static bool IsRichEditControl(HWND hwnd)
    {
        wchar_t className[64]{};
        if (!GetClassNameW(hwnd, className, static_cast<int>(_countof(className))))
            return false;
        std::wstring name = ToLower(className);
        return name.find(L"richedit") != std::wstring::npos;
    }

    static void SetRichEditSelection(HWND hwnd, LONG start, LONG end)
    {
        CHARRANGE range{ start, end };
        SendMessageW(hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    }

    void SetProtectedTemplateText(HWND hwnd, const TemplateRenderedText& rendered)
    {
        if (!hwnd)
            return;

        if (hwnd == m_templateWizardTitlePreviewEdit)
            m_templateWizardTitleEditableRanges = rendered.editableRanges;
        else if (hwnd == m_templateWizardPreviewEdit)
            m_templateWizardBodyEditableRanges = rendered.editableRanges;

        const bool richEdit = IsRichEditControl(hwnd);
        DWORD mask = 0;
        if (richEdit) {
            mask = static_cast<DWORD>(SendMessageW(hwnd, EM_GETEVENTMASK, 0, 0));
            SendMessageW(hwnd, EM_SETEVENTMASK, 0, 0);
            SendMessageW(hwnd, EM_SETREADONLY, FALSE, 0);
        }

        SetWindowTextSafe(hwnd, rendered.text);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        if (!richEdit)
            return;

        SendMessageW(hwnd, EM_SETBKGNDCOLOR, 0, kUiSurface);

        CHARFORMAT2W format{};
        format.cbSize = sizeof(format);
        format.dwMask = CFM_PROTECTED | CFM_COLOR;
        format.dwEffects = CFE_PROTECTED;
        format.crTextColor = kUiText;
        SendMessageW(hwnd, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));

        for (const TemplateEditableRange& range : rendered.editableRanges) {
            if (range.end <= range.start)
                continue;
            format.dwEffects = 0;
            format.crTextColor = RGB(0, 72, 145);
            SetRichEditSelection(hwnd, range.start, range.end);
            SendMessageW(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
        }

        if (!rendered.editableRanges.empty())
            SetRichEditSelection(hwnd, rendered.editableRanges.front().start, rendered.editableRanges.front().end);
        else
            SetRichEditSelection(hwnd, 0, 0);
        SendMessageW(hwnd, EM_SETEVENTMASK, 0, mask | ENM_PROTECTED);
    }

    void RenderTemplatesWizardStep()
    {
        if (!m_templatesWizardWnd)
            return;

        const bool weatherKindStep = TemplateWizardWeatherKindStep();
        const bool chooseStep = TemplateWizardTemplateChoiceStep();
        const bool variableStep = TemplateWizardVariablesStep();
        const bool reviewStep = TemplateWizardReviewStep();

        if (weatherKindStep) {
            SetWindowTextSafe(m_templateWizardDesc, L"Choose which weather product this template should cover.");
        }
        else if (chooseStep) {
            const wchar_t* description = L"Choose the report template to use for the selected incident.";
            if (m_templateWizardContext == TemplateContext::Earthquakes)
                description = L"Choose the report template to use for the selected earthquake.";
            else if (m_templateWizardContext == TemplateContext::WeatherSystems)
                description = L"Choose the report template to use for the selected weather system.";
            else if (m_templateWizardContext == TemplateContext::WeatherWarnings)
                description = L"Choose the report template to use for the selected weather warning.";
            else if (m_templateWizardContext == TemplateContext::Floods)
                description = L"Choose the report template to use for the selected flood.";
            SetWindowTextSafe(m_templateWizardDesc, description);
        }
        else if (variableStep)
            SetWindowTextSafe(m_templateWizardDesc, L"Review the generated variables. They are locked here so the source data remains traceable.");
        else {
            SetWindowTextSafe(m_templateWizardDesc, L"Edit the highlighted generated values in the title and template, then copy whichever section you need.");
            RefreshTemplatesWizardPreview();
        }

        constexpr int left = 18;
        constexpr int contentW = 684;
        MoveLabelToText(m_templateWizardDesc, left, 54, contentW);
        const int descH = AutoLabelHeight(m_templateWizardDesc, 22, contentW);
        const int contentTop = 54 + descH + 14;
        int contentBottom = contentTop;

        MoveWindow(m_templateWizardList, left, contentTop, contentW, 300, TRUE);
        if (reviewStep) {
            MoveWindow(m_templateWizardTitlePreviewLabel, left, contentTop, 160, 22, TRUE);
            MoveWindow(m_templateWizardTitlePreviewEdit, left, contentTop + 26, contentW, 26, TRUE);
            MoveWindow(m_templateWizardBodyPreviewLabel, left, contentTop + 66, 160, 22, TRUE);
            MoveWindow(m_templateWizardPreviewEdit, left, contentTop + 92, contentW, 208, TRUE);
            contentBottom = contentTop + 300;
        }
        else {
            MoveWindow(m_templateWizardVariablesEdit, left, contentTop, contentW, 300, TRUE);
            MoveWindow(m_templateWizardTitlePreviewLabel, left, contentTop, 160, 22, TRUE);
            MoveWindow(m_templateWizardTitlePreviewEdit, left, contentTop + 26, contentW, 26, TRUE);
            MoveWindow(m_templateWizardBodyPreviewLabel, left, contentTop + 66, 160, 22, TRUE);
            MoveWindow(m_templateWizardPreviewEdit, left, contentTop + 92, contentW, 128, TRUE);
            contentBottom = contentTop + 300;
        }

        const int buttonY = contentBottom + 40;
        MoveWindow(m_templateWizardPrevBtn, 122, buttonY, 102, 32, TRUE);
        MoveWindow(m_templateWizardNextBtn, 232, buttonY, 102, 32, TRUE);
        MoveWindow(m_templateWizardCopyTitleBtn, 342, buttonY, 104, 32, TRUE);
        MoveWindow(m_templateWizardCopyBtn, 454, buttonY, 124, 32, TRUE);
        MoveWindow(m_templateWizardCopyLocationBtn, 586, buttonY, 118, 32, TRUE);
        HWND closeBtn = GetDlgItem(m_templatesWizardWnd, IDC_TEMPLATES_WIZARD_CLOSE);
        if (closeBtn)
            MoveWindow(closeBtn, reviewStep ? 712 : 342, buttonY, reviewStep ? 68 : 102, 32, TRUE);

        ShowWindow(m_templateWizardList, (weatherKindStep || chooseStep) ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardVariablesEdit, variableStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardTitlePreviewLabel, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardTitlePreviewEdit, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardBodyPreviewLabel, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardPreviewEdit, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardCopyTitleBtn, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardCopyBtn, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardCopyLocationBtn, reviewStep ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateWizardNextBtn, reviewStep ? SW_HIDE : SW_SHOW);
        EnableWindow(m_templateWizardPrevBtn, m_templateWizardStep > 0);
        SetWindowTextSafe(m_templateWizardNextBtn, L"Next");

        AutoFitWindowToChildren(m_templatesWizardWnd);
    }

    void OnTemplatesWizardCommand(int id, int code)
    {
        if (id == IDC_TEMPLATES_WIZARD_CLOSE && code == BN_CLICKED) {
            ShowWindow(m_templatesWizardWnd, SW_HIDE);
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_LIST && code == LBN_SELCHANGE) {
            if (TemplateWizardWeatherKindStep())
                return;
            int selected = static_cast<int>(SendMessageW(m_templateWizardList, LB_GETCURSEL, 0, 0));
            const auto& templates = TemplatesForContext(m_templateWizardContext);
            if (selected >= 0 && selected < static_cast<int>(templates.size()))
                m_templateWizardTemplateIndex = static_cast<size_t>(selected);
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_PREV && code == BN_CLICKED) {
            if (m_templateWizardStep > 0) {
                --m_templateWizardStep;
                PopulateTemplatesWizardList();
            }
            RenderTemplatesWizardStep();
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_NEXT && code == BN_CLICKED) {
            if (TemplateWizardWeatherKindStep()) {
                int selected = static_cast<int>(SendMessageW(m_templateWizardList, LB_GETCURSEL, 0, 0));
                if (selected < 0)
                    selected = 0;
                if (!PrepareWeatherTemplateWizardForContext(WeatherTemplateContextFromIndex(selected)))
                    return;
                m_templateWizardStep = TemplateWizardTemplateChoiceStepIndex();
                PopulateTemplatesWizardList();
                SetWindowTextSafe(m_templateWizardVariablesEdit, FormatTemplateVariablesForEdit());
            }
            else if (TemplateWizardTemplateChoiceStep()) {
                int selected = static_cast<int>(SendMessageW(m_templateWizardList, LB_GETCURSEL, 0, 0));
                const auto& templates = TemplatesForContext(m_templateWizardContext);
                if (selected >= 0 && selected < static_cast<int>(templates.size()))
                    m_templateWizardTemplateIndex = static_cast<size_t>(selected);
                SetWindowTextSafe(m_templateWizardVariablesEdit, FormatTemplateVariablesForEdit());
                m_templateWizardStep = TemplateWizardVariablesStepIndex();
            }
            else if (TemplateWizardVariablesStep()) {
                LoadTemplateVariablesFromEdit();
                m_templateWizardStep = TemplateWizardReviewStepIndex();
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
        if (id == IDC_TEMPLATES_WIZARD_COPY_TITLE && code == BN_CLICKED) {
            std::wstring text = GetWindowTextString(m_templateWizardTitlePreviewEdit);
            if (CopyTextToClipboard(text, m_templatesWizardWnd))
                SetStatusText(L"Template title copied to clipboard.");
            else
                SetStatusText(L"Could not copy template title to clipboard.");
            return;
        }
        if (id == IDC_TEMPLATES_WIZARD_COPY_LOCATION && code == BN_CLICKED) {
            LoadTemplateVariablesFromEdit();
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
        m_templateEditorWeatherMode = false;
        EnsureDefaultTemplatesForContext(m_templateEditorContext);
        ShowTemplatesEditorWindowShell(L"Edit Road Templates");
    }

    void ShowEarthquakeTemplatesEditorWindow()
    {
        m_templateEditorContext = TemplateContext::Earthquakes;
        m_templateEditorWeatherMode = false;
        EnsureDefaultTemplatesForContext(m_templateEditorContext);
        ShowTemplatesEditorWindowShell(L"Edit Earthquake Templates");
    }

    void ShowWeatherSystemsTemplatesEditorWindow()
    {
        m_templateEditorContext = TemplateContext::WeatherSystems;
        m_templateEditorWeatherMode = true;
        EnsureDefaultWeatherSystemReportTemplates();
        EnsureDefaultWeatherWarningReportTemplates();
        EnsureDefaultFloodReportTemplates();
        ShowTemplatesEditorWindowShell(L"Edit Weather Templates");
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

        SyncTemplateEditorWeatherControls();
        SyncTemplatesEditorList();
        ShowWindow(m_templatesEditorWnd, SW_SHOW);
        SetForegroundWindow(m_templatesEditorWnd);
    }

    void CreateTemplatesEditorControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Edit Templates", 18, 18, m_headerFont);
        m_templateEditorWeatherTypeLabel = CreateAutoLabel(parent, 0, L"Weather type", 422, 24);
        m_templateEditorWeatherTypeCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 526, 20, 176, 120, parent, ControlId(IDC_TEMPLATES_EDITOR_WEATHER_TYPE), m_hInst, nullptr);
        SendMessageW(m_templateEditorWeatherTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weather Systems"));
        SendMessageW(m_templateEditorWeatherTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Weather Warnings"));
        SendMessageW(m_templateEditorWeatherTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Floods"));
        m_templateEditorList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 18, 64, 214, 340, parent, ControlId(IDC_TEMPLATES_EDITOR_LIST), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Name", 252, 64);
        m_templateEditorNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 252, 90, 450, 26, parent, ControlId(IDC_TEMPLATES_EDITOR_NAME), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Title", 252, 124);
        m_templateEditorTitleEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 252, 150, 450, 26, parent, ControlId(IDC_TEMPLATES_EDITOR_TITLE), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Template", 252, 192);
        m_templateEditorBodyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 252, 218, 450, 186, parent, ControlId(IDC_TEMPLATES_EDITOR_BODY), m_hInst, nullptr);
        HWND newBtn = CreateWindowExW(0, L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_NEW), m_hInst, nullptr);
        HWND saveBtn = CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 252, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_SAVE), m_hInst, nullptr);
        HWND deleteBtn = CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 348, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_DELETE), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 614, 424, 88, 32, parent, ControlId(IDC_TEMPLATES_EDITOR_CLOSE), m_hInst, nullptr);

        for (HWND h : { m_templateEditorWeatherTypeLabel, m_templateEditorWeatherTypeCombo, m_templateEditorList, m_templateEditorNameEdit, m_templateEditorTitleEdit, m_templateEditorBodyEdit, newBtn, saveBtn, deleteBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        EnableNativeSpellCheck(m_templateEditorNameEdit);
        EnableNativeSpellCheck(m_templateEditorTitleEdit);
        EnableNativeSpellCheck(m_templateEditorBodyEdit);

        SyncTemplateEditorWeatherControls();
        SyncTemplatesEditorList();
        AutoFitWindowToChildren(parent);
    }

    void SyncTemplateEditorWeatherControls()
    {
        if (!m_templateEditorWeatherTypeCombo)
            return;

        ShowWindow(m_templateEditorWeatherTypeLabel, m_templateEditorWeatherMode ? SW_SHOW : SW_HIDE);
        ShowWindow(m_templateEditorWeatherTypeCombo, m_templateEditorWeatherMode ? SW_SHOW : SW_HIDE);
        if (!m_templateEditorWeatherMode)
            return;

        int selected = 0;
        if (m_templateEditorContext == TemplateContext::WeatherWarnings)
            selected = 1;
        else if (m_templateEditorContext == TemplateContext::Floods)
            selected = 2;
        SendMessageW(m_templateEditorWeatherTypeCombo, CB_SETCURSEL, static_cast<WPARAM>(selected), 0);
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
            SetWindowTextSafe(m_templateEditorTitleEdit, L"");
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
        SetWindowTextSafe(m_templateEditorTitleEdit, reportTemplate.title);
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
        if (id == IDC_TEMPLATES_EDITOR_WEATHER_TYPE && code == CBN_SELCHANGE) {
            int selected = static_cast<int>(SendMessageW(m_templateEditorWeatherTypeCombo, CB_GETCURSEL, 0, 0));
            m_templateEditorContext = WeatherTemplateContextFromIndex(selected);
            EnsureDefaultTemplatesForContext(m_templateEditorContext);
            SyncTemplatesEditorList();
            return;
        }
        if (code != BN_CLICKED)
            return;

        if (id == IDC_TEMPLATES_EDITOR_NEW) {
            auto& templates = TemplatesForContext(m_templateEditorContext);
            ReportTemplate reportTemplate;
            reportTemplate.name = L"Template " + std::to_wstring(templates.size() + 1);
            reportTemplate.title = DefaultTemplateTitleForContext(m_templateEditorContext);
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
            reportTemplate.title = Trim(GetWindowTextString(m_templateEditorTitleEdit));
            reportTemplate.body = GetWindowTextString(m_templateEditorBodyEdit);
            if (reportTemplate.name.empty())
                reportTemplate.name = L"Template " + std::to_wstring(selected + 1);
            if (reportTemplate.title.empty())
                reportTemplate.title = DefaultTemplateTitleForContext(m_templateEditorContext);
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

    static LRESULT CALLBACK AccountCreatorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleAccountCreatorMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleAccountCreatorMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateAccountCreatorControls(hwnd);
            return 0;

        case WM_COMMAND:
            OnAccountCreatorCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_DRAWITEM:
            if (OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)))
                return TRUE;
            break;

        case WM_CTLCOLORSTATIC:
            SetBkColor(reinterpret_cast<HDC>(wParam), kUiBackground);
            SetTextColor(reinterpret_cast<HDC>(wParam), kUiText);
            return reinterpret_cast<LRESULT>(ModernWindowBrush());

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static void AddComboString(HWND combo, const wchar_t* text)
    {
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
    }

    void ShowAccountCreatorWindow()
    {
        if (!IsOnlineMode()) {
            MessageBoxW(m_hwnd, L"Account Creator is only available while signed in online.", L"Account Creator", MB_OK | MB_ICONINFORMATION);
            return;
        }
        if (!CanManageAccounts()) {
            MessageBoxW(m_hwnd, L"Only Administrators and Supervisors can create accounts.", L"Account Creator", MB_OK | MB_ICONINFORMATION);
            return;
        }

        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = AccountCreatorWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kAccountCreatorClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_accountCreatorWnd || !IsWindow(m_accountCreatorWnd)) {
            m_accountCreatorWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kAccountCreatorClassName,
                L"Account Creator",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                500,
                390,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        ShowWindow(m_accountCreatorWnd, SW_SHOW);
        SetForegroundWindow(m_accountCreatorWnd);
    }

    void CreateAccountCreatorControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Account Creator", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Username", 18, 66);
        m_accountCreatorUsernameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 156, 62, 300, 26, parent, ControlId(IDC_ACCOUNT_CREATOR_USERNAME), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Display Name", 18, 106);
        m_accountCreatorDisplayNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 156, 102, 300, 26, parent, ControlId(IDC_ACCOUNT_CREATOR_DISPLAY_NAME), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Password", 18, 146);
        m_accountCreatorPasswordEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_PASSWORD, 156, 142, 300, 26, parent, ControlId(IDC_ACCOUNT_CREATOR_PASSWORD), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Position", 18, 186);
        m_accountCreatorPositionCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 156, 182, 300, 160, parent, ControlId(IDC_ACCOUNT_CREATOR_POSITION), m_hInst, nullptr);

        const int rank = CurrentPositionRank();
        if (rank >= 4)
            AddComboString(m_accountCreatorPositionCombo, L"Administrator");
        if (rank >= 3)
            AddComboString(m_accountCreatorPositionCombo, L"Supervisor");
        AddComboString(m_accountCreatorPositionCombo, L"Manager");
        AddComboString(m_accountCreatorPositionCombo, L"ERC");
        SendMessageW(m_accountCreatorPositionCombo, CB_SETCURSEL, 0, 0);

        m_accountCreatorActiveCheck = CreateWindowExW(0, L"BUTTON", L"Account active", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 156, 222, 180, 24, parent, ControlId(IDC_ACCOUNT_CREATOR_ACTIVE), m_hInst, nullptr);
        SendMessageW(m_accountCreatorActiveCheck, BM_SETCHECK, BST_CHECKED, 0);

        m_accountCreatorStatusLabel = CreateAutoLabel(parent, IDC_ACCOUNT_CREATOR_STATUS, L"", 18, 266, nullptr, 438);
        m_accountCreatorCreateBtn = CreateWindowExW(0, L"BUTTON", L"Create / Update", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 156, 306, 138, 32, parent, ControlId(IDC_ACCOUNT_CREATOR_CREATE), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 368, 306, 88, 32, parent, ControlId(IDC_ACCOUNT_CREATOR_CLOSE), m_hInst, nullptr);

        for (HWND h : { m_accountCreatorUsernameEdit, m_accountCreatorDisplayNameEdit, m_accountCreatorPasswordEdit, m_accountCreatorPositionCombo, m_accountCreatorActiveCheck, m_accountCreatorStatusLabel, m_accountCreatorCreateBtn, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        AutoFitWindowToChildren(parent);
    }

    void OnAccountCreatorCommand(int id, int code)
    {
        if (id == IDC_ACCOUNT_CREATOR_CLOSE && code == BN_CLICKED) {
            ShowWindow(m_accountCreatorWnd, SW_HIDE);
            return;
        }
        if (id != IDC_ACCOUNT_CREATOR_CREATE || code != BN_CLICKED)
            return;

        std::wstring username = Trim(GetWindowTextString(m_accountCreatorUsernameEdit));
        std::wstring displayName = Trim(GetWindowTextString(m_accountCreatorDisplayNameEdit));
        std::wstring password = GetWindowTextString(m_accountCreatorPasswordEdit);
        std::wstring position = GetWindowTextString(m_accountCreatorPositionCombo);
        bool active = SendMessageW(m_accountCreatorActiveCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (username.empty() || password.empty() || position.empty()) {
            SetWindowTextSafe(m_accountCreatorStatusLabel, L"Username, password and position are required.");
            return;
        }
        if (displayName.empty())
            displayName = username;

        CreateAccountAsync(username, displayName, password, position, active);
    }

    void CreateAccountAsync(
        const std::wstring& username,
        const std::wstring& displayName,
        const std::wstring& password,
        const std::wstring& position,
        bool active)
    {
        if (!IsOnlineMode()) {
            SetWindowTextSafe(m_accountCreatorStatusLabel, L"Account creation requires online mode.");
            return;
        }
        if (!CanManageAccounts()) {
            SetWindowTextSafe(m_accountCreatorStatusLabel, L"Only Administrators and Supervisors can create accounts.");
            return;
        }

        EnableWindow(m_accountCreatorCreateBtn, FALSE);
        SetWindowTextSafe(m_accountCreatorStatusLabel, L"Creating account...");
        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            EnableWindow(m_accountCreatorCreateBtn, TRUE);
            SetWindowTextSafe(m_accountCreatorStatusLabel, L"No collaboration server is configured.");
            return;
        }
        std::wstring authHeaders = BearerAuthHeader(m_session);
        ClientSession session = m_session;
        HWND hwnd = m_hwnd;
        ScheduleBackgroundTask([hwnd, server, authHeaders, session, username, displayName, password, position, active]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::CreateAccount;

            BinaryCallResult binary;
            if (BinaryCreateAccount(server, session, username, displayName, password, position, active, binary) || binary.protocolAvailable) {
                result->ok = binary.ok;
                result->error = binary.error;
                if (!g_appQuitting.load() && IsWindow(hwnd))
                    PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
                else
                    delete result;
                return;
            }

            std::string response;
            std::wstring error;
            std::string body = "{";
            body += "\"username\":" + JsonEscape(username);
            body += ",\"displayName\":" + JsonEscape(displayName);
            body += ",\"password\":" + JsonEscape(password);
            body += ",\"position\":" + JsonEscape(position);
            body += ",\"active\":";
            body += active ? "true" : "false";
            body += "}";
            result->ok = HttpPostJsonTextWithHeaders(AppendPath(server, L"/api/users"), body, authHeaders, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            });
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

    std::wstring WeatherWarningsQueryUrl() const
    {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        wchar_t date[32]{};
        wcsftime(date, _countof(date), L"%Y-%m-%d", &local);
        return std::wstring(kWeatherWarningsSourceUrl) + L"?date=" + date;
    }

    void FetchEarthquakesAsync(bool notify)
    {
        if (m_earthquakeFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        std::wstring url = EarthquakeQueryUrl();
        const bool useServerFetch = ShouldUseServerToFetchData();
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t serverSyncEpoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        json sourceOptions = { { "url", WideToUtf8(url) } };
        ScheduleBackgroundTask([hwnd, url, notify, useServerFetch, server, session, serverSyncEpoch, sourceOptions]() {
            auto* result = new EarthquakeResult{};
            result->notify = notify;
            if (useServerFetch) {
                result->serverSourced = true;
                result->serverSource = ServerSourceKind::Earthquakes;
                result->serverSyncEpoch = serverSyncEpoch;
            }
            std::string body;
            std::wstring error;
            if (useServerFetch) {
                BinarySourceBundleResult bundle;
                if (BinaryFetchSourceBundle(server, session, L"earthquakes", 0u, sourceOptions, bundle)) {
                    result->serverGeneration = bundle.generation;
                    try {
                        if (ParseEarthquakesFromSourceBundle(bundle, result->events, error)) {
                            result->ok = true;
                        }
                    }
                    catch (const std::exception& e) {
                        error = L"Server earthquake parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    error = bundle.error.empty() ? L"Server earthquake fetch failed." : bundle.error;
                }
            }
            if (!result->ok && useServerFetch)
                result->error = error.empty() ? L"Server earthquake fetch failed." : error;
            if (!result->ok && !useServerFetch) {
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

        const bool serverSourced = result->serverSourced;
        const ServerSourceKind serverSource = result->serverSource;
        const uint32_t serverSyncEpoch = result->serverSyncEpoch;
        if (serverSourced) {
            ResetServerSourceWaitFlag(serverSource);
            if (!IsCurrentServerSourceEpoch(serverSyncEpoch)) {
                delete result;
                return;
            }
            StoreServerSourceGeneration(serverSource, result->serverGeneration);
        }

        if (!result->ok) {
            if (m_earthquakeListWnd && IsWindowVisible(m_earthquakeListWnd))
                SetStatusText(result->error);
            delete result;
            if (serverSourced)
                ScheduleServerSourceWait(serverSource);
            return;
        }

        m_allEarthquakes = std::move(result->events);
        for (EarthquakeEvent& event : m_allEarthquakes)
            event.excluded = IsEventExcluded(L"earthquake", EarthquakeStableKey(event));
        ApplyEarthquakeListFilters();
        if (result->notify)
            NotifyForMatchingEarthquakes(m_allEarthquakes);
        delete result;
        if (serverSourced)
            ScheduleServerSourceWait(serverSource);
    }

    void FetchWeatherSystemsAsync(bool notify)
    {
        if (g_weatherSystemsFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        const bool useServerFetch = ShouldUseServerToFetchData();
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t serverSyncEpoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        json sourceOptions = { { "url", WideToUtf8(std::wstring(kWeatherSystemsSourceUrl)) } };
        ScheduleBackgroundTask([hwnd, notify, useServerFetch, server, session, serverSyncEpoch, sourceOptions]() {
            auto* result = new WeatherSystemsResult{};
            result->notify = notify;
            if (useServerFetch) {
                result->serverSourced = true;
                result->serverSource = ServerSourceKind::WeatherSystems;
                result->serverSyncEpoch = serverSyncEpoch;
            }
            std::string body;
            std::wstring error;
            if (useServerFetch) {
                BinarySourceBundleResult bundle;
                if (BinaryFetchSourceBundle(server, session, L"weather_systems", 0u, sourceOptions, bundle)) {
                    result->serverGeneration = bundle.generation;
                    try {
                        if (!ParseWeatherSystemsFromSourceBundle(
                            bundle,
                            result->systems,
                            result->statusText,
                            error))
                        {
                            result->error = L"Weather systems parse failed: " + error;
                        }
                        else {
                            result->ok = true;
                        }
                    }
                    catch (const std::exception& e) {
                        result->error = L"Server weather systems parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    error = bundle.error.empty() ? L"Server weather systems fetch failed." : bundle.error;
                }
            }
            if (!result->ok && useServerFetch && result->error.empty())
                result->error = error.empty() ? L"Server weather systems fetch failed." : error;
            if (!result->ok && !useServerFetch) {
                if (HttpGetText(kWeatherSystemsSourceUrl, body, error)) {
                    try {
                        result->systems = ParseWeatherSystemEvents(body, result->statusText);
                        for (WeatherSystemEvent& system : result->systems) {
                            if (system.detailPath.empty())
                                continue;
                            std::wstring trackUrl = ResolveRelativeUrl(kWeatherSystemsSourceUrl, system.detailPath);
                            std::string trackBody;
                            std::wstring trackError;
                            if (HttpGetText(trackUrl, trackBody, trackError)) {
                                std::vector<WeatherForecastPoint> track = ParseWeatherSystemForecastTrack(trackBody);
                                if (!track.empty())
                                    system.forecastTrack = std::move(track);
                            }
                        }
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

        const bool serverSourced = result->serverSourced;
        const ServerSourceKind serverSource = result->serverSource;
        const uint32_t serverSyncEpoch = result->serverSyncEpoch;
        if (serverSourced) {
            ResetServerSourceWaitFlag(serverSource);
            if (!IsCurrentServerSourceEpoch(serverSyncEpoch)) {
                delete result;
                return;
            }
            StoreServerSourceGeneration(serverSource, result->serverGeneration);
        }

        if (!result->ok) {
            if (m_weatherSystemsListWnd && IsWindowVisible(m_weatherSystemsListWnd))
                SetStatusText(result->error);
            delete result;
            if (serverSourced)
                ScheduleServerSourceWait(serverSource);
            return;
        }

        m_allWeatherSystems = std::move(result->systems);
        for (WeatherSystemEvent& system : m_allWeatherSystems)
            system.excluded = IsEventExcluded(L"weather_system", WeatherSystemStableKey(system));
        ApplyWeatherSystemsListFilter(false);
        if (result->notify)
            NotifyForMatchingWeatherSystems(m_allWeatherSystems);
        if (m_weatherSystemsListWnd && IsWindowVisible(m_weatherSystemsListWnd)) {
            std::wstring status = L"Showing " + std::to_wstring(m_filteredWeatherSystems.size()) + L" weather system(s).";
            if (!result->statusText.empty())
                status += L" " + result->statusText;
            SetStatusText(status);
        }
        delete result;
        if (serverSourced)
            ScheduleServerSourceWait(serverSource);
    }

    void FetchWeatherWarningsAsync(bool notify)
    {
        if (m_weatherWarningsFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        std::wstring url = WeatherWarningsQueryUrl();
        const bool useServerFetch = ShouldUseServerToFetchData();
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t serverSyncEpoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        json sourceOptions = { { "url", WideToUtf8(url) } };
        ScheduleBackgroundTask([hwnd, notify, url, useServerFetch, server, session, serverSyncEpoch, sourceOptions]() {
            auto* result = new WeatherWarningsResult{};
            result->notify = notify;
            if (useServerFetch) {
                result->serverSourced = true;
                result->serverSource = ServerSourceKind::WeatherWarnings;
                result->serverSyncEpoch = serverSyncEpoch;
            }
            std::string body;
            std::wstring error;
            if (useServerFetch) {
                BinarySourceBundleResult bundle;
                if (BinaryFetchSourceBundle(server, session, L"weather_warnings", 0u, sourceOptions, bundle)) {
                    result->serverGeneration = bundle.generation;
                    try {
                        if (ParseWeatherWarningsFromSourceBundle(
                            bundle,
                            result->warnings,
                            result->statusText,
                            error))
                        {
                            result->ok = true;
                        }
                    }
                    catch (const std::exception& e) {
                        result->error = L"Server weather warnings parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    error = bundle.error.empty() ? L"Server weather warnings fetch failed." : bundle.error;
                }
            }
            if (!result->ok && useServerFetch && result->error.empty())
                result->error = error.empty() ? L"Server weather warnings fetch failed." : error;
            if (!result->ok && !useServerFetch) {
                if (HttpGetText(url, body, error)) {
                    try {
                        result->warnings = ParseWeatherWarningEvents(body, result->statusText);
                        result->ok = true;
                    }
                    catch (const std::exception& e) {
                        result->ok = false;
                        result->error = L"Weather warnings parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    result->ok = false;
                    result->error = L"Weather warnings fetch failed: " + error;
                }
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            if (!PostMessageW(hwnd, WM_APP_WEATHER_WARNINGS_READY, 0, reinterpret_cast<LPARAM>(result)))
                delete result;
            });
    }

    void OnWeatherWarningsReady(WeatherWarningsResult* result)
    {
        m_weatherWarningsFetchInProgress.store(false);
        if (!result)
            return;

        const bool serverSourced = result->serverSourced;
        const ServerSourceKind serverSource = result->serverSource;
        const uint32_t serverSyncEpoch = result->serverSyncEpoch;
        if (serverSourced) {
            ResetServerSourceWaitFlag(serverSource);
            if (!IsCurrentServerSourceEpoch(serverSyncEpoch)) {
                delete result;
                return;
            }
            StoreServerSourceGeneration(serverSource, result->serverGeneration);
        }

        if (!result->ok) {
            if (m_weatherWarningsListWnd && IsWindowVisible(m_weatherWarningsListWnd))
                SetStatusText(result->error);
            delete result;
            if (serverSourced)
                ScheduleServerSourceWait(serverSource);
            return;
        }

        m_allWeatherWarnings = std::move(result->warnings);
        for (WeatherWarningEvent& warning : m_allWeatherWarnings)
            warning.excluded = IsEventExcluded(L"weather_warning", WeatherWarningStableKey(warning));
        ApplyWeatherWarningsListFilter(false);
        if (result->notify)
            NotifyForWeatherWarnings(m_allWeatherWarnings);
        if (m_weatherWarningsListWnd && IsWindowVisible(m_weatherWarningsListWnd)) {
            std::wstring status = L"Showing " + std::to_wstring(m_filteredWeatherWarnings.size()) + L" weather warning(s).";
            if (!result->statusText.empty())
                status += L" " + result->statusText;
            SetStatusText(status);
        }
        delete result;
        if (serverSourced)
            ScheduleServerSourceWait(serverSource);
    }

    void FetchFloodsAsync(bool notify)
    {
        if (m_floodsFetchInProgress.exchange(true))
            return;

        HWND hwnd = m_hwnd;
        const bool useServerFetch = ShouldUseServerToFetchData();
        const std::wstring server = ServerBaseUrl();
        const ClientSession session = m_session;
        const uint32_t serverSyncEpoch = m_serverSourceSyncEpoch.load(std::memory_order_acquire);
        json sourceOptions = { { "url", WideToUtf8(std::wstring(kFloodsSourceUrl)) } };
        ScheduleBackgroundTask([hwnd, notify, useServerFetch, server, session, serverSyncEpoch, sourceOptions]() {
            auto* result = new FloodsResult{};
            result->notify = notify;
            if (useServerFetch) {
                result->serverSourced = true;
                result->serverSource = ServerSourceKind::Floods;
                result->serverSyncEpoch = serverSyncEpoch;
            }
            std::string body;
            std::wstring error;
            if (useServerFetch) {
                BinarySourceBundleResult bundle;
                if (BinaryFetchSourceBundle(server, session, L"floods", 0u, sourceOptions, bundle)) {
                    result->serverGeneration = bundle.generation;
                    try {
                        if (ParseFloodsFromSourceBundle(
                            bundle,
                            result->floods,
                            result->statusText,
                            error))
                        {
                            result->ok = true;
                        }
                    }
                    catch (const std::exception& e) {
                        result->error = L"Server floods parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    error = bundle.error.empty() ? L"Server floods fetch failed." : bundle.error;
                }
            }
            if (!result->ok && useServerFetch && result->error.empty())
                result->error = error.empty() ? L"Server floods fetch failed." : error;
            if (!result->ok && !useServerFetch) {
                if (HttpGetText(kFloodsSourceUrl, body, error)) {
                    try {
                        result->floods = ParseFloodEvents(body, result->statusText);
                        result->ok = true;
                    }
                    catch (const std::exception& e) {
                        result->ok = false;
                        result->error = L"Floods parse failed: " + Utf8ToWide(e.what());
                    }
                }
                else {
                    result->ok = false;
                    result->error = L"Floods fetch failed: " + error;
                }
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            if (!PostMessageW(hwnd, WM_APP_FLOODS_READY, 0, reinterpret_cast<LPARAM>(result)))
                delete result;
            });
    }

    void OnFloodsReady(FloodsResult* result)
    {
        m_floodsFetchInProgress.store(false);
        if (!result)
            return;

        const bool serverSourced = result->serverSourced;
        const ServerSourceKind serverSource = result->serverSource;
        const uint32_t serverSyncEpoch = result->serverSyncEpoch;
        if (serverSourced) {
            ResetServerSourceWaitFlag(serverSource);
            if (!IsCurrentServerSourceEpoch(serverSyncEpoch)) {
                delete result;
                return;
            }
            StoreServerSourceGeneration(serverSource, result->serverGeneration);
        }

        if (!result->ok) {
            if (m_floodsListWnd && IsWindowVisible(m_floodsListWnd))
                SetStatusText(result->error);
            delete result;
            if (serverSourced)
                ScheduleServerSourceWait(serverSource);
            return;
        }

        m_allFloods = std::move(result->floods);
        for (FloodEvent& flood : m_allFloods)
            flood.excluded = IsEventExcluded(L"flood", FloodStableKey(flood));
        ApplyFloodsListFilter(false);
        if (result->notify)
            NotifyForFloods(m_allFloods);
        if (m_floodsListWnd && IsWindowVisible(m_floodsListWnd)) {
            std::wstring status = L"Showing " + std::to_wstring(m_filteredFloods.size()) + L" flood item(s).";
            if (!result->statusText.empty())
                status += L" " + result->statusText;
            SetStatusText(status);
        }
        delete result;
        if (serverSourced)
            ScheduleServerSourceWait(serverSource);
    }

    void StoreWeatherListPeriodFiltersFromControls()
    {
        if (m_weatherWarningsListPeriodCombo)
            m_weatherWarningsListPeriodText = Trim(GetWindowTextString(m_weatherWarningsListPeriodCombo));
        if (m_floodsListPeriodCombo)
            m_floodsListPeriodText = Trim(GetWindowTextString(m_floodsListPeriodCombo));
    }

    void StoreWeatherSystemsListFiltersFromControls()
    {
        if (m_weatherSystemsListForecastCombo)
            m_weatherSystemsListForecastText = Trim(GetWindowTextString(m_weatherSystemsListForecastCombo));
    }

    int WeatherSystemsListMinimumForecastRank() const
    {
        std::wstring value = ToLower(Trim(m_weatherSystemsListForecastText));
        if (value.empty() || value == L"all")
            return -1;
        return WeatherSystemCategoryRank(value);
    }

    bool WeatherSystemMatchesListForecast(const WeatherSystemEvent& system) const
    {
        const int minimumRank = WeatherSystemsListMinimumForecastRank();
        if (minimumRank < 0)
            return true;

        int bestRank = WeatherSystemEffectiveCategoryRank(system.category, system.windKnots);
        bestRank = MaxInt(bestRank, WeatherSystemEffectiveCategoryRank(system.forecastCategory, system.forecastWindKnots));
        for (const WeatherForecastPoint& point : system.forecastTrack) {
            if (point.leadHours <= 0)
                continue;
            bestRank = MaxInt(bestRank, WeatherSystemEffectiveCategoryRank(point.category, point.windKnots));
        }
        return bestRank >= minimumRank;
    }

    void ApplyWeatherSystemsListFilter(bool save)
    {
        StoreWeatherSystemsListFiltersFromControls();
        m_filteredWeatherSystems.clear();
        for (const WeatherSystemEvent& system : m_allWeatherSystems) {
            if (WeatherSystemMatchesListForecast(system))
                m_filteredWeatherSystems.push_back(system);
        }
        RenderWeatherSystemsListRows();
        ApplyWeatherSystemVisibility();
        if (m_weatherSystemsListWnd && IsWindowVisible(m_weatherSystemsListWnd))
            SetStatusText(L"Showing " + std::to_wstring(m_filteredWeatherSystems.size()) + L" weather system(s).");
        if (save)
            SaveSettings();
    }

    bool WeatherWarningMatchesListPeriod(const WeatherWarningEvent& warning) const
    {
        if (IsAllPeriodText(m_weatherWarningsListPeriodText))
            return true;
        const int hours = PeriodHoursFromText(m_weatherWarningsListPeriodText, 24);
        if (hours >= 48)
            return true;

        std::wstring text = ToLower(warning.validFrom + L" " + warning.validTo);
        if (text.empty())
            return true;
        if (text.find(L"today") != std::wstring::npos)
            return true;
        if (text.find(L"tomorrow") != std::wstring::npos)
            return false;
        return true;
    }

    void ApplyWeatherWarningsListFilter(bool save)
    {
        StoreWeatherListPeriodFiltersFromControls();
        m_filteredWeatherWarnings.clear();
        for (const WeatherWarningEvent& warning : m_allWeatherWarnings) {
            if (WeatherWarningMatchesListPeriod(warning))
                m_filteredWeatherWarnings.push_back(warning);
        }
        RenderWeatherWarningsListRows();
        ApplyWeatherWarningVisibility();
        if (save)
            SaveSettings();
    }

    static bool TryParseEventTimeMs(std::wstring text, long long& timeMsOut)
    {
        text = Trim(text);
        if (text.empty())
            return false;
        ReplaceAllText(text, L"T", L" ");
        ReplaceAllText(text, L"Z", L"");
        size_t dot = text.find(L'.');
        if (dot != std::wstring::npos)
            text = text.substr(0, dot);
        if (text.size() > 16)
            text = text.substr(0, 16);
        return TryParseDateTimeFilter(text, timeMsOut);
    }

    bool FloodMatchesListPeriod(const FloodEvent& flood) const
    {
        if (IsAllPeriodText(m_floodsListPeriodText))
            return true;
        long long itemMs = 0;
        std::wstring timeText = flood.timeChanged.empty() ? flood.timeRaised : flood.timeChanged;
        if (!TryParseEventTimeMs(timeText, itemMs))
            return true;
        return itemMs >= PeriodStartTimeMs(m_floodsListPeriodText);
    }

    void ApplyFloodsListFilter(bool save)
    {
        StoreWeatherListPeriodFiltersFromControls();
        m_filteredFloods.clear();
        for (const FloodEvent& flood : m_allFloods) {
            if (FloodMatchesListPeriod(flood))
                m_filteredFloods.push_back(flood);
        }
        RenderFloodsListRows();
        ApplyFloodVisibility();
        if (save)
            SaveSettings();
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
            CheckMenuItem(menu, IDM_WEATHER_SYSTEM_FORECASTS, MF_BYCOMMAND | (m_showWeatherSystemForecasts ? MF_CHECKED : MF_UNCHECKED));
            EnableMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NONE, MF_BYCOMMAND | (m_showWeatherSystems ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(menu, IDM_WEATHER_SYSTEM_OVERLAY_NAME_WIND, MF_BYCOMMAND | (m_showWeatherSystems ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(menu, IDM_WEATHER_SYSTEM_FORECASTS, MF_BYCOMMAND | (m_showWeatherSystems ? MF_ENABLED : MF_GRAYED));
        }
    }

    void UpdateNotificationHistoryMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (menu)
            CheckMenuItem(menu, IDM_VIEW_NOTIFICATION_HISTORY, MF_BYCOMMAND | (m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED));
    }

    void UpdateViewMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (!menu)
            return;

        CheckMenuItem(menu, IDM_VIEW_NOTIFICATION_HISTORY, MF_BYCOMMAND | (m_showNotificationHistory ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_AREA_LABELS, MF_BYCOMMAND | (m_showAreaLabels ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_NOTIFICATION_REGION_POLYGONS, MF_BYCOMMAND | (m_showIncidentNotificationRegionPolygons ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_ROAD_DEPICTIONS, MF_BYCOMMAND | (m_showRoadDepictions ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_FPS_COUNTER, MF_BYCOMMAND | (m_showFpsCounter ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_MAP_CONTROLS, MF_BYCOMMAND | (m_showMapControls ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_COUNTDOWN_TIMER, MF_BYCOMMAND | (m_showCountdownTimer ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_VIEW_COMMS_INDICATOR, MF_BYCOMMAND | (m_showCommsIndicator ? MF_CHECKED : MF_UNCHECKED));
    }

    void ToggleNotificationHistory()
    {
        m_showNotificationHistory = !m_showNotificationHistory;
        UpdateViewMenu();
        m_map.SetNotificationHistoryVisible(m_showNotificationHistory);
        RenderNotificationHistory();
        SaveSettings();
    }

    void ToggleFpsCounter()
    {
        m_showFpsCounter = !m_showFpsCounter;
        UpdateViewMenu();
        m_map.SetFpsCounterVisible(m_showFpsCounter);
        SaveSettings();
    }

    void ToggleIncidentNotificationRegionPolygons()
    {
        m_showIncidentNotificationRegionPolygons = !m_showIncidentNotificationRegionPolygons;
        UpdateViewMenu();
        m_map.SetNotificationPolygonsVisible(m_showIncidentNotificationRegionPolygons);
        if (m_notificationRegionsShowPolygonsCheck) {
            SendMessageW(
                m_notificationRegionsShowPolygonsCheck,
                BM_SETCHECK,
                m_showIncidentNotificationRegionPolygons ? BST_CHECKED : BST_UNCHECKED,
                0);
        }
        SaveSettings();
    }

    void ToggleMapControls()
    {
        m_showMapControls = !m_showMapControls;
        UpdateViewMenu();
        m_map.SetToolbarVisible(m_showMapControls);
        SaveSettings();
    }

    void ToggleCountdownTimer()
    {
        m_showCountdownTimer = !m_showCountdownTimer;
        UpdateViewMenu();
        m_map.SetCountdownVisible(m_showCountdownTimer);
        SaveSettings();
    }

    void ToggleCommsIndicator()
    {
        m_showCommsIndicator = !m_showCommsIndicator;
        UpdateViewMenu();
        m_map.SetCommsIndicatorVisible(m_showCommsIndicator);
        SaveSettings();
    }

    void ApplySoundSettings()
    {
        SetSoundCuesEnabled(m_soundCuesEnabled);
        SetSoundOutputDeviceId(m_soundOutputDeviceId);
        SetSoundCueEnabled(SoundCue::Message, m_soundMessageEnabled);
        SetSoundCueEnabled(SoundCue::PrivateMessage, m_soundPrivateMessageEnabled);
        SetSoundCueEnabled(SoundCue::Notification, m_soundNotificationEnabled);
        SetSoundCueEnabled(SoundCue::TimerStart, m_soundTimerStartEnabled);
        SetSoundCueEnabled(SoundCue::TimerWarning, m_soundTimerWarningEnabled);
        SetSoundCueEnabled(SoundCue::TimerComplete, m_soundTimerCompleteEnabled);
    }

    void ToggleSoundCues()
    {
        m_soundCuesEnabled = !m_soundCuesEnabled;
        ApplySoundSettings();
        UpdateViewMenu();
        SaveSettings();
        if (m_soundCuesEnabled)
            PlaySoundCue(SoundCue::Message);
    }

    void ToggleUsersOverlay()
    {
        m_showUsersOverlay = !m_showUsersOverlay;
        m_map.SetUsersVisible(m_showUsersOverlay);
        RenderOnlineUsers();
        if (m_showUsersOverlay)
            PollServerAsync();
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

    void UpdateRoadsMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (!menu)
            return;

        CheckMenuItem(menu, IDM_ROADS_SHOW_INCIDENTS, MF_BYCOMMAND | (m_showIncidents ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_INCIDENT_OVERLAY_NONE, MF_BYCOMMAND | (m_showIncidentOverlayLabels ? MF_UNCHECKED : MF_CHECKED));
        CheckMenuItem(menu, IDM_INCIDENT_OVERLAY_SUMMARY, MF_BYCOMMAND | (m_showIncidentOverlayLabels ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY, MF_BYCOMMAND | (m_incidentOverlayNotifiedOnly ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(menu, IDM_INCIDENT_OVERLAY_NONE, MF_BYCOMMAND | (m_showIncidents ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, IDM_INCIDENT_OVERLAY_SUMMARY, MF_BYCOMMAND | (m_showIncidents ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, IDM_INCIDENT_OVERLAY_NOTIFIED_ONLY, MF_BYCOMMAND | (m_showIncidents ? MF_ENABLED : MF_GRAYED));
    }

    void ToggleShowIncidents()
    {
        m_showIncidents = !m_showIncidents;
        UpdateRoadsMenu();
        ApplyIncidentMapVisibility();
        SaveSettings();
    }

    void SetIncidentOverlayLabels(bool visible)
    {
        if (m_showIncidentOverlayLabels == visible)
            return;

        m_showIncidentOverlayLabels = visible;
        UpdateRoadsMenu();
        ApplyIncidentMapVisibility();
        SaveSettings();
    }

    void ToggleIncidentOverlayNotifiedOnly()
    {
        m_incidentOverlayNotifiedOnly = !m_incidentOverlayNotifiedOnly;
        UpdateRoadsMenu();
        ApplyIncidentMapVisibility();
        SaveSettings();
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
        if (!m_showWeatherSystems) {
            m_map.SetWeatherSystems({});
            return;
        }

        std::vector<WeatherSystemEvent> systems = m_allWeatherSystems;
        for (WeatherSystemEvent& system : systems) {
            const std::wstring key = WeatherSystemStableKey(system);
            if (!m_showWeatherSystemForecasts ||
                m_hiddenWeatherSystemForecastIds.find(key) != m_hiddenWeatherSystemForecastIds.end())
            {
                system.forecastTrack.clear();
                system.hasForecastLocation = false;
            }
        }
        m_map.SetWeatherSystems(systems);
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

    void ToggleWeatherSystemForecasts()
    {
        m_showWeatherSystemForecasts = !m_showWeatherSystemForecasts;
        UpdateWeatherSystemsMenu();
        ApplyWeatherSystemVisibility();
        SaveSettings();
    }

    void ApplyWeatherWarningVisibility()
    {
        m_map.SetWeatherWarningOverlayVisible(m_showWeatherWarnings && m_showWeatherWarningOverlayLabels);
        m_map.SetWeatherWarningPolygonsVisible(m_showWeatherWarnings && m_showWeatherWarningPolygons);
        if (m_showWeatherWarnings) {
            std::vector<WeatherWarningEvent> warnings = m_filteredWeatherWarnings;
            for (WeatherWarningEvent& warning : warnings) {
                if (m_hiddenWeatherWarningPolygonIds.find(WeatherWarningStableKey(warning)) != m_hiddenWeatherWarningPolygonIds.end())
                    warning.polygon.clear();
            }
            m_map.SetWeatherWarnings(warnings);
        }
        else {
            m_map.SetWeatherWarnings({});
        }
    }

    void ApplyFloodVisibility()
    {
        m_map.SetFloodOverlayVisible(m_showFloods && m_showFloodOverlayLabels);
        if (m_showFloods)
            m_map.SetFloods(m_filteredFloods);
        else
            m_map.SetFloods({});
    }

    void UpdateWeatherWarningMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (!menu)
            return;

        CheckMenuItem(menu, IDM_SHOW_WEATHER_WARNINGS, MF_BYCOMMAND | (m_showWeatherWarnings ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_WEATHER_WARNING_OVERLAY_NONE, MF_BYCOMMAND | (m_showWeatherWarningOverlayLabels ? MF_UNCHECKED : MF_CHECKED));
        CheckMenuItem(menu, IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA, MF_BYCOMMAND | (m_showWeatherWarningOverlayLabels ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_WEATHER_WARNING_POLYGONS, MF_BYCOMMAND | (m_showWeatherWarningPolygons ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(menu, IDM_WEATHER_WARNING_OVERLAY_NONE, MF_BYCOMMAND | (m_showWeatherWarnings ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, IDM_WEATHER_WARNING_OVERLAY_TYPE_AREA, MF_BYCOMMAND | (m_showWeatherWarnings ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, IDM_WEATHER_WARNING_POLYGONS, MF_BYCOMMAND | (m_showWeatherWarnings ? MF_ENABLED : MF_GRAYED));
    }

    void UpdateFloodMenu()
    {
        HMENU menu = GetMenu(m_hwnd);
        if (!menu)
            return;

        CheckMenuItem(menu, IDM_SHOW_FLOODS, MF_BYCOMMAND | (m_showFloods ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_FLOOD_OVERLAY_NONE, MF_BYCOMMAND | (m_showFloodOverlayLabels ? MF_UNCHECKED : MF_CHECKED));
        CheckMenuItem(menu, IDM_FLOOD_OVERLAY_SEVERITY_AREA, MF_BYCOMMAND | (m_showFloodOverlayLabels ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(menu, IDM_FLOOD_OVERLAY_NONE, MF_BYCOMMAND | (m_showFloods ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(menu, IDM_FLOOD_OVERLAY_SEVERITY_AREA, MF_BYCOMMAND | (m_showFloods ? MF_ENABLED : MF_GRAYED));
    }

    void ToggleShowWeatherWarnings()
    {
        m_showWeatherWarnings = !m_showWeatherWarnings;
        UpdateWeatherWarningMenu();
        ApplyWeatherWarningVisibility();
        SaveSettings();
        if (m_showWeatherWarnings && m_allWeatherWarnings.empty())
            FetchWeatherWarningsAsync(false);
    }

    void ToggleWeatherWarningPolygons()
    {
        m_showWeatherWarningPolygons = !m_showWeatherWarningPolygons;
        UpdateWeatherWarningMenu();
        ApplyWeatherWarningVisibility();
        SaveSettings();
    }

    void SetWeatherWarningOverlayLabels(bool visible)
    {
        if (m_showWeatherWarningOverlayLabels == visible)
            return;

        m_showWeatherWarningOverlayLabels = visible;
        UpdateWeatherWarningMenu();
        ApplyWeatherWarningVisibility();
        SaveSettings();
    }

    void ToggleShowFloods()
    {
        m_showFloods = !m_showFloods;
        UpdateFloodMenu();
        ApplyFloodVisibility();
        SaveSettings();
        if (m_showFloods && m_allFloods.empty())
            FetchFloodsAsync(false);
    }

    void SetFloodOverlayLabels(bool visible)
    {
        if (m_showFloodOverlayLabels == visible)
            return;

        m_showFloodOverlayLabels = visible;
        UpdateFloodMenu();
        ApplyFloodVisibility();
        SaveSettings();
    }

    void ToggleAreaLabels()
    {
        m_showAreaLabels = !m_showAreaLabels;
        UpdateViewMenu();
        m_map.SetAreaLabelsVisible(m_showAreaLabels);
        SaveSettings();
    }

    void ToggleRoadDepictions()
    {
        m_showRoadDepictions = !m_showRoadDepictions;
        UpdateViewMenu();
        m_map.SetRoadDepictionsVisible(m_showRoadDepictions);
        SaveSettings();
    }

    void SetMapDisplayMode(bool displayWorldMap)
    {
        if (m_displayWorldMap == displayWorldMap)
            return;

        m_displayWorldMap = displayWorldMap;
        m_map.SetDisplayWorldMap(m_displayWorldMap);
        SaveSettings();
        SetStatusText(m_displayWorldMap ? L"Map display: rest of world." : L"Map display: UK depiction.");
    }

    bool EarthquakeIsWithinPopulatedAreaRadius(
        const EarthquakeEvent& event,
        double radiusMiles,
        double minimumPopulation) const
    {
        if (radiusMiles <= 0.0)
            return true;
        if (!event.hasLocation)
            return false;

        if (m_populatedPlacesLoaded && !m_populatedPlaces.empty()) {
            return FindNearestPopulatedPlace(
                m_populatedPlaces,
                event.latitude,
                event.longitude,
                radiusMiles,
                minimumPopulation,
                nullptr,
                nullptr);
        }

        const_cast<MainWindow*>(this)->EnsurePopulatedPlacesAvailableAsync(false);
        return false;
    }

    double EffectiveEarthquakePopulatedRadiusMiles(double eventMagnitude, double minimumMagnitude, double baseRadiusMiles) const
    {
        if (baseRadiusMiles <= 0.0)
            return 0.0;

        const double extraTenths = MaxValue(0.0, (eventMagnitude - minimumMagnitude) * 10.0);
        return baseRadiusMiles + extraTenths * MaxValue(0.0, m_earthquakePopulatedRadiusMilesPerMagnitudeTenth);
    }

    bool EarthquakeMatchesNotification(const EarthquakeEvent& event) const
    {
        if (event.magnitude + 0.0001 < m_earthquakeNotificationMagnitude)
            return false;

        long long afterMs = 0;
        if (m_earthquakeNotificationUseDateFilter) {
            long long parsed = 0;
            if (!m_earthquakeNotificationTimeText.empty() &&
                TryParseDateTimeFilter(m_earthquakeNotificationTimeText, parsed))
            {
                afterMs = parsed;
            }
        }
        else if (!IsAllPeriodText(m_earthquakeNotificationPeriodText)) {
            afterMs = PeriodStartTimeMs(m_earthquakeNotificationPeriodText);
        }

        if (afterMs > 0 && event.timeMs > 0 && event.timeMs < afterMs)
            return false;

        if (m_earthquakeNotificationPopulatedRadiusMiles > 0.0) {
            const double radiusMiles = EffectiveEarthquakePopulatedRadiusMiles(
                event.magnitude,
                m_earthquakeNotificationMagnitude,
                m_earthquakeNotificationPopulatedRadiusMiles);
            if (!EarthquakeIsWithinPopulatedAreaRadius(
                event,
                radiusMiles,
                m_earthquakeNotificationMinimumPopulation))
                return false;
        }

        return true;
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
        const std::vector<NotificationBatchItem>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        if (lines.empty())
            return;

        std::wstring title;
        std::wstring body;
        std::wstring sourceType;
        std::wstring sourceId;
        std::vector<AppNotificationLink> links;
        if (lines.size() == 1) {
            title = singleTitle;
            body = lines.front().line;
            sourceType = lines.front().sourceType;
            sourceId = lines.front().sourceId;
            links.push_back({ lines.front().line, lines.front().sourceType, lines.front().sourceId });
        }
        else {
            title = std::to_wstring(lines.size()) + L" " + pluralSuffix;
            const size_t displayCount = MinValue<size_t>(lines.size(), 3);
            for (size_t i = 0; i < displayCount; ++i) {
                if (!body.empty())
                    body += L"\r\n";
                body += lines[i].line;
                links.push_back({ lines[i].line, lines[i].sourceType, lines[i].sourceId });
            }
            if (lines.size() > displayCount)
                body += L"\r\n...";
        }

        PublishNotification(title, body, sourceType, sourceId, links);
    }

    void NotifyForMatchingEarthquakes(const std::vector<EarthquakeEvent>& events)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<NotificationBatchItem> newLines;
        std::vector<NotificationBatchItem> updateLines;
        std::vector<NotificationBatchItem> removedLines;

        for (const EarthquakeEvent& event : events) {
            std::wstring key = EarthquakeStableKey(event);
            currentKeys.insert(key);

            if (!EarthquakeMatchesNotification(event)) {
                auto existing = m_notifiedEarthquakeStates.find(key);
                if (m_haveEarthquakeNotificationSnapshot && existing != m_notifiedEarthquakeStates.end()) {
                    removedLines.push_back({ existing->second.line, L"earthquake", key });
                    m_notifiedEarthquakeStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = EarthquakeSignature(event);
            std::wstring line = EarthquakeNotificationLine(event);
            auto existing = m_notifiedEarthquakeStates.find(key);
            if (existing == m_notifiedEarthquakeStates.end()) {
                newLines.push_back({ line, L"earthquake", key });
                m_notifiedEarthquakeStates[key] = EarthquakeNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back({ line, L"earthquake", key });
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveEarthquakeNotificationSnapshot) {
            for (auto it = m_notifiedEarthquakeStates.begin(); it != m_notifiedEarthquakeStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back({ it->second.line, L"earthquake", it->first });
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
        return KnotsToMph(system.windKnots) + 0.0001 >= m_weatherSystemNotificationWindMph;
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
        signature += system.forecastCategory;
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.forecastWindKnots)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.forecastLatitude * 1000.0)));
        signature += L"|";
        signature += std::to_wstring(static_cast<int>(std::round(system.forecastLongitude * 1000.0)));
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
        if (system.windKnots > 0.0) {
            line += L" ";
            line += FormatKnotsAsMph(system.windKnots);
        }
        else if (!system.windText.empty()) {
            line += L" ";
            line += system.windText;
        }
        return line;
    }

    void PublishWeatherSystemNotificationBatch(
        const std::vector<NotificationBatchItem>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        PublishEarthquakeNotificationBatch(lines, singleTitle, pluralSuffix);
    }

    void NotifyForMatchingWeatherSystems(const std::vector<WeatherSystemEvent>& systems)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<NotificationBatchItem> newLines;
        std::vector<NotificationBatchItem> updateLines;
        std::vector<NotificationBatchItem> removedLines;

        for (const WeatherSystemEvent& system : systems) {
            std::wstring key = WeatherSystemStableKey(system);
            currentKeys.insert(key);

            if (!WeatherSystemMatchesNotification(system)) {
                auto existing = m_notifiedWeatherSystemStates.find(key);
                if (m_haveWeatherSystemNotificationSnapshot && existing != m_notifiedWeatherSystemStates.end()) {
                    removedLines.push_back({ existing->second.line, L"weather_system", key });
                    m_notifiedWeatherSystemStates.erase(existing);
                }
                continue;
            }

            std::wstring signature = WeatherSystemSignature(system);
            std::wstring line = WeatherSystemNotificationLine(system);
            auto existing = m_notifiedWeatherSystemStates.find(key);
            if (existing == m_notifiedWeatherSystemStates.end()) {
                newLines.push_back({ line, L"weather_system", key });
                m_notifiedWeatherSystemStates[key] = WeatherSystemNotificationState{ signature, line };
            }
            else {
                if (existing->second.signature != signature) {
                    updateLines.push_back({ line, L"weather_system", key });
                    existing->second.signature = std::move(signature);
                    existing->second.line = std::move(line);
                }
            }
        }

        if (m_haveWeatherSystemNotificationSnapshot) {
            for (auto it = m_notifiedWeatherSystemStates.begin(); it != m_notifiedWeatherSystemStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back({ it->second.line, L"weather_system", it->first });
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

    std::wstring WeatherWarningStableKey(const WeatherWarningEvent& warning) const
    {
        if (!warning.id.empty())
            return warning.id;
        return warning.colour + L"|" + warning.type + L"|" + warning.validFrom + L"|" + warning.headline;
    }

    std::wstring WeatherWarningSignature(const WeatherWarningEvent& warning) const
    {
        return warning.colour + L"|" + warning.type + L"|" + warning.area + L"|" +
            warning.validFrom + L"|" + warning.validTo + L"|" + warning.headline + L"|" + warning.issuedText;
    }

    std::wstring WeatherWarningNotificationLine(const WeatherWarningEvent& warning) const
    {
        std::wstring line = warning.colour.empty() ? L"Weather warning" : warning.colour + L" warning";
        if (!warning.type.empty()) {
            line += L" - ";
            line += warning.type;
        }
        if (!warning.area.empty()) {
            line += L" - ";
            line += warning.area;
        }
        if (!warning.validFrom.empty() || !warning.validTo.empty()) {
            line += L" (";
            line += warning.validFrom;
            if (!warning.validTo.empty()) {
                line += L" to ";
                line += warning.validTo;
            }
            line += L")";
        }
        return line;
    }

    std::wstring FloodStableKey(const FloodEvent& flood) const
    {
        if (!flood.id.empty())
            return flood.id;
        return flood.area + L"|" + flood.region + L"|" + flood.riverOrSea;
    }

    std::wstring FloodSignature(const FloodEvent& flood) const
    {
        return flood.severity + L"|" + flood.area + L"|" + flood.region + L"|" +
            flood.riverOrSea + L"|" + flood.message + L"|" + flood.timeChanged + L"|" +
            std::to_wstring(flood.severityLevel);
    }

    std::wstring FloodNotificationLine(const FloodEvent& flood) const
    {
        std::wstring line = flood.severity.empty() ? L"Flood alert" : flood.severity;
        if (!flood.area.empty()) {
            line += L" - ";
            line += flood.area;
        }
        if (!flood.riverOrSea.empty()) {
            line += L" - ";
            line += flood.riverOrSea;
        }
        return line;
    }

    void PublishWeatherWarningNotificationBatch(
        const std::vector<NotificationBatchItem>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        PublishEarthquakeNotificationBatch(lines, singleTitle, pluralSuffix);
    }

    void PublishFloodNotificationBatch(
        const std::vector<NotificationBatchItem>& lines,
        const std::wstring& singleTitle,
        const std::wstring& pluralSuffix)
    {
        PublishEarthquakeNotificationBatch(lines, singleTitle, pluralSuffix);
    }

    void NotifyForWeatherWarnings(const std::vector<WeatherWarningEvent>& warnings)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<NotificationBatchItem> newLines;
        std::vector<NotificationBatchItem> updateLines;
        std::vector<NotificationBatchItem> removedLines;

        for (const WeatherWarningEvent& warning : warnings) {
            std::wstring key = WeatherWarningStableKey(warning);
            currentKeys.insert(key);

            std::wstring signature = WeatherWarningSignature(warning);
            std::wstring line = WeatherWarningNotificationLine(warning);
            auto existing = m_notifiedWeatherWarningStates.find(key);
            if (existing == m_notifiedWeatherWarningStates.end()) {
                newLines.push_back({ line, L"weather_warning", key });
                m_notifiedWeatherWarningStates[key] = WeatherWarningNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back({ line, L"weather_warning", key });
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveWeatherWarningNotificationSnapshot) {
            for (auto it = m_notifiedWeatherWarningStates.begin(); it != m_notifiedWeatherWarningStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back({ it->second.line, L"weather_warning", it->first });
                    it = m_notifiedWeatherWarningStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveWeatherWarningNotificationSnapshot = true;
        PublishWeatherWarningNotificationBatch(newLines, L"Weather warning", L"weather warnings");
        PublishWeatherWarningNotificationBatch(updateLines, L"Weather warning update", L"weather warning updates");
        PublishWeatherWarningNotificationBatch(removedLines, L"Weather warning removed", L"weather warning removals");
    }

    void NotifyForFloods(const std::vector<FloodEvent>& floods)
    {
        std::unordered_set<std::wstring> currentKeys;
        std::vector<NotificationBatchItem> newLines;
        std::vector<NotificationBatchItem> updateLines;
        std::vector<NotificationBatchItem> removedLines;

        for (const FloodEvent& flood : floods) {
            std::wstring key = FloodStableKey(flood);
            currentKeys.insert(key);

            std::wstring signature = FloodSignature(flood);
            std::wstring line = FloodNotificationLine(flood);
            auto existing = m_notifiedFloodStates.find(key);
            if (existing == m_notifiedFloodStates.end()) {
                newLines.push_back({ line, L"flood", key });
                m_notifiedFloodStates[key] = FloodNotificationState{ signature, line };
            }
            else if (existing->second.signature != signature) {
                updateLines.push_back({ line, L"flood", key });
                existing->second.signature = std::move(signature);
                existing->second.line = std::move(line);
            }
        }

        if (m_haveFloodNotificationSnapshot) {
            for (auto it = m_notifiedFloodStates.begin(); it != m_notifiedFloodStates.end();) {
                if (currentKeys.find(it->first) == currentKeys.end()) {
                    removedLines.push_back({ it->second.line, L"flood", it->first });
                    it = m_notifiedFloodStates.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        m_haveFloodNotificationSnapshot = true;
        PublishFloodNotificationBatch(newLines, L"Flood notification", L"flood notifications");
        PublishFloodNotificationBatch(updateLines, L"Flood update", L"flood updates");
        PublishFloodNotificationBatch(removedLines, L"Flood removed", L"flood removals");
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
        case WM_NOTIFY:
            return OnEarthquakeListNotify(reinterpret_cast<NMHDR*>(lParam));
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
                1020,
                590,
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
        m_earthquakeListMagnitudeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 88, 120, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_MAG_EDIT), m_hInst, nullptr);
        m_earthquakeListDateRadio = CreateWindowExW(0, L"BUTTON", L"After date/time", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 210, 58, 150, 24, parent, ControlId(IDC_EARTHQUAKE_LIST_DATE_RADIO), m_hInst, nullptr);
        m_earthquakeListTimeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 210, 88, 180, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_TIME_EDIT), m_hInst, nullptr);
        m_earthquakeListPeriodRadio = CreateWindowExW(0, L"BUTTON", L"Period", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 430, 58, 96, 24, parent, ControlId(IDC_EARTHQUAKE_LIST_PERIOD_RADIO), m_hInst, nullptr);
        m_earthquakeListPeriodCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 430, 88, 130, 120, parent, ControlId(IDC_EARTHQUAKE_LIST_PERIOD_COMBO), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Populated area radius (mi)", 598, 58);
        m_earthquakeListPopulatedRadiusEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 598, 88, 150, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_POPULATED_RADIUS_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Minimum population", 780, 58);
        m_earthquakeListMinimumPopulationEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 780, 88, 150, 26, parent, ControlId(IDC_EARTHQUAKE_LIST_MINIMUM_POPULATION_EDIT), m_hInst, nullptr);
        m_earthquakeListRegionBtn = CreateWindowExW(0, L"BUTTON", L"Draw region", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 124, 118, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_REGION_BTN), m_hInst, nullptr);
        m_earthquakeListClearRegionBtn = CreateWindowExW(0, L"BUTTON", L"Clear region", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 148, 124, 118, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_CLEAR_REGION_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 858, 124, 102, 32, parent, ControlId(IDC_EARTHQUAKE_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_earthquakeListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 166, 950, 350, parent, ControlId(IDC_EARTHQUAKE_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { m_earthquakeListMagnitudeEdit, m_earthquakeListDateRadio, m_earthquakeListTimeEdit, m_earthquakeListPeriodRadio, m_earthquakeListPeriodCombo, m_earthquakeListPopulatedRadiusEdit, m_earthquakeListMinimumPopulationEdit, m_earthquakeListRegionBtn, m_earthquakeListClearRegionBtn, closeBtn, m_earthquakeListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_earthquakeListMagnitudeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2.5"));
        SendMessageW(m_earthquakeListTimeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2026-05-14 09:00"));
        SendMessageW(m_earthquakeListPopulatedRadiusEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Optional, e.g. 125"));
        SendMessageW(m_earthquakeListMinimumPopulationEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1"));
        PopulatePeriodCombo(m_earthquakeListPeriodCombo, m_earthquakeListPeriodText);
        SetWindowTextSafe(m_earthquakeListMagnitudeEdit, m_earthquakeListMagnitudeText);
        SetWindowTextSafe(m_earthquakeListTimeEdit, m_earthquakeListTimeText);
        SetWindowTextSafe(m_earthquakeListPopulatedRadiusEdit, m_earthquakeListPopulatedRadiusText);
        SetWindowTextSafe(m_earthquakeListMinimumPopulationEdit, m_earthquakeListMinimumPopulationText);
        SyncEarthquakeListDateModeControls();
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
        col.cx = 470;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 2, reinterpret_cast<LPARAM>(&col));
        std::wstring c3 = L"Depth km";
        col.pszText = const_cast<LPWSTR>(c3.c_str());
        col.cx = 100;
        SendMessageW(m_earthquakeListView, LVM_INSERTCOLUMNW, 3, reinterpret_cast<LPARAM>(&col));
        AutoFitWindowToChildren(parent);
    }

    void PopulatePeriodCombo(HWND combo, const std::wstring& selected)
    {
        if (!combo)
            return;

        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
        const wchar_t* periods[] = { L"All", L"24h", L"48h", L"72h", L"120h" };
        for (const wchar_t* period : periods)
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(period));

        std::wstring value = selected.empty() ? L"24h" : selected;
        LRESULT index = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(value.c_str()));
        if (index == CB_ERR)
            index = 0;
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }

    void PopulateForecastMinimumCombo(HWND combo, const std::wstring& selected)
    {
        if (!combo)
            return;

        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
        const wchar_t* options[] = { L"All", L"TD", L"TS", L"Cat 1", L"Cat 2", L"Cat 3", L"Cat 4", L"Cat 5" };
        for (const wchar_t* option : options)
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option));

        std::wstring value = selected.empty() ? L"All" : selected;
        LRESULT index = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(value.c_str()));
        if (index == CB_ERR)
            index = 0;
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }

    void SyncEarthquakeListDateModeControls()
    {
        if (m_earthquakeListDateRadio)
            SendMessageW(m_earthquakeListDateRadio, BM_SETCHECK, m_earthquakeListUseDateFilter ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_earthquakeListPeriodRadio)
            SendMessageW(m_earthquakeListPeriodRadio, BM_SETCHECK, m_earthquakeListUseDateFilter ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_earthquakeListTimeEdit)
            EnableWindow(m_earthquakeListTimeEdit, m_earthquakeListUseDateFilter);
        if (m_earthquakeListPeriodCombo)
            EnableWindow(m_earthquakeListPeriodCombo, !m_earthquakeListUseDateFilter);
    }

    void StoreEarthquakeListFiltersFromControls()
    {
        if (m_earthquakeListMagnitudeEdit)
            m_earthquakeListMagnitudeText = Trim(GetWindowTextString(m_earthquakeListMagnitudeEdit));
        if (m_earthquakeListTimeEdit)
            m_earthquakeListTimeText = Trim(GetWindowTextString(m_earthquakeListTimeEdit));
        if (m_earthquakeListDateRadio)
            m_earthquakeListUseDateFilter = SendMessageW(m_earthquakeListDateRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (m_earthquakeListPeriodCombo)
            m_earthquakeListPeriodText = Trim(GetWindowTextString(m_earthquakeListPeriodCombo));
        if (m_earthquakeListPopulatedRadiusEdit) {
            std::wstring radiusText = Trim(GetWindowTextString(m_earthquakeListPopulatedRadiusEdit));
            double parsedRadius = 0.0;
            if (radiusText.empty()) {
                m_earthquakeListPopulatedRadiusText.clear();
                m_earthquakeListPopulatedRadiusMiles = 0.0;
            }
            else if (TryParseDoubleText(radiusText, parsedRadius) && parsedRadius >= 0.0) {
                m_earthquakeListPopulatedRadiusText = radiusText;
                m_earthquakeListPopulatedRadiusMiles = parsedRadius;
            }
        }
        if (m_earthquakeListMinimumPopulationEdit) {
            std::wstring populationText = Trim(GetWindowTextString(m_earthquakeListMinimumPopulationEdit));
            double parsedPopulation = 0.0;
            if (TryParseDoubleText(populationText, parsedPopulation) && parsedPopulation >= 1.0) {
                m_earthquakeListMinimumPopulationText = populationText;
                m_earthquakeListMinimumPopulation = parsedPopulation;
            }
        }
    }

    bool EarthquakeMatchesListFilters(const EarthquakeEvent& event) const
    {
        double minMagnitude = 0.0;
        bool hasMinMagnitude = false;
        if (!m_earthquakeListMagnitudeText.empty() &&
            TryParseDoubleText(m_earthquakeListMagnitudeText, minMagnitude))
        {
            hasMinMagnitude = true;
            if (event.magnitude + 0.0001 < minMagnitude)
                return false;
        }

        long long afterMs = 0;
        if (m_earthquakeListUseDateFilter) {
            long long dateFilterMs = 0;
            if (!m_earthquakeListTimeText.empty() && TryParseDateTimeFilter(m_earthquakeListTimeText, dateFilterMs))
                afterMs = dateFilterMs;
        }
        else {
            afterMs = PeriodStartTimeMs(m_earthquakeListPeriodText);
        }
        if (event.timeMs > 0 && event.timeMs < afterMs)
        {
            return false;
        }

        if (m_earthquakeFilterRegion.size() >= 3) {
            if (!event.hasLocation || !PointInPolygon(event.latitude, event.longitude, m_earthquakeFilterRegion))
                return false;
        }

        if (m_earthquakeListPopulatedRadiusMiles > 0.0) {
            const double radiusMiles = hasMinMagnitude
                ? EffectiveEarthquakePopulatedRadiusMiles(event.magnitude, minMagnitude, m_earthquakeListPopulatedRadiusMiles)
                : m_earthquakeListPopulatedRadiusMiles;
            if (!EarthquakeIsWithinPopulatedAreaRadius(
                event,
                radiusMiles,
                m_earthquakeListMinimumPopulation))
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

    LRESULT OnEarthquakeListNotify(NMHDR* nmh)
    {
        if (!nmh || nmh->hwndFrom != m_earthquakeListView)
            return 0;
        if (nmh->code == NM_CUSTOMDRAW)
            return OnExcludedEventListCustomDraw(
                reinterpret_cast<NMLVCUSTOMDRAW*>(nmh),
                m_filteredEarthquakes);
        if (nmh->code == NM_RCLICK) {
            const int row = ListView_GetNextItem(m_earthquakeListView, -1, LVNI_SELECTED);
            if (row >= 0 && row < static_cast<int>(m_filteredEarthquakes.size())) {
                const EarthquakeEvent event = m_filteredEarthquakes[static_cast<size_t>(row)];
                const std::wstring key = EventExclusionKey(L"earthquake", EarthquakeStableKey(event));
                ShowExclusionContextMenu(key, event.excluded, m_earthquakeListView, [this, event]() {
                    AddEventExclusionAsync(event);
                    });
            }
            return 0;
        }
        if (nmh->code != LVN_ITEMCHANGED || m_syncingControls)
            return 0;

        NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
        if (lv->iItem < 0 || lv->iItem >= static_cast<int>(m_filteredEarthquakes.size()))
            return 0;
        if ((lv->uChanged & LVIF_STATE) == 0)
            return 0;

        const bool becameSelected = (lv->uNewState & LVIS_SELECTED) != 0 && (lv->uOldState & LVIS_SELECTED) == 0;
        if (becameSelected)
            SelectEarthquakeEventFromList(static_cast<size_t>(lv->iItem), true);

        return 0;
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
        if (id == IDC_EARTHQUAKE_LIST_POPULATED_RADIUS_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_earthquakeListPopulatedRadiusEdit));
            double parsed = 0.0;
            if (text.empty() || (TryParseDoubleText(text, parsed) && parsed >= 0.0)) {
                ApplyEarthquakeListFilters();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_earthquakeListPopulatedRadiusEdit, m_earthquakeListPopulatedRadiusText);
                SetStatusText(L"Populated area radius should be a distance in miles, e.g. 125.");
            }
            return;
        }
        if (id == IDC_EARTHQUAKE_LIST_MINIMUM_POPULATION_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_earthquakeListMinimumPopulationEdit));
            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 1.0) {
                ApplyEarthquakeListFilters();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_earthquakeListMinimumPopulationEdit, m_earthquakeListMinimumPopulationText);
                SetStatusText(L"Minimum population should be 1 or greater.");
            }
            return;
        }
        if ((id == IDC_EARTHQUAKE_LIST_MAG_EDIT || id == IDC_EARTHQUAKE_LIST_TIME_EDIT) &&
            (code == EN_CHANGE || code == EN_KILLFOCUS))
        {
            ApplyEarthquakeListFilters();
        }
        if ((id == IDC_EARTHQUAKE_LIST_DATE_RADIO || id == IDC_EARTHQUAKE_LIST_PERIOD_RADIO) && code == BN_CLICKED) {
            m_earthquakeListUseDateFilter = id == IDC_EARTHQUAKE_LIST_DATE_RADIO;
            SyncEarthquakeListDateModeControls();
            ApplyEarthquakeListFilters();
        }
        if (id == IDC_EARTHQUAKE_LIST_PERIOD_COMBO && code == CBN_SELCHANGE) {
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
                590,
                310,
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
        CreateAutoLabel(parent, 0, L"Notify when earthquakes match this magnitude and time window.", 18, 58, nullptr, 520);
        CreateAutoLabel(parent, 0, L"Minimum magnitude", 18, 104);
        m_earthquakeNotificationMagnitudeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 130, 120, 26, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_MAG_EDIT), m_hInst, nullptr);
        m_earthquakeNotificationDateRadio = CreateWindowExW(0, L"BUTTON", L"After date/time", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 170, 104, 150, 24, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_DATE_RADIO), m_hInst, nullptr);
        m_earthquakeNotificationTimeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 170, 130, 180, 26, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_TIME_EDIT), m_hInst, nullptr);
        m_earthquakeNotificationPeriodRadio = CreateWindowExW(0, L"BUTTON", L"Period", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 382, 104, 96, 24, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_RADIO), m_hInst, nullptr);
        m_earthquakeNotificationPeriodCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 382, 130, 130, 120, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_COMBO), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Populated area radius (mi)", 18, 178);
        m_earthquakeNotificationPopulatedRadiusEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 204, 120, 26, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_POPULATED_RADIUS_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Minimum population", 170, 178);
        m_earthquakeNotificationMinimumPopulationEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 170, 204, 150, 26, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_MINIMUM_POPULATION_EDIT), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 430, 204, 102, 32, parent, ControlId(IDC_EARTHQUAKE_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);
        for (HWND h : { m_earthquakeNotificationMagnitudeEdit, m_earthquakeNotificationDateRadio, m_earthquakeNotificationTimeEdit, m_earthquakeNotificationPeriodRadio, m_earthquakeNotificationPeriodCombo, m_earthquakeNotificationPopulatedRadiusEdit, m_earthquakeNotificationMinimumPopulationEdit, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_earthquakeNotificationMagnitudeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"4.0"));
        SendMessageW(m_earthquakeNotificationTimeEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"2026-05-14 09:00"));
        SendMessageW(m_earthquakeNotificationPopulatedRadiusEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Optional, e.g. 125"));
        SendMessageW(m_earthquakeNotificationMinimumPopulationEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"1"));
        PopulatePeriodCombo(m_earthquakeNotificationPeriodCombo, m_earthquakeNotificationPeriodText);
        SyncEarthquakeNotificationControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncEarthquakeNotificationControls()
    {
        m_syncingControls = true;
        if (m_earthquakeNotificationMagnitudeEdit)
            SetWindowTextSafe(m_earthquakeNotificationMagnitudeEdit, m_earthquakeNotificationMagnitudeText);
        if (m_earthquakeNotificationTimeEdit)
            SetWindowTextSafe(m_earthquakeNotificationTimeEdit, m_earthquakeNotificationTimeText);
        if (m_earthquakeNotificationPeriodCombo)
            PopulatePeriodCombo(m_earthquakeNotificationPeriodCombo, m_earthquakeNotificationPeriodText.empty() ? L"All" : m_earthquakeNotificationPeriodText);
        if (m_earthquakeNotificationPopulatedRadiusEdit)
            SetWindowTextSafe(m_earthquakeNotificationPopulatedRadiusEdit, m_earthquakeNotificationPopulatedRadiusText);
        if (m_earthquakeNotificationMinimumPopulationEdit)
            SetWindowTextSafe(m_earthquakeNotificationMinimumPopulationEdit, m_earthquakeNotificationMinimumPopulationText);
        if (m_earthquakeNotificationDateRadio)
            SendMessageW(m_earthquakeNotificationDateRadio, BM_SETCHECK, m_earthquakeNotificationUseDateFilter ? BST_CHECKED : BST_UNCHECKED, 0);
        if (m_earthquakeNotificationPeriodRadio)
            SendMessageW(m_earthquakeNotificationPeriodRadio, BM_SETCHECK, m_earthquakeNotificationUseDateFilter ? BST_UNCHECKED : BST_CHECKED, 0);
        if (m_earthquakeNotificationTimeEdit)
            EnableWindow(m_earthquakeNotificationTimeEdit, m_earthquakeNotificationUseDateFilter);
        if (m_earthquakeNotificationPeriodCombo)
            EnableWindow(m_earthquakeNotificationPeriodCombo, !m_earthquakeNotificationUseDateFilter);
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
        if ((id == IDC_EARTHQUAKE_NOTIFICATIONS_DATE_RADIO || id == IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_RADIO) && code == BN_CLICKED) {
            m_earthquakeNotificationUseDateFilter = id == IDC_EARTHQUAKE_NOTIFICATIONS_DATE_RADIO;
            SyncEarthquakeNotificationControls();
            SaveSettings();
            return;
        }
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_TIME_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            m_earthquakeNotificationTimeText = Trim(GetWindowTextString(m_earthquakeNotificationTimeEdit));
            if (code == EN_KILLFOCUS && !m_earthquakeNotificationTimeText.empty()) {
                long long parsed = 0;
                if (!TryParseDateTimeFilter(m_earthquakeNotificationTimeText, parsed))
                    SetStatusText(L"Earthquake notification date should be like 2026-05-14 09:00.");
            }
            SaveSettings();
            return;
        }
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_PERIOD_COMBO && code == CBN_SELCHANGE) {
            m_earthquakeNotificationPeriodText = Trim(GetWindowTextString(m_earthquakeNotificationPeriodCombo));
            SaveSettings();
            return;
        }
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_POPULATED_RADIUS_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_earthquakeNotificationPopulatedRadiusEdit));
            if (text.empty()) {
                m_earthquakeNotificationPopulatedRadiusText.clear();
                m_earthquakeNotificationPopulatedRadiusMiles = 0.0;
                SaveSettings();
                return;
            }

            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 0.0) {
                m_earthquakeNotificationPopulatedRadiusText = text;
                m_earthquakeNotificationPopulatedRadiusMiles = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_earthquakeNotificationPopulatedRadiusEdit, m_earthquakeNotificationPopulatedRadiusText);
                SetStatusText(L"Populated area radius should be a distance in miles, e.g. 125.");
            }
            return;
        }
        if (id == IDC_EARTHQUAKE_NOTIFICATIONS_MINIMUM_POPULATION_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_earthquakeNotificationMinimumPopulationEdit));
            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 1.0) {
                m_earthquakeNotificationMinimumPopulationText = text;
                m_earthquakeNotificationMinimumPopulation = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_earthquakeNotificationMinimumPopulationEdit, m_earthquakeNotificationMinimumPopulationText);
                SetStatusText(L"Minimum population should be 1 or greater.");
            }
            return;
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
        case WM_NOTIFY:
            return OnWeatherSystemsListNotify(reinterpret_cast<NMHDR*>(lParam));
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
        CreateAutoLabel(parent, 0, L"Forecast minimum", 18, 58);
        m_weatherSystemsListForecastCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 84, 160, 160, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_FORECAST_COMBO), m_hInst, nullptr);
        HWND refreshBtn = CreateWindowExW(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 626, 54, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_REFRESH_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 742, 54, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_weatherSystemsListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 146, 826, 306, parent, ControlId(IDC_WEATHER_SYSTEMS_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { m_weatherSystemsListForecastCombo, refreshBtn, closeBtn, m_weatherSystemsListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        PopulateForecastMinimumCombo(m_weatherSystemsListForecastCombo, m_weatherSystemsListForecastText);
        SendMessageW(m_weatherSystemsListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

        struct ColumnDef { const wchar_t* text; int width; };
        const ColumnDef columns[] = {
            { L"System", 140 },
            { L"Basin", 120 },
            { L"Lat", 70 },
            { L"Long", 80 },
            { L"Wind", 80 },
            { L"Cat", 70 },
            { L"Peak Lat", 90 },
            { L"Peak Long", 95 },
            { L"Peak Wind", 100 },
            { L"Peak Cat", 95 }
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

    const WeatherForecastPoint* WeatherSystemPeakForecastForList(const WeatherSystemEvent& system) const
    {
        const WeatherForecastPoint* best = nullptr;
        for (const WeatherForecastPoint& point : system.forecastTrack) {
            if (!point.hasLocation || point.leadHours <= 0)
                continue;
            const int pointRank = WeatherSystemEffectiveCategoryRank(point.category, point.windKnots);
            const int bestRank = best ? WeatherSystemEffectiveCategoryRank(best->category, best->windKnots) : -1;
            if (!best || pointRank > bestRank || (pointRank == bestRank && point.leadHours > best->leadHours))
                best = &point;
        }
        return best;
    }

    static std::wstring DisplayWeatherSystemCategory(const std::wstring& category, double windKnots)
    {
        const int categoryRank = WeatherSystemCategoryRank(category);
        const int effectiveRank = WeatherSystemEffectiveCategoryRank(category, windKnots);
        if (effectiveRank > categoryRank)
            return WeatherSystemCategoryRankName(effectiveRank);
        return category;
    }

    void RenderWeatherSystemsListRows()
    {
        if (!m_weatherSystemsListView)
            return;

        m_syncingControls = true;
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
            const bool forecastVisible = m_hiddenWeatherSystemForecastIds.find(WeatherSystemStableKey(system)) == m_hiddenWeatherSystemForecastIds.end();
            ListView_SetCheckState(m_weatherSystemsListView, inserted, forecastVisible);

            std::wstring lat = system.hasLocation ? FormatCoordinateForList(system.latitude, true) : L"";
            std::wstring lon = system.hasLocation ? FormatCoordinateForList(system.longitude, false) : L"";
            const WeatherForecastPoint* forecast = WeatherSystemPeakForecastForList(system);
            std::wstring forecastLat;
            std::wstring forecastLon;
            std::wstring forecastWind;
            std::wstring forecastCategory;
            if (forecast) {
                forecastLat = FormatCoordinateForList(forecast->latitude, true);
                forecastLon = FormatCoordinateForList(forecast->longitude, false);
                forecastWind = forecast->windText;
                forecastCategory = DisplayWeatherSystemCategory(forecast->category, forecast->windKnots);
            }
            else if (system.hasForecastLocation) {
                forecastLat = FormatCoordinateForList(system.forecastLatitude, true);
                forecastLon = FormatCoordinateForList(system.forecastLongitude, false);
                forecastWind = system.forecastWindText;
                forecastCategory = DisplayWeatherSystemCategory(system.forecastCategory, system.forecastWindKnots);
            }
            const std::wstring values[] = {
                system.basin,
                lat,
                lon,
                system.windText,
                DisplayWeatherSystemCategory(system.category, system.windKnots),
                forecastLat,
                forecastLon,
                forecastWind,
                forecastCategory
            };
            for (int i = 0; i < static_cast<int>(_countof(values)); ++i) {
                LVITEMW sub{};
                sub.iSubItem = i + 1;
                sub.pszText = const_cast<LPWSTR>(values[i].c_str());
                SendMessageW(m_weatherSystemsListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
            }
            ++row;
        }
        m_syncingControls = false;
    }

    LRESULT OnWeatherSystemsListNotify(NMHDR* nmh)
    {
        if (!nmh || nmh->hwndFrom != m_weatherSystemsListView)
            return 0;
        if (nmh->code == NM_CUSTOMDRAW)
            return OnExcludedEventListCustomDraw(
                reinterpret_cast<NMLVCUSTOMDRAW*>(nmh),
                m_filteredWeatherSystems);
        if (nmh->code == NM_RCLICK) {
            const int row = ListView_GetNextItem(m_weatherSystemsListView, -1, LVNI_SELECTED);
            if (row >= 0 && row < static_cast<int>(m_filteredWeatherSystems.size())) {
                const WeatherSystemEvent event = m_filteredWeatherSystems[static_cast<size_t>(row)];
                const std::wstring key = EventExclusionKey(L"weather_system", WeatherSystemStableKey(event));
                ShowExclusionContextMenu(key, event.excluded, m_weatherSystemsListView, [this, event]() {
                    AddEventExclusionAsync(event);
                    });
            }
            return 0;
        }
        if (nmh->code != LVN_ITEMCHANGED || m_syncingControls)
            return 0;

        NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
        if (lv->iItem < 0 || lv->iItem >= static_cast<int>(m_filteredWeatherSystems.size()))
            return 0;
        if ((lv->uChanged & LVIF_STATE) == 0)
            return 0;

        const bool becameSelected = (lv->uNewState & LVIS_SELECTED) != 0 && (lv->uOldState & LVIS_SELECTED) == 0;
        if (becameSelected)
            SelectWeatherSystemEventFromList(static_cast<size_t>(lv->iItem), true);

        const UINT oldCheck = lv->uOldState & LVIS_STATEIMAGEMASK;
        const UINT newCheck = lv->uNewState & LVIS_STATEIMAGEMASK;
        if (oldCheck == newCheck)
            return 0;

        const std::wstring id = WeatherSystemStableKey(m_filteredWeatherSystems[static_cast<size_t>(lv->iItem)]);
        if (ListView_GetCheckState(m_weatherSystemsListView, lv->iItem))
            m_hiddenWeatherSystemForecastIds.erase(id);
        else
            m_hiddenWeatherSystemForecastIds.insert(id);
        ApplyWeatherSystemVisibility();
        SaveSettings();
        return 0;
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
        if (id == IDC_WEATHER_SYSTEMS_LIST_FORECAST_COMBO && code == CBN_SELCHANGE) {
            ApplyWeatherSystemsListFilter(true);
            return;
        }
    }

    static LRESULT CALLBACK WeatherWarningsListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleWeatherWarningsListMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleWeatherWarningsListMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateWeatherWarningsListControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnWeatherWarningsListCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_NOTIFY:
            return OnWeatherWarningsListNotify(reinterpret_cast<NMHDR*>(lParam));
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

    void ShowWeatherWarningsListWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = WeatherWarningsListWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kWeatherWarningsListClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_weatherWarningsListWnd || !IsWindow(m_weatherWarningsListWnd)) {
            m_weatherWarningsListWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kWeatherWarningsListClassName,
                L"Weather Warnings",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                480,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        RenderWeatherWarningsListRows();
        ShowWindow(m_weatherWarningsListWnd, SW_SHOW);
        SetForegroundWindow(m_weatherWarningsListWnd);
        if (m_allWeatherWarnings.empty())
            FetchWeatherWarningsAsync(false);
    }

    void CreateWeatherWarningsListControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Weather Warnings", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Period", 18, 58);
        m_weatherWarningsListPeriodCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 84, 130, 120, parent, ControlId(IDC_WEATHER_WARNINGS_LIST_PERIOD_COMBO), m_hInst, nullptr);
        HWND refreshBtn = CreateWindowExW(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 626, 54, 102, 32, parent, ControlId(IDC_WEATHER_WARNINGS_LIST_REFRESH_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 742, 54, 102, 32, parent, ControlId(IDC_WEATHER_WARNINGS_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_weatherWarningsListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 146, 826, 286, parent, ControlId(IDC_WEATHER_WARNINGS_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { m_weatherWarningsListPeriodCombo, refreshBtn, closeBtn, m_weatherWarningsListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        PopulatePeriodCombo(m_weatherWarningsListPeriodCombo, m_weatherWarningsListPeriodText);
        SendMessageW(m_weatherWarningsListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

        struct ColumnDef { const wchar_t* text; int width; };
        const ColumnDef columns[] = {
            { L"Colour", 80 },
            { L"Type", 120 },
            { L"Area", 260 },
            { L"From", 120 },
            { L"To", 120 },
            { L"Headline", 260 }
        };
        for (int i = 0; i < static_cast<int>(_countof(columns)); ++i) {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<LPWSTR>(columns[i].text);
            col.cx = columns[i].width;
            col.iSubItem = i;
            SendMessageW(m_weatherWarningsListView, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&col));
        }
        RenderWeatherWarningsListRows();
        AutoFitWindowToChildren(parent);
    }

    void RenderWeatherWarningsListRows()
    {
        if (!m_weatherWarningsListView)
            return;

        m_syncingControls = true;
        SendMessageW(m_weatherWarningsListView, LVM_DELETEALLITEMS, 0, 0);
        int row = 0;
        for (const WeatherWarningEvent& warning : m_filteredWeatherWarnings) {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(warning.colour.c_str());
            int inserted = static_cast<int>(SendMessageW(m_weatherWarningsListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (inserted < 0)
                continue;
            const bool polygonVisible = m_hiddenWeatherWarningPolygonIds.find(WeatherWarningStableKey(warning)) == m_hiddenWeatherWarningPolygonIds.end();
            ListView_SetCheckState(m_weatherWarningsListView, inserted, polygonVisible);

            const std::wstring values[] = {
                warning.type,
                warning.area,
                warning.validFrom,
                warning.validTo,
                warning.headline
            };
            for (int i = 0; i < static_cast<int>(_countof(values)); ++i) {
                LVITEMW sub{};
                sub.iSubItem = i + 1;
                sub.pszText = const_cast<LPWSTR>(values[i].c_str());
                SendMessageW(m_weatherWarningsListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
            }
            ++row;
        }
        m_syncingControls = false;
    }

    LRESULT OnWeatherWarningsListNotify(NMHDR* nmh)
    {
        if (!nmh || nmh->hwndFrom != m_weatherWarningsListView)
            return 0;
        if (nmh->code == NM_CUSTOMDRAW)
            return OnExcludedEventListCustomDraw(
                reinterpret_cast<NMLVCUSTOMDRAW*>(nmh),
                m_filteredWeatherWarnings);
        if (nmh->code == NM_RCLICK) {
            const int row = ListView_GetNextItem(m_weatherWarningsListView, -1, LVNI_SELECTED);
            if (row >= 0 && row < static_cast<int>(m_filteredWeatherWarnings.size())) {
                const WeatherWarningEvent event = m_filteredWeatherWarnings[static_cast<size_t>(row)];
                const std::wstring key = EventExclusionKey(L"weather_warning", WeatherWarningStableKey(event));
                ShowExclusionContextMenu(key, event.excluded, m_weatherWarningsListView, [this, event]() {
                    AddEventExclusionAsync(event);
                    });
            }
            return 0;
        }
        if (nmh->code != LVN_ITEMCHANGED || m_syncingControls)
            return 0;

        NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
        if (lv->iItem < 0 || lv->iItem >= static_cast<int>(m_filteredWeatherWarnings.size()))
            return 0;
        if ((lv->uChanged & LVIF_STATE) == 0)
            return 0;

        const bool becameSelected = (lv->uNewState & LVIS_SELECTED) != 0 && (lv->uOldState & LVIS_SELECTED) == 0;
        if (becameSelected)
            SelectWeatherWarningEventFromList(static_cast<size_t>(lv->iItem), true);

        const UINT oldCheck = lv->uOldState & LVIS_STATEIMAGEMASK;
        const UINT newCheck = lv->uNewState & LVIS_STATEIMAGEMASK;
        if (oldCheck == newCheck)
            return 0;

        const std::wstring id = WeatherWarningStableKey(m_filteredWeatherWarnings[static_cast<size_t>(lv->iItem)]);
        if (ListView_GetCheckState(m_weatherWarningsListView, lv->iItem))
            m_hiddenWeatherWarningPolygonIds.erase(id);
        else
            m_hiddenWeatherWarningPolygonIds.insert(id);
        ApplyWeatherWarningVisibility();
        SaveSettings();
        return 0;
    }

    void OnWeatherWarningsListCommand(int id, int code)
    {
        if (id == IDC_WEATHER_WARNINGS_LIST_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_weatherWarningsListWnd, SW_HIDE);
            return;
        }
        if (id == IDC_WEATHER_WARNINGS_LIST_REFRESH_BTN && code == BN_CLICKED) {
            FetchWeatherWarningsAsync(false);
            return;
        }
        if (id == IDC_WEATHER_WARNINGS_LIST_PERIOD_COMBO && code == CBN_SELCHANGE) {
            ApplyWeatherWarningsListFilter(true);
            return;
        }
    }

    static LRESULT CALLBACK FloodsListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleFloodsListMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleFloodsListMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateFloodsListControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnFloodsListCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_NOTIFY:
            return OnFloodsListNotify(reinterpret_cast<NMHDR*>(lParam));
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

    void ShowFloodsListWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = FloodsListWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kFloodsListClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_floodsListWnd || !IsWindow(m_floodsListWnd)) {
            m_floodsListWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kFloodsListClassName,
                L"Floods",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                480,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }

        RenderFloodsListRows();
        ShowWindow(m_floodsListWnd, SW_SHOW);
        SetForegroundWindow(m_floodsListWnd);
        if (m_allFloods.empty())
            FetchFloodsAsync(false);
    }

    void CreateFloodsListControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Floods", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Period", 18, 58);
        m_floodsListPeriodCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 84, 130, 120, parent, ControlId(IDC_FLOODS_LIST_PERIOD_COMBO), m_hInst, nullptr);
        HWND refreshBtn = CreateWindowExW(0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 626, 54, 102, 32, parent, ControlId(IDC_FLOODS_LIST_REFRESH_BTN), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 742, 54, 102, 32, parent, ControlId(IDC_FLOODS_LIST_CLOSE_BTN), m_hInst, nullptr);
        m_floodsListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 18, 146, 826, 286, parent, ControlId(IDC_FLOODS_LIST_LISTVIEW), m_hInst, nullptr);

        for (HWND h : { m_floodsListPeriodCombo, refreshBtn, closeBtn, m_floodsListView }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        PopulatePeriodCombo(m_floodsListPeriodCombo, m_floodsListPeriodText);
        SendMessageW(m_floodsListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

        struct ColumnDef { const wchar_t* text; int width; };
        const ColumnDef columns[] = {
            { L"Severity", 150 },
            { L"Area", 280 },
            { L"Region", 130 },
            { L"River/Sea", 170 },
            { L"Updated", 140 },
            { L"Message", 260 }
        };
        for (int i = 0; i < static_cast<int>(_countof(columns)); ++i) {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.pszText = const_cast<LPWSTR>(columns[i].text);
            col.cx = columns[i].width;
            col.iSubItem = i;
            SendMessageW(m_floodsListView, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&col));
        }
        RenderFloodsListRows();
        AutoFitWindowToChildren(parent);
    }

    void RenderFloodsListRows()
    {
        if (!m_floodsListView)
            return;

        SendMessageW(m_floodsListView, LVM_DELETEALLITEMS, 0, 0);
        int row = 0;
        for (const FloodEvent& flood : m_filteredFloods) {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(flood.severity.c_str());
            int inserted = static_cast<int>(SendMessageW(m_floodsListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
            if (inserted < 0)
                continue;

            const std::wstring values[] = {
                flood.area,
                flood.region,
                flood.riverOrSea,
                flood.timeChanged.empty() ? flood.timeRaised : flood.timeChanged,
                flood.message
            };
            for (int i = 0; i < static_cast<int>(_countof(values)); ++i) {
                LVITEMW sub{};
                sub.iSubItem = i + 1;
                sub.pszText = const_cast<LPWSTR>(values[i].c_str());
                SendMessageW(m_floodsListView, LVM_SETITEMTEXTW, inserted, reinterpret_cast<LPARAM>(&sub));
            }
            ++row;
        }
    }

    LRESULT OnFloodsListNotify(NMHDR* nmh)
    {
        if (!nmh || nmh->hwndFrom != m_floodsListView)
            return 0;
        if (nmh->code == NM_CUSTOMDRAW)
            return OnExcludedEventListCustomDraw(
                reinterpret_cast<NMLVCUSTOMDRAW*>(nmh),
                m_filteredFloods);
        if (nmh->code == NM_RCLICK) {
            const int row = ListView_GetNextItem(m_floodsListView, -1, LVNI_SELECTED);
            if (row >= 0 && row < static_cast<int>(m_filteredFloods.size())) {
                const FloodEvent event = m_filteredFloods[static_cast<size_t>(row)];
                const std::wstring key = EventExclusionKey(L"flood", FloodStableKey(event));
                ShowExclusionContextMenu(key, event.excluded, m_floodsListView, [this, event]() {
                    AddEventExclusionAsync(event);
                    });
            }
            return 0;
        }
        if (nmh->code != LVN_ITEMCHANGED || m_syncingControls)
            return 0;

        NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
        if (lv->iItem < 0 || lv->iItem >= static_cast<int>(m_filteredFloods.size()))
            return 0;
        if ((lv->uChanged & LVIF_STATE) == 0)
            return 0;

        const bool becameSelected = (lv->uNewState & LVIS_SELECTED) != 0 && (lv->uOldState & LVIS_SELECTED) == 0;
        if (becameSelected)
            SelectFloodEventFromList(static_cast<size_t>(lv->iItem), true);

        return 0;
    }

    void OnFloodsListCommand(int id, int code)
    {
        if (id == IDC_FLOODS_LIST_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_floodsListWnd, SW_HIDE);
            return;
        }
        if (id == IDC_FLOODS_LIST_REFRESH_BTN && code == BN_CLICKED) {
            FetchFloodsAsync(false);
            return;
        }
        if (id == IDC_FLOODS_LIST_PERIOD_COMBO && code == CBN_SELCHANGE) {
            ApplyFloodsListFilter(true);
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
        CreateAutoLabel(parent, 0, L"Minimum wind (mph)", 18, 104);
        m_weatherSystemNotificationWindEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 130, 120, 26, parent, ControlId(IDC_WEATHER_SYSTEM_NOTIFICATIONS_WIND_EDIT), m_hInst, nullptr);
        HWND closeBtn = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 130, 102, 32, parent, ControlId(IDC_WEATHER_SYSTEM_NOTIFICATIONS_CLOSE_BTN), m_hInst, nullptr);
        for (HWND h : { m_weatherSystemNotificationWindEdit, closeBtn }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_weatherSystemNotificationWindEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"39"));
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
                m_weatherSystemNotificationWindMph = parsed;
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_weatherSystemNotificationWindEdit, m_weatherSystemNotificationWindText);
                SetStatusText(L"Weather system wind should be a number such as 39.");
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
        bool disabled = (dis->itemState & ODS_DISABLED) != 0;

        HBRUSH parentBg = CreateSolidBrush(kUiBackground);
        FillRect(dis->hDC, &dis->rcItem, parentBg);
        DeleteObject(parentBg);

        RECT buttonRect = dis->rcItem;
        InflateRect(&buttonRect, -1, -1);

        COLORREF fillColor = disabled
            ? RGB(174, 187, 202)
            : (pressed ? RGB(21, 92, 171) : (hot ? RGB(32, 124, 229) : RGB(0, 103, 192)));
        HBRUSH fill = CreateSolidBrush(fillColor);
        HPEN pen = CreatePen(PS_SOLID, 1, disabled ? RGB(151, 164, 180) : RGB(88, 166, 255));
        HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
        HGDIOBJ oldBrush = SelectObject(dis->hDC, fill);
        RoundRect(dis->hDC, buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom, 10, 10);
        SelectObject(dis->hDC, oldBrush);
        SelectObject(dis->hDC, oldPen);
        DeleteObject(pen);
        DeleteObject(fill);

        wchar_t text[128]{};
        GetWindowTextW(dis->hwndItem, text, _countof(text));
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, disabled ? RGB(238, 242, 247) : RGB(255, 255, 255));
        HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dis->hDC, m_font));
        RECT textRc = buttonRect;
        InflateRect(&textRc, -8, -2);
        SIZE preferred = MeasureControlText(dis->hwndItem);
        const bool needsWrap = preferred.cx > (textRc.right - textRc.left) || preferred.cy + 8 > (buttonRect.bottom - buttonRect.top);
        if (needsWrap) {
            RECT calcRc = textRc;
            DrawTextW(dis->hDC, text, -1, &calcRc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
            int textH = calcRc.bottom - calcRc.top;
            if (textH > 0 && textH < (textRc.bottom - textRc.top)) {
                int offset = ((textRc.bottom - textRc.top) - textH) / 2;
                textRc.top += offset;
            }
            DrawTextW(dis->hDC, text, -1, &textRc, DT_CENTER | DT_WORDBREAK);
        }
        else {
            DrawTextW(dis->hDC, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
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

    static LRESULT CALLBACK SoundsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleSoundsMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
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
                CW_USEDEFAULT, CW_USEDEFAULT, 470, 640, m_hwnd, nullptr, m_hInst, this);
        }
        SyncSettingsControls();
        ShowWindow(m_settingsWnd, SW_SHOW);
        SetForegroundWindow(m_settingsWnd);
    }

    LRESULT HandleSoundsMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            CreateSoundsControls(hwnd);
            return 0;
        case WM_COMMAND:
            OnSoundsCommand(LOWORD(wParam), HIWORD(wParam));
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

    void ShowSoundsWindow()
    {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = SoundsWndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = ModernWindowBrush();
            wc.lpszClassName = kSoundsClassName;
            RegisterClassExW(&wc);
            registered = true;
        }

        if (!m_soundsWnd || !IsWindow(m_soundsWnd)) {
            m_soundsWnd = CreateWindowExW(
                WS_EX_TOOLWINDOW,
                kSoundsClassName,
                L"Sounds",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                500,
                420,
                m_hwnd,
                nullptr,
                m_hInst,
                this);
        }
        SyncSoundsControls();
        ShowWindow(m_soundsWnd, SW_SHOW);
        SetForegroundWindow(m_soundsWnd);
    }

    void CreateSoundsControls(HWND parent)
    {
        CreateAutoLabel(parent, 0, L"Sounds", 18, 18, m_headerFont);
        CreateAutoLabel(parent, 0, L"Choose where ERC Tools plays cues and which actions should make a sound.", 18, 58, nullptr, 418);

        CreateAutoLabel(parent, 0, L"Audio output device", 18, 104);
        m_soundsDeviceCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            18,
            130,
            426,
            180,
            parent,
            ControlId(IDC_SOUNDS_DEVICE_COMBO),
            m_hInst,
            nullptr);

        m_soundsMasterCheck = CreateWindowExW(0, L"BUTTON", L"Enable sounds", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 18, 176, 170, 24, parent, ControlId(IDC_SOUNDS_MASTER_CHECK), m_hInst, nullptr);
        m_soundsNotificationCheck = CreateWindowExW(0, L"BUTTON", L"Notifications", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 18, 218, 180, 24, parent, ControlId(IDC_SOUNDS_NOTIFICATION_CHECK), m_hInst, nullptr);
        m_soundsMessageCheck = CreateWindowExW(0, L"BUTTON", L"Responder chat messages", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 220, 218, 220, 24, parent, ControlId(IDC_SOUNDS_MESSAGE_CHECK), m_hInst, nullptr);
        m_soundsPrivateMessageCheck = CreateWindowExW(0, L"BUTTON", L"Private messages", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 18, 252, 180, 24, parent, ControlId(IDC_SOUNDS_PRIVATE_MESSAGE_CHECK), m_hInst, nullptr);
        m_soundsTimerStartCheck = CreateWindowExW(0, L"BUTTON", L"Countdown start", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 220, 252, 180, 24, parent, ControlId(IDC_SOUNDS_TIMER_START_CHECK), m_hInst, nullptr);
        m_soundsTimerWarningCheck = CreateWindowExW(0, L"BUTTON", L"Countdown warning", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 18, 286, 190, 24, parent, ControlId(IDC_SOUNDS_TIMER_WARNING_CHECK), m_hInst, nullptr);
        m_soundsTimerCompleteCheck = CreateWindowExW(0, L"BUTTON", L"Countdown complete", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 220, 286, 190, 24, parent, ControlId(IDC_SOUNDS_TIMER_COMPLETE_CHECK), m_hInst, nullptr);

        HWND test = CreateWindowExW(0, L"BUTTON", L"Test", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 234, 332, 102, 32, parent, ControlId(IDC_SOUNDS_TEST_BTN), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_PUSHBUTTON, 342, 332, 102, 32, parent, ControlId(IDC_SOUNDS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : {
            m_soundsDeviceCombo,
            m_soundsMasterCheck,
            m_soundsNotificationCheck,
            m_soundsMessageCheck,
            m_soundsPrivateMessageCheck,
            m_soundsTimerStartCheck,
            m_soundsTimerWarningCheck,
            m_soundsTimerCompleteCheck,
            test,
            close
            }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }

        SyncSoundsControls();
        AutoFitWindowToChildren(parent);
    }

    void SyncSoundsControls()
    {
        if (!m_soundsWnd)
            return;

        m_syncingControls = true;

        if (m_soundsDeviceCombo) {
            SendMessageW(m_soundsDeviceCombo, CB_RESETCONTENT, 0, 0);
            m_soundOutputDevices = EnumerateSoundOutputDevices();
            int selectedIndex = -1;
            for (const SoundOutputDevice& device : m_soundOutputDevices) {
                const std::wstring label = device.isDefault ? device.name : L"Device: " + device.name;
                const int index = static_cast<int>(SendMessageW(m_soundsDeviceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
                if (index >= 0) {
                    SendMessageW(m_soundsDeviceCombo, CB_SETITEMDATA, index, static_cast<LPARAM>(device.id));
                    if (device.id == m_soundOutputDeviceId)
                        selectedIndex = index;
                }
            }
            if (selectedIndex < 0) {
                selectedIndex = 0;
                m_soundOutputDeviceId = kDefaultSoundOutputDeviceId;
                ApplySoundSettings();
            }
            SendMessageW(m_soundsDeviceCombo, CB_SETCURSEL, selectedIndex, 0);
        }

        auto setCheck = [](HWND control, bool checked) {
            if (control)
                SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            };
        setCheck(m_soundsMasterCheck, m_soundCuesEnabled);
        setCheck(m_soundsMessageCheck, m_soundMessageEnabled);
        setCheck(m_soundsPrivateMessageCheck, m_soundPrivateMessageEnabled);
        setCheck(m_soundsNotificationCheck, m_soundNotificationEnabled);
        setCheck(m_soundsTimerStartCheck, m_soundTimerStartEnabled);
        setCheck(m_soundsTimerWarningCheck, m_soundTimerWarningEnabled);
        setCheck(m_soundsTimerCompleteCheck, m_soundTimerCompleteEnabled);

        for (HWND control : {
            m_soundsMessageCheck,
            m_soundsPrivateMessageCheck,
            m_soundsNotificationCheck,
            m_soundsTimerStartCheck,
            m_soundsTimerWarningCheck,
            m_soundsTimerCompleteCheck
            }) {
            if (control)
                EnableWindow(control, m_soundCuesEnabled);
        }

        m_syncingControls = false;
    }

    void OnSoundsCommand(int id, int code)
    {
        if (m_syncingControls)
            return;

        if (id == IDC_SOUNDS_DEVICE_COMBO && code == CBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageW(m_soundsDeviceCombo, CB_GETCURSEL, 0, 0));
            if (index >= 0) {
                const LRESULT data = SendMessageW(m_soundsDeviceCombo, CB_GETITEMDATA, index, 0);
                if (data != CB_ERR) {
                    m_soundOutputDeviceId = static_cast<unsigned int>(data);
                    ApplySoundSettings();
                    SaveSettings();
                    SetStatusText(L"Sound output device updated.");
                }
            }
        }
        else if (id == IDC_SOUNDS_MASTER_CHECK && code == BN_CLICKED) {
            m_soundCuesEnabled = SendMessageW(m_soundsMasterCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            ApplySoundSettings();
            SyncSoundsControls();
            SaveSettings();
        }
        else if (code == BN_CLICKED &&
            (id == IDC_SOUNDS_MESSAGE_CHECK ||
                id == IDC_SOUNDS_PRIVATE_MESSAGE_CHECK ||
                id == IDC_SOUNDS_NOTIFICATION_CHECK ||
                id == IDC_SOUNDS_TIMER_START_CHECK ||
                id == IDC_SOUNDS_TIMER_WARNING_CHECK ||
                id == IDC_SOUNDS_TIMER_COMPLETE_CHECK)) {
            m_soundMessageEnabled = SendMessageW(m_soundsMessageCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_soundPrivateMessageEnabled = SendMessageW(m_soundsPrivateMessageCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_soundNotificationEnabled = SendMessageW(m_soundsNotificationCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_soundTimerStartEnabled = SendMessageW(m_soundsTimerStartCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_soundTimerWarningEnabled = SendMessageW(m_soundsTimerWarningCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_soundTimerCompleteEnabled = SendMessageW(m_soundsTimerCompleteCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            ApplySoundSettings();
            SaveSettings();
        }
        else if (id == IDC_SOUNDS_TEST_BTN && code == BN_CLICKED) {
            if (!m_soundCuesEnabled) {
                SetStatusText(L"Enable sounds before testing a cue.");
                return;
            }
            if (m_soundPrivateMessageEnabled)
                PlaySoundCue(SoundCue::PrivateMessage);
            else if (m_soundNotificationEnabled)
                PlaySoundCue(SoundCue::Notification);
            else if (m_soundMessageEnabled)
                PlaySoundCue(SoundCue::Message);
            else if (m_soundTimerCompleteEnabled)
                PlaySoundCue(SoundCue::TimerComplete);
            else
                SetStatusText(L"Enable at least one sound cue before testing.");
        }
        else if (id == IDC_SOUNDS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_soundsWnd, SW_HIDE);
        }
    }

    void CreateSettingsControls(HWND parent)
    {
        m_serverLabel = CreateAutoLabel(parent, IDC_SETTINGS_SERVER_LABEL, L"Collaboration and data server", 18, 18);
        m_serverEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 44, 410, 26, parent, ControlId(IDC_SERVER_EDIT), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_SYNC_LABEL, L"Remote Settings", 18, 86);
        m_settingsSyncLocalRadio = CreateWindowExW(0, L"BUTTON", L"Local settings", WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON, 18, 112, 130, 24, parent, ControlId(IDC_SETTINGS_SYNC_LOCAL_RADIO), m_hInst, nullptr);
        m_settingsSyncServerRadio = CreateWindowExW(0, L"BUTTON", L"Remote Settings", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 178, 112, 160, 24, parent, ControlId(IDC_SETTINGS_SYNC_SERVER_RADIO), m_hInst, nullptr);
        m_settingsPushRemoteBtn = CreateWindowExW(0, L"BUTTON", L"Push Settings Remotely", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 18, 146, 220, 32, parent, ControlId(IDC_SETTINGS_PUSH_REMOTE_BTN), m_hInst, nullptr);
        CreateAutoLabel(parent, 0, L"Earthquake populated radius ratio (mi per +0.1 mag)", 18, 198);
        m_settingsEarthquakeRadiusRatioEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 18, 224, 120, 26, parent, ControlId(IDC_SETTINGS_EARTHQUAKE_RADIUS_RATIO_EDIT), m_hInst, nullptr);
        m_settingsNotificationAvoidanceCheck = CreateWindowExW(
            0,
            L"BUTTON",
            L"Move map overlays aside for notifications",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            18,
            264,
            320,
            26,
            parent,
            ControlId(IDC_SETTINGS_NOTIFICATION_AVOIDANCE_CHECK),
            m_hInst,
            nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_FILTER_LABEL, L"Road incident filter", 18, 306);
        m_settingsFilterCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 332, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_FILTER), m_hInst, nullptr);
        CreateAutoLabel(parent, IDC_SETTINGS_ORDER_LABEL, L"Incident display order", 18, 370);
        m_settingsOrderCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 18, 396, 410, 160, parent, ControlId(IDC_SETTINGS_ALERT_ORDER), m_hInst, nullptr);
        HWND close = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_PUSHBUTTON, 326, 442, 102, 32, parent, ControlId(IDC_SETTINGS_CLOSE_BTN), m_hInst, nullptr);

        for (HWND h : { m_serverEdit, m_settingsSyncLocalRadio, m_settingsSyncServerRadio, m_settingsPushRemoteBtn, m_settingsEarthquakeRadiusRatioEdit, m_settingsNotificationAvoidanceCheck, m_settingsFilterCombo, m_settingsOrderCombo, close }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_serverEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"213.254.181.35:8081"));
        SendMessageW(m_settingsEarthquakeRadiusRatioEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"10"));

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
        if (m_serverEdit)
            SetWindowTextSafe(m_serverEdit, m_serverBaseUrl);
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
        if (m_settingsPushRemoteBtn) {
            const bool canPush = IsOnlineMode() && CanManageAccounts();
            ShowWindow(m_settingsPushRemoteBtn, canPush ? SW_SHOW : SW_HIDE);
            EnableWindow(m_settingsPushRemoteBtn, canPush);
        }
        if (m_settingsEarthquakeRadiusRatioEdit)
            SetWindowTextSafe(m_settingsEarthquakeRadiusRatioEdit, m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText);
        if (m_settingsNotificationAvoidanceCheck)
            SendMessageW(
                m_settingsNotificationAvoidanceCheck,
                BM_SETCHECK,
                m_avoidOverlaysForNotifications ? BST_CHECKED : BST_UNCHECKED,
                0);
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

        if (id == IDC_SERVER_EDIT && code == EN_CHANGE) {
            m_serverBaseUrl = NormalizeUrl(GetWindowTextString(m_serverEdit));
            SaveSettings();
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
        else if (id == IDC_SETTINGS_PUSH_REMOTE_BTN && code == BN_CLICKED) {
            PushGlobalSettingsToServerAsync();
        }
        else if (id == IDC_SETTINGS_EARTHQUAKE_RADIUS_RATIO_EDIT && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            std::wstring text = Trim(GetWindowTextString(m_settingsEarthquakeRadiusRatioEdit));
            double parsed = 0.0;
            if (TryParseDoubleText(text, parsed) && parsed >= 0.0) {
                m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText = text;
                m_earthquakePopulatedRadiusMilesPerMagnitudeTenth = parsed;
                ApplyEarthquakeListFilters();
                SaveSettings();
            }
            else if (code == EN_KILLFOCUS) {
                SetWindowTextSafe(m_settingsEarthquakeRadiusRatioEdit, m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText);
                SetStatusText(L"Earthquake radius ratio should be miles per +0.1 magnitude, e.g. 10.");
            }
        }
        else if (id == IDC_SETTINGS_NOTIFICATION_AVOIDANCE_CHECK && code == BN_CLICKED) {
            m_avoidOverlaysForNotifications =
                SendMessageW(m_settingsNotificationAvoidanceCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            m_map.SetNotificationAvoidanceEnabled(m_avoidOverlaysForNotifications);
            SaveSettings();
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
        else if (id == IDC_SETTINGS_CLOSE_BTN && code == BN_CLICKED) {
            ShowWindow(m_settingsWnd, SW_HIDE);
        }
    }

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInst = nullptr;
    HFONT m_font = nullptr;
    HFONT m_boldFont = nullptr;
    HFONT m_headerFont = nullptr;

    HWND m_searchLabel = nullptr;
    HWND m_severityLabel = nullptr;
    HWND m_statusBar = nullptr;
    HWND m_serverLabel = nullptr;
    HWND m_panelTabBtn = nullptr;
    HWND m_serverEdit = nullptr;
    HWND m_searchEdit = nullptr;
    HWND m_severityCombo = nullptr;
    HWND m_listView = nullptr;
    HWND m_detailsEdit = nullptr;
    HWND m_settingsWnd = nullptr;
    HWND m_soundsWnd = nullptr;
    HWND m_cacheManagerWnd = nullptr;
    HWND m_incidentFiltersWnd = nullptr;
    HWND m_incidentNotificationsWnd = nullptr;
    HWND m_notificationRegionsWnd = nullptr;
    HWND m_earthquakeListWnd = nullptr;
    HWND m_earthquakeNotificationsWnd = nullptr;
    HWND m_templatesWizardWnd = nullptr;
    HWND m_templatesEditorWnd = nullptr;
    HWND m_accountCreatorWnd = nullptr;
    HWND m_adminLogWnd = nullptr;
    HWND m_adminLogTypeCombo = nullptr;
    HWND m_adminLogListView = nullptr;
    HWND m_legendWnd = nullptr;
    HWND m_roadDepictionsWnd = nullptr;
    HWND m_roadDepictionsList = nullptr;
    HWND m_roadDepictionsRoadEdit = nullptr;
    HWND m_settingsFilterCombo = nullptr;
    HWND m_settingsOrderCombo = nullptr;
    HWND m_settingsWorldOffRadio = nullptr;
    HWND m_settingsWorldOnRadio = nullptr;
    HWND m_settingsSyncLocalRadio = nullptr;
    HWND m_settingsSyncServerRadio = nullptr;
    HWND m_settingsPushRemoteBtn = nullptr;
    HWND m_settingsEarthquakeRadiusRatioEdit = nullptr;
    HWND m_settingsNotificationAvoidanceCheck = nullptr;
    HWND m_settingsWorldBoundaryBtn = nullptr;
    HWND m_settingsRoadsBtn = nullptr;
    HWND m_soundsDeviceCombo = nullptr;
    HWND m_soundsMasterCheck = nullptr;
    HWND m_soundsMessageCheck = nullptr;
    HWND m_soundsPrivateMessageCheck = nullptr;
    HWND m_soundsNotificationCheck = nullptr;
    HWND m_soundsTimerStartCheck = nullptr;
    HWND m_soundsTimerWarningCheck = nullptr;
    HWND m_soundsTimerCompleteCheck = nullptr;
    HWND m_cacheProgressBar = nullptr;
    HWND m_cacheStatusLabel = nullptr;
    HWND m_incidentSevereCheck = nullptr;
    HWND m_incidentModerateCheck = nullptr;
    HWND m_incidentMinorCheck = nullptr;
    HWND m_incidentUnknownCheck = nullptr;
    HWND m_incidentUnplannedCheck = nullptr;
    HWND m_incidentPlannedCheck = nullptr;
    HWND m_incidentSidePanelListOnlyCheck = nullptr;
    HWND m_incidentShowUnresolvedCheck = nullptr;
    HWND m_incidentNotifyRoadsEdit = nullptr;
    HWND m_incidentNotifyRoadExclusionsEdit = nullptr;
    HWND m_incidentNotifyLaneThresholdEdit = nullptr;
    HWND m_incidentNotifyDelayThresholdEdit = nullptr;
    HWND m_incidentNotifyAndRadio = nullptr;
    HWND m_incidentNotifyOrRadio = nullptr;
    HWND m_incidentNotifyUpdatesRadio = nullptr;
    HWND m_incidentIgnoreUpdatesRadio = nullptr;
    HWND m_incidentNotifyRegionsBtn = nullptr;
    HWND m_incidentNotifyReasonExclusionsEdit = nullptr;
    HWND m_incidentNotifyLocationExclusionsEdit = nullptr;
    HWND m_notificationRegionsShowPolygonsCheck = nullptr;
    HWND m_notificationRegionsList = nullptr;
    HWND m_trafficScotlandWnd = nullptr;
    HWND m_trafficScotlandEnabledCheck = nullptr;
    HWND m_trafficScotlandUrlEdit = nullptr;
    HWND m_trafficScotlandStatusLabel = nullptr;
    HWND m_incidentExclusionsWnd = nullptr;
    HWND m_incidentExclusionsList = nullptr;
    HWND m_incidentsListWnd = nullptr;
    HWND m_incidentsListSearchEdit = nullptr;
    HWND m_incidentsListSeverityCombo = nullptr;
    HWND m_incidentsListView = nullptr;
    HWND m_earthquakeListMagnitudeEdit = nullptr;
    HWND m_earthquakeListDateRadio = nullptr;
    HWND m_earthquakeListTimeEdit = nullptr;
    HWND m_earthquakeListPeriodRadio = nullptr;
    HWND m_earthquakeListPeriodCombo = nullptr;
    HWND m_earthquakeListPopulatedRadiusEdit = nullptr;
    HWND m_earthquakeListMinimumPopulationEdit = nullptr;
    HWND m_earthquakeListRegionBtn = nullptr;
    HWND m_earthquakeListClearRegionBtn = nullptr;
    HWND m_earthquakeListView = nullptr;
    HWND m_earthquakeNotificationMagnitudeEdit = nullptr;
    HWND m_earthquakeNotificationDateRadio = nullptr;
    HWND m_earthquakeNotificationTimeEdit = nullptr;
    HWND m_earthquakeNotificationPeriodRadio = nullptr;
    HWND m_earthquakeNotificationPeriodCombo = nullptr;
    HWND m_earthquakeNotificationPopulatedRadiusEdit = nullptr;
    HWND m_earthquakeNotificationMinimumPopulationEdit = nullptr;
    HWND m_weatherSystemsListWnd = nullptr;
    HWND m_weatherSystemNotificationsWnd = nullptr;
    HWND m_weatherSystemsListView = nullptr;
    HWND m_weatherSystemsListForecastCombo = nullptr;
    HWND m_weatherSystemNotificationWindEdit = nullptr;
    HWND m_weatherWarningsListWnd = nullptr;
    HWND m_weatherWarningsListView = nullptr;
    HWND m_weatherWarningsListPeriodCombo = nullptr;
    HWND m_floodsListWnd = nullptr;
    HWND m_floodsListView = nullptr;
    HWND m_floodsListPeriodCombo = nullptr;
    HWND m_templateWizardDesc = nullptr;
    HWND m_templateWizardList = nullptr;
    HWND m_templateWizardVariablesEdit = nullptr;
    HWND m_templateWizardTitlePreviewLabel = nullptr;
    HWND m_templateWizardTitlePreviewEdit = nullptr;
    HWND m_templateWizardBodyPreviewLabel = nullptr;
    HWND m_templateWizardPreviewEdit = nullptr;
    HWND m_templateWizardPrevBtn = nullptr;
    HWND m_templateWizardNextBtn = nullptr;
    HWND m_templateWizardCopyTitleBtn = nullptr;
    HWND m_templateWizardCopyBtn = nullptr;
    HWND m_templateWizardCopyLocationBtn = nullptr;
    HWND m_templateEditorWeatherTypeLabel = nullptr;
    HWND m_templateEditorWeatherTypeCombo = nullptr;
    HWND m_templateEditorList = nullptr;
    HWND m_templateEditorNameEdit = nullptr;
    HWND m_templateEditorTitleEdit = nullptr;
    HWND m_templateEditorBodyEdit = nullptr;
    HWND m_accountCreatorUsernameEdit = nullptr;
    HWND m_accountCreatorDisplayNameEdit = nullptr;
    HWND m_accountCreatorPasswordEdit = nullptr;
    HWND m_accountCreatorPositionCombo = nullptr;
    HWND m_accountCreatorActiveCheck = nullptr;
    HWND m_accountCreatorStatusLabel = nullptr;
    HWND m_accountCreatorCreateBtn = nullptr;

    MapView m_map;
    ClientSession m_session;

    std::vector<TrafficAlert> m_allAlerts;
    std::vector<TrafficAlert> m_filteredAlerts;
    std::vector<ChatMessage> m_chatMessages;
    std::vector<PrivateMessage> m_privateMessages;
    std::vector<MapNote> m_notes;
    std::vector<OnlineUser> m_onlineUsers;
    std::vector<AdminLogEntry> m_adminLogEntries;
    std::unordered_map<std::wstring, MapNote> m_pendingNoteEdits;
    std::vector<AppNotification> m_notificationHistory;
    std::vector<GeoPolygon> m_incidentNotificationRegions;
    std::vector<IncidentExclusion> m_incidentExclusions;
    std::vector<ReportTemplate> m_reportTemplates;
    std::vector<ReportTemplate> m_earthquakeReportTemplates;
    std::vector<ReportTemplate> m_weatherSystemReportTemplates;
    std::vector<ReportTemplate> m_weatherWarningReportTemplates;
    std::vector<ReportTemplate> m_floodReportTemplates;
    std::vector<std::pair<std::wstring, std::wstring>> m_templateWizardVariables;
    std::vector<TemplateEditableRange> m_templateWizardTitleEditableRanges;
    std::vector<TemplateEditableRange> m_templateWizardBodyEditableRanges;
    std::vector<PopulatedPlace> m_populatedPlaces;
    std::vector<EarthquakeEvent> m_allEarthquakes;
    std::vector<EarthquakeEvent> m_filteredEarthquakes;
    std::vector<WeatherSystemEvent> m_allWeatherSystems;
    std::vector<WeatherSystemEvent> m_filteredWeatherSystems;
    std::vector<WeatherWarningEvent> m_allWeatherWarnings;
    std::vector<WeatherWarningEvent> m_filteredWeatherWarnings;
    std::vector<FloodEvent> m_allFloods;
    std::vector<FloodEvent> m_filteredFloods;
    std::vector<GeoPoint> m_earthquakeFilterRegion;
    std::vector<std::wstring> m_incidentsListKeys;
    std::wstring m_selectedId;
    std::wstring m_templateWizardAlertId;
    std::wstring m_templateWizardEarthquakeId;
    std::wstring m_templateWizardWeatherSystemId;
    std::wstring m_templateWizardWeatherWarningId;
    std::wstring m_templateWizardFloodId;
    size_t m_templateWizardStep = 0;
    size_t m_templateWizardTemplateIndex = 0;
    TemplateContext m_templateWizardContext = TemplateContext::Roads;
    TemplateContext m_templateEditorContext = TemplateContext::Roads;
    bool m_templateWizardWeatherChooser = false;
    bool m_templateEditorWeatherMode = false;
    bool m_programmaticSelection = false;
    bool m_syncingControls = false;
    bool m_isSidePanelVisible = true;
    bool m_showNotificationHistory = false;
    bool m_showUsersOverlay = false;
    bool m_alertFilterUnplannedOnly = true;
    bool m_incidentFilterSevere = true;
    bool m_incidentFilterModerate = true;
    bool m_incidentFilterMinor = true;
    bool m_incidentFilterUnknown = true;
    bool m_incidentFilterUnplanned = true;
    bool m_incidentFilterPlanned = true;
    bool m_incidentSidePanelListOnly = false;
    bool m_showUnresolvedIncidents = false;
    std::wstring m_incidentNotifyRoads = L"M*, A*, A1(M), A2, A15, A16, A17, A20, A4, A52";
    std::wstring m_incidentNotifyRoadExclusions;
    std::wstring m_incidentNotifyLaneThresholdText = L"50%";
    double m_incidentNotifyLaneThreshold = 50.0;
    std::wstring m_incidentNotifyDelayThresholdText = L"1 hour";
    double m_incidentNotifyDelayThresholdMinutes = 60.0;
    bool m_incidentNotifyThresholdUseOr = false;
    bool m_incidentIgnoreUpdates = false;
    bool m_showIncidentNotificationRegionPolygons = true;
    bool m_trafficScotlandEnabled = true;
    std::wstring m_trafficScotlandIncidentsUrl = L"https://www.traffic.gov.scot/traffic-information/incidents";
    std::wstring m_incidentNotifyReasonExclusions = L"Road Management";
    std::wstring m_incidentNotifyLocationExclusions = L"entry, exit";
    bool m_showEarthquakes = false;
    bool m_showEarthquakeOverlayLabels = false;
    bool m_showIncidents = true;
    bool m_showIncidentOverlayLabels = false;
    bool m_incidentOverlayNotifiedOnly = false;
    bool m_showWeatherSystems = false;
    bool m_showWeatherSystemOverlayLabels = false;
    bool m_showWeatherSystemForecasts = true;
    bool m_showWeatherWarnings = false;
    bool m_showWeatherWarningOverlayLabels = false;
    bool m_showWeatherWarningPolygons = true;
    bool m_showFloods = false;
    bool m_showFloodOverlayLabels = false;
    bool m_showAreaLabels = true;
    bool m_showRoadDepictions = false;
    bool m_showFpsCounter = false;
    bool m_showMapControls = true;
    bool m_showCountdownTimer = false;
    bool m_showCommsIndicator = true;
    std::array<std::wstring, 3> m_countdownPresets{ L"05:00", L"10:00", L"15:00" };
    bool m_avoidOverlaysForNotifications = true;
    bool m_soundCuesEnabled = true;
    unsigned int m_soundOutputDeviceId = kDefaultSoundOutputDeviceId;
    bool m_soundMessageEnabled = true;
    bool m_soundPrivateMessageEnabled = true;
    bool m_soundNotificationEnabled = true;
    bool m_soundTimerStartEnabled = true;
    bool m_soundTimerWarningEnabled = true;
    bool m_soundTimerCompleteEnabled = true;
    std::vector<SoundOutputDevice> m_soundOutputDevices;
    bool m_chatSoundBaselineReady = false;
    bool m_privateMessageSoundBaselineReady = false;
    std::unordered_map<std::wstring, size_t> m_privateMessageUnreadCounts;
    std::wstring m_activePrivateChatUsername;
    bool m_displayWorldMap = false;
    bool m_showIreland = true;
    bool m_syncSettingsFromServer = false;
    bool m_populatedPlacesLoaded = false;
    bool m_populatedPlacesLoadAttempted = false;
    bool m_populatedPlacesDownloadAttempted = false;
    bool m_cacheBoundaryBusy = false;
    bool m_cacheRoadDepictionsBusy = false;
    bool m_cachePopulatedPlacesBusy = false;
    std::wstring m_earthquakeListMagnitudeText;
    std::wstring m_earthquakeListTimeText;
    bool m_earthquakeListUseDateFilter = false;
    std::wstring m_earthquakeListPeriodText = L"24h";
    std::wstring m_earthquakeListPopulatedRadiusText;
    double m_earthquakeListPopulatedRadiusMiles = 0.0;
    std::wstring m_earthquakeListMinimumPopulationText = L"1";
    double m_earthquakeListMinimumPopulation = 1.0;
    std::wstring m_earthquakePopulatedRadiusMilesPerMagnitudeTenthText = L"10";
    double m_earthquakePopulatedRadiusMilesPerMagnitudeTenth = 10.0;
    std::wstring m_weatherSystemsListForecastText = L"All";
    std::wstring m_weatherWarningsListPeriodText = L"24h";
    std::wstring m_floodsListPeriodText = L"24h";
    std::wstring m_earthquakeNotificationMagnitudeText = L"4.0";
    double m_earthquakeNotificationMagnitude = 4.0;
    std::wstring m_earthquakeNotificationTimeText;
    bool m_earthquakeNotificationUseDateFilter = false;
    std::wstring m_earthquakeNotificationPeriodText = L"All";
    std::wstring m_earthquakeNotificationPopulatedRadiusText;
    double m_earthquakeNotificationPopulatedRadiusMiles = 0.0;
    std::wstring m_earthquakeNotificationMinimumPopulationText = L"1";
    double m_earthquakeNotificationMinimumPopulation = 1.0;
    std::wstring m_weatherSystemNotificationWindText = L"39";
    double m_weatherSystemNotificationWindMph = 39.0;
    std::wstring m_alertOrder = L"Road";
    std::wstring m_alertsEndpoint;
    std::wstring m_serverBaseUrl = L"http://213.254.181.35:8081";
    bool m_periodicRefreshEnabled = false;
    bool m_hasLoadedAlerts = false;
    std::wstring m_refreshIntervalText = L"300s";
    UINT m_refreshIntervalMs = 5 * 60 * 1000;
    uint32_t m_collaborationVersion = 0;
    std::atomic_bool m_serverRequestInProgress{ false };
    std::atomic_bool m_earthquakeFetchInProgress{ false };
    std::atomic_bool m_weatherWarningsFetchInProgress{ false };
    std::atomic_bool m_floodsFetchInProgress{ false };
    std::atomic<uint32_t> m_serverSourceSyncEpoch{ 1 };
    std::atomic_bool m_roadsSourceWaitInProgress{ false };
    std::atomic_bool m_earthquakeSourceWaitInProgress{ false };
    std::atomic_bool m_weatherSystemsSourceWaitInProgress{ false };
    std::atomic_bool m_weatherWarningsSourceWaitInProgress{ false };
    std::atomic_bool m_floodsSourceWaitInProgress{ false };
    uint32_t m_roadsSourceGeneration = 0;
    uint32_t m_earthquakeSourceGeneration = 0;
    uint32_t m_weatherSystemsSourceGeneration = 0;
    uint32_t m_weatherWarningsSourceGeneration = 0;
    uint32_t m_floodsSourceGeneration = 0;
    bool m_notificationIconAdded = false;
    bool m_logoutRequested = false;
    bool m_logoutSent = false;
    bool m_haveIncidentNotificationSnapshot = false;
    bool m_haveEarthquakeNotificationSnapshot = false;
    bool m_haveWeatherSystemNotificationSnapshot = false;
    bool m_haveWeatherWarningNotificationSnapshot = false;
    bool m_haveFloodNotificationSnapshot = false;
    bool m_reportTemplatesConfigured = false;
    bool m_earthquakeReportTemplatesConfigured = false;
    bool m_weatherSystemReportTemplatesConfigured = false;
    bool m_weatherWarningReportTemplatesConfigured = false;
    bool m_floodReportTemplatesConfigured = false;
    std::unordered_map<std::wstring, IncidentNotificationState> m_notifiedIncidentStates;
    std::unordered_map<std::wstring, EarthquakeNotificationState> m_notifiedEarthquakeStates;
    std::unordered_map<std::wstring, WeatherSystemNotificationState> m_notifiedWeatherSystemStates;
    std::unordered_map<std::wstring, WeatherWarningNotificationState> m_notifiedWeatherWarningStates;
    std::unordered_map<std::wstring, FloodNotificationState> m_notifiedFloodStates;
    std::unordered_set<std::wstring> m_hiddenWeatherSystemForecastIds;
    std::unordered_set<std::wstring> m_hiddenWeatherWarningPolygonIds;
    std::unordered_set<std::wstring> m_incidentExclusionKeys;
    std::vector<std::wstring> m_roadDepictionRoadLabels = DefaultRoadDepictionRoadLabels();
    std::unordered_set<std::wstring> m_hiddenRoadDepictionIds;
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
