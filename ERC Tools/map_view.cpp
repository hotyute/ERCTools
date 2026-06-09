// =================================================================================
// FILE: map_view.cpp
// =================================================================================

#include "map_view.h"
#include "app_state.h"
#include "http.h"
#include "map_overlay_ui.h"
#include "parsing.h"
#include "road_data.h"
#include "util.h"
#include "weather_intensity.h"

struct TileKey
{
    int z{};
    int x{};
    int y{};
};

inline bool operator==(const TileKey& a, const TileKey& b) noexcept
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
    ULONGLONG lastUsedMs = 0;
    std::vector<BYTE> bytes;
    ComPtr<ID2D1Bitmap> bitmap;
};

struct BoundarySegment
{
    GeoPoint a;
    GeoPoint b;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
};

struct BoundaryLod
{
    std::vector<GeoPoint> points;
    std::vector<BoundarySegment> segmentsByMinLat;
    ComPtr<ID2D1PathGeometry> fillGeometry;
    int fillGeometryZoom = -1;
    double geometryMinX = 0.0;
    double geometryMaxX = 0.0;
    double geometryMinY = 0.0;
    double geometryMaxY = 0.0;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
};

struct BoundaryRing
{
    std::vector<GeoPoint> points;
    std::vector<BoundarySegment> segmentsByMinLat;
    ComPtr<ID2D1PathGeometry> fillGeometry;
    int fillGeometryZoom = -1;
    double geometryMinX = 0.0;
    double geometryMaxX = 0.0;
    double geometryMinY = 0.0;
    double geometryMaxY = 0.0;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
    std::vector<BoundaryLod> worldLods;
};

struct BoundaryRenderSource
{
    const std::vector<BoundarySegment>* segments = nullptr;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
};

struct SceneTileKey
{
    int z = 0;
    int x = 0;
    int y = 0;
    bool world = false;
};

inline bool operator==(const SceneTileKey& a, const SceneTileKey& b) noexcept
{
    return a.z == b.z && a.x == b.x && a.y == b.y && a.world == b.world;
}

struct SceneTileKeyHash
{
    std::size_t operator()(const SceneTileKey& k) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(k.z);
        std::size_t h2 = std::hash<int>{}(k.x);
        std::size_t h3 = std::hash<int>{}(k.y);
        std::size_t h4 = std::hash<bool>{}(k.world);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

struct SceneTileEntry
{
    bool loading = false;
    bool failed = false;
    ULONGLONG lastUsedMs = 0;
    ULONGLONG lastAttemptMs = 0;
    ComPtr<ID2D1Bitmap> bitmap;
};

static bool RoadDepictionLabelMatches(const std::wstring& a, const std::wstring& b)
{
    std::wstring left = ToLower(Trim(a));
    std::wstring right = ToLower(Trim(b));
    return !left.empty() && left == right;
}

static bool IsRoadDepictionHidden(const std::vector<std::wstring>& hiddenRoadLabels, const std::wstring& label)
{
    for (const std::wstring& hidden : hiddenRoadLabels) {
        if (RoadDepictionLabelMatches(hidden, label))
            return true;
    }
    return false;
}

static bool IsRoadDepictionHidden(const std::unordered_set<std::wstring>& hiddenRoadLabels, const std::wstring& label)
{
    for (const std::wstring& hidden : hiddenRoadLabels) {
        if (RoadDepictionLabelMatches(hidden, label))
            return true;
    }
    return false;
}

static std::unordered_set<std::wstring> NormalizeRoadDepictionHiddenLabels(const std::unordered_set<std::wstring>& hiddenRoadLabels)
{
    std::unordered_set<std::wstring> normalized;
    normalized.reserve(hiddenRoadLabels.size());
    for (const std::wstring& hidden : hiddenRoadLabels) {
        std::wstring key = ToLower(Trim(hidden));
        if (!key.empty())
            normalized.insert(std::move(key));
    }
    return normalized;
}

static bool IsRoadDepictionHiddenFast(const std::unordered_set<std::wstring>& normalizedHiddenRoadLabels, const RoadDepictionRoute& route)
{
    if (normalizedHiddenRoadLabels.empty())
        return false;
    if (!route.normalizedLabel.empty())
        return normalizedHiddenRoadLabels.find(route.normalizedLabel) != normalizedHiddenRoadLabels.end();
    return normalizedHiddenRoadLabels.find(ToLower(Trim(route.label))) != normalizedHiddenRoadLabels.end();
}

// Tile/cache tuning.
constexpr int kMaxConcurrentTileDownloads = 6;
constexpr size_t kMaxTileCacheEntries = 768;
constexpr int kMaxFallbackTileZoomDelta = 1;
constexpr int kMaxTileBitmapDecodesPerFrame = 3;
constexpr int kMaxInteractiveTileRequestsPerFrame = 2;
constexpr int kAlertFocusZoom = 9;

// Interaction timing.
constexpr UINT_PTR kInteractionIdleTimer = 1;
constexpr UINT_PTR kOverlayAnimationTimer = 2;
constexpr UINT_PTR kSceneCacheRefreshTimer = 3;
constexpr UINT WM_APP_SCENE_CACHE_READY = WM_APP + 60;
constexpr UINT WM_APP_SCENE_TILE_READY = WM_APP + 61;
constexpr UINT kInteractionIdleMs = 120;
constexpr UINT kOverlayAnimationMs = 220;
constexpr UINT kSceneCacheRefreshMs = 450;
constexpr UINT kSceneCacheMinRebuildIntervalMs = 1200;

// Map styling.
constexpr float kMapWaterR = 0.80f;
constexpr float kMapWaterG = 0.91f;
constexpr float kMapWaterB = 0.98f;
constexpr float kOverlayUiMargin = 12.0f;
constexpr float kOverlayUiPadding = 14.0f;
constexpr float kOverlayUiGap = 10.0f;
constexpr float kOverlayTogglePadding = 8.0f;
constexpr float kOverlayToggleSize = 28.0f;
constexpr float kNotificationScrollStep = 56.0f;
constexpr float kNoteBubbleWidth = 204.0f;
constexpr float kNoteBubbleHeight = 64.0f;
constexpr float kNoteBubbleMaxHeight = 154.0f;
constexpr float kNoteEditorWidth = 300.0f;
constexpr float kNoteEditorMinHeight = 144.0f;

// Boundary rendering.
constexpr double kBoundaryDrawMarginPixels = 512.0;
constexpr int kSceneCachePanMarginPixels = 1024;
constexpr int kSceneCacheMaxScaledZoomDelta = 0;
constexpr int kSceneTileSize = 512;
constexpr size_t kMaxSceneTileCacheEntries = 256;
constexpr int kMaxSceneTileBuildsInFlight = 2;
constexpr int kMaxSceneTileRequestsPerPaint = 4;
constexpr int kSceneTilePrefetchMarginTiles = 1;
constexpr int kFullBoundaryMaxZoom = 7;
constexpr int kWorldCachedGeographyMaxZoom = 7;
constexpr int kWorldGeometryFillMaxZoom = 13;
constexpr double kWorldLodDetailToleranceDegrees = 0.012;
constexpr double kWorldLodMidToleranceDegrees = 0.035;
constexpr double kWorldLodFarToleranceDegrees = 0.09;
constexpr double kWorldLodGlobalToleranceDegrees = 0.22;

std::atomic<int> g_activeTileDownloads{ 0 };

template <typename Fn>
static void ScheduleMapTask(Fn&& fn)
{
    std::thread(std::forward<Fn>(fn)).detach();
}

// ============================================================
// MapView
// ============================================================

class MapView::Impl
{
    enum class NoteEditorMode
    {
        None,
        New,
        Edit
    };

    enum class OverlayInputFocus
    {
        None,
        NoteEditor,
        ResponderChat,
        PrivateChat
    };

    struct PolygonPointHit
    {
        bool hit = false;
        size_t polygonIndex = static_cast<size_t>(-1);
        size_t pointIndex = static_cast<size_t>(-1);
    };

public:
    using SelectCallback = std::function<void(const std::wstring&)>;
    using NoteCreateCallback = std::function<void(const std::wstring& text, double lat, double lon)>;
    using NoteUpdateCallback = std::function<void(size_t index, const std::wstring& text)>;
    using NoteDeleteCallback = std::function<void(size_t index)>;
    using PolygonPointCallback = std::function<void(double lat, double lon)>;
    using PolygonPointMoveCallback = std::function<void(size_t polygonIndex, size_t pointIndex, double lat, double lon)>;
    using PolygonPointDeleteCallback = std::function<void(size_t polygonIndex, size_t pointIndex)>;
    using PolygonClearCallback = std::function<void(size_t polygonIndex)>;
    using RefreshCallback = std::function<void()>;
    using NotificationHistoryClearCallback = std::function<void()>;
    using NotificationHistoryActivateCallback = std::function<void(const AppNotification&)>;
    using NotificationHistoryDeleteCallback = std::function<void(size_t index)>;
    using ChatSendCallback = std::function<void(const std::wstring& text)>;
    using PrivateChatSendCallback = std::function<void(const std::wstring& recipientUsername, const std::wstring& text)>;
    using ChatClearCallback = std::function<void()>;
    using ChatMessageActionCallback = std::function<void(const ChatMessage& message, const std::wstring& action)>;
    using UserActionCallback = std::function<void(const OnlineUser& user, const std::wstring& action)>;
    using PanelCloseCallback = std::function<void(const std::wstring& panelName)>;
    using MapDisplayModeCallback = std::function<void(bool displayWorldMap)>;

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

    void SetNoteCreateCallback(NoteCreateCallback cb)
    {
        m_onNoteCreate = std::move(cb);
    }

    void SetNoteUpdateCallback(NoteUpdateCallback cb)
    {
        m_onNoteUpdate = std::move(cb);
    }

    void SetNoteDeleteCallback(NoteDeleteCallback cb)
    {
        m_onNoteDelete = std::move(cb);
    }

    void SetPolygonPointCallback(PolygonPointCallback cb)
    {
        m_onPolygonPoint = std::move(cb);
    }

    void SetPolygonPointMoveCallback(PolygonPointMoveCallback cb)
    {
        m_onPolygonPointMove = std::move(cb);
    }

    void SetPolygonPointDeleteCallback(PolygonPointDeleteCallback cb)
    {
        m_onPolygonPointDelete = std::move(cb);
    }

    void SetPolygonClearCallback(PolygonClearCallback cb)
    {
        m_onPolygonClear = std::move(cb);
    }

    void SetRefreshCallback(RefreshCallback cb)
    {
        m_onRefresh = std::move(cb);
    }

    void SetNotificationHistoryClearCallback(NotificationHistoryClearCallback cb)
    {
        m_onNotificationHistoryClear = std::move(cb);
    }

    void SetNotificationHistoryActivateCallback(NotificationHistoryActivateCallback cb)
    {
        m_onNotificationHistoryActivate = std::move(cb);
    }

    void SetNotificationHistoryDeleteCallback(NotificationHistoryDeleteCallback cb)
    {
        m_onNotificationHistoryDelete = std::move(cb);
    }

    void SetChatSendCallback(ChatSendCallback cb)
    {
        m_onChatSend = std::move(cb);
    }

    void SetPrivateChatSendCallback(PrivateChatSendCallback cb)
    {
        m_onPrivateChatSend = std::move(cb);
    }

    void SetChatClearCallback(ChatClearCallback cb)
    {
        m_onChatClear = std::move(cb);
    }

    void SetChatMessageActionCallback(ChatMessageActionCallback cb)
    {
        m_onChatMessageAction = std::move(cb);
    }

    void SetUserActionCallback(UserActionCallback cb)
    {
        m_onUserAction = std::move(cb);
    }

    void SetPanelCloseCallback(PanelCloseCallback cb)
    {
        m_onPanelClose = std::move(cb);
    }

    void SetMapDisplayModeCallback(MapDisplayModeCallback cb)
    {
        m_onMapDisplayMode = std::move(cb);
    }

    void SetChatClearEnabled(bool enabled)
    {
        if (m_canClearResponderChat == enabled)
            return;

        m_canClearResponderChat = enabled;
        Invalidate();
    }

    void SetAlerts(const std::vector<TrafficAlert>& alerts)
    {
        m_alerts = alerts;
        Invalidate();
    }

    void SetIncidentOverlayVisible(bool visible)
    {
        if (m_showIncidentOverlayLabels == visible)
            return;

        m_showIncidentOverlayLabels = visible;
        Invalidate();
    }

    void SetNotes(const std::vector<MapNote>& notes)
    {
        if (NotesEqual(m_notes, notes))
            return;

        m_notes = notes;
        if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex >= m_notes.size())
            CancelNoteEditor();
        Invalidate();
    }

    void SetChatMessages(const std::vector<ChatMessage>& messages)
    {
        m_chatMessages = messages;
        Invalidate();
    }

    void SetPrivateMessages(const std::vector<PrivateMessage>& messages)
    {
        m_privateMessages = messages;
        Invalidate();
    }

    void SetOnlineUsers(const std::vector<OnlineUser>& users)
    {
        m_onlineUsers = users;
        Invalidate();
    }

    void OpenPrivateChat(const OnlineUser& user)
    {
        m_privateChatUser = user;
        m_privateChatVisible = true;
        m_privateChatDraft.clear();
        m_privateChatInputFocused = false;
        Invalidate();
    }

    void SetUsersVisible(bool visible)
    {
        if (m_showUsersPanel == visible)
            return;

        m_showUsersPanel = visible;
        if (visible)
            m_usersPanelCollapsed = false;
        StartUsersPanelAnimation(visible ? 1.0f : 0.0f);
        Invalidate();
    }

    void SetNotificationPolygons(const std::vector<GeoPolygon>& polygons)
    {
        m_notificationPolygons = polygons;
        Invalidate();
    }

    void SetActiveNotificationPolygonIndex(size_t index)
    {
        m_activeNotificationPolygonIndex = index;
        Invalidate();
    }

    void SetDraftPolygon(const std::vector<GeoPoint>& points)
    {
        m_draftPolygon = points;
        Invalidate();
    }

    void SetPolygonCaptureActive(bool active)
    {
        m_polygonCaptureActive = active;
        Invalidate();
    }

    void SetEarthquakes(const std::vector<EarthquakeEvent>& earthquakes)
    {
        m_earthquakes = earthquakes;
        Invalidate();
    }

    void SetEarthquakeOverlayVisible(bool visible)
    {
        if (m_showEarthquakeOverlayLabels == visible)
            return;

        m_showEarthquakeOverlayLabels = visible;
        Invalidate();
    }

    void SetWeatherSystems(const std::vector<WeatherSystemEvent>& systems)
    {
        m_weatherSystems = systems;
        Invalidate();
    }

    void SetWeatherSystemOverlayVisible(bool visible)
    {
        if (m_showWeatherSystemOverlayLabels == visible)
            return;

        m_showWeatherSystemOverlayLabels = visible;
        Invalidate();
    }

    void SetWeatherWarnings(const std::vector<WeatherWarningEvent>& warnings)
    {
        m_weatherWarnings = warnings;
        Invalidate();
    }

    void SetWeatherWarningOverlayVisible(bool visible)
    {
        if (m_showWeatherWarningOverlayLabels == visible)
            return;

        m_showWeatherWarningOverlayLabels = visible;
        Invalidate();
    }

    void SetWeatherWarningPolygonsVisible(bool visible)
    {
        if (m_showWeatherWarningPolygons == visible)
            return;

        m_showWeatherWarningPolygons = visible;
        Invalidate();
    }

    void SetFloods(const std::vector<FloodEvent>& floods)
    {
        m_floods = floods;
        Invalidate();
    }

    void SetFloodOverlayVisible(bool visible)
    {
        if (m_showFloodOverlayLabels == visible)
            return;

        m_showFloodOverlayLabels = visible;
        Invalidate();
    }

    void SetAreaLabelsVisible(bool visible)
    {
        if (m_showAreaLabels == visible)
            return;

        m_showAreaLabels = visible;
        InvalidateSceneCache();
        Invalidate();
    }

    void SetRoadDepictionsVisible(bool visible)
    {
        if (m_showRoadDepictions == visible)
            return;

        m_showRoadDepictions = visible;
        InvalidateSceneCache();
        Invalidate();
    }

    std::vector<std::wstring> RoadDepictionLabels() const
    {
        std::vector<std::wstring> labels;
        std::unordered_set<std::wstring> seen;
        if (m_roadDepictionRoutes) {
            labels.reserve(m_roadDepictionRoutes->size());
            for (const RoadDepictionRoute& route : *m_roadDepictionRoutes) {
                std::wstring label = Trim(route.label);
                if (!label.empty() && seen.insert(ToLower(label)).second)
                    labels.push_back(std::move(label));
            }
        }
        std::sort(labels.begin(), labels.end());
        return labels;
    }

    bool LoadRoadDepictionsFromFile(
        const std::filesystem::path& path,
        std::wstring* errorOut = nullptr,
        const std::unordered_set<std::wstring>* allowedLabels = nullptr)
    {
        std::vector<RoadDepictionRoute> loaded;
        if (!LoadRoadDepictionRoutesFromGeoJson(path, loaded, errorOut, allowedLabels))
            return false;

        m_roadDepictionRoutes = std::make_shared<std::vector<RoadDepictionRoute>>(std::move(loaded));
        InvalidateSceneCache();
        Invalidate();
        return true;
    }

    void SetHiddenRoadDepictions(const std::unordered_set<std::wstring>& hiddenRoadLabels)
    {
        if (m_hiddenRoadDepictionIds == hiddenRoadLabels)
            return;

        m_hiddenRoadDepictionIds = hiddenRoadLabels;
        m_normalizedHiddenRoadDepictionIds = NormalizeRoadDepictionHiddenLabels(m_hiddenRoadDepictionIds);
        InvalidateSceneCache();
        Invalidate();
    }

    void SetDisplayWorldMap(bool visible)
    {
        if (m_displayWorldMap == visible)
            return;

        m_displayWorldMap = visible;
        InvalidateSceneCache();
        Invalidate();
    }

    void SetFpsCounterVisible(bool visible)
    {
        if (m_showFpsCounter == visible)
            return;

        m_showFpsCounter = visible;
        m_fpsFrameCount = 0;
        m_fpsLastSampleMs = GetTickCount64();
        Invalidate();
    }

    void SetToolbarVisible(bool visible)
    {
        if (m_showToolbarPanel == visible)
            return;

        m_showToolbarPanel = visible;
        if (!m_showToolbarPanel)
            m_addNoteMode = false;
        Invalidate();
    }

    void SetActiveNotification(const AppNotification& notification)
    {
        m_activeNotification = notification;
        m_hasActiveNotification = !notification.title.empty() || !notification.body.empty();
        m_activeNotificationClosing = false;
        if (m_hasActiveNotification) {
            m_activeNotificationProgress = 0.0f;
            StartActiveNotificationAnimation(1.0f);
        }
        Invalidate();
    }

    void ClearActiveNotification()
    {
        if (!m_hasActiveNotification)
            return;

        m_activeNotificationClosing = true;
        StartActiveNotificationAnimation(0.0f);
        Invalidate();
    }

    void SetNotificationHistory(const std::vector<AppNotification>& notifications)
    {
        m_notificationHistory = notifications;
        if (m_notificationContextMenuIndex >= m_notificationHistory.size()) {
            m_notificationContextMenuVisible = false;
            m_notificationContextMenuIndex = static_cast<size_t>(-1);
        }
        m_notificationHistoryScroll = ClampNotificationHistoryScroll(m_notificationHistoryScroll);
        Invalidate();
    }

    void SetNotificationHistoryVisible(bool visible)
    {
        if (m_showNotificationHistory == visible)
            return;

        m_showNotificationHistory = visible;
        if (visible)
            m_notificationHistoryCollapsed = false;
        StartNotificationHistoryAnimation(visible ? 1.0f : 0.0f);
        m_notificationHistoryScroll = ClampNotificationHistoryScroll(m_notificationHistoryScroll);
        Invalidate();
    }

    void SetSelectedId(const std::wstring& id)
    {
        m_selectedId = id;
        Invalidate();
    }

    static float EaseOverlay(float t)
    {
        t = ClampValue(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    void EnsureOverlayAnimationTimer()
    {
        if (m_hwnd)
            SetTimer(m_hwnd, kOverlayAnimationTimer, 16, nullptr);
    }

    void StartNotificationHistoryAnimation(float target)
    {
        m_notificationHistoryAnimationStart = m_notificationHistoryOpenProgress;
        m_notificationHistoryAnimationTarget = ClampValue(target, 0.0f, 1.0f);
        m_notificationHistoryAnimationStartMs = GetTickCount64();
        m_notificationHistoryAnimating = true;
        EnsureOverlayAnimationTimer();
    }

    void StartActiveNotificationAnimation(float target)
    {
        m_activeNotificationAnimationStart = m_activeNotificationProgress;
        m_activeNotificationAnimationTarget = ClampValue(target, 0.0f, 1.0f);
        m_activeNotificationAnimationStartMs = GetTickCount64();
        m_activeNotificationAnimating = true;
        EnsureOverlayAnimationTimer();
    }

    void StartResponderChatAnimation(float target)
    {
        m_responderChatAnimationStart = m_responderChatOpenProgress;
        m_responderChatAnimationTarget = ClampValue(target, 0.0f, 1.0f);
        m_responderChatAnimationStartMs = GetTickCount64();
        m_responderChatAnimating = true;
        EnsureOverlayAnimationTimer();
    }

    void StartUsersPanelAnimation(float target)
    {
        m_usersPanelAnimationStart = m_usersPanelOpenProgress;
        m_usersPanelAnimationTarget = ClampValue(target, 0.0f, 1.0f);
        m_usersPanelAnimationStartMs = GetTickCount64();
        m_usersPanelAnimating = true;
        EnsureOverlayAnimationTimer();
    }

    static bool AdvanceAnimatedValue(float& value, float start, float target, ULONGLONG startMs)
    {
        const ULONGLONG now = GetTickCount64();
        const float t = static_cast<float>(std::min<ULONGLONG>(kOverlayAnimationMs, now - startMs)) /
            static_cast<float>(kOverlayAnimationMs);
        value = start + (target - start) * EaseOverlay(t);
        if (t >= 1.0f) {
            value = target;
            return true;
        }
        return false;
    }

    void UpdateOverlayAnimations()
    {
        bool anyRunning = false;

        if (m_notificationHistoryAnimating) {
            const bool done = AdvanceAnimatedValue(
                m_notificationHistoryOpenProgress,
                m_notificationHistoryAnimationStart,
                m_notificationHistoryAnimationTarget,
                m_notificationHistoryAnimationStartMs);
            m_notificationHistoryAnimating = !done;
            anyRunning = anyRunning || !done;
        }

        if (m_activeNotificationAnimating) {
            const bool done = AdvanceAnimatedValue(
                m_activeNotificationProgress,
                m_activeNotificationAnimationStart,
                m_activeNotificationAnimationTarget,
                m_activeNotificationAnimationStartMs);
            m_activeNotificationAnimating = !done;
            anyRunning = anyRunning || !done;
            if (done && m_activeNotificationClosing && m_activeNotificationProgress <= 0.0f) {
                m_hasActiveNotification = false;
                m_activeNotificationClosing = false;
                m_activeNotification = {};
            }
        }

        if (m_responderChatAnimating) {
            const bool done = AdvanceAnimatedValue(
                m_responderChatOpenProgress,
                m_responderChatAnimationStart,
                m_responderChatAnimationTarget,
                m_responderChatAnimationStartMs);
            m_responderChatAnimating = !done;
            anyRunning = anyRunning || !done;
        }

        if (m_usersPanelAnimating) {
            const bool done = AdvanceAnimatedValue(
                m_usersPanelOpenProgress,
                m_usersPanelAnimationStart,
                m_usersPanelAnimationTarget,
                m_usersPanelAnimationStartMs);
            m_usersPanelAnimating = !done;
            anyRunning = anyRunning || !done;
        }

        if (!anyRunning)
            KillTimer(m_hwnd, kOverlayAnimationTimer);

        Invalidate();
    }

    void CenterOnAlert(const std::wstring& id)
    {
        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (m_alerts[i].id == id && m_alerts[i].hasLocation) {
                m_centerLat = m_alerts[i].latitude;
                m_centerLon = m_alerts[i].longitude;
                m_zoom = kAlertFocusZoom;
                NormalizeCenter();
                Invalidate();
                return;
            }
        }
    }

    void FitToPoints(const std::vector<GeoPoint>& points, int singlePointZoom = 8)
    {
        std::vector<GeoPoint> pts;
        pts.reserve(points.size());
        for (const GeoPoint& pt : points) {
            if (std::isfinite(pt.lat) && std::isfinite(pt.lon))
                pts.push_back(pt);
        }

        if (pts.empty()) {
            return;
        }

        if (pts.size() == 1) {
            m_centerLat = pts[0].lat;
            m_centerLon = pts[0].lon;
            m_zoom = ClampValue(singlePointZoom, kMinZoom, kMaxZoom);
            NormalizeCenter();
            Invalidate();
            return;
        }

        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

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

    void ResetView()
    {
        if (m_displayWorldMap) {
            m_centerLat = 0.0;
            m_centerLon = 0.0;
            m_zoom = kMinZoom;
        }
        else {
            m_centerLat = kDefaultCenterLat;
            m_centerLon = kDefaultCenterLon;
            m_zoom = kDefaultZoom;
        }
        NormalizeCenter();
        Invalidate();
    }

    void FitToAlerts()
    {
        std::vector<GeoPoint> pts;
        for (const TrafficAlert& alert : m_alerts) {
            if (alert.hasLocation)
                pts.push_back({ alert.latitude, alert.longitude });
        }

        if (m_displayWorldMap) {
            for (const EarthquakeEvent& event : m_earthquakes) {
                if (event.hasLocation)
                    pts.push_back({ event.latitude, event.longitude });
            }

            for (const WeatherSystemEvent& system : m_weatherSystems) {
                if (system.hasLocation)
                    pts.push_back({ system.latitude, system.longitude });
                for (const WeatherForecastPoint& point : system.forecastTrack) {
                    if (point.hasLocation)
                        pts.push_back({ point.latitude, point.longitude });
                }
            }

            for (const WeatherWarningEvent& warning : m_weatherWarnings) {
                if (warning.hasLocation)
                    pts.push_back({ warning.latitude, warning.longitude });
                if (m_showWeatherWarningPolygons) {
                    for (const GeoPoint& point : warning.polygon)
                        pts.push_back(point);
                }
            }

            for (const FloodEvent& flood : m_floods) {
                if (flood.hasLocation)
                    pts.push_back({ flood.latitude, flood.longitude });
            }
        }

        if (pts.empty()) {
            ResetView();
            return;
        }

        FitToPoints(pts, m_displayWorldMap ? 5 : 12);
    }

    static bool SameGeoPoint(const GeoPoint& a, const GeoPoint& b)
    {
        return std::abs(a.lat - b.lat) < 1e-10 && std::abs(a.lon - b.lon) < 1e-10;
    }

    static void RebuildBoundarySegments(const std::vector<GeoPoint>& points, std::vector<BoundarySegment>& segments)
    {
        segments.clear();
        if (points.size() < 2)
            return;

        segments.reserve(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            BoundarySegment segment;
            segment.a = points[i];
            segment.b = points[(i + 1) % points.size()];
            if (SameGeoPoint(segment.a, segment.b))
                continue;
            segment.minLat = MinValue(segment.a.lat, segment.b.lat);
            segment.maxLat = MaxValue(segment.a.lat, segment.b.lat);
            segment.minLon = MinValue(segment.a.lon, segment.b.lon);
            segment.maxLon = MaxValue(segment.a.lon, segment.b.lon);
            segments.push_back(segment);
        }

        std::sort(segments.begin(), segments.end(), [](const auto& a, const auto& b) {
            return a.minLat < b.minLat;
            });
    }

    static void SetBoundaryBounds(const std::vector<GeoPoint>& points, double& minLat, double& maxLat, double& minLon, double& maxLon)
    {
        if (points.empty()) {
            minLat = maxLat = minLon = maxLon = 0.0;
            return;
        }

        minLat = maxLat = points.front().lat;
        minLon = maxLon = points.front().lon;
        for (const GeoPoint& pt : points) {
            minLat = MinValue(minLat, pt.lat);
            maxLat = MaxValue(maxLat, pt.lat);
            minLon = MinValue(minLon, pt.lon);
            maxLon = MaxValue(maxLon, pt.lon);
        }
    }

    static std::vector<GeoPoint> SimplifyBoundaryRingPoints(const std::vector<GeoPoint>& source, double toleranceDegrees)
    {
        if (source.size() < 18 || toleranceDegrees <= 0.0)
            return source;

        std::vector<GeoPoint> input = source;
        if (input.size() > 1 && SameGeoPoint(input.front(), input.back()))
            input.pop_back();
        if (input.size() < 18)
            return source;

        const double toleranceSq = toleranceDegrees * toleranceDegrees;
        std::vector<GeoPoint> simplified;
        simplified.reserve(input.size());
        simplified.push_back(input.front());

        GeoPoint lastKept = input.front();
        for (size_t i = 1; i + 1 < input.size(); ++i) {
            const GeoPoint& pt = input[i];
            const double dLat = pt.lat - lastKept.lat;
            const double dLon = pt.lon - lastKept.lon;
            if (dLat * dLat + dLon * dLon >= toleranceSq) {
                simplified.push_back(pt);
                lastKept = pt;
            }
        }

        simplified.push_back(input.back());
        if (simplified.size() < 4)
            return source;
        return simplified;
    }

    static BoundaryLod BuildBoundaryLod(const BoundaryRing& source, double toleranceDegrees)
    {
        BoundaryLod lod;
        lod.points = SimplifyBoundaryRingPoints(source.points, toleranceDegrees);
        SetBoundaryBounds(lod.points, lod.minLat, lod.maxLat, lod.minLon, lod.maxLon);
        RebuildBoundarySegments(lod.points, lod.segmentsByMinLat);
        return lod;
    }

    bool LoadBoundaryRingsFromFile(
        const std::filesystem::path& path,
        std::vector<BoundaryRing>& target,
        std::wstring* errorOut = nullptr,
        bool buildWorldLods = false)
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

        target.clear();
        target.reserve(rings.size());
        for (auto& ring : rings) {
            if (ring.empty())
                continue;

            BoundaryRing cached;
            cached.points = std::move(ring);
            SetBoundaryBounds(cached.points, cached.minLat, cached.maxLat, cached.minLon, cached.maxLon);
            RebuildBoundarySegments(cached.points, cached.segmentsByMinLat);
            if (buildWorldLods) {
                cached.worldLods.reserve(4);
                cached.worldLods.push_back(BuildBoundaryLod(cached, kWorldLodDetailToleranceDegrees));
                cached.worldLods.push_back(BuildBoundaryLod(cached, kWorldLodMidToleranceDegrees));
                cached.worldLods.push_back(BuildBoundaryLod(cached, kWorldLodFarToleranceDegrees));
                cached.worldLods.push_back(BuildBoundaryLod(cached, kWorldLodGlobalToleranceDegrees));
                for (auto& lod : cached.worldLods)
                    EnsureBoundaryFillGeometry(lod);
            }
            target.push_back(std::move(cached));
        }

        InvalidateSceneCache();
        Invalidate();
        return true;
    }

    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr)
    {
        return LoadBoundaryRingsFromFile(path, m_ukBoundaryRings, errorOut, false);
    }

    bool LoadWorldBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr)
    {
        return LoadBoundaryRingsFromFile(path, m_worldBoundaryRings, errorOut, true);
    }

private:
    static bool NotesEqual(const std::vector<MapNote>& a, const std::vector<MapNote>& b)
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

    static bool RegisterClass()
    {
        static bool registered = false;
        if (registered)
            return true;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = MapWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = kMapClassName;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        registered = true;
        return true;
    }

    static LRESULT CALLBACK MapWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Impl* self = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<Impl*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        }
        else {
            self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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
            if (m_hwndRt) {
                UINT w = static_cast<UINT>(std::max<LONG>(1L, LOWORD(lParam)));
                UINT h = static_cast<UINT>(std::max<LONG>(1L, HIWORD(lParam)));
                m_hwndRt->Resize(D2D1::SizeU(w, h));
                InvalidateSceneCache();
            }
            ClampToolbarPanelOffsets(BuildViewState());
            ClampUsersPanelOffsets(BuildViewState());
            ClampPrivateChatPanelOffsets(BuildViewState());
            return 0;

        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_TIMER:
            if (wParam == kInteractionIdleTimer) {
                KillTimer(m_hwnd, kInteractionIdleTimer);
                m_interactivePan = false;
                Invalidate();
                return 0;
            }
            if (wParam == kOverlayAnimationTimer) {
                UpdateOverlayAnimations();
                return 0;
            }
            if (wParam == kSceneCacheRefreshTimer) {
                KillTimer(m_hwnd, kSceneCacheRefreshTimer);
                m_sceneCacheRefreshPending = false;

                const ULONGLONG now = GetTickCount64();
                if (m_sceneCacheDirty && m_sceneCacheAllowDirtyUntilMs > now) {
                    const UINT delay = static_cast<UINT>(ClampValue<ULONGLONG>(m_sceneCacheAllowDirtyUntilMs - now, 16, kSceneCacheRefreshMs));
                    SetTimer(m_hwnd, kSceneCacheRefreshTimer, delay, nullptr);
                    m_sceneCacheRefreshPending = true;
                    return 0;
                }

                if (m_sceneCacheDirty && (m_interactivePan || IsOverlayUiDragActive())) {
                    m_sceneCacheAllowDirtyUntilMs = now + kSceneCacheRefreshMs;
                    SetTimer(m_hwnd, kSceneCacheRefreshTimer, kSceneCacheRefreshMs, nullptr);
                    m_sceneCacheRefreshPending = true;
                    return 0;
                }

                if (m_sceneCacheDirty &&
                    m_sceneBitmap &&
                    m_sceneCacheLastRebuildMs != 0 &&
                    now < m_sceneCacheLastRebuildMs + kSceneCacheMinRebuildIntervalMs)
                {
                    const ULONGLONG due = m_sceneCacheLastRebuildMs + kSceneCacheMinRebuildIntervalMs;
                    const UINT delay = static_cast<UINT>(ClampValue<ULONGLONG>(due - now, 16, kSceneCacheMinRebuildIntervalMs));
                    m_sceneCacheAllowDirtyUntilMs = due;
                    SetTimer(m_hwnd, kSceneCacheRefreshTimer, delay, nullptr);
                    m_sceneCacheRefreshPending = true;
                    return 0;
                }

                m_sceneCacheAllowDirtyUntilMs = 0;
                InvalidateRect(m_hwnd, nullptr, FALSE);
                return 0;
            }
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_LBUTTONDOWN:
            SetFocus(m_hwnd);
            m_hoverPoint = POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            m_leftButtonDown = true;
            if (HandleOverlayContextMenuPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandlePrivateChatPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandleUsersPanelPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandleResponderChatPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandleNotificationPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HitUsersPanelInterface(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) ||
                HitPrivateChatInterface(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) ||
                HitResponderChatInterface(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) ||
                HitNotificationInterface(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
                SetCapture(m_hwnd);
                m_notificationUiMouseDown = true;
                return 0;
            }
            EnsureDeviceResources();
            if (m_rt && HandleNotePointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandlePolygonPointPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            ClearOverlayInputFocus();
            SetCapture(m_hwnd);
            m_notificationUiMouseDown = false;
            m_mouseDown.x = GET_X_LPARAM(lParam);
            m_mouseDown.y = GET_Y_LPARAM(lParam);
            m_lastMouse = m_mouseDown;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), static_cast<UINT>(wParam));
            return 0;

        case WM_MOUSELEAVE:
            m_trackingMouseLeave = false;
            m_hoverPoint = POINT{ -10000, -10000 };
            m_leftButtonDown = false;
            if (!m_hoveredAlertId.empty() || !m_hoveredEarthquakeId.empty() || !m_hoveredWeatherSystemId.empty() ||
                !m_hoveredWeatherWarningId.empty() || !m_hoveredFloodId.empty()) {
                m_hoveredAlertId.clear();
                m_hoveredEarthquakeId.clear();
                m_hoveredWeatherSystemId.clear();
                m_hoveredWeatherWarningId.clear();
                m_hoveredFloodId.clear();
                Invalidate();
            }
            return 0;

        case WM_LBUTTONUP:
            m_hoverPoint = POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            m_leftButtonDown = false;
            OnLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_RBUTTONDOWN:
            if (HandleUserContextRightClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandleChatContextRightClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandleNotificationHistoryRightClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            HideOverlayContextMenus();
            if (HandlePolygonRightClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            break;

        case WM_LBUTTONDBLCLK:
            OnDoubleClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_MOUSEWHEEL:
            OnMouseWheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;

        case WM_KEYDOWN:
            if (m_overlayInputFocus == OverlayInputFocus::PrivateChat) {
                if (HandlePrivateChatKeyDown(wParam))
                    return 0;
            }
            else if (m_overlayInputFocus == OverlayInputFocus::ResponderChat) {
                if (HandleResponderChatKeyDown(wParam))
                    return 0;
            }
            else if (m_overlayInputFocus == OverlayInputFocus::NoteEditor) {
                if (HandleNoteEditorKeyDown(wParam))
                    return 0;
            }
            break;

        case WM_CHAR:
            if (m_overlayInputFocus == OverlayInputFocus::PrivateChat) {
                if (HandlePrivateChatChar(wParam))
                    return 0;
            }
            else if (m_overlayInputFocus == OverlayInputFocus::ResponderChat) {
                if (HandleResponderChatChar(wParam))
                    return 0;
            }
            else if (m_overlayInputFocus == OverlayInputFocus::NoteEditor) {
                if (HandleNoteEditorChar(wParam))
                    return 0;
            }
            break;

        case WM_APP_TILE_READY:
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;

        case WM_APP_SCENE_CACHE_READY:
            AdoptAsyncSceneCacheResult(std::unique_ptr<SceneCacheBuildResult>(reinterpret_cast<SceneCacheBuildResult*>(lParam)));
            return 0;

        case WM_APP_SCENE_TILE_READY:
            AdoptSceneTileResult(std::unique_ptr<SceneTileBuildResult>(reinterpret_cast<SceneTileBuildResult*>(lParam)));
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

        double worldSize = 256.0 * static_cast<double>(1 << zoom);
        double x = (lon + 180.0) / 360.0 * worldSize;
        double sinLat = std::sin(lat * kPi / 180.0);
        double y = (0.5 - std::log((1.0 + sinLat) / (1.0 - sinLat)) / (4.0 * kPi)) * worldSize;

        return { x, y };
    }

    static double WorldPixelSizeForZoom(int zoom)
    {
        return 256.0 * static_cast<double>(1 << zoom);
    }

    static int SceneTileCountForZoom(int zoom)
    {
        const double worldSize = WorldPixelSizeForZoom(zoom);
        return MaxValue(1, static_cast<int>(std::ceil(worldSize / static_cast<double>(kSceneTileSize))));
    }

    static int NormalizeSceneTileX(int x, int zoom)
    {
        const int count = SceneTileCountForZoom(zoom);
        if (count <= 1)
            return 0;
        int normalized = x % count;
        if (normalized < 0)
            normalized += count;
        return normalized;
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

    struct ViewState
    {
        int width = 1;
        int height = 1;
        WorldPoint centerWorld{};
        double minLat = -kMaxMercatorLat;
        double maxLat = kMaxMercatorLat;
        double minLon = -180.0;
        double maxLon = 180.0;
        bool wrapsLongitude = false;
        bool allLongitudes = true;
    };

    struct SceneCacheRingSnapshot
    {
        std::vector<GeoPoint> points;
    };

    struct SceneCacheBuildRequest
    {
        int zoom = kDefaultZoom;
        int cacheWidth = 1;
        int cacheHeight = 1;
        int viewportWidth = 1;
        int viewportHeight = 1;
        bool displayWorldMap = false;
        bool includeRoadDepictions = false;
        WorldPoint centerWorld{};
        ViewState boundaryView{};
        std::vector<SceneCacheRingSnapshot> rings;
        std::shared_ptr<const std::vector<RoadDepictionRoute>> roadRoutes;
        std::unordered_set<std::wstring> hiddenRoadDepictionIds;
    };

    struct SceneCacheBuildResult
    {
        int zoom = kDefaultZoom;
        int cacheWidth = 1;
        int cacheHeight = 1;
        int viewportWidth = 1;
        int viewportHeight = 1;
        bool displayWorldMap = false;
        WorldPoint centerWorld{};
        std::vector<BYTE> pixels;
        bool success = false;
    };

    struct SceneTileBuildRequest
    {
        SceneTileKey key{};
        uint64_t generation = 0;
        int tileSize = kSceneTileSize;
        bool includeRoadDepictions = false;
        WorldPoint tileCenterWorld{};
        ViewState boundaryView{};
        std::vector<SceneCacheRingSnapshot> rings;
        std::shared_ptr<const std::vector<RoadDepictionRoute>> roadRoutes;
        std::unordered_set<std::wstring> hiddenRoadDepictionIds;
    };

    struct SceneTileBuildResult
    {
        SceneTileKey key{};
        uint64_t generation = 0;
        int tileSize = kSceneTileSize;
        std::vector<BYTE> pixels;
        bool success = false;
    };

    struct SceneTileDrawItem
    {
        SceneTileKey key{};
        int drawTileX = 0;
        int drawTileY = 0;
        D2D1_RECT_F dest{};
    };

    static double NormalizeLongitude(double lon)
    {
        while (lon < -180.0) lon += 360.0;
        while (lon > 180.0) lon -= 360.0;
        return lon;
    }

    ViewState BuildViewState(double marginPixels = 0.0) const
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        return BuildViewStateForSize(
            std::max(1, static_cast<int>(rc.right - rc.left)),
            std::max(1, static_cast<int>(rc.bottom - rc.top)),
            marginPixels);
    }

    ViewState BuildViewStateForSize(int width, int height, double marginPixels = 0.0) const
    {
        ViewState view;
        view.width = std::max(1, width);
        view.height = std::max(1, height);
        view.centerWorld = GeoToWorld(m_centerLat, m_centerLon, m_zoom);

        const double left = view.centerWorld.x - view.width * 0.5 - marginPixels;
        const double right = view.centerWorld.x + view.width * 0.5 + marginPixels;
        const double top = view.centerWorld.y - view.height * 0.5 - marginPixels;
        const double bottom = view.centerWorld.y + view.height * 0.5 + marginPixels;

        GeoPoint nw = WorldToGeo(left, top, m_zoom);
        GeoPoint se = WorldToGeo(right, bottom, m_zoom);

        view.minLat = ClampValue(MinValue(nw.lat, se.lat), -kMaxMercatorLat, kMaxMercatorLat);
        view.maxLat = ClampValue(MaxValue(nw.lat, se.lat), -kMaxMercatorLat, kMaxMercatorLat);

        // Longitude can wrap when the viewport crosses the antimeridian. Keep this
        // explicit so high-zoom culling remains cheap without hiding wrapped points.
        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        view.allLongitudes = (right - left) >= worldSize;
        view.minLon = NormalizeLongitude(nw.lon);
        view.maxLon = NormalizeLongitude(se.lon);
        view.wrapsLongitude = !view.allLongitudes && view.minLon > view.maxLon;

        return view;
    }

    static ViewState BuildViewStateFromCenter(
        int width,
        int height,
        int zoom,
        const WorldPoint& centerWorld,
        double marginPixels = 0.0)
    {
        ViewState view;
        view.width = std::max(1, width);
        view.height = std::max(1, height);
        view.centerWorld = centerWorld;

        const double left = view.centerWorld.x - view.width * 0.5 - marginPixels;
        const double right = view.centerWorld.x + view.width * 0.5 + marginPixels;
        const double top = view.centerWorld.y - view.height * 0.5 - marginPixels;
        const double bottom = view.centerWorld.y + view.height * 0.5 + marginPixels;

        GeoPoint nw = WorldToGeo(left, top, zoom);
        GeoPoint se = WorldToGeo(right, bottom, zoom);

        view.minLat = ClampValue(MinValue(nw.lat, se.lat), -kMaxMercatorLat, kMaxMercatorLat);
        view.maxLat = ClampValue(MaxValue(nw.lat, se.lat), -kMaxMercatorLat, kMaxMercatorLat);

        const double worldSize = WorldPixelSizeForZoom(zoom);
        view.allLongitudes = (right - left) >= worldSize;
        view.minLon = NormalizeLongitude(nw.lon);
        view.maxLon = NormalizeLongitude(se.lon);
        view.wrapsLongitude = !view.allLongitudes && view.minLon > view.maxLon;

        return view;
    }

    D2D1_POINT_2F GeoToScreen(const ViewState& view, double lat, double lon) const
    {
        WorldPoint p = GeoToWorld(lat, lon, m_zoom);
        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        while (p.x - view.centerWorld.x > worldSize * 0.5)
            p.x -= worldSize;
        while (p.x - view.centerWorld.x < -worldSize * 0.5)
            p.x += worldSize;

        float x = static_cast<float>((p.x - view.centerWorld.x) + view.width * 0.5);
        float y = static_cast<float>((p.y - view.centerWorld.y) + view.height * 0.5);

        return D2D1::Point2F(x, y);
    }

    static D2D1_POINT_2F GeoToScreenForZoom(const ViewState& view, double lat, double lon, int zoom)
    {
        WorldPoint p = GeoToWorld(lat, lon, zoom);
        const double worldSize = WorldPixelSizeForZoom(zoom);
        while (p.x - view.centerWorld.x > worldSize * 0.5)
            p.x -= worldSize;
        while (p.x - view.centerWorld.x < -worldSize * 0.5)
            p.x += worldSize;

        return D2D1::Point2F(
            static_cast<float>((p.x - view.centerWorld.x) + view.width * 0.5),
            static_cast<float>((p.y - view.centerWorld.y) + view.height * 0.5));
    }

    D2D1_POINT_2F GeoToScreen(double lat, double lon) const
    {
        return GeoToScreen(BuildViewState(), lat, lon);
    }

    static bool IsGeoPointInView(const ViewState& view, double lat, double lon)
    {
        if (!std::isfinite(lat) || !std::isfinite(lon))
            return false;

        if (lat < view.minLat || lat > view.maxLat)
            return false;

        if (view.allLongitudes)
            return true;

        lon = NormalizeLongitude(lon);

        if (view.wrapsLongitude)
            return lon >= view.minLon || lon <= view.maxLon;

        return lon >= view.minLon && lon <= view.maxLon;
    }

    static bool AnyPointInViewStatic(const ViewState& view, const std::vector<GeoPoint>& points)
    {
        for (const GeoPoint& pt : points) {
            if (IsGeoPointInView(view, pt.lat, pt.lon))
                return true;
        }
        return false;
    }

    static bool RoadRouteIntersectsView(const ViewState& view, const RoadDepictionRoute& route)
    {
        if (route.points.size() < 2)
            return false;
        if (route.maxLat < view.minLat || route.minLat > view.maxLat)
            return false;
        return AnyPointInViewStatic(view, route.points);
    }

    static const std::vector<GeoPoint>& RoadRoutePointsForZoom(const RoadDepictionRoute& route, int zoom)
    {
        if (zoom <= 7 && route.farPoints.size() >= 2)
            return route.farPoints;
        if (zoom <= 10 && route.midPoints.size() >= 2)
            return route.midPoints;
        if (zoom <= 13 && route.nearPoints.size() >= 2)
            return route.nearPoints;
        return route.points;
    }

    static void RoadRouteBoundsForZoom(
        const RoadDepictionRoute& route,
        int zoom,
        double& minLat,
        double& maxLat,
        double& minLon,
        double& maxLon)
    {
        if (zoom <= 7 && route.farPoints.size() >= 2) {
            minLat = route.farMinLat;
            maxLat = route.farMaxLat;
            minLon = route.farMinLon;
            maxLon = route.farMaxLon;
            return;
        }
        if (zoom <= 10 && route.midPoints.size() >= 2) {
            minLat = route.midMinLat;
            maxLat = route.midMaxLat;
            minLon = route.midMinLon;
            maxLon = route.midMaxLon;
            return;
        }
        if (zoom <= 13 && route.nearPoints.size() >= 2) {
            minLat = route.nearMinLat;
            maxLat = route.nearMaxLat;
            minLon = route.nearMinLon;
            maxLon = route.nearMaxLon;
            return;
        }

        minLat = route.minLat;
        maxLat = route.maxLat;
        minLon = route.minLon;
        maxLon = route.maxLon;
    }

    static bool RoadBoundsIntersectView(const ViewState& view, double minLat, double maxLat, double minLon, double maxLon)
    {
        if (maxLat < view.minLat || minLat > view.maxLat)
            return false;
        return LongitudeRangesIntersect(view, minLon, maxLon);
    }

    static bool RoadRouteIntersectsView(const ViewState& view, const RoadDepictionRoute& route, int zoom)
    {
        double minLat = 0.0;
        double maxLat = 0.0;
        double minLon = 0.0;
        double maxLon = 0.0;
        RoadRouteBoundsForZoom(route, zoom, minLat, maxLat, minLon, maxLon);
        return RoadBoundsIntersectView(view, minLat, maxLat, minLon, maxLon);
    }

    static bool RoadSegmentIntersectsView(const ViewState& view, const GeoPoint& a, const GeoPoint& b)
    {
        const double minLat = MinValue(a.lat, b.lat);
        const double maxLat = MaxValue(a.lat, b.lat);
        const double minLon = MinValue(a.lon, b.lon);
        const double maxLon = MaxValue(a.lon, b.lon);
        return RoadBoundsIntersectView(view, minLat, maxLat, minLon, maxLon);
    }

    static bool RectsIntersect(const D2D1_RECT_F& a, const D2D1_RECT_F& b)
    {
        return a.left < b.right && a.right > b.left &&
            a.top < b.bottom && a.bottom > b.top;
    }

    static float RectIntersectionArea(const D2D1_RECT_F& a, const D2D1_RECT_F& b)
    {
        const float left = MaxValue(a.left, b.left);
        const float top = MaxValue(a.top, b.top);
        const float right = MinValue(a.right, b.right);
        const float bottom = MinValue(a.bottom, b.bottom);
        if (right <= left || bottom <= top)
            return 0.0f;
        return (right - left) * (bottom - top);
    }

    static float RectCenterDistanceSq(const D2D1_RECT_F& rect, float x, float y)
    {
        const float centerX = (rect.left + rect.right) * 0.5f;
        const float centerY = (rect.top + rect.bottom) * 0.5f;
        const float dx = centerX - x;
        const float dy = centerY - y;
        return dx * dx + dy * dy;
    }

    static int PositiveModulo(int value, int modulus)
    {
        int result = value % modulus;
        return result < 0 ? result + modulus : result;
    }

    void NormalizeCenter()
    {
        if (m_centerLon < -540.0 || m_centerLon > 540.0)
            m_centerLon = NormalizeLongitude(m_centerLon);
        m_centerLat = ClampValue(m_centerLat, -kMaxMercatorLat, kMaxMercatorLat);
        m_zoom = ClampValue(m_zoom, kMinZoom, kMaxZoom);
    }

    void Invalidate()
    {
        if (m_hwnd)
            InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void InvalidateSceneCache()
    {
        KillTimer(m_hwnd, kSceneCacheRefreshTimer);
        m_sceneBitmap.Reset();
        m_sceneBitmapBack.Reset();
        m_sceneCacheDirty = false;
        m_sceneCacheRefreshPending = false;
        m_sceneCacheAllowDirtyUntilMs = 0;
        m_sceneCacheLastRebuildMs = 0;
        m_sceneBitmapWidth = 0;
        m_sceneBitmapHeight = 0;
        m_sceneViewportWidth = 0;
        m_sceneViewportHeight = 0;
        ClearSceneTileCache();
    }

    void MarkSceneCacheDirtyDeferred(bool invalidateIfMissing = true)
    {
        m_sceneCacheDirty = true;
        m_sceneCacheAllowDirtyUntilMs = GetTickCount64() + kSceneCacheRefreshMs;
        if (!m_sceneCacheRefreshPending) {
            SetTimer(m_hwnd, kSceneCacheRefreshTimer, kSceneCacheRefreshMs, nullptr);
            m_sceneCacheRefreshPending = true;
        }

        if (invalidateIfMissing && !m_sceneBitmap)
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
        double bestDist = 28.0;
        const ViewState view = BuildViewState(bestDist + 8.0);

        for (size_t i = 0; i < m_alerts.size(); ++i) {
            if (!m_alerts[i].hasLocation ||
                !IsGeoPointInView(view, m_alerts[i].latitude, m_alerts[i].longitude))
            {
                continue;
            }

            D2D1_POINT_2F pt = GeoToScreen(view, m_alerts[i].latitude, m_alerts[i].longitude);
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

    std::wstring HitTestEarthquake(int x, int y) const
    {
        std::wstring bestId;
        double bestDist = 26.0;
        const ViewState view = BuildViewState(bestDist + 8.0);

        if (m_zoom <= 4 && m_earthquakes.size() > 800)
            return bestId;

        for (const EarthquakeEvent& event : m_earthquakes) {
            if (event.id.empty() || !event.hasLocation || !IsGeoPointInView(view, event.latitude, event.longitude))
                continue;

            D2D1_POINT_2F pt = GeoToScreen(view, event.latitude, event.longitude);
            const double radius = ClampValue(4.0 + event.magnitude * 2.2, 5.0, 22.0) + 6.0;
            const double dx = pt.x - x;
            const double dy = pt.y - y;
            const double d = std::sqrt(dx * dx + dy * dy);

            if (d <= radius && d < bestDist) {
                bestDist = d;
                bestId = event.id;
            }
        }

        return bestId;
    }

    std::wstring HitTestWeatherSystem(int x, int y) const
    {
        std::wstring bestId;
        double bestDist = 30.0;
        const ViewState view = BuildViewState(bestDist + 8.0);

        for (const WeatherSystemEvent& system : m_weatherSystems) {
            if (system.id.empty() || !system.hasLocation || !IsGeoPointInView(view, system.latitude, system.longitude))
                continue;

            D2D1_POINT_2F pt = GeoToScreen(view, system.latitude, system.longitude);
            const double radius = ClampValue(8.0 + system.windKnots * 0.08, 9.0, 22.0) + 7.0;
            const double dx = pt.x - x;
            const double dy = pt.y - y;
            const double d = std::sqrt(dx * dx + dy * dy);

            if (d <= radius && d < bestDist) {
                bestDist = d;
                bestId = system.id;
            }
        }

        return bestId;
    }

    std::wstring HitTestWeatherWarning(int x, int y) const
    {
        std::wstring bestId;
        double bestDist = 28.0;
        const ViewState view = BuildViewState(bestDist + 8.0);

        for (const WeatherWarningEvent& warning : m_weatherWarnings) {
            if (warning.id.empty() || !warning.hasLocation || !IsGeoPointInView(view, warning.latitude, warning.longitude))
                continue;

            D2D1_POINT_2F pt = GeoToScreen(view, warning.latitude, warning.longitude);
            const double radius = 17.0;
            const double dx = pt.x - x;
            const double dy = pt.y - y;
            const double d = std::sqrt(dx * dx + dy * dy);

            if (d <= radius && d < bestDist) {
                bestDist = d;
                bestId = warning.id;
            }
        }

        return bestId;
    }

    std::wstring HitTestFlood(int x, int y) const
    {
        std::wstring bestId;
        double bestDist = 28.0;
        const ViewState view = BuildViewState(bestDist + 8.0);

        for (const FloodEvent& flood : m_floods) {
            if (flood.id.empty() || !flood.hasLocation || !IsGeoPointInView(view, flood.latitude, flood.longitude))
                continue;

            D2D1_POINT_2F pt = GeoToScreen(view, flood.latitude, flood.longitude);
            const double radius = 16.0;
            const double dx = pt.x - x;
            const double dy = pt.y - y;
            const double d = std::sqrt(dx * dx + dy * dy);

            if (d <= radius && d < bestDist) {
                bestDist = d;
                bestId = flood.id;
            }
        }

        return bestId;
    }

    D2D1_RECT_F ClampRectToView(D2D1_RECT_F rect, const ViewState& view) const
    {
        const float margin = 8.0f;
        const float width = rect.right - rect.left;
        const float height = rect.bottom - rect.top;
        if (rect.right > view.width - margin) {
            rect.left = static_cast<float>(view.width) - margin - width;
            rect.right = rect.left + width;
        }
        if (rect.left < margin) {
            rect.left = margin;
            rect.right = rect.left + width;
        }
        if (rect.bottom > view.height - margin) {
            rect.top = static_cast<float>(view.height) - margin - height;
            rect.bottom = rect.top + height;
        }
        if (rect.top < margin) {
            rect.top = margin;
            rect.bottom = rect.top + height;
        }
        return rect;
    }

    float MeasureMapTextHeight(const std::wstring& text, IDWriteTextFormat* format, float width) const
    {
        if (!format || text.empty() || !g_dwriteFactory)
            return 0.0f;

        width = ClampValue(width, 1.0f, 4000.0f);
        ComPtr<IDWriteTextLayout> layout;
        if (FAILED(g_dwriteFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            format,
            width,
            4000.0f,
            &layout)))
        {
            return 0.0f;
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(layout->GetMetrics(&metrics)))
            return 0.0f;
        return metrics.height;
    }

    float MeasureMapTextWidth(const std::wstring& text, IDWriteTextFormat* format, float width) const
    {
        if (!format || text.empty() || !g_dwriteFactory)
            return 0.0f;

        width = ClampValue(width, 1.0f, 4000.0f);
        ComPtr<IDWriteTextLayout> layout;
        if (FAILED(g_dwriteFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            format,
            width,
            4000.0f,
            &layout)))
        {
            return 0.0f;
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(layout->GetMetrics(&metrics)))
            return 0.0f;
        return metrics.widthIncludingTrailingWhitespace;
    }

    D2D1_RECT_F BuildMeasuredBlipLabelRect(
        const ViewState& view,
        D2D1_POINT_2F anchor,
        float markerRadius,
        const std::wstring& text,
        float minWidth,
        float maxWidth,
        float horizontalPadding,
        float verticalPadding,
        float* contentWidthOut = nullptr,
        float* textHeightOut = nullptr) const
    {
        const float viewMaxWidth = MaxValue(minWidth, static_cast<float>(view.width) - 18.0f);
        maxWidth = ClampValue(maxWidth, minWidth, viewMaxWidth);
        const float naturalWidth = MeasureMapTextWidth(text, m_noteTextFormat.Get(), 4000.0f);
        const float labelWidth = ClampValue(naturalWidth + horizontalPadding * 2.0f, minWidth, maxWidth);
        const float contentWidth = MaxValue(1.0f, labelWidth - horizontalPadding * 2.0f);
        const float textHeight = MaxValue(18.0f, MeasureMapTextHeight(text, m_noteTextFormat.Get(), contentWidth));
        const float labelHeight = MinValue(
            MaxValue(28.0f, textHeight + verticalPadding * 2.0f),
            MaxValue(28.0f, static_cast<float>(view.height) - 18.0f));

        D2D1_RECT_F rect = D2D1::RectF(
            anchor.x + markerRadius + 8.0f,
            anchor.y - labelHeight * 0.5f,
            anchor.x + markerRadius + 8.0f + labelWidth,
            anchor.y + labelHeight * 0.5f);

        if (rect.right > static_cast<float>(view.width) - 8.0f)
            rect = D2D1::RectF(anchor.x - markerRadius - 8.0f - labelWidth, rect.top, anchor.x - markerRadius - 8.0f, rect.bottom);
        rect = ClampRectToView(rect, view);

        if (contentWidthOut)
            *contentWidthOut = contentWidth;
        if (textHeightOut)
            *textHeightOut = textHeight;
        return rect;
    }

    void DrawMeasuredBlipLabel(
        const ViewState& view,
        D2D1_POINT_2F anchor,
        float markerRadius,
        const std::wstring& text,
        ID2D1Brush* stroke,
        float minWidth,
        float maxWidth)
    {
        if (!m_rt || !m_noteTextFormat || text.empty())
            return;

        float textH = 0.0f;
        const float padX = 9.0f;
        const float padY = 6.0f;
        D2D1_RECT_F rect = BuildMeasuredBlipLabelRect(view, anchor, markerRadius, text, minWidth, maxWidth, padX, padY, nullptr, &textH);
        const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, 7.0f, 7.0f);
        m_rt->FillRoundedRectangle(rounded, m_panelBrush.Get());
        m_rt->DrawRoundedRectangle(rounded, stroke ? stroke : m_borderBrush.Get(), 1.2f);

        D2D1_RECT_F textRect = D2D1::RectF(rect.left + padX, rect.top + padY, rect.right - padX, rect.top + padY + textH + 2.0f);
        m_rt->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), m_noteTextFormat.Get(), textRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    D2D1_RECT_F BuildNoteBubbleRect(const ViewState& view, D2D1_POINT_2F anchor, float width, float height) const
    {
        D2D1_RECT_F rect = D2D1::RectF(anchor.x + 10.0f, anchor.y - height + 20.0f, anchor.x + 10.0f + width, anchor.y + 20.0f);
        if (rect.right > view.width - 8.0f)
            rect = D2D1::RectF(anchor.x - width - 12.0f, anchor.y - height + 20.0f, anchor.x - 12.0f, anchor.y + 20.0f);
        if (rect.top < 8.0f)
            rect = D2D1::RectF(rect.left, anchor.y + 18.0f, rect.right, anchor.y + 18.0f + height);
        return ClampRectToView(rect, view);
    }

    std::wstring NoteDisplayText(const MapNote& note) const
    {
        std::wstring text = note.text;
        if (!note.author.empty())
            text = note.author + L": " + text;
        if (text.size() > 220)
            text = text.substr(0, 217) + L"...";
        return text;
    }

    float NoteBubbleHeightForText(const std::wstring& text) const
    {
        if (!m_noteTextFormat || text.empty())
            return kNoteBubbleHeight;

        const float contentW = MaxValue(1.0f, kNoteBubbleWidth - 44.0f);
        const float textH = MaxValue(18.0f, MeasureMapTextHeight(text, m_noteTextFormat.Get(), contentW));
        return ClampValue(textH + 24.0f, kNoteBubbleHeight, kNoteBubbleMaxHeight);
    }

    D2D1_RECT_F BuildAddNoteButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        const float buttonW = 120.0f;
        const float buttonH = 32.0f;
        return D2D1::RectF(panel.left + 142.0f, panel.top + 86.0f, panel.left + 142.0f + buttonW, panel.top + 86.0f + buttonH);
    }

    D2D1_RECT_F BuildResetViewButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        const float buttonW = 120.0f;
        const float buttonH = 32.0f;
        return D2D1::RectF(panel.left + 142.0f, panel.top + 44.0f, panel.left + 142.0f + buttonW, panel.top + 44.0f + buttonH);
    }

    D2D1_RECT_F BuildFitAlertsButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        const float buttonW = 120.0f;
        const float buttonH = 32.0f;
        return D2D1::RectF(panel.left + 12.0f, panel.top + 86.0f, panel.left + 12.0f + buttonW, panel.top + 86.0f + buttonH);
    }

    D2D1_RECT_F BuildAddNotePromptRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        return D2D1::RectF(panel.left, panel.bottom + 8.0f, panel.right, panel.bottom + 40.0f);
    }

    D2D1_RECT_F BuildRefreshButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        const float buttonW = 120.0f;
        const float buttonH = 32.0f;
        return D2D1::RectF(panel.left + 12.0f, panel.top + 44.0f, panel.left + 12.0f + buttonW, panel.top + 44.0f + buttonH);
    }

    D2D1_RECT_F BuildMapDisplayUkButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        return D2D1::RectF(panel.left + 12.0f, panel.top + 134.0f, panel.left + 132.0f, panel.top + 166.0f);
    }

    D2D1_RECT_F BuildMapDisplayWorldButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        return D2D1::RectF(panel.left + 142.0f, panel.top + 134.0f, panel.left + 262.0f, panel.top + 166.0f);
    }

    D2D1_RECT_F BuildToolbarPanelRect() const
    {
        const float left = 18.0f + m_toolbarPanelOffsetX;
        const float top = 18.0f + m_toolbarPanelOffsetY;
        return D2D1::RectF(left, top, left + 274.0f, top + 178.0f);
    }

    D2D1_RECT_F BuildToolbarCloseButtonRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        return D2D1::RectF(panel.right - 34.0f, panel.top + 10.0f, panel.right - 10.0f, panel.top + 34.0f);
    }

    D2D1_RECT_F BuildToolbarDragRect() const
    {
        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        const D2D1_RECT_F closeRect = BuildToolbarCloseButtonRect();
        return D2D1::RectF(panel.left + 12.0f, panel.top, closeRect.left - 6.0f, panel.top + 38.0f);
    }

    void SetOverlayInputFocus(OverlayInputFocus focus)
    {
        m_overlayInputFocus = focus;
        m_responderChatInputFocused = focus == OverlayInputFocus::ResponderChat;
        m_privateChatInputFocused = focus == OverlayInputFocus::PrivateChat;
        Invalidate();
    }

    void ClearOverlayInputFocus()
    {
        SetOverlayInputFocus(OverlayInputFocus::None);
    }

    D2D1_RECT_F BuildNoteCloseRect(const D2D1_RECT_F& bubble) const
    {
        return D2D1::RectF(bubble.right - 24.0f, bubble.top + 6.0f, bubble.right - 6.0f, bubble.top + 24.0f);
    }

    D2D1_RECT_F BuildNoteEditorRect(const ViewState& view) const
    {
        D2D1_POINT_2F anchor = GeoToScreen(view, m_noteEditorLat, m_noteEditorLon);
        float textHeight = 46.0f;
        if (m_overlayUi.BodyFormat()) {
            const float textWidth = kNoteEditorWidth - 44.0f;
            textHeight = MaxValue(46.0f, m_overlayUi.MeasureTextHeight(m_noteEditorText.empty() ? L" " : m_noteEditorText, m_overlayUi.BodyFormat(), textWidth));
        }
        const float desiredHeight = 42.0f + textHeight + 78.0f;
        const float maxHeight = MaxValue(kNoteEditorMinHeight, static_cast<float>(view.height) - 28.0f);
        const float height = ClampValue(desiredHeight, kNoteEditorMinHeight, MinValue(360.0f, maxHeight));
        return BuildNoteBubbleRect(view, anchor, kNoteEditorWidth, height);
    }

    void StartDraftNoteAt(double lat, double lon)
    {
        m_noteEditorMode = NoteEditorMode::New;
        m_noteEditorIndex = static_cast<size_t>(-1);
        m_noteEditorLat = lat;
        m_noteEditorLon = lon;
        m_noteEditorText.clear();
        m_noteEditorCursor = 0;
        m_addNoteMode = false;
        SetOverlayInputFocus(OverlayInputFocus::NoteEditor);
        SetFocus(m_hwnd);
        Invalidate();
    }

    void StartEditNote(size_t index)
    {
        if (index >= m_notes.size())
            return;

        m_noteEditorMode = NoteEditorMode::Edit;
        m_noteEditorIndex = index;
        m_noteEditorLat = m_notes[index].latitude;
        m_noteEditorLon = m_notes[index].longitude;
        m_noteEditorText = m_notes[index].text;
        m_noteEditorCursor = m_noteEditorText.size();
        m_addNoteMode = false;
        SetOverlayInputFocus(OverlayInputFocus::NoteEditor);
        SetFocus(m_hwnd);
        Invalidate();
    }

    void CancelNoteEditor()
    {
        m_noteEditorMode = NoteEditorMode::None;
        m_noteEditorIndex = static_cast<size_t>(-1);
        m_noteEditorText.clear();
        m_noteEditorCursor = 0;
        if (m_overlayInputFocus == OverlayInputFocus::NoteEditor)
            ClearOverlayInputFocus();
        Invalidate();
    }

    void CommitNoteEditor()
    {
        std::wstring text = Trim(m_noteEditorText);
        if (text.empty()) {
            CancelNoteEditor();
            return;
        }

        if (m_noteEditorMode == NoteEditorMode::New) {
            if (m_onNoteCreate)
                m_onNoteCreate(text, m_noteEditorLat, m_noteEditorLon);
        }
        else if (m_noteEditorMode == NoteEditorMode::Edit) {
            if (m_onNoteUpdate && m_noteEditorIndex < m_notes.size())
                m_onNoteUpdate(m_noteEditorIndex, text);
        }
        CancelNoteEditor();
    }

    void DeleteNoteAt(size_t index)
    {
        if (index >= m_notes.size())
            return;

        if (m_onNoteDelete)
            m_onNoteDelete(index);
        if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex == index)
            CancelNoteEditor();
        Invalidate();
    }

    void InsertNoteEditorText(wchar_t ch)
    {
        if (m_noteEditorMode == NoteEditorMode::None)
            return;
        if (ch < 32 && ch != L'\n' && ch != L'\r')
            return;
        if (ch == L'\r')
            ch = L'\n';

        m_noteEditorCursor = MinValue(m_noteEditorCursor, m_noteEditorText.size());
        m_noteEditorText.insert(m_noteEditorText.begin() + static_cast<std::ptrdiff_t>(m_noteEditorCursor), ch);
        ++m_noteEditorCursor;
        Invalidate();
    }

    bool HandleNoteEditorKeyDown(WPARAM key)
    {
        if (m_noteEditorMode == NoteEditorMode::None)
            return false;

        switch (key) {
        case VK_ESCAPE:
            CancelNoteEditor();
            return true;
        case VK_RETURN:
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                CommitNoteEditor();
                return true;
            }
            return false;
        case VK_BACK:
            if (m_noteEditorCursor > 0 && !m_noteEditorText.empty()) {
                m_noteEditorText.erase(m_noteEditorText.begin() + static_cast<std::ptrdiff_t>(m_noteEditorCursor - 1));
                --m_noteEditorCursor;
                Invalidate();
            }
            return true;
        case VK_DELETE:
            if (m_noteEditorCursor < m_noteEditorText.size()) {
                m_noteEditorText.erase(m_noteEditorText.begin() + static_cast<std::ptrdiff_t>(m_noteEditorCursor));
                Invalidate();
            }
            return true;
        case VK_LEFT:
            if (m_noteEditorCursor > 0) {
                --m_noteEditorCursor;
                Invalidate();
            }
            return true;
        case VK_RIGHT:
            if (m_noteEditorCursor < m_noteEditorText.size()) {
                ++m_noteEditorCursor;
                Invalidate();
            }
            return true;
        case VK_HOME:
            m_noteEditorCursor = 0;
            Invalidate();
            return true;
        case VK_END:
            m_noteEditorCursor = m_noteEditorText.size();
            Invalidate();
            return true;
        }
        return false;
    }

    bool HandleNoteEditorChar(WPARAM ch)
    {
        if (m_noteEditorMode == NoteEditorMode::None)
            return false;
        if (ch == L'\b' || ch == 0x1B)
            return true;
        InsertNoteEditorText(static_cast<wchar_t>(ch));
        return true;
    }

    struct NoteHit
    {
        bool hit = false;
        bool close = false;
        size_t index = 0;
    };

    NoteHit HitTestNote(int x, int y) const
    {
        NoteHit result;
        const ViewState view = BuildViewState(80.0);

        for (size_t i = m_notes.size(); i > 0; --i) {
            const size_t index = i - 1;
            if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex == index)
                continue;
            const MapNote& note = m_notes[index];
            if (!IsGeoPointInView(view, note.latitude, note.longitude))
                continue;

            D2D1_POINT_2F anchor = GeoToScreen(view, note.latitude, note.longitude);
            const std::wstring text = NoteDisplayText(note);
            D2D1_RECT_F rect = BuildNoteBubbleRect(view, anchor, kNoteBubbleWidth, NoteBubbleHeightForText(text));
            if (PointInRect(x, y, BuildNoteCloseRect(rect))) {
                result.hit = true;
                result.close = true;
                result.index = index;
                return result;
            }
            if (PointInRect(x, y, rect)) {
                result.hit = true;
                result.index = index;
                return result;
            }
        }

        return result;
    }

    bool HitNoteInterface(int x, int y) const
    {
        if (m_showToolbarPanel) {
            if (PointInRect(x, y, BuildToolbarPanelRect()))
                return true;
            if (m_addNoteMode && PointInRect(x, y, BuildAddNotePromptRect()))
                return true;
        }
        if (m_noteEditorMode != NoteEditorMode::None) {
            D2D1_RECT_F editor = BuildNoteEditorRect(BuildViewState());
            if (PointInRect(x, y, editor))
                return true;
        }
        return HitTestNote(x, y).hit;
    }

    bool HandleNotePointerDown(int x, int y)
    {
        if (!m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return false;

        if (m_showToolbarPanel) {
            const D2D1_RECT_F addButton = BuildAddNoteButtonRect();
            const D2D1_RECT_F fitButton = BuildFitAlertsButtonRect();
            const D2D1_RECT_F resetButton = BuildResetViewButtonRect();
            const D2D1_RECT_F refreshButton = BuildRefreshButtonRect();
            const D2D1_RECT_F ukButton = BuildMapDisplayUkButtonRect();
            const D2D1_RECT_F worldButton = BuildMapDisplayWorldButtonRect();
            const D2D1_RECT_F closeButton = BuildToolbarCloseButtonRect();
            if (PointInRect(x, y, closeButton)) {
                ClearOverlayInputFocus();
                if (m_onPanelClose)
                    m_onPanelClose(L"map_controls");
                Invalidate();
                return true;
            }
            if (PointInRect(x, y, BuildToolbarDragRect())) {
                ClearOverlayInputFocus();
                SetCapture(m_hwnd);
                m_draggingToolbarPanel = true;
                m_notificationUiMouseDown = true;
                m_lastMouse = POINT{ x, y };
                Invalidate();
                return true;
            }

            if (PointInRect(x, y, refreshButton)) {
                if (m_onRefresh)
                    m_onRefresh();
                Invalidate();
                return true;
            }

            if (PointInRect(x, y, resetButton)) {
                ResetView();
                return true;
            }

            if (PointInRect(x, y, fitButton)) {
                FitToAlerts();
                return true;
            }

            if (PointInRect(x, y, addButton)) {
                m_addNoteMode = !m_addNoteMode;
                if (m_addNoteMode)
                    CancelNoteEditor();
                Invalidate();
                return true;
            }

            if (PointInRect(x, y, ukButton)) {
                if (m_displayWorldMap) {
                    if (m_onMapDisplayMode)
                        m_onMapDisplayMode(false);
                    else
                        SetDisplayWorldMap(false);
                }
                Invalidate();
                return true;
            }

            if (PointInRect(x, y, worldButton)) {
                if (!m_displayWorldMap) {
                    if (m_onMapDisplayMode)
                        m_onMapDisplayMode(true);
                    else
                        SetDisplayWorldMap(true);
                }
                Invalidate();
                return true;
            }
        }

        if (m_noteEditorMode != NoteEditorMode::None) {
            const D2D1_RECT_F editor = BuildNoteEditorRect(BuildViewState());
            const D2D1_RECT_F closeRect = BuildNoteCloseRect(editor);
            const D2D1_RECT_F saveRect = D2D1::RectF(editor.right - 156.0f, editor.bottom - 42.0f, editor.right - 84.0f, editor.bottom - 12.0f);
            const D2D1_RECT_F cancelRect = D2D1::RectF(editor.right - 78.0f, editor.bottom - 42.0f, editor.right - 12.0f, editor.bottom - 12.0f);
            if (PointInRect(x, y, closeRect)) {
                if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex < m_notes.size())
                    DeleteNoteAt(m_noteEditorIndex);
                else
                    CancelNoteEditor();
                return true;
            }
            if (PointInRect(x, y, saveRect)) {
                CommitNoteEditor();
                return true;
            }
            if (PointInRect(x, y, cancelRect)) {
                CancelNoteEditor();
                return true;
            }
            if (PointInRect(x, y, editor)) {
                SetFocus(m_hwnd);
                SetOverlayInputFocus(OverlayInputFocus::NoteEditor);
                return true;
            }
        }

        NoteHit hit = HitTestNote(x, y);
        if (hit.hit) {
            if (hit.close)
                DeleteNoteAt(hit.index);
            else
                StartEditNote(hit.index);
            return true;
        }

        if (m_addNoteMode) {
            GeoPoint geo = ScreenToGeo(x, y);
            StartDraftNoteAt(geo.lat, geo.lon);
            return true;
        }

        return false;
    }

    PolygonPointHit HitTestEditablePolygonPoint(int x, int y) const
    {
        PolygonPointHit result;
        if (!m_polygonCaptureActive ||
            m_activeNotificationPolygonIndex >= m_notificationPolygons.size())
            return result;

        const GeoPolygon& polygon = m_notificationPolygons[m_activeNotificationPolygonIndex];
        const ViewState view = BuildViewState(32.0);
        constexpr float hitRadius = 12.0f;
        float bestDistance = hitRadius;
        for (size_t i = 0; i < polygon.points.size(); ++i) {
            const GeoPoint& point = polygon.points[i];
            if (!IsGeoPointInView(view, point.lat, point.lon))
                continue;
            D2D1_POINT_2F screen = GeoToScreen(view, point.lat, point.lon);
            float dx = screen.x - static_cast<float>(x);
            float dy = screen.y - static_cast<float>(y);
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance <= bestDistance) {
                bestDistance = distance;
                result.hit = true;
                result.polygonIndex = m_activeNotificationPolygonIndex;
                result.pointIndex = i;
            }
        }
        return result;
    }

    bool HandlePolygonPointPointerDown(int x, int y)
    {
        PolygonPointHit hit = HitTestEditablePolygonPoint(x, y);
        if (!hit.hit)
            return false;

        SetCapture(m_hwnd);
        m_draggingPolygonPoint = true;
        m_draggingPolygonIndex = hit.polygonIndex;
        m_draggingPolygonPointIndex = hit.pointIndex;
        m_mouseDown.x = x;
        m_mouseDown.y = y;
        m_lastMouse = m_mouseDown;
        m_dragging = true;
        return true;
    }

    static bool PointInGeoPolygon(double lat, double lon, const std::vector<GeoPoint>& points)
    {
        if (points.size() < 3)
            return false;

        bool inside = false;
        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            const GeoPoint& a = points[i];
            const GeoPoint& b = points[j];
            const bool intersects = ((a.lat > lat) != (b.lat > lat)) &&
                (lon < (b.lon - a.lon) * (lat - a.lat) / ((b.lat - a.lat) == 0.0 ? 1e-12 : (b.lat - a.lat)) + a.lon);
            if (intersects)
                inside = !inside;
        }
        return inside;
    }

    bool HandlePolygonRightClick(int x, int y)
    {
        if (!m_polygonCaptureActive ||
            m_activeNotificationPolygonIndex >= m_notificationPolygons.size())
            return false;

        PolygonPointHit hit = HitTestEditablePolygonPoint(x, y);
        if (hit.hit) {
            if (hit.polygonIndex < m_notificationPolygons.size() &&
                hit.pointIndex < m_notificationPolygons[hit.polygonIndex].points.size())
            {
                m_notificationPolygons[hit.polygonIndex].points.erase(m_notificationPolygons[hit.polygonIndex].points.begin() + hit.pointIndex);
                if (m_onPolygonPointDelete)
                    m_onPolygonPointDelete(hit.polygonIndex, hit.pointIndex);
                Invalidate();
            }
            return true;
        }

        GeoPoint geo = ScreenToGeo(x, y);
        if (PointInGeoPolygon(geo.lat, geo.lon, m_notificationPolygons[m_activeNotificationPolygonIndex].points)) {
            m_notificationPolygons[m_activeNotificationPolygonIndex].points.clear();
            if (m_onPolygonClear)
                m_onPolygonClear(m_activeNotificationPolygonIndex);
            Invalidate();
            return true;
        }

        return false;
    }

    void OnMouseMove(int x, int y, UINT buttons)
    {
        const POINT oldHover = m_hoverPoint;
        const bool oldHoverOverlay = HitAnyOverlayInterface(oldHover.x, oldHover.y);
        m_hoverPoint = POINT{ x, y };

        if (!m_trackingMouseLeave) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            if (TrackMouseEvent(&tme))
                m_trackingMouseLeave = true;
        }

        if (m_draggingToolbarPanel && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            POINT pt{ x, y };
            const int dx = pt.x - m_lastMouse.x;
            const int dy = pt.y - m_lastMouse.y;
            if (dx != 0 || dy != 0) {
                m_toolbarPanelOffsetX += static_cast<float>(dx);
                m_toolbarPanelOffsetY += static_cast<float>(dy);
                ClampToolbarPanelOffsets(BuildViewState());
                m_lastMouse = pt;
                Invalidate();
            }
            return;
        }

        if (m_draggingUsersPanel && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            POINT pt{ x, y };
            const int dx = pt.x - m_lastMouse.x;
            const int dy = pt.y - m_lastMouse.y;
            if (dx != 0 || dy != 0) {
                m_usersPanelOffsetX += static_cast<float>(dx);
                m_usersPanelOffsetY += static_cast<float>(dy);
                ClampUsersPanelOffsets(BuildViewState());
                m_lastMouse = pt;
                Invalidate();
            }
            return;
        }

        if (m_draggingPrivateChatPanel && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            POINT pt{ x, y };
            const int dx = pt.x - m_lastMouse.x;
            const int dy = pt.y - m_lastMouse.y;
            if (dx != 0 || dy != 0) {
                m_privateChatOffsetX += static_cast<float>(dx);
                m_privateChatOffsetY += static_cast<float>(dy);
                ClampPrivateChatPanelOffsets(BuildViewState());
                m_lastMouse = pt;
                Invalidate();
            }
            return;
        }

        if (m_draggingNotificationHistoryScrollbar && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            SetNotificationHistoryScrollFromThumbY(static_cast<float>(y), m_notificationHistoryScrollbarDragOffset);
            return;
        }

        if (m_draggingNotificationHistoryContent && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
            const float maxScroll = layout.hasHistory ? MaxNotificationHistoryScroll(layout.historyRect) : 0.0f;
            const float dy = static_cast<float>(y - m_lastMouse.y);
            if (dy != 0.0f && maxScroll > 0.0f) {
                m_notificationHistoryScroll = ClampValue(m_notificationHistoryScroll - dy, 0.0f, maxScroll);
                m_lastMouse = POINT{ x, y };
                Invalidate();
            }
            return;
        }

        if (m_draggingPolygonPoint && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            if (m_draggingPolygonIndex < m_notificationPolygons.size() &&
                m_draggingPolygonPointIndex < m_notificationPolygons[m_draggingPolygonIndex].points.size())
            {
                GeoPoint geo = ScreenToGeo(x, y);
                m_notificationPolygons[m_draggingPolygonIndex].points[m_draggingPolygonPointIndex] = geo;
                if (m_onPolygonPointMove)
                    m_onPolygonPointMove(m_draggingPolygonIndex, m_draggingPolygonPointIndex, geo.lat, geo.lon);
                Invalidate();
            }
            return;
        }

        const bool hoverOverlay = HitAnyOverlayInterface(x, y);
        if ((oldHoverOverlay || hoverOverlay) && (oldHover.x != x || oldHover.y != y))
            Invalidate();

        if (m_notificationUiMouseDown || hoverOverlay) {
            if (!m_hoveredAlertId.empty() || !m_hoveredEarthquakeId.empty() || !m_hoveredWeatherSystemId.empty() ||
                !m_hoveredWeatherWarningId.empty() || !m_hoveredFloodId.empty()) {
                m_hoveredAlertId.clear();
                m_hoveredEarthquakeId.clear();
                m_hoveredWeatherSystemId.clear();
                m_hoveredWeatherWarningId.clear();
                m_hoveredFloodId.clear();
                Invalidate();
            }
            return;
        }

        std::wstring hoveredId = HitTestAlert(x, y);
        std::wstring hoveredEarthquakeId = hoveredId.empty() ? HitTestEarthquake(x, y) : L"";
        std::wstring hoveredWeatherSystemId = hoveredId.empty() && hoveredEarthquakeId.empty() ? HitTestWeatherSystem(x, y) : L"";
        std::wstring hoveredWeatherWarningId = hoveredId.empty() && hoveredEarthquakeId.empty() && hoveredWeatherSystemId.empty() ? HitTestWeatherWarning(x, y) : L"";
        std::wstring hoveredFloodId = hoveredId.empty() && hoveredEarthquakeId.empty() && hoveredWeatherSystemId.empty() && hoveredWeatherWarningId.empty() ? HitTestFlood(x, y) : L"";
        if (hoveredId != m_hoveredAlertId ||
            hoveredEarthquakeId != m_hoveredEarthquakeId ||
            hoveredWeatherSystemId != m_hoveredWeatherSystemId ||
            hoveredWeatherWarningId != m_hoveredWeatherWarningId ||
            hoveredFloodId != m_hoveredFloodId)
        {
            m_hoveredAlertId = std::move(hoveredId);
            m_hoveredEarthquakeId = std::move(hoveredEarthquakeId);
            m_hoveredWeatherSystemId = std::move(hoveredWeatherSystemId);
            m_hoveredWeatherWarningId = std::move(hoveredWeatherWarningId);
            m_hoveredFloodId = std::move(hoveredFloodId);
            Invalidate();
        }

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
                m_interactivePan = true;
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

        if (m_draggingToolbarPanel) {
            m_draggingToolbarPanel = false;
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_draggingUsersPanel) {
            m_draggingUsersPanel = false;
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_draggingPrivateChatPanel) {
            m_draggingPrivateChatPanel = false;
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_draggingNotificationHistoryScrollbar) {
            m_draggingNotificationHistoryScrollbar = false;
            m_notificationHistoryScrollbarDragOffset = 0.0f;
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_draggingNotificationHistoryContent) {
            m_draggingNotificationHistoryContent = false;
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_draggingPolygonPoint) {
            m_draggingPolygonPoint = false;
            m_draggingPolygonIndex = static_cast<size_t>(-1);
            m_draggingPolygonPointIndex = static_cast<size_t>(-1);
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (m_notificationUiMouseDown || HitUsersPanelInterface(x, y) || HitPrivateChatInterface(x, y) || HitResponderChatInterface(x, y) || HitNotificationInterface(x, y) || HitNoteInterface(x, y)) {
            m_notificationUiMouseDown = false;
            m_dragging = false;
            m_interactivePan = false;
            KillTimer(m_hwnd, kInteractionIdleTimer);
            Invalidate();
            return;
        }

        if (!m_dragging) {
            if (m_polygonCaptureActive && m_onPolygonPoint) {
                GeoPoint geo = ScreenToGeo(x, y);
                m_onPolygonPoint(geo.lat, geo.lon);
                m_dragging = false;
                m_interactivePan = false;
                KillTimer(m_hwnd, kInteractionIdleTimer);
                Invalidate();
                return;
            }

            std::wstring id = HitTestAlert(x, y);
            if (!id.empty() && m_onSelect)
                m_onSelect(id);
        }

        m_dragging = false;
        m_interactivePan = false;
        KillTimer(m_hwnd, kInteractionIdleTimer);
        Invalidate();
    }

    void OnDoubleClick(int x, int y)
    {
        if (m_notificationContextMenuVisible) {
            HandleNotificationContextMenuPointerDown(x, y);
            return;
        }

        if (TryActivateNotificationHistoryItem(x, y))
            return;

        if (HitUsersPanelInterface(x, y) || HitPrivateChatInterface(x, y) || HitResponderChatInterface(x, y) || HitNotificationInterface(x, y) || HitNoteInterface(x, y))
            return;

        GeoPoint geo = ScreenToGeo(x, y);
        StartDraftNoteAt(geo.lat, geo.lon);
    }

    void OnMouseWheel(int screenX, int screenY, short delta)
    {
        POINT pt{ screenX, screenY };
        ScreenToClient(m_hwnd, &pt);

        if (HitUsersPanelInterface(pt.x, pt.y))
            return;
        if (HitPrivateChatInterface(pt.x, pt.y))
            return;
        if (HitResponderChatInterface(pt.x, pt.y))
            return;

        if (TryScrollNotificationHistoryAt(pt.x, pt.y, delta))
            return;

        int newZoom = m_zoom + ((delta > 0) ? 1 : -1);
        newZoom = ClampValue(newZoom, kMinZoom, kMaxZoom);
        if (newZoom == m_zoom)
            return;

        GeoPoint geoUnderCursor = ScreenToGeo(pt.x, pt.y);

        m_interactivePan = true;
        SetTimer(m_hwnd, kInteractionIdleTimer, kInteractionIdleMs, nullptr);

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

        ComPtr<ID2D1HwndRenderTarget> hwndRt;
        if (FAILED(g_d2dFactory->CreateHwndRenderTarget(rtProps, hwndProps, &hwndRt)))
            return;

        m_hwndRt = hwndRt;
        hwndRt.As(&m_rt);

        // Keep Direct2D drawing units aligned with Win32 mouse/client coordinates.
        // Otherwise high-DPI scaling can make ScreenToGeo and GeoToScreen disagree,
        // which places newly-created notes away from the double-clicked map point.
        m_hwndRt->SetDpi(96.0f, 96.0f);

        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.10f, 0.10f, 0.95f), &m_severeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.62f, 0.18f, 0.95f), &m_moderateBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.52f, 0.90f, 0.95f), &m_minorBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.20f, 0.70f, 0.55f, 0.95f), &m_unknownBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.92f, 0.2f, 0.55f), &m_selectedBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(kMapWaterR, kMapWaterG, kMapWaterB), &m_placeholderBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.35f, 0.35f), &m_borderBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.30f, 0.55f, 0.25f, 0.18f), &m_outlineFillBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.28f, 0.12f, 0.92f), &m_outlineStrokeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.05f, 0.10f, 0.18f, 0.72f), &m_panelBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.90f), &m_textBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.40f, 0.20f, 0.95f, 0.95f), &m_noteBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.22f, 0.24f, 0.27f, 0.96f), &m_laneTileBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.42f, 0.78f, 0.18f), &m_polygonFillBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.30f, 0.68f, 0.88f), &m_polygonStrokeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.68f, 0.10f, 0.22f), &m_draftFillBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.88f, 0.42f, 0.02f, 0.95f), &m_draftStrokeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.70f, 0.10f, 0.16f, 0.86f), &m_earthquakeBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.02f, 0.48f, 0.64f, 0.92f), &m_weatherSystemBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.02f, 0.06f, 0.09f, 0.68f), &m_forecastRingBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.80f, 0.10f, 0.96f), &m_weatherWarningBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.80f, 0.10f, 0.18f), &m_weatherWarningFillBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.05f, 0.42f, 0.84f, 0.94f), &m_floodBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.97f, 0.84f, 0.88f), &m_roadBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.11f, 0.19f, 0.24f, 0.70f), &m_roadCasingBrush);
        if (!m_forecastErrorStrokeStyle) {
            D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties(
                D2D1_CAP_STYLE_FLAT,
                D2D1_CAP_STYLE_FLAT,
                D2D1_CAP_STYLE_ROUND,
                D2D1_LINE_JOIN_ROUND,
                10.0f,
                D2D1_DASH_STYLE_DASH,
                0.0f);
            g_d2dFactory->CreateStrokeStyle(&strokeProps, nullptr, 0, &m_forecastErrorStrokeStyle);
        }

        if (g_dwriteFactory && !m_noteTextFormat) {
            g_dwriteFactory->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                13.0f,
                L"en-gb",
                &m_noteTextFormat);
            if (m_noteTextFormat) {
                m_noteTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
                m_noteTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }
        }
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
        m_hwndRt.Reset();
        m_outlineFillBrush.Reset();
        m_outlineStrokeBrush.Reset();
        m_panelBrush.Reset();
        m_textBrush.Reset();
        m_noteBrush.Reset();
        m_laneTileBrush.Reset();
        m_polygonFillBrush.Reset();
        m_polygonStrokeBrush.Reset();
        m_draftFillBrush.Reset();
        m_draftStrokeBrush.Reset();
        m_earthquakeBrush.Reset();
        m_weatherSystemBrush.Reset();
        m_forecastRingBrush.Reset();
        m_weatherWarningBrush.Reset();
        m_weatherWarningFillBrush.Reset();
        m_floodBrush.Reset();
        m_roadBrush.Reset();
        m_roadCasingBrush.Reset();
        m_forecastErrorStrokeStyle.Reset();
        m_laneBitmaps.clear();
        m_noteTextFormat.Reset();
        m_overlayUi.DiscardDeviceResources();
        InvalidateSceneCache();
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

    std::shared_ptr<TileEntry> FindTile(const TileKey& key)
    {
        std::lock_guard<std::mutex> lk(m_tileMutex);
        auto it = m_tiles.find(key);
        if (it == m_tiles.end())
            return {};
        return it->second;
    }

    ComPtr<ID2D1Bitmap> GetCachedTileBitmap(const TileKey& key)
    {
        auto entry = FindTile(key);
        if (!entry)
            return {};

        std::lock_guard<std::mutex> lk(entry->mutex);
        entry->lastUsedMs = GetTickCount64();
        return entry->bitmap;
    }

    bool TryDrawFallbackTile(const TileKey& key, const D2D1_RECT_F& dest)
    {
        if (!m_rt || key.z <= kMinZoom)
            return false;

        const int maxDelta = MinValue(kMaxFallbackTileZoomDelta, key.z - kMinZoom);
        for (int delta = 1; delta <= maxDelta; ++delta) {
            const int parentZ = key.z - delta;
            const int scale = 1 << delta;
            TileKey parentKey{ parentZ, key.x / scale, key.y / scale };
            ComPtr<ID2D1Bitmap> parentBmp = GetCachedTileBitmap(parentKey);
            if (!parentBmp)
                continue;

            const int subX = key.x % scale;
            const int subY = key.y % scale;
            const float srcSize = 256.0f / static_cast<float>(scale);
            const D2D1_RECT_F src = D2D1::RectF(
                subX * srcSize,
                subY * srcSize,
                (subX + 1) * srcSize,
                (subY + 1) * srcSize);

            m_rt->DrawBitmap(parentBmp.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, src);
            return true;
        }

        return false;
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
        }

        int active = g_activeTileDownloads.load();
        while (active < kMaxConcurrentTileDownloads) {
            if (g_activeTileDownloads.compare_exchange_weak(active, active + 1))
                break;
        }
        if (active >= kMaxConcurrentTileDownloads)
            return;

        {
            std::lock_guard<std::mutex> lk(entry->mutex);
            if (entry->ready || entry->loading) {
                --g_activeTileDownloads;
                return;
            }

            entry->loading = true;
            entry->lastAttemptMs = GetTickCount64();
        }

        HWND hwnd = m_hwnd;

        ScheduleMapTask([hwnd, key, entry]() {
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

            --g_activeTileDownloads;

            if (hwnd && !g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_TILE_READY, 0, 0);
            });
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

    void PruneTileCache()
    {
        std::lock_guard<std::mutex> lk(m_tileMutex);
        if (m_tiles.size() <= kMaxTileCacheEntries)
            return;

        std::vector<std::pair<TileKey, ULONGLONG>> candidates;
        candidates.reserve(m_tiles.size());

        for (const auto& item : m_tiles) {
            std::lock_guard<std::mutex> entryLock(item.second->mutex);
            if (!item.second->loading)
                candidates.push_back({ item.first, item.second->lastUsedMs });
        }

        if (candidates.empty())
            return;

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
            });

        const size_t removeCount = MinValue(candidates.size(), m_tiles.size() - kMaxTileCacheEntries);
        for (size_t i = 0; i < removeCount; ++i)
            m_tiles.erase(candidates[i].first);
    }

    ID2D1SolidColorBrush* BrushForSeverity(const std::wstring& severity)
    {
        std::wstring b = SeverityBucket(severity);
        if (b == L"severe") return m_severeBrush.Get();
        if (b == L"moderate") return m_moderateBrush.Get();
        if (b == L"minor") return m_minorBrush.Get();
        return m_unknownBrush.Get();
    }

    const TrafficAlert* FindAlertById(const std::wstring& id) const
    {
        if (id.empty())
            return nullptr;
        for (const TrafficAlert& alert : m_alerts) {
            if (alert.id == id)
                return &alert;
        }
        return nullptr;
    }

    static bool HasLaneClosureOverlay(const TrafficAlert& alert)
    {
        return alert.lanesClosed > 0 || alert.lanesTotal > 0 ||
            !alert.laneImageUrls.empty() || !alert.laneClosedStates.empty();
    }

    bool AnyPointInView(const ViewState& view, const std::vector<GeoPoint>& points) const
    {
        return AnyPointInViewStatic(view, points);
    }

    void DrawPolygonPath(
        const ViewState& view,
        const std::vector<GeoPoint>& points,
        bool closed,
        ID2D1Brush* fill,
        ID2D1Brush* stroke,
        float strokeWidth)
    {
        if (!m_rt || !g_d2dFactory || points.size() < 2 || !AnyPointInView(view, points))
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        D2D1_FIGURE_BEGIN begin = closed && points.size() >= 3 ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW;
        sink->BeginFigure(GeoToScreen(view, points[0].lat, points[0].lon), begin);
        for (size_t i = 1; i < points.size(); ++i)
            sink->AddLine(GeoToScreen(view, points[i].lat, points[i].lon));
        sink->EndFigure(closed && points.size() >= 3 ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);

        if (FAILED(sink->Close()))
            return;

        if (closed && points.size() >= 3 && fill)
            m_rt->FillGeometry(geom.Get(), fill);
        if (stroke)
            m_rt->DrawGeometry(geom.Get(), stroke, strokeWidth);

        for (const GeoPoint& pt : points) {
            D2D1_POINT_2F p = GeoToScreen(view, pt.lat, pt.lon);
            m_rt->FillEllipse(D2D1::Ellipse(p, 3.5f, 3.5f), stroke);
        }
    }

    void DrawNotificationPolygons(const ViewState& view)
    {
        for (size_t i = 0; i < m_notificationPolygons.size(); ++i) {
            const bool active = m_polygonCaptureActive && i == m_activeNotificationPolygonIndex;
            const GeoPolygon& polygon = m_notificationPolygons[i];
            DrawPolygonPath(view, polygon.points, true, m_polygonFillBrush.Get(), active ? m_draftStrokeBrush.Get() : m_polygonStrokeBrush.Get(), active ? 3.0f : 2.0f);
            if (active) {
                for (const GeoPoint& point : polygon.points) {
                    D2D1_POINT_2F screen = GeoToScreen(view, point.lat, point.lon);
                    m_rt->FillEllipse(D2D1::Ellipse(screen, 7.0f, 7.0f), m_selectedBrush.Get());
                    m_rt->DrawEllipse(D2D1::Ellipse(screen, 7.0f, 7.0f), m_draftStrokeBrush.Get(), 2.0f);
                }
            }
        }

        DrawPolygonPath(view, m_draftPolygon, m_draftPolygon.size() >= 3, m_draftFillBrush.Get(), m_draftStrokeBrush.Get(), 2.0f);
    }

    std::wstring EarthquakeOverlayText(const EarthquakeEvent& event) const
    {
        wchar_t mag[32]{};
        swprintf_s(mag, L"M%.1f", event.magnitude);
        std::wstring text = mag;
        if (!event.place.empty()) {
            text += L" ";
            text += event.place;
        }
        return text;
    }

    static std::wstring EarthquakeRenderKey(const EarthquakeEvent& event)
    {
        if (!event.id.empty())
            return event.id;
        return event.place + L"|" + std::to_wstring(event.timeMs);
    }

    void DrawEarthquakeOverlayLabel(const ViewState& view, const EarthquakeEvent& event, D2D1_POINT_2F anchor, float radius, bool forceVisible = false)
    {
        if ((!m_showEarthquakeOverlayLabels && !forceVisible) || !m_noteTextFormat)
            return;

        std::wstring text = EarthquakeOverlayText(event);
        if (text.empty())
            return;

        DrawMeasuredBlipLabel(view, anchor, radius, text, m_earthquakeBrush.Get(), 122.0f, 320.0f);
    }

    void DrawEarthquakes(const ViewState& view)
    {
        if (!m_rt || m_earthquakes.empty())
            return;

        if (m_zoom <= 4 && m_earthquakes.size() > 800) {
            struct EarthquakeCell
            {
                D2D1_POINT_2F point{};
                double magnitude = 0.0;
                int count = 0;
            };

            const float cellSize = m_zoom <= 2 ? 18.0f : 12.0f;
            std::unordered_map<long long, EarthquakeCell> cells;
            cells.reserve(1024);
            const EarthquakeEvent* selectedEvent = nullptr;

            for (const EarthquakeEvent& event : m_earthquakes) {
                if (!event.hasLocation || !IsGeoPointInView(view, event.latitude, event.longitude))
                    continue;

                if (!m_selectedId.empty() && EarthquakeRenderKey(event) == m_selectedId)
                    selectedEvent = &event;

                D2D1_POINT_2F p = GeoToScreen(view, event.latitude, event.longitude);
                int cellX = static_cast<int>(std::floor(p.x / cellSize));
                int cellY = static_cast<int>(std::floor(p.y / cellSize));
                long long key = (static_cast<long long>(static_cast<unsigned int>(cellX)) << 32) ^
                    static_cast<unsigned int>(cellY);
                EarthquakeCell& cell = cells[key];
                ++cell.count;
                if (cell.count == 1 || event.magnitude > cell.magnitude) {
                    cell.point = p;
                    cell.magnitude = event.magnitude;
                }
            }

            for (const auto& item : cells) {
                const EarthquakeCell& cell = item.second;
                float radius = static_cast<float>(ClampValue(4.0 + cell.magnitude * 1.65 + std::log2(static_cast<double>(std::max(1, cell.count))) * 0.85, 5.0, 18.0));
                m_rt->FillEllipse(D2D1::Ellipse(cell.point, radius, radius), m_earthquakeBrush.Get());
                m_rt->DrawEllipse(D2D1::Ellipse(cell.point, radius, radius), m_borderBrush.Get(), 1.15f);
            }
            if (selectedEvent) {
                D2D1_POINT_2F p = GeoToScreen(view, selectedEvent->latitude, selectedEvent->longitude);
                float radius = static_cast<float>(ClampValue(4.0 + selectedEvent->magnitude * 2.2, 5.0, 22.0));
                m_rt->DrawEllipse(D2D1::Ellipse(p, radius + 8.0f, radius + 8.0f), m_selectedBrush.Get(), 4.0f);
                m_rt->DrawEllipse(D2D1::Ellipse(p, radius + 13.0f, radius + 13.0f), m_selectedBrush.Get(), 1.6f);
                m_rt->FillEllipse(D2D1::Ellipse(p, radius, radius), m_earthquakeBrush.Get());
                m_rt->DrawEllipse(D2D1::Ellipse(p, radius, radius), m_selectedBrush.Get(), 2.4f);
                DrawEarthquakeOverlayLabel(view, *selectedEvent, p, radius, true);
            }
            return;
        }

        for (const EarthquakeEvent& event : m_earthquakes) {
            if (!event.hasLocation || !IsGeoPointInView(view, event.latitude, event.longitude))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, event.latitude, event.longitude);
            float radius = static_cast<float>(ClampValue(4.0 + event.magnitude * 2.2, 5.0, 22.0));
            const bool selected = !m_selectedId.empty() && EarthquakeRenderKey(event) == m_selectedId;
            if (selected) {
                m_rt->DrawEllipse(D2D1::Ellipse(p, radius + 8.0f, radius + 8.0f), m_selectedBrush.Get(), 4.0f);
                m_rt->DrawEllipse(D2D1::Ellipse(p, radius + 13.0f, radius + 13.0f), m_selectedBrush.Get(), 1.6f);
            }
            m_rt->FillEllipse(D2D1::Ellipse(p, radius, radius), m_earthquakeBrush.Get());
            m_rt->DrawEllipse(D2D1::Ellipse(p, radius, radius), selected ? m_selectedBrush.Get() : m_borderBrush.Get(), selected ? 2.4f : 1.25f);
            DrawEarthquakeOverlayLabel(view, event, p, radius, selected || event.id == m_hoveredEarthquakeId);
        }
    }

    std::wstring WeatherSystemOverlayText(const WeatherSystemEvent& system) const
    {
        std::wstring text = system.name.empty() ? L"Weather system" : system.name;
        if (!system.category.empty()) {
            text += L" ";
            text += system.category;
        }
        if (system.windKnots > 0.0) {
            text += L" ";
            text += FormatKnotsAsMph(system.windKnots);
        }
        else if (!system.windText.empty()) {
            text += L" ";
            text += system.windText;
        }
        if (!system.basin.empty()) {
            text += L" - ";
            text += system.basin;
        }
        return text;
    }

    void DrawWeatherSystemOverlayLabel(const ViewState& view, const WeatherSystemEvent& system, D2D1_POINT_2F anchor, float radius, bool forceVisible = false)
    {
        if ((!m_showWeatherSystemOverlayLabels && !forceVisible) || !m_noteTextFormat)
            return;

        std::wstring text = WeatherSystemOverlayText(system);
        if (text.empty())
            return;

        DrawMeasuredBlipLabel(view, anchor, radius, text, m_weatherSystemBrush.Get(), 150.0f, 360.0f);
    }

    static std::wstring FormatKnotsAsMph(double knots)
    {
        const int mph = static_cast<int>(std::round(knots * 1.150779448));
        return std::to_wstring(mph) + L" mph";
    }

    ID2D1Brush* WeatherForecastBrush(const std::wstring& category) const
    {
        const int rank = WeatherSystemCategoryRank(category);
        if (rank >= 5)
            return m_severeBrush.Get();
        if (rank >= 3)
            return m_moderateBrush.Get();
        if (rank >= 2)
            return m_selectedBrush.Get();
        if (rank == 1)
            return m_unknownBrush.Get();
        return m_weatherSystemBrush.Get();
    }

    static bool SegmentIntersectsView(D2D1_POINT_2F a, D2D1_POINT_2F b, const ViewState& view, float margin = 80.0f)
    {
        D2D1_RECT_F segment = D2D1::RectF(
            MinValue(a.x, b.x) - margin,
            MinValue(a.y, b.y) - margin,
            MaxValue(a.x, b.x) + margin,
            MaxValue(a.y, b.y) + margin);
        D2D1_RECT_F screen = D2D1::RectF(-margin, -margin, static_cast<float>(view.width) + margin, static_cast<float>(view.height) + margin);
        return RectsIntersect(segment, screen);
    }

    float NauticalMilesToScreenRadius(const ViewState& view, double lat, double lon, double nm) const
    {
        if (nm <= 0.0)
            return 0.0f;

        const double latDelta = nm / 60.0;
        const double sampleLat = ClampValue(lat + latDelta, -kMaxMercatorLat, kMaxMercatorLat);
        const D2D1_POINT_2F center = GeoToScreen(view, lat, lon);
        D2D1_POINT_2F sample = GeoToScreen(view, sampleLat, lon);
        double pixels = std::abs(static_cast<double>(sample.y - center.y));

        if (pixels < 1.0) {
            const double cosLat = std::max(0.15, std::cos(lat * kPi / 180.0));
            const double lonDelta = nm / (60.0 * cosLat);
            sample = GeoToScreen(view, lat, lon + lonDelta);
            pixels = std::abs(static_cast<double>(sample.x - center.x));
        }

        return static_cast<float>(ClampValue(pixels, 4.0, 900.0));
    }

    void DrawForecastArrow(D2D1_POINT_2F from, D2D1_POINT_2F to, ID2D1Brush* brush)
    {
        if (!m_rt || !brush)
            return;
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 6.0f)
            return;

        const float ux = dx / len;
        const float uy = dy / len;
        const float px = -uy;
        const float py = ux;
        const float back = 22.0f;
        const float half = 10.0f;
        D2D1_POINT_2F left = D2D1::Point2F(to.x - ux * back + px * half, to.y - uy * back + py * half);
        D2D1_POINT_2F right = D2D1::Point2F(to.x - ux * back - px * half, to.y - uy * back - py * half);
        if (m_forecastRingBrush) {
            m_rt->DrawLine(to, left, m_forecastRingBrush.Get(), 4.0f);
            m_rt->DrawLine(to, right, m_forecastRingBrush.Get(), 4.0f);
        }
        m_rt->DrawLine(to, left, brush, 2.8f);
        m_rt->DrawLine(to, right, brush, 2.8f);
    }

    void DrawWeatherSystems(const ViewState& view)
    {
        if (!m_rt || m_weatherSystems.empty())
            return;

        for (const WeatherSystemEvent& system : m_weatherSystems) {
            const bool currentInView = system.hasLocation && IsGeoPointInView(view, system.latitude, system.longitude);
            bool anyForecastInView = false;
            for (const WeatherForecastPoint& point : system.forecastTrack) {
                if (point.hasLocation && IsGeoPointInView(view, point.latitude, point.longitude)) {
                    anyForecastInView = true;
                    break;
                }
            }
            if (!anyForecastInView && system.hasForecastLocation)
                anyForecastInView = IsGeoPointInView(view, system.forecastLatitude, system.forecastLongitude);
            if (!currentInView && !anyForecastInView)
                continue;

            D2D1_POINT_2F currentPoint{};
            ID2D1Brush* currentBrush = WeatherForecastBrush(system.category);
            bool haveCurrentPoint = false;
            if (system.hasLocation) {
                currentPoint = GeoToScreen(view, system.latitude, system.longitude);
                haveCurrentPoint = true;
            }

            D2D1_POINT_2F previous{};
            bool havePrevious = false;
            D2D1_POINT_2F lastSegmentStart{};
            D2D1_POINT_2F lastSegmentEnd{};
            ID2D1Brush* lastSegmentBrush = nullptr;
            bool haveLastSegment = false;
            for (const WeatherForecastPoint& point : system.forecastTrack) {
                if (!point.hasLocation)
                    continue;
                D2D1_POINT_2F forecast = GeoToScreen(view, point.latitude, point.longitude);
                ID2D1Brush* forecastBrush = WeatherForecastBrush(point.category);
                if (!havePrevious && haveCurrentPoint) {
                    const float dx = forecast.x - currentPoint.x;
                    const float dy = forecast.y - currentPoint.y;
                    if ((dx * dx + dy * dy) > 9.0f && SegmentIntersectsView(currentPoint, forecast, view)) {
                        ID2D1Brush* connectorBrush = forecastBrush ? forecastBrush : (currentBrush ? currentBrush : m_weatherSystemBrush.Get());
                        m_rt->DrawLine(currentPoint, forecast, connectorBrush, 2.1f);
                        lastSegmentStart = currentPoint;
                        lastSegmentEnd = forecast;
                        lastSegmentBrush = connectorBrush;
                        haveLastSegment = true;
                    }
                }
                if (havePrevious && SegmentIntersectsView(previous, forecast, view)) {
                    m_rt->DrawLine(previous, forecast, forecastBrush ? forecastBrush : m_weatherSystemBrush.Get(), 2.1f);
                    lastSegmentStart = previous;
                    lastSegmentEnd = forecast;
                    lastSegmentBrush = forecastBrush ? forecastBrush : m_weatherSystemBrush.Get();
                    haveLastSegment = true;
                }
                if (IsGeoPointInView(view, point.latitude, point.longitude)) {
                    if (point.errorRadiusNm > 0.0) {
                        const float errorRadius = NauticalMilesToScreenRadius(view, point.latitude, point.longitude, point.errorRadiusNm);
                        if (errorRadius >= 4.0f) {
                            m_rt->DrawEllipse(D2D1::Ellipse(forecast, errorRadius, errorRadius), m_forecastRingBrush ? m_forecastRingBrush.Get() : m_borderBrush.Get(), 3.2f, m_forecastErrorStrokeStyle.Get());
                            m_rt->DrawEllipse(D2D1::Ellipse(forecast, errorRadius, errorRadius), forecastBrush ? forecastBrush : m_weatherSystemBrush.Get(), 1.8f, m_forecastErrorStrokeStyle.Get());
                        }
                    }
                    const float pointRadius = point.leadHours == 0 ? 5.0f : 4.0f;
                    m_rt->FillEllipse(D2D1::Ellipse(forecast, pointRadius, pointRadius), forecastBrush ? forecastBrush : m_weatherSystemBrush.Get());
                    m_rt->DrawEllipse(D2D1::Ellipse(forecast, pointRadius + 1.5f, pointRadius + 1.5f), m_borderBrush.Get(), 0.9f);
                }
                previous = forecast;
                havePrevious = true;
            }
            if (haveLastSegment)
                DrawForecastArrow(lastSegmentStart, lastSegmentEnd, lastSegmentBrush);

            if (!havePrevious && currentInView && system.hasForecastLocation && IsGeoPointInView(view, system.forecastLatitude, system.forecastLongitude)) {
                D2D1_POINT_2F p = GeoToScreen(view, system.latitude, system.longitude);
                D2D1_POINT_2F forecast = GeoToScreen(view, system.forecastLatitude, system.forecastLongitude);
                ID2D1Brush* forecastBrush = WeatherForecastBrush(system.forecastCategory);
                m_rt->DrawLine(p, forecast, forecastBrush ? forecastBrush : m_weatherSystemBrush.Get(), 1.4f);
                m_rt->FillEllipse(D2D1::Ellipse(forecast, 4.5f, 4.5f), forecastBrush ? forecastBrush : m_weatherSystemBrush.Get());
                DrawForecastArrow(p, forecast, forecastBrush ? forecastBrush : m_weatherSystemBrush.Get());
            }

            if (!currentInView)
                continue;

            D2D1_POINT_2F p = currentPoint;
            const float radius = static_cast<float>(ClampValue(8.0 + system.windKnots * 0.08, 9.0, 22.0));
            m_rt->FillEllipse(D2D1::Ellipse(p, radius, radius), currentBrush ? currentBrush : m_weatherSystemBrush.Get());
            m_rt->DrawEllipse(D2D1::Ellipse(p, radius, radius), m_borderBrush.Get(), 1.35f);
            m_rt->DrawEllipse(D2D1::Ellipse(p, radius + 5.0f, radius + 5.0f), currentBrush ? currentBrush : m_weatherSystemBrush.Get(), 1.1f);

            DrawWeatherSystemOverlayLabel(view, system, p, radius, system.id == m_hoveredWeatherSystemId);
        }
    }

    std::wstring WeatherWarningOverlayText(const WeatherWarningEvent& warning) const
    {
        std::wstring text = warning.colour.empty() ? L"Weather warning" : warning.colour + L" warning";
        if (!warning.type.empty()) {
            text += L" - ";
            text += warning.type;
        }
        if (!warning.validFrom.empty() || !warning.validTo.empty()) {
            text += L"\n";
            text += L"From ";
            text += warning.validFrom.empty() ? L"unknown" : warning.validFrom;
            text += L" to ";
            text += warning.validTo.empty() ? L"unknown" : warning.validTo;
        }
        if (!warning.area.empty()) {
            text += L"\n";
            text += warning.area;
        }
        else if (!warning.headline.empty()) {
            text += L"\n";
            text += warning.headline;
        }
        return text;
    }

    ID2D1SolidColorBrush* WeatherWarningBrush(const WeatherWarningEvent& warning) const
    {
        std::wstring colour = ToLower(warning.colour);
        if (colour == L"red")
            return m_severeBrush.Get();
        if (colour == L"amber")
            return m_moderateBrush.Get();
        return m_weatherWarningBrush.Get();
    }

    void DrawWeatherWarningOverlayLabel(const ViewState& view, const WeatherWarningEvent& warning, D2D1_POINT_2F anchor, float radius, bool forceVisible = false)
    {
        if ((!m_showWeatherWarningOverlayLabels && !forceVisible) || !m_noteTextFormat)
            return;

        std::wstring text = WeatherWarningOverlayText(warning);
        if (text.empty())
            return;

        DrawMeasuredBlipLabel(view, anchor, radius, text, WeatherWarningBrush(warning), 160.0f, 380.0f);
    }

    void DrawWarningTriangle(D2D1_POINT_2F p, float radius, ID2D1Brush* fill, ID2D1Brush* stroke)
    {
        if (!m_rt || !g_d2dFactory)
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        sink->BeginFigure(D2D1::Point2F(p.x, p.y - radius), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(p.x + radius * 0.92f, p.y + radius * 0.68f));
        sink->AddLine(D2D1::Point2F(p.x - radius * 0.92f, p.y + radius * 0.68f));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        sink->Close();

        m_rt->FillGeometry(geom.Get(), fill);
        m_rt->DrawGeometry(geom.Get(), stroke ? stroke : m_borderBrush.Get(), 1.4f);
    }

    void DrawWeatherWarnings(const ViewState& view)
    {
        if (!m_rt || m_weatherWarnings.empty())
            return;

        for (const WeatherWarningEvent& warning : m_weatherWarnings) {
            if (!warning.hasLocation || !IsGeoPointInView(view, warning.latitude, warning.longitude))
                continue;

            if (m_showWeatherWarningPolygons && warning.polygon.size() >= 3) {
                DrawPolygonPath(
                    view,
                    warning.polygon,
                    true,
                    m_weatherWarningFillBrush.Get(),
                    WeatherWarningBrush(warning),
                    warning.id == m_hoveredWeatherWarningId ? 2.4f : 1.3f);
            }

            D2D1_POINT_2F p = GeoToScreen(view, warning.latitude, warning.longitude);
            const float radius = 13.0f;
            DrawWarningTriangle(p, radius, WeatherWarningBrush(warning), m_borderBrush.Get());
            m_rt->DrawLine(D2D1::Point2F(p.x, p.y - 5.0f), D2D1::Point2F(p.x, p.y + 3.0f), m_panelBrush.Get(), 2.4f);
            m_rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(p.x, p.y + 7.0f), 1.7f, 1.7f), m_panelBrush.Get());
            DrawWeatherWarningOverlayLabel(view, warning, p, radius, warning.id == m_hoveredWeatherWarningId);
        }
    }

    std::wstring FloodOverlayText(const FloodEvent& flood) const
    {
        std::wstring text = flood.severity.empty() ? L"Flood alert" : flood.severity;
        if (!flood.area.empty()) {
            text += L"\n";
            text += flood.area;
        }
        if (!flood.riverOrSea.empty()) {
            text += L"\n";
            text += flood.riverOrSea;
        }
        return text;
    }

    void DrawFloodOverlayLabel(const ViewState& view, const FloodEvent& flood, D2D1_POINT_2F anchor, float radius, bool forceVisible = false)
    {
        if ((!m_showFloodOverlayLabels && !forceVisible) || !m_noteTextFormat)
            return;

        std::wstring text = FloodOverlayText(flood);
        if (text.empty())
            return;

        DrawMeasuredBlipLabel(view, anchor, radius, text, m_floodBrush.Get(), 150.0f, 380.0f);
    }

    void DrawFloods(const ViewState& view)
    {
        if (!m_rt || m_floods.empty())
            return;

        for (const FloodEvent& flood : m_floods) {
            if (!flood.hasLocation || !IsGeoPointInView(view, flood.latitude, flood.longitude))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, flood.latitude, flood.longitude);
            const float radius = flood.severityLevel == 1 ? 12.5f : (flood.severityLevel == 2 ? 11.0f : 9.5f);
            D2D1_ROUNDED_RECT diamond = D2D1::RoundedRect(
                D2D1::RectF(p.x - radius, p.y - radius, p.x + radius, p.y + radius),
                4.0f,
                4.0f);
            m_rt->FillRoundedRectangle(diamond, m_floodBrush.Get());
            m_rt->DrawRoundedRectangle(diamond, m_borderBrush.Get(), 1.35f);
            m_rt->DrawLine(D2D1::Point2F(p.x - radius * 0.50f, p.y + 1.0f), D2D1::Point2F(p.x - radius * 0.15f, p.y - 2.0f), m_textBrush.Get(), 1.6f);
            m_rt->DrawLine(D2D1::Point2F(p.x - radius * 0.15f, p.y - 2.0f), D2D1::Point2F(p.x + radius * 0.15f, p.y + 2.0f), m_textBrush.Get(), 1.6f);
            m_rt->DrawLine(D2D1::Point2F(p.x + radius * 0.15f, p.y + 2.0f), D2D1::Point2F(p.x + radius * 0.50f, p.y - 1.0f), m_textBrush.Get(), 1.6f);
            DrawFloodOverlayLabel(view, flood, p, radius, flood.id == m_hoveredFloodId);
        }
    }

    ComPtr<ID2D1Bitmap> LoadCachedLaneBitmap(const std::wstring& url)
    {
        auto cached = m_laneBitmaps.find(url);
        if (cached != m_laneBitmaps.end())
            return cached->second;

        ComPtr<ID2D1Bitmap> bitmap;
        std::filesystem::path path = GetLaneImageCachePath(url);
        if (std::filesystem::exists(path)) {
            std::ifstream in(path, std::ios::binary);
            if (in) {
                std::vector<BYTE> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                bitmap = CreateBitmapFromBytes(bytes);
            }
        }

        if (bitmap)
            m_laneBitmaps[url] = bitmap;
        return bitmap;
    }

    void DrawFallbackLaneIcon(float left, float top, float size, bool closed)
    {
        D2D1_ROUNDED_RECT tile = D2D1::RoundedRect(D2D1::RectF(left, top, left + size, top + size), 4.0f, 4.0f);
        m_rt->FillRoundedRectangle(tile, m_laneTileBrush.Get());
        m_rt->DrawRoundedRectangle(tile, m_borderBrush.Get(), 1.0f);

        const float cx = left + size * 0.5f;
        const float cy = top + size * 0.5f;
        if (closed) {
            m_rt->DrawLine(D2D1::Point2F(cx - 7.0f, cy - 7.0f), D2D1::Point2F(cx + 7.0f, cy + 7.0f), m_severeBrush.Get(), 3.0f);
            m_rt->DrawLine(D2D1::Point2F(cx + 7.0f, cy - 7.0f), D2D1::Point2F(cx - 7.0f, cy + 7.0f), m_severeBrush.Get(), 3.0f);
        }
        else {
            m_rt->DrawLine(D2D1::Point2F(cx, cy - 8.0f), D2D1::Point2F(cx, cy + 7.0f), m_textBrush.Get(), 3.0f);
            m_rt->DrawLine(D2D1::Point2F(cx, cy + 7.0f), D2D1::Point2F(cx - 6.0f, cy + 1.0f), m_textBrush.Get(), 3.0f);
            m_rt->DrawLine(D2D1::Point2F(cx, cy + 7.0f), D2D1::Point2F(cx + 6.0f, cy + 1.0f), m_textBrush.Get(), 3.0f);
        }
    }

    struct NotificationLayout
    {
        D2D1_RECT_F bannerRect{};
        D2D1_RECT_F historyRect{};
        D2D1_RECT_F historyToggleRect{};
        bool hasHistory = false;
        float historyProgress = 0.0f;
    };

    static bool PointInRect(int x, int y, const D2D1_RECT_F& rect)
    {
        return static_cast<float>(x) >= rect.left && static_cast<float>(x) <= rect.right &&
            static_cast<float>(y) >= rect.top && static_cast<float>(y) <= rect.bottom;
    }

    static bool RectsOverlap(const D2D1_RECT_F& a, const D2D1_RECT_F& b)
    {
        return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
    }

    bool IsOverlayHot(const D2D1_RECT_F& rect) const
    {
        return PointInRect(m_hoverPoint.x, m_hoverPoint.y, rect);
    }

    bool IsOverlayPressed(const D2D1_RECT_F& rect) const
    {
        return m_leftButtonDown && IsOverlayHot(rect);
    }

    OverlayButton MakeOverlayButton(const std::wstring& text, const D2D1_RECT_F& rect, bool enabled = true) const
    {
        OverlayButton button;
        button.text = text;
        button.bounds = rect;
        button.enabled = enabled;
        button.hot = enabled && IsOverlayHot(rect);
        button.pressed = enabled && IsOverlayPressed(rect);
        return button;
    }

    bool HitAnyOverlayInterface(int x, int y) const
    {
        return HitOverlayContextMenu(x, y) ||
            HitPrivateChatInterface(x, y) ||
            HitUsersPanelInterface(x, y) ||
            HitResponderChatInterface(x, y) ||
            HitNotificationInterface(x, y) ||
            HitNoteInterface(x, y);
    }

    bool HitOverlayContextMenu(int x, int y) const
    {
        return (m_notificationContextMenuVisible && PointInRect(x, y, m_notificationContextMenuRect)) ||
            (m_chatContextMenuVisible && PointInRect(x, y, m_chatContextMenuRect)) ||
            (m_userContextMenuVisible && PointInRect(x, y, m_userContextMenuRect));
    }

    void HideOverlayContextMenus()
    {
        if (!m_notificationContextMenuVisible && !m_chatContextMenuVisible && !m_userContextMenuVisible)
            return;

        m_notificationContextMenuVisible = false;
        m_notificationContextMenuIndex = static_cast<size_t>(-1);
        m_chatContextMenuVisible = false;
        m_chatContextMenuIndex = static_cast<size_t>(-1);
        m_userContextMenuVisible = false;
        m_userContextMenuIndex = static_cast<size_t>(-1);
        Invalidate();
    }

    struct ResponderChatLayout
    {
        D2D1_RECT_F panelRect{};
        D2D1_RECT_F toggleRect{};
        D2D1_RECT_F contentRect{};
        D2D1_RECT_F inputRect{};
        D2D1_RECT_F sendRect{};
        D2D1_RECT_F clearRect{};
        float progress = 1.0f;
    };

    struct UsersPanelLayout
    {
        D2D1_RECT_F panelRect{};
        D2D1_RECT_F toggleRect{};
        D2D1_RECT_F closeRect{};
        D2D1_RECT_F dragRect{};
        D2D1_RECT_F contentRect{};
        float progress = 0.0f;
        bool hasPanel = false;
    };

    struct PrivateChatLayout
    {
        D2D1_RECT_F panelRect{};
        D2D1_RECT_F closeRect{};
        D2D1_RECT_F dragRect{};
        D2D1_RECT_F contentRect{};
        D2D1_RECT_F inputRect{};
        D2D1_RECT_F sendRect{};
        bool hasPanel = false;
    };

    ResponderChatLayout BuildResponderChatLayout(const ViewState& view) const
    {
        ResponderChatLayout layout;
        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = ClampValue(width * 0.42f, 420.0f, 680.0f);
        const float clippedPanelW = MinValue(panelW, MaxValue(220.0f, width - kOverlayUiMargin * 2.0f));
        const float panelH = ClampValue(height * 0.27f, 168.0f, 238.0f);
        const float tabH = 38.0f;
        layout.progress = ClampValue(m_responderChatOpenProgress, 0.0f, 1.0f);
        const float offset = (1.0f - layout.progress) * MaxValue(0.0f, panelH - tabH);
        const float left = kOverlayUiMargin;
        const float bottom = height - kOverlayUiMargin + offset;
        layout.panelRect = D2D1::RectF(left, bottom - panelH, left + clippedPanelW, bottom);
        layout.toggleRect = D2D1::RectF(
            layout.panelRect.left + 10.0f,
            layout.panelRect.top + 8.0f,
            layout.panelRect.left + 38.0f,
            layout.panelRect.top + 35.0f);
        layout.sendRect = D2D1::RectF(
            layout.panelRect.right - kOverlayUiPadding - 78.0f,
            layout.panelRect.bottom - kOverlayUiPadding - 32.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.panelRect.bottom - kOverlayUiPadding);
        layout.clearRect = D2D1::RectF(
            layout.panelRect.right - kOverlayUiPadding - 72.0f,
            layout.panelRect.top + kOverlayUiPadding - 2.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding + 25.0f);
        layout.inputRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.sendRect.top,
            layout.sendRect.left - 8.0f,
            layout.sendRect.bottom);
        layout.contentRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + 52.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.inputRect.top - 10.0f);
        return layout;
    }

    UsersPanelLayout BuildUsersPanelLayout(const ViewState& view) const
    {
        UsersPanelLayout layout;
        if (!m_showUsersPanel)
            return layout;

        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = ClampValue(width * 0.20f, 230.0f, 300.0f);
        const float panelH = ClampValue(height * 0.36f, 220.0f, 360.0f);
        const float clippedPanelW = MinValue(panelW, MaxValue(190.0f, width - kOverlayUiMargin * 2.0f));
        const float clippedPanelH = MinValue(panelH, MaxValue(170.0f, height - kOverlayUiMargin * 2.0f - 120.0f));
        const float tabW = 32.0f;

        layout.progress = ClampValue(m_usersPanelOpenProgress, 0.0f, 1.0f);
        const float offset = (1.0f - layout.progress) * MaxValue(0.0f, clippedPanelW - tabW);
        const float openLeft = ClampValue(
            kOverlayUiMargin + m_usersPanelOffsetX,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, width - kOverlayUiMargin - clippedPanelW));
        const float openTop = ClampValue(
            kOverlayUiMargin + 64.0f + m_usersPanelOffsetY,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, height - kOverlayUiMargin - clippedPanelH));
        const float left = openLeft - offset;
        const float top = openTop;
        layout.panelRect = D2D1::RectF(left, top, left + clippedPanelW, top + clippedPanelH);
        layout.closeRect = D2D1::RectF(
            layout.panelRect.right - kOverlayTogglePadding - kOverlayToggleSize,
            layout.panelRect.top + 10.0f,
            layout.panelRect.right - kOverlayTogglePadding,
            layout.panelRect.top + 10.0f + kOverlayToggleSize);
        layout.toggleRect = D2D1::RectF(
            layout.closeRect.left - kOverlayTogglePadding - kOverlayToggleSize,
            layout.panelRect.top + 10.0f,
            layout.closeRect.left - kOverlayTogglePadding,
            layout.panelRect.top + 10.0f + kOverlayToggleSize);
        layout.dragRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top,
            layout.toggleRect.left - kOverlayTogglePadding,
            layout.panelRect.top + 54.0f);
        layout.contentRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + 58.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.panelRect.bottom - kOverlayUiPadding);
        layout.hasPanel = true;
        return layout;
    }

    PrivateChatLayout BuildPrivateChatLayout(const ViewState& view) const
    {
        PrivateChatLayout layout;
        if (!m_privateChatVisible)
            return layout;

        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = MinValue(390.0f, MaxValue(260.0f, width - kOverlayUiMargin * 2.0f));
        const float panelH = MinValue(290.0f, MaxValue(210.0f, height - kOverlayUiMargin * 2.0f));
        const float baseLeft = MaxValue(kOverlayUiMargin, (width - panelW) * 0.50f);
        const float baseTop = MaxValue(kOverlayUiMargin, (height - panelH) * 0.22f);
        const float left = ClampValue(baseLeft + m_privateChatOffsetX, kOverlayUiMargin, MaxValue(kOverlayUiMargin, width - kOverlayUiMargin - panelW));
        const float top = ClampValue(baseTop + m_privateChatOffsetY, kOverlayUiMargin, MaxValue(kOverlayUiMargin, height - kOverlayUiMargin - panelH));

        layout.panelRect = D2D1::RectF(left, top, left + panelW, top + panelH);
        layout.closeRect = D2D1::RectF(layout.panelRect.right - 34.0f, layout.panelRect.top + 10.0f, layout.panelRect.right - 10.0f, layout.panelRect.top + 34.0f);
        layout.dragRect = D2D1::RectF(layout.panelRect.left + kOverlayUiPadding, layout.panelRect.top, layout.closeRect.left - 8.0f, layout.panelRect.top + 42.0f);
        layout.sendRect = D2D1::RectF(layout.panelRect.right - kOverlayUiPadding - 78.0f, layout.panelRect.bottom - kOverlayUiPadding - 32.0f, layout.panelRect.right - kOverlayUiPadding, layout.panelRect.bottom - kOverlayUiPadding);
        layout.inputRect = D2D1::RectF(layout.panelRect.left + kOverlayUiPadding, layout.sendRect.top, layout.sendRect.left - 8.0f, layout.sendRect.bottom);
        layout.contentRect = D2D1::RectF(layout.panelRect.left + kOverlayUiPadding, layout.panelRect.top + 58.0f, layout.panelRect.right - kOverlayUiPadding, layout.inputRect.top - 10.0f);
        layout.hasPanel = true;
        return layout;
    }

    void ClampUsersPanelOffsets(const ViewState& view)
    {
        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = ClampValue(width * 0.20f, 230.0f, 300.0f);
        const float panelH = ClampValue(height * 0.36f, 220.0f, 360.0f);
        const float clippedPanelW = MinValue(panelW, MaxValue(190.0f, width - kOverlayUiMargin * 2.0f));
        const float clippedPanelH = MinValue(panelH, MaxValue(170.0f, height - kOverlayUiMargin * 2.0f - 120.0f));
        const float baseTop = kOverlayUiMargin + 64.0f;
        const float openLeft = ClampValue(
            kOverlayUiMargin + m_usersPanelOffsetX,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, width - kOverlayUiMargin - clippedPanelW));
        const float openTop = ClampValue(
            baseTop + m_usersPanelOffsetY,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, height - kOverlayUiMargin - clippedPanelH));
        m_usersPanelOffsetX = openLeft - kOverlayUiMargin;
        m_usersPanelOffsetY = openTop - baseTop;
    }

    void ClampPrivateChatPanelOffsets(const ViewState& view)
    {
        if (!m_privateChatVisible)
            return;

        const PrivateChatLayout layout = BuildPrivateChatLayout(view);
        if (!layout.hasPanel)
            return;

        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = layout.panelRect.right - layout.panelRect.left;
        const float panelH = layout.panelRect.bottom - layout.panelRect.top;
        const float baseLeft = MaxValue(kOverlayUiMargin, (width - panelW) * 0.50f);
        const float baseTop = MaxValue(kOverlayUiMargin, (height - panelH) * 0.22f);
        const float left = ClampValue(layout.panelRect.left, kOverlayUiMargin, MaxValue(kOverlayUiMargin, width - kOverlayUiMargin - panelW));
        const float top = ClampValue(layout.panelRect.top, kOverlayUiMargin, MaxValue(kOverlayUiMargin, height - kOverlayUiMargin - panelH));
        m_privateChatOffsetX = left - baseLeft;
        m_privateChatOffsetY = top - baseTop;
    }

    void ClampToolbarPanelOffsets(const ViewState& view)
    {
        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float panelW = 274.0f;
        const float panelH = 178.0f;
        const float baseLeft = 18.0f;
        const float baseTop = 18.0f;
        const float left = ClampValue(
            baseLeft + m_toolbarPanelOffsetX,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, width - kOverlayUiMargin - panelW));
        const float top = ClampValue(
            baseTop + m_toolbarPanelOffsetY,
            kOverlayUiMargin,
            MaxValue(kOverlayUiMargin, height - kOverlayUiMargin - panelH));
        m_toolbarPanelOffsetX = left - baseLeft;
        m_toolbarPanelOffsetY = top - baseTop;
    }

    static std::wstring ChatPositionKey(std::wstring position)
    {
        position = ToLower(Trim(position));
        if (position == L"admin")
            return L"administrator";
        if (position == L"supervisor" || position == L"sup")
            return L"supervisor";
        if (position == L"manager" || position == L"mgr")
            return L"manager";
        if (position == L"erc")
            return L"erc";
        return position;
    }

    std::wstring ChatRolePrefixText(const ChatMessage& msg) const
    {
        const std::wstring role = ChatPositionKey(msg.position);
        if (role == L"administrator")
            return L"[ADM]";
        if (role == L"supervisor")
            return L"[SUP]";
        if (role == L"manager")
            return L"[MGR]";
        if (role == L"erc")
            return L"[ERC]";
        return L"";
    }

    ID2D1Brush* ChatRoleBrush(const ChatMessage& msg) const
    {
        const std::wstring role = ChatPositionKey(msg.position);
        if (role == L"administrator")
            return m_severeBrush.Get();
        if (role == L"supervisor")
            return m_moderateBrush.Get();
        if (role == L"manager")
            return m_weatherSystemBrush.Get();
        return m_overlayUi.TextBrush();
    }

    ID2D1Brush* UserRoleBrush(const OnlineUser& user) const
    {
        ChatMessage msg;
        msg.position = user.position;
        return ChatRoleBrush(msg);
    }

    std::wstring UserRolePrefixText(const OnlineUser& user) const
    {
        ChatMessage msg;
        msg.position = user.position;
        return ChatRolePrefixText(msg);
    }

    std::wstring UserDisplayText(const OnlineUser& user) const
    {
        std::wstring name = user.displayName.empty() ? user.username : user.displayName;
        if (name.empty())
            name = L"Unknown";

        std::wstring text = name;
        if (!user.pod.empty())
            text += L" - " + user.pod;
        return text;
    }

    bool HitUsersPanelInterface(int x, int y) const
    {
        const UsersPanelLayout layout = BuildUsersPanelLayout(BuildViewState());
        if (!layout.hasPanel)
            return false;
        if (PointInRect(x, y, layout.toggleRect))
            return true;
        return layout.progress > 0.04f && PointInRect(x, y, layout.panelRect);
    }

    bool HandleUsersPanelPointerDown(int x, int y)
    {
        const UsersPanelLayout layout = BuildUsersPanelLayout(BuildViewState());
        if (!layout.hasPanel)
            return false;

        if (PointInRect(x, y, layout.toggleRect)) {
            ClearOverlayInputFocus();
            m_usersPanelCollapsed = !m_usersPanelCollapsed;
            StartUsersPanelAnimation(m_usersPanelCollapsed ? 0.0f : 1.0f);
            Invalidate();
            return true;
        }

        if (layout.progress > 0.04f && PointInRect(x, y, layout.closeRect)) {
            ClearOverlayInputFocus();
            if (m_onPanelClose)
                m_onPanelClose(L"users");
            Invalidate();
            return true;
        }

        if (layout.progress > 0.04f && PointInRect(x, y, layout.dragRect)) {
            ClearOverlayInputFocus();
            SetCapture(m_hwnd);
            m_draggingUsersPanel = true;
            m_notificationUiMouseDown = true;
            m_lastMouse = POINT{ x, y };
            return true;
        }

        if (layout.progress > 0.04f && PointInRect(x, y, layout.panelRect)) {
            ClearOverlayInputFocus();
            return true;
        }

        return false;
    }

    bool OnlineUserIndexAtPoint(int x, int y, size_t& indexOut) const
    {
        const UsersPanelLayout layout = BuildUsersPanelLayout(BuildViewState());
        if (!layout.hasPanel || layout.progress <= 0.04f || !PointInRect(x, y, layout.contentRect))
            return false;

        const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
        float rowY = layout.contentRect.top + 4.0f;
        for (size_t i = 0; i < m_onlineUsers.size(); ++i) {
            const OnlineUser& user = m_onlineUsers[i];
            const std::wstring prefix = UserRolePrefixText(user);
            const std::wstring display = UserDisplayText(user);
            const float prefixW = prefix.empty()
                ? 0.0f
                : m_overlayUi.MeasureTextWidth(prefix + L" ", m_overlayUi.BodyFormat(), contentW);
            const float bodyW = MaxValue(1.0f, contentW - prefixW);
            const float bodyH = MaxValue(18.0f, m_overlayUi.MeasureTextHeight(display, m_overlayUi.BodyFormat(), bodyW));
            const std::wstring sub = user.position.empty()
                ? user.lastSeen
                : user.position + (user.lastSeen.empty() ? L"" : L" - " + user.lastSeen);
            const float subH = sub.empty()
                ? 0.0f
                : MaxValue(14.0f, m_overlayUi.MeasureTextHeight(sub, m_overlayUi.SmallFormat(), contentW));
            const float rowH = bodyH + (subH > 0.0f ? subH + 3.0f : 0.0f) + 10.0f;
            if (static_cast<float>(y) >= rowY && static_cast<float>(y) <= rowY + rowH) {
                indexOut = i;
                return true;
            }
            rowY += rowH;
        }
        return false;
    }

    bool HandleUserContextRightClick(int x, int y)
    {
        if (!m_onUserAction)
            return false;
        EnsureDeviceResources();
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return false;

        size_t index = static_cast<size_t>(-1);
        if (!OnlineUserIndexAtPoint(x, y, index))
            return false;

        HideOverlayContextMenus();
        m_userContextMenuVisible = true;
        m_userContextMenuIndex = index;
        m_userContextMenuRect = BuildOverlayContextMenuRect(x, y, 172.0f, 116.0f);
        ClearOverlayInputFocus();
        Invalidate();
        return true;
    }

    void DrawUsersPanel(const ViewState& view)
    {
        if (!m_showUsersPanel || !m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        const UsersPanelLayout layout = BuildUsersPanelLayout(view);
        if (!layout.hasPanel)
            return;

        const OverlayButton toggleButton = MakeOverlayButton(m_usersPanelCollapsed ? L">" : L"<", layout.toggleRect);
        if (layout.progress <= 0.04f) {
            m_overlayUi.DrawButton(toggleButton);
            return;
        }

        if (layout.panelRect.right - layout.panelRect.left < 160.0f ||
            layout.panelRect.bottom - layout.panelRect.top < 150.0f)
        {
            return;
        }

        m_overlayUi.DrawGlassPanel(layout.panelRect, 12.0f);
        m_overlayUi.DrawButton(toggleButton);
        m_overlayUi.DrawButton(MakeOverlayButton(L"X", layout.closeRect));

        D2D1_RECT_F titleRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding - 1.0f,
            layout.toggleRect.left - kOverlayTogglePadding,
            layout.panelRect.top + kOverlayUiPadding + 22.0f);
        m_overlayUi.DrawLabel(L"Users", m_overlayUi.TitleFormat(), titleRect);

        std::wstring countText = m_onlineUsers.empty()
            ? L"No online users"
            : std::to_wstring(m_onlineUsers.size()) + L" online";
        D2D1_RECT_F countRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding + 23.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding + 43.0f);
        m_overlayUi.DrawLabel(countText, m_overlayUi.SmallFormat(), countRect, m_overlayUi.MutedTextBrush());
        m_overlayUi.DrawSeparator(layout.panelRect.left + kOverlayUiPadding, layout.panelRect.right - kOverlayUiPadding, layout.panelRect.top + 52.0f);

        m_rt->PushAxisAlignedClip(layout.contentRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        if (m_onlineUsers.empty()) {
            D2D1_RECT_F emptyRect = D2D1::RectF(layout.contentRect.left, layout.contentRect.top + 6.0f, layout.contentRect.right, layout.contentRect.top + 42.0f);
            m_overlayUi.DrawLabel(L"No online users.", m_overlayUi.BodyFormat(), emptyRect, m_overlayUi.MutedTextBrush());
        }
        else {
            const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
            float y = layout.contentRect.top + 4.0f;
            for (const OnlineUser& user : m_onlineUsers) {
                if (y > layout.contentRect.bottom - 10.0f)
                    break;

                const std::wstring prefix = UserRolePrefixText(user);
                const std::wstring display = UserDisplayText(user);
                const float prefixW = prefix.empty()
                    ? 0.0f
                    : m_overlayUi.MeasureTextWidth(prefix + L" ", m_overlayUi.BodyFormat(), contentW);
                const float bodyW = MaxValue(1.0f, contentW - prefixW);
                const float bodyH = MaxValue(18.0f, m_overlayUi.MeasureTextHeight(display, m_overlayUi.BodyFormat(), bodyW));
                const std::wstring sub = user.position.empty()
                    ? user.lastSeen
                    : user.position + (user.lastSeen.empty() ? L"" : L" - " + user.lastSeen);
                const float subH = sub.empty()
                    ? 0.0f
                    : MaxValue(14.0f, m_overlayUi.MeasureTextHeight(sub, m_overlayUi.SmallFormat(), contentW));
                const float rowH = bodyH + (subH > 0.0f ? subH + 3.0f : 0.0f) + 10.0f;

                D2D1_RECT_F prefixRect = D2D1::RectF(layout.contentRect.left, y, layout.contentRect.left + prefixW, y + bodyH + 2.0f);
                D2D1_RECT_F bodyRect = D2D1::RectF(layout.contentRect.left + prefixW, y, layout.contentRect.right, y + bodyH + 2.0f);
                if (!prefix.empty())
                    m_overlayUi.DrawLabel(prefix, m_overlayUi.BodyFormat(), prefixRect, UserRoleBrush(user));
                m_overlayUi.DrawLabel(display, m_overlayUi.BodyFormat(), bodyRect);
                if (!sub.empty()) {
                    D2D1_RECT_F subRect = D2D1::RectF(layout.contentRect.left, y + bodyH + 3.0f, layout.contentRect.right, y + bodyH + subH + 5.0f);
                    m_overlayUi.DrawLabel(sub, m_overlayUi.SmallFormat(), subRect, m_overlayUi.MutedTextBrush());
                }

                m_overlayUi.DrawSeparator(layout.contentRect.left, layout.contentRect.right, y + rowH - 4.0f);
                y += rowH;
            }
        }
        m_rt->PopAxisAlignedClip();
    }

    std::wstring PrivateChatTitle() const
    {
        std::wstring name = m_privateChatUser.displayName.empty() ? m_privateChatUser.username : m_privateChatUser.displayName;
        if (name.empty())
            name = L"User";
        return L"Private Chat - " + name;
    }

    bool PrivateMessageMatchesOpenChat(const PrivateMessage& message) const
    {
        const std::wstring peer = ToLower(Trim(m_privateChatUser.username));
        if (peer.empty())
            return false;
        return ToLower(Trim(message.senderUsername)) == peer ||
            ToLower(Trim(message.recipientUsername)) == peer;
    }

    std::wstring FormatPrivateMessageLine(const PrivateMessage& message) const
    {
        std::wstring line;
        if (!message.timestamp.empty())
            line += L"[" + message.timestamp + L"] ";
        std::wstring sender = message.senderDisplayName.empty() ? message.senderUsername : message.senderDisplayName;
        if (sender.empty())
            sender = L"User";
        line += sender + L": " + message.text;
        return line;
    }

    float PrivateMessageHeight(const PrivateMessage& message, float contentW) const
    {
        return MaxValue(18.0f, m_overlayUi.MeasureTextHeight(FormatPrivateMessageLine(message), m_overlayUi.BodyFormat(), contentW));
    }

    bool HitPrivateChatInterface(int x, int y) const
    {
        const PrivateChatLayout layout = BuildPrivateChatLayout(BuildViewState());
        return layout.hasPanel && PointInRect(x, y, layout.panelRect);
    }

    void SubmitPrivateChatDraft()
    {
        std::wstring text = Trim(m_privateChatDraft);
        if (text.empty() || m_privateChatUser.username.empty())
            return;

        m_privateChatDraft.clear();
        if (m_onPrivateChatSend)
            m_onPrivateChatSend(m_privateChatUser.username, text);
        Invalidate();
    }

    bool HandlePrivateChatPointerDown(int x, int y)
    {
        const PrivateChatLayout layout = BuildPrivateChatLayout(BuildViewState());
        if (!layout.hasPanel || !PointInRect(x, y, layout.panelRect))
            return false;

        if (PointInRect(x, y, layout.closeRect)) {
            m_privateChatVisible = false;
            m_privateChatDraft.clear();
            ClearOverlayInputFocus();
            Invalidate();
            return true;
        }

        if (PointInRect(x, y, layout.dragRect)) {
            ClearOverlayInputFocus();
            SetCapture(m_hwnd);
            m_draggingPrivateChatPanel = true;
            m_notificationUiMouseDown = true;
            m_lastMouse = POINT{ x, y };
            return true;
        }

        if (PointInRect(x, y, layout.sendRect)) {
            SubmitPrivateChatDraft();
            SetOverlayInputFocus(OverlayInputFocus::PrivateChat);
            return true;
        }

        SetOverlayInputFocus(PointInRect(x, y, layout.inputRect) ? OverlayInputFocus::PrivateChat : OverlayInputFocus::None);
        Invalidate();
        return true;
    }

    bool HandlePrivateChatKeyDown(WPARAM key)
    {
        if (!m_privateChatInputFocused)
            return false;

        if (key == VK_RETURN) {
            SubmitPrivateChatDraft();
            return true;
        }
        if (key == VK_BACK) {
            if (!m_privateChatDraft.empty())
                m_privateChatDraft.pop_back();
            Invalidate();
            return true;
        }
        if (key == VK_ESCAPE) {
            ClearOverlayInputFocus();
            Invalidate();
            return true;
        }
        return false;
    }

    bool HandlePrivateChatChar(WPARAM ch)
    {
        if (!m_privateChatInputFocused)
            return false;

        if (ch == L'\r' || ch == L'\n' || ch == 8 || ch == 27)
            return true;

        if (ch >= 32 && ch != 127 && m_privateChatDraft.size() < 512) {
            m_privateChatDraft.push_back(static_cast<wchar_t>(ch));
            Invalidate();
        }
        return true;
    }

    void DrawPrivateChat(const ViewState& view)
    {
        if (!m_privateChatVisible || !m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        const PrivateChatLayout layout = BuildPrivateChatLayout(view);
        if (!layout.hasPanel)
            return;

        m_overlayUi.DrawGlassPanel(layout.panelRect, 12.0f);
        m_overlayUi.DrawButton(MakeOverlayButton(L"X", layout.closeRect));

        D2D1_RECT_F titleRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding - 1.0f,
            layout.closeRect.left - 8.0f,
            layout.panelRect.top + kOverlayUiPadding + 23.0f);
        m_overlayUi.DrawLabel(PrivateChatTitle(), m_overlayUi.TitleFormat(), titleRect);
        m_overlayUi.DrawSeparator(layout.panelRect.left + kOverlayUiPadding, layout.panelRect.right - kOverlayUiPadding, layout.panelRect.top + 50.0f);

        const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
        m_rt->PushAxisAlignedClip(layout.contentRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        float y = layout.contentRect.bottom;
        bool any = false;
        for (auto it = m_privateMessages.rbegin(); it != m_privateMessages.rend(); ++it) {
            if (!PrivateMessageMatchesOpenChat(*it))
                continue;
            any = true;
            const float lineH = PrivateMessageHeight(*it, contentW);
            y -= lineH + 7.0f;
            if (y < layout.contentRect.top)
                break;

            ChatMessage roleProbe;
            roleProbe.position = it->senderPosition;
            D2D1_RECT_F lineRect = D2D1::RectF(layout.contentRect.left, y, layout.contentRect.right, y + lineH + 2.0f);
            m_overlayUi.DrawLabel(FormatPrivateMessageLine(*it), m_overlayUi.BodyFormat(), lineRect, ChatRoleBrush(roleProbe));
        }
        if (!any) {
            D2D1_RECT_F emptyRect = D2D1::RectF(layout.contentRect.left, layout.contentRect.top + 6.0f, layout.contentRect.right, layout.contentRect.top + 40.0f);
            m_overlayUi.DrawLabel(L"No private messages yet.", m_overlayUi.BodyFormat(), emptyRect, m_overlayUi.MutedTextBrush());
        }
        m_rt->PopAxisAlignedClip();

        OverlayTextBox input;
        input.text = m_privateChatInputFocused && (GetTickCount64() / 550) % 2 == 0
            ? m_privateChatDraft + L"|"
            : m_privateChatDraft;
        input.placeholder = L"Private message...";
        input.bounds = layout.inputRect;
        input.focused = m_privateChatInputFocused;
        m_overlayUi.DrawTextBox(input);
        m_overlayUi.DrawButton(MakeOverlayButton(L"Send", layout.sendRect, !Trim(m_privateChatDraft).empty()));
    }

    std::wstring ChatTimestampText(const ChatMessage& msg) const
    {
        if (!msg.timestamp.empty())
            return L"[" + msg.timestamp + L"] ";
        return L"";
    }

    std::wstring FormatChatLineBody(const ChatMessage& msg) const
    {
        std::wstring line;
        if (!msg.author.empty())
            line += msg.author + L": ";
        line += msg.text;
        return line;
    }

    std::wstring ExtractExternalLink(const std::wstring& text) const
    {
        static const std::wregex linkRegex(LR"(\bhttps?://[^\s<>"']+)", std::regex_constants::icase);
        std::wsmatch match;
        if (!std::regex_search(text, match, linkRegex))
            return L"";

        std::wstring url = match.str(0);
        while (!url.empty() && std::wstring(L".,;:!?)]}\"'").find(url.back()) != std::wstring::npos)
            url.pop_back();
        return url;
    }

    float ChatMessageHeight(const ChatMessage& msg, float contentW) const
    {
        const std::wstring timestamp = ChatTimestampText(msg);
        const std::wstring prefix = ChatRolePrefixText(msg);
        const float timestampW = timestamp.empty()
            ? 0.0f
            : m_overlayUi.MeasureTextWidth(timestamp, m_overlayUi.BodyFormat(), contentW);
        const float prefixW = prefix.empty()
            ? 0.0f
            : m_overlayUi.MeasureTextWidth(prefix + L" ", m_overlayUi.BodyFormat(), contentW);
        const float bodyW = MaxValue(1.0f, contentW - timestampW - prefixW);
        const float bodyH = m_overlayUi.MeasureTextHeight(FormatChatLineBody(msg), m_overlayUi.BodyFormat(), bodyW);
        return MaxValue(18.0f, bodyH);
    }

    void DrawChatMessageLine(const ChatMessage& msg, const D2D1_RECT_F& rect, float contentW)
    {
        const std::wstring timestamp = ChatTimestampText(msg);
        const std::wstring prefix = ChatRolePrefixText(msg);
        const std::wstring body = FormatChatLineBody(msg);
        const bool hasLink = !ExtractExternalLink(msg.text).empty();
        const float timestampW = timestamp.empty()
            ? 0.0f
            : m_overlayUi.MeasureTextWidth(timestamp, m_overlayUi.BodyFormat(), contentW);
        const float prefixW = prefix.empty()
            ? 0.0f
            : m_overlayUi.MeasureTextWidth(prefix + L" ", m_overlayUi.BodyFormat(), contentW);

        if (!timestamp.empty()) {
            D2D1_RECT_F timestampRect = D2D1::RectF(rect.left, rect.top, MinValue(rect.right, rect.left + timestampW), rect.bottom);
            m_overlayUi.DrawLabel(timestamp, m_overlayUi.BodyFormat(), timestampRect, m_overlayUi.MutedTextBrush());
        }

        if (!prefix.empty()) {
            D2D1_RECT_F prefixRect = D2D1::RectF(rect.left + timestampW, rect.top, MinValue(rect.right, rect.left + timestampW + prefixW), rect.bottom);
            m_overlayUi.DrawLabel(prefix, m_overlayUi.BodyFormat(), prefixRect, ChatRoleBrush(msg));
        }

        D2D1_RECT_F bodyRect = D2D1::RectF(rect.left + timestampW + prefixW, rect.top, rect.right, rect.bottom);
        m_overlayUi.DrawLabel(body, m_overlayUi.BodyFormat(), bodyRect, hasLink ? m_overlayUi.AccentBrush() : m_overlayUi.TextBrush());
    }

    bool TryOpenResponderChatLinkAt(int x, int y, const ResponderChatLayout& layout)
    {
        if (!PointInRect(x, y, layout.contentRect))
            return false;

        const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
        float rowY = layout.contentRect.bottom;
        for (auto it = m_chatMessages.rbegin(); it != m_chatMessages.rend(); ++it) {
            const float lineH = ChatMessageHeight(*it, contentW);
            rowY -= lineH + 6.0f;
            if (rowY < layout.contentRect.top)
                break;
            D2D1_RECT_F lineRect = D2D1::RectF(layout.contentRect.left, rowY, layout.contentRect.right, rowY + lineH + 2.0f);
            if (PointInRect(x, y, lineRect)) {
                const std::wstring url = ExtractExternalLink(it->text);
                if (!url.empty()) {
                    ShellExecuteW(m_hwnd, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    bool ChatMessageIndexAtPoint(int x, int y, size_t& indexOut) const
    {
        const ResponderChatLayout layout = BuildResponderChatLayout(BuildViewState());
        if (layout.progress <= 0.04f || !PointInRect(x, y, layout.contentRect))
            return false;

        const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
        float rowY = layout.contentRect.bottom;
        for (size_t reverseIndex = 0; reverseIndex < m_chatMessages.size(); ++reverseIndex) {
            const size_t index = m_chatMessages.size() - 1 - reverseIndex;
            const ChatMessage& message = m_chatMessages[index];
            const float lineH = ChatMessageHeight(message, contentW);
            rowY -= lineH + 6.0f;
            if (rowY < layout.contentRect.top)
                break;
            const D2D1_RECT_F lineRect = D2D1::RectF(layout.contentRect.left, rowY, layout.contentRect.right, rowY + lineH + 2.0f);
            if (PointInRect(x, y, lineRect)) {
                indexOut = index;
                return true;
            }
        }
        return false;
    }

    bool HandleChatContextRightClick(int x, int y)
    {
        if (!m_onChatMessageAction)
            return false;
        EnsureDeviceResources();
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return false;

        size_t index = static_cast<size_t>(-1);
        if (!ChatMessageIndexAtPoint(x, y, index))
            return false;

        HideOverlayContextMenus();
        m_chatContextMenuVisible = true;
        m_chatContextMenuIndex = index;
        m_chatContextMenuRect = BuildOverlayContextMenuRect(x, y, 132.0f, 42.0f);
        ClearOverlayInputFocus();
        Invalidate();
        return true;
    }

    bool HitResponderChatInterface(int x, int y) const
    {
        const ResponderChatLayout layout = BuildResponderChatLayout(BuildViewState());
        if (PointInRect(x, y, layout.toggleRect))
            return true;
        return layout.progress > 0.04f && PointInRect(x, y, layout.panelRect);
    }

    void SubmitResponderChatDraft()
    {
        std::wstring text = Trim(m_responderChatDraft);
        if (text.empty())
            return;

        m_responderChatDraft.clear();
        if (m_onChatSend)
            m_onChatSend(text);
        Invalidate();
    }

    bool HandleResponderChatPointerDown(int x, int y)
    {
        const ResponderChatLayout layout = BuildResponderChatLayout(BuildViewState());
        if (PointInRect(x, y, layout.toggleRect)) {
            m_responderChatCollapsed = !m_responderChatCollapsed;
            if (m_responderChatCollapsed)
                ClearOverlayInputFocus();
            StartResponderChatAnimation(m_responderChatCollapsed ? 0.0f : 1.0f);
            Invalidate();
            return true;
        }

        if (layout.progress <= 0.04f || !PointInRect(x, y, layout.panelRect))
            return false;

        if (PointInRect(x, y, layout.clearRect)) {
            ClearOverlayInputFocus();
            if (m_canClearResponderChat && !m_chatMessages.empty() && m_onChatClear)
                m_onChatClear();
            Invalidate();
            return true;
        }

        if (PointInRect(x, y, layout.sendRect)) {
            SubmitResponderChatDraft();
            SetOverlayInputFocus(OverlayInputFocus::ResponderChat);
            return true;
        }

        if (TryOpenResponderChatLinkAt(x, y, layout)) {
            ClearOverlayInputFocus();
            return true;
        }

        SetOverlayInputFocus(PointInRect(x, y, layout.inputRect) ? OverlayInputFocus::ResponderChat : OverlayInputFocus::None);
        Invalidate();
        return true;
    }

    bool HandleResponderChatKeyDown(WPARAM key)
    {
        if (!m_responderChatInputFocused)
            return false;

        if (key == VK_RETURN) {
            SubmitResponderChatDraft();
            return true;
        }
        if (key == VK_BACK) {
            if (!m_responderChatDraft.empty())
                m_responderChatDraft.pop_back();
            Invalidate();
            return true;
        }
        if (key == VK_ESCAPE) {
            ClearOverlayInputFocus();
            Invalidate();
            return true;
        }
        return false;
    }

    bool HandleResponderChatChar(WPARAM ch)
    {
        if (!m_responderChatInputFocused)
            return false;

        if (ch == L'\r' || ch == L'\n' || ch == 8 || ch == 27)
            return true;

        if (ch >= 32 && ch != 127 && m_responderChatDraft.size() < 512) {
            m_responderChatDraft.push_back(static_cast<wchar_t>(ch));
            Invalidate();
        }
        return true;
    }

    void DrawResponderChat(const ViewState& view)
    {
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        const ResponderChatLayout layout = BuildResponderChatLayout(view);
        const OverlayButton toggleButton = MakeOverlayButton(m_responderChatCollapsed ? L"^" : L"v", layout.toggleRect);
        if (layout.progress <= 0.04f) {
            m_overlayUi.DrawButton(toggleButton);
            return;
        }

        if (layout.panelRect.right - layout.panelRect.left < 180.0f ||
            layout.panelRect.bottom - layout.panelRect.top < 120.0f)
        {
            return;
        }

        m_overlayUi.DrawGlassPanel(layout.panelRect, 12.0f);
        m_overlayUi.DrawButton(toggleButton);

        D2D1_RECT_F titleRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding + 30.0f,
            layout.panelRect.top + kOverlayUiPadding - 2.0f,
            layout.clearRect.left - 8.0f,
            layout.panelRect.top + kOverlayUiPadding + 22.0f);
        m_overlayUi.DrawLabel(L"Responders Chat", m_overlayUi.TitleFormat(), titleRect);
        if (m_canClearResponderChat)
            m_overlayUi.DrawButton(MakeOverlayButton(L"Clear", layout.clearRect, !m_chatMessages.empty()));

        std::wstring countText = m_chatMessages.empty()
            ? L"No responder messages yet"
            : std::to_wstring(m_chatMessages.size()) + L" responder message(s)";
        D2D1_RECT_F countRect = D2D1::RectF(
            layout.panelRect.left + kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding + 23.0f,
            layout.panelRect.right - kOverlayUiPadding,
            layout.panelRect.top + kOverlayUiPadding + 42.0f);
        m_overlayUi.DrawLabel(countText, m_overlayUi.SmallFormat(), countRect, m_overlayUi.MutedTextBrush());
        m_overlayUi.DrawSeparator(layout.panelRect.left + kOverlayUiPadding, layout.panelRect.right - kOverlayUiPadding, layout.panelRect.top + 50.0f);

        const float contentW = MaxValue(1.0f, layout.contentRect.right - layout.contentRect.left);
        m_rt->PushAxisAlignedClip(layout.contentRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        if (m_chatMessages.empty()) {
            D2D1_RECT_F emptyRect = D2D1::RectF(layout.contentRect.left, layout.contentRect.top + 6.0f, layout.contentRect.right, layout.contentRect.top + 34.0f);
            m_overlayUi.DrawLabel(L"No responder messages yet.", m_overlayUi.BodyFormat(), emptyRect, m_overlayUi.MutedTextBrush());
        }
        else {
            float y = layout.contentRect.bottom;
            for (auto it = m_chatMessages.rbegin(); it != m_chatMessages.rend(); ++it) {
                const float lineH = ChatMessageHeight(*it, contentW);
                y -= lineH + 6.0f;
                if (y < layout.contentRect.top)
                    break;
                D2D1_RECT_F lineRect = D2D1::RectF(layout.contentRect.left, y, layout.contentRect.right, y + lineH + 2.0f);
                DrawChatMessageLine(*it, lineRect, contentW);
            }
        }
        m_rt->PopAxisAlignedClip();

        OverlayTextBox input;
        input.text = m_responderChatInputFocused && (GetTickCount64() / 550) % 2 == 0
            ? m_responderChatDraft + L"|"
            : m_responderChatDraft;
        input.placeholder = L"Message local responders...";
        input.bounds = layout.inputRect;
        input.focused = m_responderChatInputFocused;
        m_overlayUi.DrawTextBox(input);

        OverlayButton sendButton = MakeOverlayButton(L"Send", layout.sendRect, !Trim(m_responderChatDraft).empty());
        m_overlayUi.DrawButton(sendButton);
    }

    NotificationLayout BuildNotificationLayout(const ViewState& view) const
    {
        NotificationLayout layout;
        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float usableW = MaxValue(1.0f, width - kOverlayUiMargin * 2.0f);
        const float usableH = MaxValue(1.0f, height - kOverlayUiMargin * 2.0f);

        float historyW = 0.0f;
        if (m_showNotificationHistory) {
            historyW = ClampValue(width * 0.32f, 280.0f, 420.0f);
            historyW = MinValue(historyW, MaxValue(180.0f, usableW));
            const float tabW = 34.0f;
            layout.historyProgress = ClampValue(m_notificationHistoryOpenProgress, 0.0f, 1.0f);
            const float offset = (1.0f - layout.historyProgress) * MaxValue(0.0f, historyW - tabW);
            layout.historyRect = D2D1::RectF(
                width - kOverlayUiMargin - historyW + offset,
                kOverlayUiMargin,
                width - kOverlayUiMargin + offset,
                kOverlayUiMargin + MaxValue(120.0f, usableH));
            layout.historyToggleRect = D2D1::RectF(
                layout.historyRect.left + kOverlayTogglePadding,
                layout.historyRect.top + 12.0f,
                layout.historyRect.left + kOverlayTogglePadding + kOverlayToggleSize,
                layout.historyRect.top + 12.0f + kOverlayToggleSize);
            const ResponderChatLayout chatLayout = BuildResponderChatLayout(view);
            if (chatLayout.progress > 0.04f && RectsOverlap(layout.historyRect, chatLayout.panelRect)) {
                const float clippedBottom = chatLayout.panelRect.top - kOverlayUiGap;
                if (clippedBottom > layout.historyRect.top + 120.0f)
                    layout.historyRect.bottom = MinValue(layout.historyRect.bottom, clippedBottom);
            }
            layout.hasHistory = true;
        }

        const float bannerRight = layout.hasHistory
            ? MaxValue(kOverlayUiMargin + 180.0f, layout.historyRect.left - kOverlayUiGap)
            : width - kOverlayUiMargin;
        layout.bannerRect = D2D1::RectF(
            kOverlayUiMargin,
            kOverlayUiMargin,
            MinValue(width - kOverlayUiMargin, bannerRight),
            kOverlayUiMargin + 72.0f);

        return layout;
    }

    D2D1_RECT_F NotificationHistoryContentRect(const D2D1_RECT_F& panelRect) const
    {
        return D2D1::RectF(
            panelRect.left + kOverlayUiPadding,
            panelRect.top + 58.0f,
            panelRect.right - kOverlayUiPadding - 10.0f,
            panelRect.bottom - kOverlayUiPadding);
    }

    D2D1_RECT_F NotificationHistoryScrollTrackRect(const D2D1_RECT_F& panelRect) const
    {
        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(panelRect);
        return D2D1::RectF(panelRect.right - 12.0f, contentRect.top, panelRect.right - 7.0f, contentRect.bottom);
    }

    D2D1_RECT_F NotificationHistoryClearRect(const D2D1_RECT_F& panelRect) const
    {
        return D2D1::RectF(
            panelRect.right - kOverlayUiPadding - 102.0f,
            panelRect.top + kOverlayUiPadding - 2.0f,
            panelRect.right - kOverlayUiPadding - 32.0f,
            panelRect.top + kOverlayUiPadding + 25.0f);
    }

    D2D1_RECT_F NotificationHistoryCloseRect(const D2D1_RECT_F& panelRect) const
    {
        return D2D1::RectF(
            panelRect.right - kOverlayUiPadding - 24.0f,
            panelRect.top + kOverlayUiPadding - 1.0f,
            panelRect.right - kOverlayUiPadding,
            panelRect.top + kOverlayUiPadding + 23.0f);
    }

    static std::vector<std::wstring> NotificationBodyLines(const std::wstring& body)
    {
        std::vector<std::wstring> lines;
        std::wstring current;
        for (wchar_t ch : body) {
            if (ch == L'\r')
                continue;
            if (ch == L'\n') {
                lines.push_back(current);
                current.clear();
                continue;
            }
            current.push_back(ch);
        }
        if (!current.empty() || lines.empty())
            lines.push_back(current);
        return lines;
    }

    float NotificationTimestampHeight(const AppNotification& notification, float width) const
    {
        return notification.timestamp.empty()
            ? 0.0f
            : MaxValue(14.0f, m_overlayUi.MeasureTextHeight(notification.timestamp, m_overlayUi.SmallFormat(), width));
    }

    float NotificationTitleHeight(const AppNotification& notification, float width) const
    {
        return MaxValue(18.0f, m_overlayUi.MeasureTextHeight(notification.title, m_overlayUi.TitleFormat(), width));
    }

    float NotificationBodyLineHeight(const std::wstring& line, float width) const
    {
        return MaxValue(18.0f, m_overlayUi.MeasureTextHeight(line.empty() ? L" " : line, m_overlayUi.BodyFormat(), width));
    }

    float NotificationBodyHeight(const AppNotification& notification, float width) const
    {
        if (notification.body.empty())
            return 0.0f;

        float height = 0.0f;
        const std::vector<std::wstring> lines = NotificationBodyLines(notification.body);
        for (const std::wstring& line : lines)
            height += NotificationBodyLineHeight(line, width) + 2.0f;
        return MaxValue(18.0f, height);
    }

    float NotificationBodyTop(const AppNotification& notification, float itemTop, float width) const
    {
        float y = itemTop + 6.0f;
        const float timeH = NotificationTimestampHeight(notification, width);
        if (timeH > 0.0f)
            y += timeH + 3.0f;
        y += NotificationTitleHeight(notification, width) + 3.0f;
        return y;
    }

    int NotificationBodyLineIndexAtY(const AppNotification& notification, float itemTop, float width, float y) const
    {
        if (notification.body.empty())
            return -1;

        float lineTop = NotificationBodyTop(notification, itemTop, width);
        const std::vector<std::wstring> lines = NotificationBodyLines(notification.body);
        for (size_t i = 0; i < lines.size(); ++i) {
            const float lineH = NotificationBodyLineHeight(lines[i], width) + 2.0f;
            if (y >= lineTop && y <= lineTop + lineH)
                return static_cast<int>(i);
            lineTop += lineH;
        }
        return -1;
    }

    float NotificationItemHeight(const AppNotification& notification, float width) const
    {
        width = MaxValue(1.0f, width);
        const float timeH = NotificationTimestampHeight(notification, width);
        const float titleH = NotificationTitleHeight(notification, width);
        const float bodyH = NotificationBodyHeight(notification, width);

        float height = 14.0f + titleH + 14.0f;
        if (timeH > 0.0f)
            height += timeH + 3.0f;
        if (bodyH > 0.0f)
            height += bodyH + 5.0f;
        return height;
    }

    float NotificationHistoryContentHeight(float width) const
    {
        if (m_notificationHistory.empty())
            return 34.0f;

        float height = 0.0f;
        for (const AppNotification& notification : m_notificationHistory)
            height += NotificationItemHeight(notification, width);
        return height;
    }

    static D2D1_RECT_F ScrollbarThumbRect(const D2D1_RECT_F& track, float contentHeight, float viewportHeight, float scrollOffset)
    {
        if (contentHeight <= viewportHeight || viewportHeight <= 1.0f || track.bottom <= track.top)
            return D2D1::RectF(0, 0, 0, 0);

        const float trackHeight = track.bottom - track.top;
        const float thumbHeight = ClampValue(trackHeight * viewportHeight / contentHeight, 26.0f, trackHeight);
        const float maxScroll = MaxValue(1.0f, contentHeight - viewportHeight);
        const float thumbTop = track.top + (trackHeight - thumbHeight) * ClampValue(scrollOffset / maxScroll, 0.0f, 1.0f);
        return D2D1::RectF(track.left, thumbTop, track.right, thumbTop + thumbHeight);
    }

    bool SetNotificationHistoryScrollFromThumbY(float y, float dragOffset)
    {
        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (!layout.hasHistory)
            return false;

        EnsureDeviceResources();
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return false;

        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(layout.historyRect);
        const float viewportH = MaxValue(1.0f, contentRect.bottom - contentRect.top);
        const float contentW = MaxValue(1.0f, contentRect.right - contentRect.left);
        const float contentH = NotificationHistoryContentHeight(contentW);
        if (contentH <= viewportH)
            return false;

        const D2D1_RECT_F track = NotificationHistoryScrollTrackRect(layout.historyRect);
        const D2D1_RECT_F thumb = ScrollbarThumbRect(track, contentH, viewportH, m_notificationHistoryScroll);
        const float travel = MaxValue(1.0f, (track.bottom - track.top) - (thumb.bottom - thumb.top));
        const float thumbTop = ClampValue(y - dragOffset, track.top, track.bottom - (thumb.bottom - thumb.top));
        const float maxScroll = MaxValue(0.0f, contentH - viewportH);
        m_notificationHistoryScroll = ClampValue(((thumbTop - track.top) / travel) * maxScroll, 0.0f, maxScroll);
        Invalidate();
        return true;
    }

    float ClampNotificationHistoryScroll(float offset) const
    {
        return MaxValue(0.0f, offset);
    }

    float MaxNotificationHistoryScroll(const D2D1_RECT_F& panelRect) const
    {
        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(panelRect);
        const float viewportH = MaxValue(1.0f, contentRect.bottom - contentRect.top);
        const float contentH = NotificationHistoryContentHeight(MaxValue(1.0f, contentRect.right - contentRect.left));
        return MaxValue(0.0f, contentH - viewportH);
    }

    bool HitNotificationInterface(int x, int y) const
    {
        if (!m_showNotificationHistory && !m_hasActiveNotification)
            return false;

        if (m_notificationContextMenuVisible && PointInRect(x, y, m_notificationContextMenuRect))
            return true;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (m_showNotificationHistory && PointInRect(x, y, layout.historyToggleRect))
            return true;
        if (m_showNotificationHistory && layout.historyProgress > 0.04f && PointInRect(x, y, layout.historyRect))
            return true;
        if (m_hasActiveNotification && m_hasLastActiveNotificationRect && PointInRect(x, y, m_lastActiveNotificationRect))
            return true;
        if (m_hasActiveNotification) {
            D2D1_RECT_F fallback = layout.bannerRect;
            fallback.bottom = fallback.top + 160.0f;
            return PointInRect(x, y, fallback);
        }
        return false;
    }

    D2D1_RECT_F BuildOverlayContextMenuRect(int x, int y, float width, float height) const
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        const float pad = 8.0f;
        float left = static_cast<float>(x);
        float top = static_cast<float>(y);
        const float maxLeft = MaxValue(pad, static_cast<float>(rc.right) - width - pad);
        const float maxTop = MaxValue(pad, static_cast<float>(rc.bottom) - height - pad);
        left = ClampValue(left, pad, maxLeft);
        top = ClampValue(top, pad, maxTop);
        return D2D1::RectF(left, top, left + width, top + height);
    }

    D2D1_RECT_F BuildNotificationContextMenuRect(int x, int y) const
    {
        return BuildOverlayContextMenuRect(x, y, 132.0f, 42.0f);
    }

    static D2D1_RECT_F ContextMenuItemRect(const D2D1_RECT_F& menuRect, int index, int count)
    {
        const float pad = 6.0f;
        const float gap = 5.0f;
        const float itemH = (menuRect.bottom - menuRect.top - pad * 2.0f - gap * MaxValue(0, count - 1)) / MaxValue(1, count);
        const float top = menuRect.top + pad + static_cast<float>(index) * (itemH + gap);
        return D2D1::RectF(menuRect.left + pad, top, menuRect.right - pad, top + itemH);
    }

    bool HandleOverlayContextMenuPointerDown(int x, int y)
    {
        if (m_chatContextMenuVisible) {
            const bool hitMenu = PointInRect(x, y, m_chatContextMenuRect);
            const size_t index = m_chatContextMenuIndex;
            HideOverlayContextMenus();
            if (hitMenu && index < m_chatMessages.size() && m_onChatMessageAction) {
                m_onChatMessageAction(m_chatMessages[index], L"delete");
                return true;
            }
            return hitMenu;
        }

        if (m_userContextMenuVisible) {
            const bool hitMenu = PointInRect(x, y, m_userContextMenuRect);
            const size_t index = m_userContextMenuIndex;
            std::wstring action;
            if (hitMenu) {
                if (PointInRect(x, y, ContextMenuItemRect(m_userContextMenuRect, 0, 3)))
                    action = L"private";
                else if (PointInRect(x, y, ContextMenuItemRect(m_userContextMenuRect, 1, 3)))
                    action = L"mute";
                else if (PointInRect(x, y, ContextMenuItemRect(m_userContextMenuRect, 2, 3)))
                    action = L"kick";
            }

            HideOverlayContextMenus();
            if (!action.empty() && index < m_onlineUsers.size() && m_onUserAction) {
                m_onUserAction(m_onlineUsers[index], action);
                return true;
            }
            return hitMenu;
        }

        if (m_notificationContextMenuVisible)
            return HandleNotificationContextMenuPointerDown(x, y);

        return false;
    }

    bool HandleNotificationContextMenuPointerDown(int x, int y)
    {
        if (!m_notificationContextMenuVisible)
            return false;

        const bool hitMenu = PointInRect(x, y, m_notificationContextMenuRect);
        const size_t index = m_notificationContextMenuIndex;
        m_notificationContextMenuVisible = false;
        m_notificationContextMenuIndex = static_cast<size_t>(-1);

        if (hitMenu && m_onNotificationHistoryDelete && index < m_notificationHistory.size()) {
            m_onNotificationHistoryDelete(index);
            Invalidate();
            return true;
        }

        Invalidate();
        return hitMenu;
    }

    bool HandleNotificationPointerDown(int x, int y)
    {
        if (HandleNotificationContextMenuPointerDown(x, y))
            return true;

        if (!m_showNotificationHistory)
            return false;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (!layout.hasHistory)
            return false;

        if (PointInRect(x, y, layout.historyToggleRect)) {
            ClearOverlayInputFocus();
            m_notificationHistoryCollapsed = !m_notificationHistoryCollapsed;
            StartNotificationHistoryAnimation(m_notificationHistoryCollapsed ? 0.0f : 1.0f);
            Invalidate();
            return true;
        }

        if (layout.historyProgress <= 0.04f || !PointInRect(x, y, layout.historyRect))
            return false;

        if (PointInRect(x, y, NotificationHistoryCloseRect(layout.historyRect))) {
            ClearOverlayInputFocus();
            if (m_onPanelClose)
                m_onPanelClose(L"notification_history");
            Invalidate();
            return true;
        }

        if (PointInRect(x, y, NotificationHistoryClearRect(layout.historyRect))) {
            ClearOverlayInputFocus();
            m_notificationHistoryScroll = 0.0f;
            if (m_onNotificationHistoryClear)
                m_onNotificationHistoryClear();
            Invalidate();
            return true;
        }

        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(layout.historyRect);
        const float viewportH = MaxValue(1.0f, contentRect.bottom - contentRect.top);
        const float contentW = MaxValue(1.0f, contentRect.right - contentRect.left);
        const float contentH = NotificationHistoryContentHeight(contentW);
        const D2D1_RECT_F track = NotificationHistoryScrollTrackRect(layout.historyRect);
        if (contentH > viewportH && PointInRect(x, y, track)) {
            const D2D1_RECT_F thumb = ScrollbarThumbRect(track, contentH, viewportH, m_notificationHistoryScroll);
            m_draggingNotificationHistoryScrollbar = true;
            m_notificationHistoryScrollbarDragOffset = PointInRect(x, y, thumb)
                ? static_cast<float>(y) - thumb.top
                : (thumb.bottom - thumb.top) * 0.5f;
            ClearOverlayInputFocus();
            SetCapture(m_hwnd);
            SetNotificationHistoryScrollFromThumbY(static_cast<float>(y), m_notificationHistoryScrollbarDragOffset);
            return true;
        }

        if (contentH > viewportH && PointInRect(x, y, contentRect)) {
            m_draggingNotificationHistoryContent = true;
            m_notificationUiMouseDown = true;
            m_lastMouse = POINT{ x, y };
            ClearOverlayInputFocus();
            SetCapture(m_hwnd);
            return true;
        }

        ClearOverlayInputFocus();
        return false;
    }

    bool NotificationHistoryNotificationAtPoint(int x, int y, AppNotification& notificationOut, size_t* indexOut = nullptr) const
    {
        if (!m_showNotificationHistory)
            return false;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (!layout.hasHistory || layout.historyProgress <= 0.04f || !PointInRect(x, y, layout.historyRect))
            return false;
        if (PointInRect(x, y, layout.historyToggleRect) ||
            PointInRect(x, y, NotificationHistoryClearRect(layout.historyRect)) ||
            PointInRect(x, y, NotificationHistoryCloseRect(layout.historyRect)))
            return false;

        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(layout.historyRect);
        if (!PointInRect(x, y, contentRect))
            return false;

        const float contentW = MaxValue(1.0f, contentRect.right - contentRect.left);
        float itemTop = contentRect.top - m_notificationHistoryScroll;
        for (size_t i = 0; i < m_notificationHistory.size(); ++i) {
            const AppNotification& notification = m_notificationHistory[i];
            const float itemH = NotificationItemHeight(notification, contentW);
            if (static_cast<float>(y) >= itemTop && static_cast<float>(y) <= itemTop + itemH) {
                notificationOut = notification;
                if (indexOut)
                    *indexOut = i;
                const int lineIndex = NotificationBodyLineIndexAtY(notification, itemTop, contentW, static_cast<float>(y));
                if (lineIndex >= 0 && static_cast<size_t>(lineIndex) < notification.links.size()) {
                    const AppNotificationLink& link = notification.links[static_cast<size_t>(lineIndex)];
                    if (!link.sourceType.empty() && !link.sourceId.empty()) {
                        notificationOut.sourceType = link.sourceType;
                        notificationOut.sourceId = link.sourceId;
                        notificationOut.body = link.text;
                        notificationOut.links.clear();
                    }
                }
                else if (!notification.links.empty() &&
                    notification.sourceType.empty() &&
                    notification.sourceId.empty() &&
                    !notification.links.front().sourceType.empty() &&
                    !notification.links.front().sourceId.empty())
                {
                    notificationOut.sourceType = notification.links.front().sourceType;
                    notificationOut.sourceId = notification.links.front().sourceId;
                    notificationOut.body = notification.links.front().text;
                    notificationOut.links.clear();
                }
                return true;
            }
            itemTop += itemH;
        }
        return false;
    }

    bool HandleNotificationHistoryRightClick(int x, int y)
    {
        if (!m_onNotificationHistoryDelete)
            return false;

        AppNotification notification;
        size_t index = static_cast<size_t>(-1);
        if (!NotificationHistoryNotificationAtPoint(x, y, notification, &index))
            return false;
        if (index == static_cast<size_t>(-1))
            return false;

        m_notificationContextMenuVisible = true;
        m_notificationContextMenuIndex = index;
        m_notificationContextMenuRect = BuildNotificationContextMenuRect(x, y);
        ClearOverlayInputFocus();
        Invalidate();
        return true;
    }

    bool TryActivateNotificationHistoryItem(int x, int y)
    {
        AppNotification notification;
        if (!NotificationHistoryNotificationAtPoint(x, y, notification))
            return false;

        if (m_onNotificationHistoryActivate)
            m_onNotificationHistoryActivate(notification);
        return true;
    }

    bool TryScrollNotificationHistoryAt(int x, int y, short delta)
    {
        if (!m_showNotificationHistory)
            return false;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (layout.historyProgress <= 0.04f || !PointInRect(x, y, layout.historyRect))
            return false;

        EnsureDeviceResources();
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return true;

        const float maxScroll = MaxNotificationHistoryScroll(layout.historyRect);
        const float direction = delta > 0 ? -1.0f : 1.0f;
        m_notificationHistoryScroll = ClampValue(
            m_notificationHistoryScroll + direction * kNotificationScrollStep,
            0.0f,
            maxScroll);
        Invalidate();
        return true;
    }

    void DrawActiveNotification(const NotificationLayout& layout, const ViewState& view)
    {
        m_hasLastActiveNotificationRect = false;
        if (!m_hasActiveNotification || !m_rt)
            return;

        D2D1_RECT_F rect = layout.bannerRect;
        const float contentW = MaxValue(1.0f, rect.right - rect.left - kOverlayUiPadding * 2.0f);
        const std::wstring title = m_activeNotification.title.empty() ? L"Notification" : m_activeNotification.title;
        const float titleH = MaxValue(20.0f, m_overlayUi.MeasureTextHeight(title, m_overlayUi.TitleFormat(), contentW));
        const float bodyH = m_activeNotification.body.empty()
            ? 0.0f
            : MaxValue(18.0f, m_overlayUi.MeasureTextHeight(m_activeNotification.body, m_overlayUi.BodyFormat(), contentW));
        const float timestampH = m_activeNotification.timestamp.empty()
            ? 0.0f
            : MaxValue(13.0f, m_overlayUi.MeasureTextHeight(m_activeNotification.timestamp, m_overlayUi.SmallFormat(), contentW));
        const float desiredH = kOverlayUiPadding * 2.0f + titleH + timestampH + (bodyH > 0.0f ? bodyH + 6.0f : 0.0f);
        rect.bottom = rect.top + ClampValue(desiredH, 58.0f, MaxValue(82.0f, static_cast<float>(view.height) * 0.38f));
        const float progress = ClampValue(m_activeNotificationProgress, 0.0f, 1.0f);
        if (progress <= 0.02f)
            return;

        const float offsetY = -(rect.bottom - rect.top + 12.0f) * (1.0f - progress);
        rect.top += offsetY;
        rect.bottom += offsetY;
        m_lastActiveNotificationRect = rect;
        m_hasLastActiveNotificationRect = true;

        m_overlayUi.DrawGlassPanel(rect, 12.0f);
        m_rt->FillRectangle(D2D1::RectF(rect.left, rect.top + 10.0f, rect.left + 3.0f, rect.bottom - 10.0f), m_overlayUi.AccentBrush());

        float y = rect.top + kOverlayUiPadding - 1.0f;
        D2D1_RECT_F titleRect = D2D1::RectF(rect.left + kOverlayUiPadding, y, rect.right - kOverlayUiPadding, y + titleH + 3.0f);
        m_overlayUi.DrawLabel(title, m_overlayUi.TitleFormat(), titleRect);
        y += titleH + 3.0f;

        if (!m_activeNotification.timestamp.empty()) {
            D2D1_RECT_F timeRect = D2D1::RectF(rect.left + kOverlayUiPadding, y, rect.right - kOverlayUiPadding, y + timestampH + 2.0f);
            m_overlayUi.DrawLabel(m_activeNotification.timestamp, m_overlayUi.SmallFormat(), timeRect, m_overlayUi.MutedTextBrush());
            y += timestampH + 2.0f;
        }

        if (!m_activeNotification.body.empty()) {
            D2D1_RECT_F bodyRect = D2D1::RectF(rect.left + kOverlayUiPadding, y + 3.0f, rect.right - kOverlayUiPadding, rect.bottom - kOverlayUiPadding + 4.0f);
            m_overlayUi.DrawLabel(m_activeNotification.body, m_overlayUi.BodyFormat(), bodyRect);
        }
    }

    void DrawNotificationHistory(const NotificationLayout& layout)
    {
        m_hasLastNotificationHistoryRect = false;
        if (!m_showNotificationHistory || !layout.hasHistory || !m_rt)
            return;

        const D2D1_RECT_F rect = layout.historyRect;
        const OverlayButton toggleButton = MakeOverlayButton(m_notificationHistoryCollapsed ? L"<" : L">", layout.historyToggleRect);
        if (layout.historyProgress <= 0.04f) {
            m_overlayUi.DrawButton(toggleButton);
            return;
        }

        if (rect.right - rect.left < 120.0f || rect.bottom - rect.top < 120.0f)
            return;

        m_lastNotificationHistoryRect = rect;
        m_hasLastNotificationHistoryRect = true;
        m_overlayUi.DrawGlassPanel(rect, 12.0f);
        m_overlayUi.DrawButton(toggleButton);

        D2D1_RECT_F titleRect = D2D1::RectF(
            layout.historyToggleRect.right + kOverlayTogglePadding,
            rect.top + kOverlayUiPadding - 1.0f,
            NotificationHistoryClearRect(rect).left - 8.0f,
            rect.top + kOverlayUiPadding + 22.0f);
        m_overlayUi.DrawLabel(L"Notification History", m_overlayUi.TitleFormat(), titleRect);
        m_overlayUi.DrawButton(MakeOverlayButton(L"Clear", NotificationHistoryClearRect(rect), !m_notificationHistory.empty()));
        m_overlayUi.DrawButton(MakeOverlayButton(L"X", NotificationHistoryCloseRect(rect)));

        std::wstring countText = m_notificationHistory.empty()
            ? L"No notifications yet"
            : std::to_wstring(m_notificationHistory.size()) + L" recent notification(s)";
        D2D1_RECT_F countRect = D2D1::RectF(
            layout.historyToggleRect.right + kOverlayTogglePadding,
            rect.top + kOverlayUiPadding + 23.0f,
            rect.right - kOverlayUiPadding,
            rect.top + kOverlayUiPadding + 43.0f);
        m_overlayUi.DrawLabel(countText, m_overlayUi.SmallFormat(), countRect, m_overlayUi.MutedTextBrush());
        m_overlayUi.DrawSeparator(rect.left + kOverlayUiPadding, rect.right - kOverlayUiPadding, rect.top + 52.0f);

        const D2D1_RECT_F contentRect = NotificationHistoryContentRect(rect);
        const float viewportH = MaxValue(1.0f, contentRect.bottom - contentRect.top);
        const float contentW = MaxValue(1.0f, contentRect.right - contentRect.left);
        const float contentH = NotificationHistoryContentHeight(contentW);
        const float maxScroll = MaxValue(0.0f, contentH - viewportH);
        m_notificationHistoryScroll = ClampValue(m_notificationHistoryScroll, 0.0f, maxScroll);

        m_rt->PushAxisAlignedClip(contentRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        if (m_notificationHistory.empty()) {
            D2D1_RECT_F emptyRect = D2D1::RectF(contentRect.left, contentRect.top + 6.0f, contentRect.right, contentRect.top + 44.0f);
            m_overlayUi.DrawLabel(L"No notifications yet.", m_overlayUi.BodyFormat(), emptyRect, m_overlayUi.MutedTextBrush());
        }
        else {
            float y = contentRect.top - m_notificationHistoryScroll;
            for (const AppNotification& notification : m_notificationHistory) {
                const float itemH = NotificationItemHeight(notification, contentW);
                if (y + itemH >= contentRect.top && y <= contentRect.bottom) {
                    float itemY = y + 6.0f;
                    if (!notification.timestamp.empty()) {
                        const float timeH = NotificationTimestampHeight(notification, contentW);
                        D2D1_RECT_F timeRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + timeH + 1.0f);
                        m_overlayUi.DrawLabel(notification.timestamp, m_overlayUi.SmallFormat(), timeRect, m_overlayUi.MutedTextBrush());
                        itemY += timeH + 3.0f;
                    }

                    const float titleH = NotificationTitleHeight(notification, contentW);
                    D2D1_RECT_F itemTitleRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + titleH + 2.0f);
                    m_overlayUi.DrawLabel(notification.title, m_overlayUi.TitleFormat(), itemTitleRect);
                    itemY += titleH + 3.0f;

                    if (!notification.body.empty()) {
                        const std::vector<std::wstring> lines = NotificationBodyLines(notification.body);
                        for (const std::wstring& line : lines) {
                            const float lineH = NotificationBodyLineHeight(line, contentW);
                            D2D1_RECT_F bodyRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + lineH + 2.0f);
                            m_overlayUi.DrawLabel(line, m_overlayUi.BodyFormat(), bodyRect, m_overlayUi.TextBrush());
                            itemY += lineH + 2.0f;
                        }
                    }

                    m_overlayUi.DrawSeparator(contentRect.left, contentRect.right, y + itemH - 5.0f);
                }
                y += itemH;
            }
        }
        m_rt->PopAxisAlignedClip();

        D2D1_RECT_F scrollTrack = NotificationHistoryScrollTrackRect(rect);
        m_overlayUi.DrawScrollbar(scrollTrack, contentH, viewportH, m_notificationHistoryScroll);
    }

    void DrawNotificationContextMenu()
    {
        if (!m_notificationContextMenuVisible || !m_rt)
            return;

        m_overlayUi.DrawGlassPanel(m_notificationContextMenuRect, 8.0f);
        D2D1_RECT_F buttonRect = D2D1::RectF(
            m_notificationContextMenuRect.left + 6.0f,
            m_notificationContextMenuRect.top + 6.0f,
            m_notificationContextMenuRect.right - 6.0f,
            m_notificationContextMenuRect.bottom - 6.0f);
        OverlayButton deleteButton = MakeOverlayButton(L"Delete", buttonRect, m_notificationContextMenuIndex < m_notificationHistory.size());
        m_overlayUi.DrawButton(deleteButton);
    }

    void DrawChatContextMenu()
    {
        if (!m_chatContextMenuVisible || !m_rt)
            return;

        m_overlayUi.DrawGlassPanel(m_chatContextMenuRect, 8.0f);
        OverlayButton deleteButton = MakeOverlayButton(
            L"Delete",
            ContextMenuItemRect(m_chatContextMenuRect, 0, 1),
            m_chatContextMenuIndex < m_chatMessages.size() && !m_chatMessages[m_chatContextMenuIndex].id.empty());
        m_overlayUi.DrawButton(deleteButton);
    }

    void DrawUserContextMenu()
    {
        if (!m_userContextMenuVisible || !m_rt)
            return;

        m_overlayUi.DrawGlassPanel(m_userContextMenuRect, 8.0f);
        const bool hasUser = m_userContextMenuIndex < m_onlineUsers.size();
        m_overlayUi.DrawButton(MakeOverlayButton(L"Private message", ContextMenuItemRect(m_userContextMenuRect, 0, 3), hasUser));
        m_overlayUi.DrawButton(MakeOverlayButton(L"Mute 15m", ContextMenuItemRect(m_userContextMenuRect, 1, 3), hasUser));
        m_overlayUi.DrawButton(MakeOverlayButton(L"Kick", ContextMenuItemRect(m_userContextMenuRect, 2, 3), hasUser));
    }

    void DrawOverlayContextMenus()
    {
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        DrawNotificationContextMenu();
        DrawChatContextMenu();
        DrawUserContextMenu();
    }

    void DrawNotificationInterface(const ViewState& view)
    {
        m_hasLastActiveNotificationRect = false;
        m_hasLastNotificationHistoryRect = false;
        if ((!m_hasActiveNotification && !m_showNotificationHistory) || !m_rt)
            return;
        if (!m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        const NotificationLayout layout = BuildNotificationLayout(view);
        DrawNotificationHistory(layout);
        DrawActiveNotification(layout, view);
    }

    void DrawAlertOverlay(const ViewState& view)
    {
        const TrafficAlert* alert = FindAlertById(m_hoveredAlertId);
        if (!alert || !alert->hasLocation)
            return;

        D2D1_POINT_2F marker = GeoToScreen(view, alert->latitude, alert->longitude);
        const bool hasLaneOverlay = HasLaneClosureOverlay(*alert);
        int total = 0;
        int closed = 0;
        if (hasLaneOverlay) {
            total = alert->lanesTotal > 0 ? alert->lanesTotal : static_cast<int>(alert->laneImageUrls.size());
            if (total == 0)
                total = static_cast<int>(alert->laneClosedStates.size());
            total = ClampValue(total, 1, 8);
            closed = ClampValue(alert->lanesClosed, 0, total);
            if (closed == 0 && alert->lanesTotal == 0 && !alert->laneImageUrls.empty())
                closed = total;
        }

        const float icon = 34.0f;
        const float gap = 4.0f;
        const float panelPad = 8.0f;
        const float iconsW = hasLaneOverlay ? (total * icon + (total - 1) * gap) : 0.0f;

        std::wstring roadTitle = alert->road.empty() ? alert->region : alert->road;
        if (roadTitle.empty())
            roadTitle = L"Traffic alert";
        std::wstring alertTitle = alert->title.empty() ? BuildSeverityDisplay(alert->severity) : alert->title;
        std::wstring laneTitle;
        if (hasLaneOverlay)
            laneTitle = L"Lanes closed: " + std::to_wstring(closed) + L" of " + std::to_wstring(total);

        const float maxPanelW = MinValue(420.0f, MaxValue(238.0f, static_cast<float>(view.width) - 16.0f));
        float naturalW = MaxValue(
            MeasureMapTextWidth(roadTitle, m_noteTextFormat.Get(), 4000.0f),
            MeasureMapTextWidth(alertTitle, m_noteTextFormat.Get(), 4000.0f));
        if (hasLaneOverlay)
            naturalW = MaxValue(naturalW, MeasureMapTextWidth(laneTitle, m_noteTextFormat.Get(), 4000.0f));
        float panelW = ClampValue(naturalW + panelPad * 2.0f, 238.0f, maxPanelW);
        if (hasLaneOverlay)
            panelW = MaxValue(panelW, MinValue(maxPanelW, iconsW + panelPad * 2.0f));

        const float contentW = MaxValue(1.0f, panelW - panelPad * 2.0f);
        const float roadH = MaxValue(18.0f, MeasureMapTextHeight(roadTitle, m_noteTextFormat.Get(), contentW));
        const float titleH = MaxValue(18.0f, MeasureMapTextHeight(alertTitle, m_noteTextFormat.Get(), contentW));
        const float laneH = hasLaneOverlay ? MaxValue(18.0f, MeasureMapTextHeight(laneTitle, m_noteTextFormat.Get(), contentW)) : 0.0f;
        const float textH = roadH + 2.0f + titleH + (hasLaneOverlay ? laneH + 2.0f : 0.0f);
        const float panelH = panelPad * 2.0f + textH + (hasLaneOverlay ? icon + 8.0f : 0.0f);

        float left = marker.x + 18.0f;
        float top = marker.y - panelH - 18.0f;
        if (left + panelW > view.width - 8.0f)
            left = marker.x - panelW - 18.0f;
        if (left < 8.0f)
            left = 8.0f;
        if (top < 8.0f)
            top = marker.y + 22.0f;
        if (top + panelH > view.height - 8.0f)
            top = view.height - panelH - 8.0f;

        D2D1_ROUNDED_RECT panel = D2D1::RoundedRect(D2D1::RectF(left, top, left + panelW, top + panelH), 8.0f, 8.0f);
        m_rt->FillRoundedRectangle(panel, m_panelBrush.Get());
        m_rt->DrawRoundedRectangle(panel, m_borderBrush.Get(), 1.0f);

        if (m_noteTextFormat) {
            float y = top + panelPad - 1.0f;
            D2D1_RECT_F roadRect = D2D1::RectF(left + panelPad, y, left + panelW - panelPad, y + roadH + 2.0f);
            m_rt->DrawTextW(roadTitle.c_str(), static_cast<UINT32>(roadTitle.size()), m_noteTextFormat.Get(), roadRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            y += roadH + 2.0f;

            D2D1_RECT_F titleRect = D2D1::RectF(left + panelPad, y, left + panelW - panelPad, y + titleH + 2.0f);
            m_rt->DrawTextW(alertTitle.c_str(), static_cast<UINT32>(alertTitle.size()), m_noteTextFormat.Get(), titleRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            y += titleH + 2.0f;

            if (hasLaneOverlay) {
                D2D1_RECT_F laneRect = D2D1::RectF(left + panelPad, y, left + panelW - panelPad, y + laneH + 2.0f);
                m_rt->DrawTextW(laneTitle.c_str(), static_cast<UINT32>(laneTitle.size()), m_noteTextFormat.Get(), laneRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!hasLaneOverlay)
            return;

        float x = left + panelPad;
        const float y = top + panelPad + textH + 6.0f;
        for (int i = 0; i < total; ++i) {
            bool drewBitmap = false;
            if (i < static_cast<int>(alert->laneImageUrls.size())) {
                ComPtr<ID2D1Bitmap> bmp = LoadCachedLaneBitmap(alert->laneImageUrls[i]);
                if (bmp) {
                    m_rt->DrawBitmap(bmp.Get(), D2D1::RectF(x, y, x + icon, y + icon));
                    drewBitmap = true;
                }
            }
            if (!drewBitmap) {
                bool laneClosed = i < closed;
                if (i < static_cast<int>(alert->laneClosedStates.size()))
                    laneClosed = alert->laneClosedStates[static_cast<size_t>(i)];
                DrawFallbackLaneIcon(x, y, icon, laneClosed);
            }
            x += icon + gap;
        }
    }

    void DrawMarkers(const ViewState& view)
    {
        const int width = view.width;
        const int height = view.height;

        for (size_t i = 0; i < m_alerts.size(); ++i) {
            const bool selected = (m_alerts[i].id == m_selectedId);

            if (!m_alerts[i].hasLocation ||
                !IsGeoPointInView(view, m_alerts[i].latitude, m_alerts[i].longitude))
            {
                continue;
            }

            D2D1_POINT_2F p = GeoToScreen(view, m_alerts[i].latitude, m_alerts[i].longitude);
            if (p.x < -20.0f || p.y < -20.0f || p.x > width + 20.0f || p.y > height + 20.0f)
                continue;

            ID2D1SolidColorBrush* sevBrush = BrushForSeverity(m_alerts[i].severity);

            float outerR = selected ? 15.0f : 11.0f;
            float innerR = selected ? 8.0f : 6.0f;

            D2D1_POINT_2F tip = D2D1::Point2F(p.x, p.y + outerR + 7.0f);
            D2D1_POINT_2F left = D2D1::Point2F(p.x - outerR * 0.58f, p.y + outerR * 0.35f);
            D2D1_POINT_2F right = D2D1::Point2F(p.x + outerR * 0.58f, p.y + outerR * 0.35f);
            ComPtr<ID2D1PathGeometry> pinGeom;
            if (SUCCEEDED(g_d2dFactory->CreatePathGeometry(&pinGeom))) {
                ComPtr<ID2D1GeometrySink> sink;
                if (SUCCEEDED(pinGeom->Open(&sink))) {
                    sink->BeginFigure(left, D2D1_FIGURE_BEGIN_FILLED);
                    sink->AddLine(tip);
                    sink->AddLine(right);
                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    if (SUCCEEDED(sink->Close()))
                        m_rt->FillGeometry(pinGeom.Get(), sevBrush);
                }
            }

            D2D1_ELLIPSE outer = D2D1::Ellipse(p, outerR, outerR);
            D2D1_ELLIPSE inner = D2D1::Ellipse(p, innerR, innerR);

            if (selected)
                m_rt->FillEllipse(D2D1::Ellipse(p, outerR + 5.0f, outerR + 5.0f), m_selectedBrush.Get());

            m_rt->FillEllipse(outer, sevBrush);
            m_rt->FillEllipse(inner, m_textBrush.Get());
            m_rt->DrawEllipse(outer, m_borderBrush.Get(), selected ? 2.5f : 1.5f);

            if (m_showIncidentOverlayLabels) {
                std::wstring label = BuildAlertSummary(m_alerts[i]);
                if (!label.empty())
                    DrawMeasuredBlipLabel(view, p, outerR, label, sevBrush, 150.0f, 380.0f);
            }
        }
    }

    void DrawTilesForView(const ViewState& tileView, bool interactive, const D2D1_RECT_F* clip = nullptr, bool requestMissingTiles = true)
    {
        int width = std::max(1, tileView.width);
        int height = std::max(1, tileView.height);

        const D2D1_RECT_F viewport = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        if (clip && !RectsIntersect(*clip, viewport))
            return;

        const float drawLeft = clip ? ClampValue(clip->left, 0.0f, viewport.right) : viewport.left;
        const float drawTop = clip ? ClampValue(clip->top, 0.0f, viewport.bottom) : viewport.top;
        const float drawRight = clip ? ClampValue(clip->right, 0.0f, viewport.right) : viewport.right;
        const float drawBottom = clip ? ClampValue(clip->bottom, 0.0f, viewport.bottom) : viewport.bottom;
        if (drawRight <= drawLeft || drawBottom <= drawTop)
            return;

        double originX = tileView.centerWorld.x - width * 0.5;
        double originY = tileView.centerWorld.y - height * 0.5;

        int startTileX = static_cast<int>(std::floor((originX + drawLeft) / 256.0)) - 1;
        int endTileX = static_cast<int>(std::floor((originX + drawRight) / 256.0)) + 1;
        int startTileY = static_cast<int>(std::floor((originY + drawTop) / 256.0)) - 1;
        int endTileY = static_cast<int>(std::floor((originY + drawBottom) / 256.0)) + 1;

        int tilesPerAxis = 1 << m_zoom;

        for (int ty = startTileY; ty <= endTileY; ++ty) {
            if (ty < 0 || ty >= tilesPerAxis)
                continue;

            for (int tx = startTileX; tx <= endTileX; ++tx) {
                D2D1_RECT_F dest = D2D1::RectF(
                    static_cast<float>(tx * 256.0 - originX),
                    static_cast<float>(ty * 256.0 - originY),
                    static_cast<float>(tx * 256.0 - originX + 256.0),
                    static_cast<float>(ty * 256.0 - originY + 256.0));
                if (clip && !RectsIntersect(dest, *clip))
                    continue;

                TileKey key{ m_zoom, PositiveModulo(tx, tilesPerAxis), ty };
                auto entry = interactive ? FindTile(key) : GetOrCreateTile(key);
                if (entry) {
                    std::lock_guard<std::mutex> lk(entry->mutex);
                    entry->lastUsedMs = GetTickCount64();
                }

                ComPtr<ID2D1Bitmap> bmp;
                std::vector<BYTE> bytesCopy;

                if (entry) {
                    std::lock_guard<std::mutex> lk(entry->mutex);
                    if (entry->bitmap) {
                        bmp = entry->bitmap;
                    }
                    else if (!interactive && entry->ready && !entry->failed && !entry->bytes.empty()) {
                        if (m_tileBitmapDecodesThisFrame < kMaxTileBitmapDecodesPerFrame) {
                            bytesCopy = entry->bytes;
                            ++m_tileBitmapDecodesThisFrame;
                        }
                        else {
                            m_pendingTileBitmapDecode = true;
                        }
                    }
                }

                if (!interactive && entry && !bmp && !bytesCopy.empty()) {
                    bmp = CreateBitmapFromBytes(bytesCopy);
                    if (bmp) {
                        std::lock_guard<std::mutex> lk(entry->mutex);
                        if (!entry->bitmap)
                            entry->bitmap = bmp;
                    }
                    else {
                        std::lock_guard<std::mutex> lk(entry->mutex);
                        if (!entry->bitmap && entry->ready && !entry->bytes.empty()) {
                            entry->bytes.clear();
                            entry->ready = false;
                            entry->failed = true;
                            entry->lastAttemptMs = GetTickCount64();
                        }
                    }
                }

                if (bmp) {
                    m_rt->DrawBitmap(bmp.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                }
                else {
                    const bool drewFallback = TryDrawFallbackTile(key, dest);
                    if (!requestMissingTiles) {
                        // Offscreen scene-cache renders should never fan out into a
                        // large background tile request burst. Visible paints drive
                        // tile loading; cache renders reuse whatever is already ready.
                    }
                    else if (!interactive) {
                        RequestTile(key);
                    }
                    else if (m_interactiveTileRequestsThisFrame < kMaxInteractiveTileRequestsPerFrame) {
                        RequestTile(key);
                        ++m_interactiveTileRequestsThisFrame;
                    }

                    if (!drewFallback)
                        m_rt->FillRectangle(dest, m_placeholderBrush.Get());

                    // Keep the unloaded-tile grid stable during interaction too;
                    // otherwise placeholder/fallback tile boundaries visibly blink
                    // off while panning or zooming.
                    m_rt->DrawRectangle(dest, m_borderBrush.Get(), 0.5f);
                }
            }
        }

        if (!interactive)
            PruneTileCache();
    }

    void DrawTiles(bool interactive, const D2D1_RECT_F* clip = nullptr)
    {
        DrawTilesForView(BuildViewState(), interactive, clip);
    }



    static void DrawWorkerRoadDepictionLines(
        ID2D1RenderTarget* rt,
        ID2D1Brush* roadBrush,
        ID2D1Brush* casingBrush,
        const ViewState& view,
        int zoom,
        const std::shared_ptr<const std::vector<RoadDepictionRoute>>& roadRoutes,
        const std::unordered_set<std::wstring>& hiddenRoadDepictionIds)
    {
        if (!rt || !roadBrush || !casingBrush)
            return;
        if (!roadRoutes)
            return;

        const float casingWidth = zoom >= 8 ? 7.0f : 5.0f;
        const float roadWidth = zoom >= 8 ? 4.0f : 3.0f;
        for (const RoadDepictionRoute& route : *roadRoutes) {
            if (IsRoadDepictionHiddenFast(hiddenRoadDepictionIds, route))
                continue;
            if (!RoadRouteIntersectsView(view, route, zoom))
                continue;

            const std::vector<GeoPoint>& points = RoadRoutePointsForZoom(route, zoom);
            for (size_t i = 1; i < points.size(); ++i) {
                if (!RoadSegmentIntersectsView(view, points[i - 1], points[i]))
                    continue;

                D2D1_POINT_2F a = GeoToScreenForZoom(view, points[i - 1].lat, points[i - 1].lon, zoom);
                D2D1_POINT_2F b = GeoToScreenForZoom(view, points[i].lat, points[i].lon, zoom);
                rt->DrawLine(a, b, casingBrush, casingWidth);
                rt->DrawLine(a, b, roadBrush, roadWidth);
            }
        }
    }

    void DrawRoadDepictionLines(const ViewState& view)
    {
        if (!m_rt || !m_showRoadDepictions)
            return;
        if (!m_roadDepictionRoutes)
            return;

        const float casingWidth = m_zoom >= 8 ? 7.0f : 5.0f;
        const float roadWidth = m_zoom >= 8 ? 4.0f : 3.0f;
        for (const RoadDepictionRoute& route : *m_roadDepictionRoutes) {
            if (IsRoadDepictionHiddenFast(m_normalizedHiddenRoadDepictionIds, route))
                continue;
            if (!RoadRouteIntersectsView(view, route, m_zoom))
                continue;

            const std::vector<GeoPoint>& points = RoadRoutePointsForZoom(route, m_zoom);
            for (size_t i = 1; i < points.size(); ++i) {
                if (!RoadSegmentIntersectsView(view, points[i - 1], points[i]))
                    continue;

                D2D1_POINT_2F a = GeoToScreen(view, points[i - 1].lat, points[i - 1].lon);
                D2D1_POINT_2F b = GeoToScreen(view, points[i].lat, points[i].lon);
                m_rt->DrawLine(a, b, m_roadCasingBrush.Get(), casingWidth);
                m_rt->DrawLine(a, b, m_roadBrush.Get(), roadWidth);
            }
        }
    }

    void DrawRoadDepictionLabels(const ViewState& view)
    {
        if (!m_rt || !m_showRoadDepictions || !m_noteTextFormat || m_zoom < 7 || m_interactivePan)
            return;
        if (!m_roadDepictionRoutes)
            return;

        std::unordered_set<std::wstring> drawnLabels;
        for (const RoadDepictionRoute& route : *m_roadDepictionRoutes) {
            if (IsRoadDepictionHiddenFast(m_normalizedHiddenRoadDepictionIds, route))
                continue;
            if (!RoadRouteIntersectsView(view, route, m_zoom))
                continue;

            std::wstring labelKey = !route.normalizedLabel.empty() ? route.normalizedLabel : ToLower(route.label);
            if (!drawnLabels.insert(labelKey).second)
                continue;
            const std::vector<GeoPoint>& points = RoadRoutePointsForZoom(route, m_zoom);
            if (points.empty())
                continue;
            const GeoPoint& mid = points[points.size() / 2];
            if (IsGeoPointInView(view, mid.lat, mid.lon)) {
                D2D1_POINT_2F p = GeoToScreen(view, mid.lat, mid.lon);
                D2D1_RECT_F rect = D2D1::RectF(p.x + 6.0f, p.y - 12.0f, p.x + 74.0f, p.y + 12.0f);
                m_rt->DrawTextW(route.label.c_str(), static_cast<UINT32>(route.label.size()), m_noteTextFormat.Get(), rect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }

    void DrawCityAnchors(const ViewState& view)
    {
        if (!m_showAreaLabels)
            return;

        struct CityAnchor
        {
            const wchar_t* name;
            GeoPoint point;
        };

        static const CityAnchor cities[] = {
            { L"London", { 51.5074, -0.1278 } },
            { L"Birmingham", { 52.4862, -1.8904 } },
            { L"Manchester", { 53.4808, -2.2426 } },
            { L"Leeds", { 53.8008, -1.5491 } },
            { L"Edinburgh", { 55.9533, -3.1883 } },
            { L"Cardiff", { 51.4816, -3.1791 } },
            { L"Portsmouth", { 50.8198, -1.0880 } },
            { L"Newcastle", { 54.9783, -1.6178 } }
        };

        const int width = view.width;
        const int height = view.height;

        for (const CityAnchor& city : cities) {
            if (!IsGeoPointInView(view, city.point.lat, city.point.lon))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, city.point.lat, city.point.lon);
            if (p.x < -16.0f || p.y < -16.0f || p.x > width + 16.0f || p.y > height + 16.0f)
                continue;

            m_rt->FillEllipse(D2D1::Ellipse(p, 4.0f, 4.0f), m_textBrush.Get());
            m_rt->DrawEllipse(D2D1::Ellipse(p, 8.0f, 8.0f), m_panelBrush.Get(), 2.0f);
            if (m_zoom >= 6 && m_noteTextFormat) {
                D2D1_RECT_F rect = D2D1::RectF(p.x + 9.0f, p.y - 10.0f, p.x + 118.0f, p.y + 14.0f);
                m_rt->DrawTextW(city.name, static_cast<UINT32>(wcslen(city.name)), m_noteTextFormat.Get(), rect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }
    }

    void DrawNotes(const ViewState& view)
    {
        if (!m_rt)
            return;

        int width = view.width;
        int height = view.height;

        for (const auto& note : m_notes) {
            size_t noteIndex = static_cast<size_t>(&note - m_notes.data());
            if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex == noteIndex)
                continue;
            if (!IsGeoPointInView(view, note.latitude, note.longitude))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, note.latitude, note.longitude);
            if (p.x < -80.0f || p.y < -80.0f || p.x > width + 80.0f || p.y > height + 80.0f)
                continue;

            const std::wstring text = NoteDisplayText(note);
            D2D1_RECT_F bubbleRect = BuildNoteBubbleRect(view, p, kNoteBubbleWidth, NoteBubbleHeightForText(text));
            D2D1_RECT_F textRect = D2D1::RectF(bubbleRect.left + 14.0f, bubbleRect.top + 10.0f, bubbleRect.right - 30.0f, bubbleRect.bottom - 8.0f);
            D2D1_ROUNDED_RECT bubble = D2D1::RoundedRect(bubbleRect, 10.0f, 10.0f);
            m_rt->FillRoundedRectangle(bubble, m_panelBrush.Get());
            m_rt->DrawRoundedRectangle(bubble, m_noteBrush.Get(), 1.5f);
            m_rt->FillEllipse(D2D1::Ellipse(p, 8.0f, 8.0f), m_noteBrush.Get());
            m_rt->DrawLine(p, D2D1::Point2F(bubbleRect.left + 6.0f, bubbleRect.bottom - 12.0f), m_noteBrush.Get(), 2.0f);
            D2D1_RECT_F closeRect = BuildNoteCloseRect(bubbleRect);
            D2D1_POINT_2F c = D2D1::Point2F((closeRect.left + closeRect.right) * 0.5f, (closeRect.top + closeRect.bottom) * 0.5f);
            m_rt->DrawEllipse(D2D1::Ellipse(c, 8.0f, 8.0f), m_noteBrush.Get(), 1.2f);
            m_rt->DrawLine(D2D1::Point2F(c.x - 4.0f, c.y - 4.0f), D2D1::Point2F(c.x + 4.0f, c.y + 4.0f), m_textBrush.Get(), 1.6f);
            m_rt->DrawLine(D2D1::Point2F(c.x + 4.0f, c.y - 4.0f), D2D1::Point2F(c.x - 4.0f, c.y + 4.0f), m_textBrush.Get(), 1.6f);
            if (m_noteTextFormat && !text.empty())
                m_rt->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), m_noteTextFormat.Get(), textRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    void DrawNoteEditor(const ViewState& view)
    {
        if (m_noteEditorMode == NoteEditorMode::None || !m_rt)
            return;

        const D2D1_POINT_2F anchor = GeoToScreen(view, m_noteEditorLat, m_noteEditorLon);
        const D2D1_RECT_F rect = BuildNoteEditorRect(view);
        m_rt->DrawLine(anchor, D2D1::Point2F(rect.left + 8.0f, rect.bottom - 18.0f), m_noteBrush.Get(), 2.0f);
        m_rt->FillEllipse(D2D1::Ellipse(anchor, 8.0f, 8.0f), m_noteBrush.Get());

        D2D1_ROUNDED_RECT bubble = D2D1::RoundedRect(rect, 12.0f, 12.0f);
        m_rt->FillRoundedRectangle(bubble, m_panelBrush.Get());
        m_rt->DrawRoundedRectangle(bubble, m_noteBrush.Get(), 1.7f);

        const std::wstring title = m_noteEditorMode == NoteEditorMode::New ? L"New map note" : L"Edit map note";
        D2D1_RECT_F titleRect = D2D1::RectF(rect.left + 14.0f, rect.top + 10.0f, rect.right - 38.0f, rect.top + 34.0f);
        m_overlayUi.DrawLabel(title, m_overlayUi.TitleFormat(), titleRect);

        D2D1_RECT_F closeRect = BuildNoteCloseRect(rect);
        OverlayButton closeButton = MakeOverlayButton(L"x", closeRect);
        m_overlayUi.DrawButton(closeButton);

        OverlayTextBox textBox;
        textBox.text = m_noteEditorText;
        textBox.placeholder = L"Type note text here...";
        textBox.bounds = D2D1::RectF(rect.left + 12.0f, rect.top + 42.0f, rect.right - 12.0f, rect.bottom - 54.0f);
        textBox.focused = m_overlayInputFocus == OverlayInputFocus::NoteEditor;
        m_overlayUi.DrawTextBox(textBox);

        const bool showCaret = (GetTickCount64() / 550) % 2 == 0;
        if (textBox.focused && showCaret && m_noteEditorText.empty()) {
            m_rt->DrawLine(
                D2D1::Point2F(textBox.bounds.left + 10.0f, textBox.bounds.top + 10.0f),
                D2D1::Point2F(textBox.bounds.left + 10.0f, textBox.bounds.bottom - 10.0f),
                m_overlayUi.AccentBrush(),
                1.4f);
        }

        OverlayButton saveButton = MakeOverlayButton(L"Save", D2D1::RectF(rect.right - 156.0f, rect.bottom - 42.0f, rect.right - 84.0f, rect.bottom - 12.0f));
        m_overlayUi.DrawButton(saveButton);

        OverlayButton cancelButton = MakeOverlayButton(L"Cancel", D2D1::RectF(rect.right - 78.0f, rect.bottom - 42.0f, rect.right - 12.0f, rect.bottom - 12.0f));
        m_overlayUi.DrawButton(cancelButton);
    }

    void DrawNoteToolbar()
    {
        if (!m_rt || !m_showToolbarPanel)
            return;

        const D2D1_RECT_F panel = BuildToolbarPanelRect();
        m_overlayUi.DrawGlassPanel(panel, 10.0f);
        m_overlayUi.DrawLabel(
            L"Map Controls",
            m_overlayUi.TitleFormat(),
            D2D1::RectF(panel.left + 12.0f, panel.top + 11.0f, BuildToolbarCloseButtonRect().left - 6.0f, panel.top + 34.0f));
        m_overlayUi.DrawButton(MakeOverlayButton(L"X", BuildToolbarCloseButtonRect()));

        m_overlayUi.DrawButton(MakeOverlayButton(L"Refresh", BuildRefreshButtonRect()));
        m_overlayUi.DrawButton(MakeOverlayButton(L"Reset View", BuildResetViewButtonRect()));
        m_overlayUi.DrawButton(MakeOverlayButton(L"Fit Alerts", BuildFitAlertsButtonRect()));

        OverlayButton addButton = MakeOverlayButton(m_addNoteMode ? L"Adding" : L"+ Note", BuildAddNoteButtonRect());
        addButton.hot = addButton.hot || m_addNoteMode;
        m_overlayUi.DrawButton(addButton);

        m_overlayUi.DrawLabel(
            L"Map Display",
            m_overlayUi.SmallFormat(),
            D2D1::RectF(panel.left + 12.0f, panel.top + 119.0f, panel.right - 12.0f, panel.top + 134.0f),
            m_overlayUi.MutedTextBrush());
        OverlayButton ukButton = MakeOverlayButton(L"UK", BuildMapDisplayUkButtonRect());
        ukButton.hot = ukButton.hot || !m_displayWorldMap;
        m_overlayUi.DrawButton(ukButton);
        OverlayButton worldButton = MakeOverlayButton(L"World", BuildMapDisplayWorldButtonRect());
        worldButton.hot = worldButton.hot || m_displayWorldMap;
        m_overlayUi.DrawButton(worldButton);

        if (m_addNoteMode) {
            D2D1_RECT_F promptRect = BuildAddNotePromptRect();
            m_overlayUi.DrawGlassPanel(promptRect, 8.0f);
            D2D1_RECT_F textRect = D2D1::RectF(promptRect.left + 10.0f, promptRect.top + 7.0f, promptRect.right - 10.0f, promptRect.bottom - 4.0f);
            m_overlayUi.DrawLabel(L"Click the map to place the note", m_overlayUi.ControlFormat(), textRect);
        }
    }

    void DrawNoteInterface(const ViewState& view)
    {
        if (!m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        DrawNoteToolbar();
        DrawNoteEditor(view);
    }

    void DrawMapChrome()
    {
        if (!m_rt)
            return;

        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);

        float scaleW = ClampValue(width / static_cast<float>(m_zoom + 2), 70.0f, 180.0f);
        D2D1_POINT_2F a = D2D1::Point2F(width - scaleW - 34.0f, height - 34.0f);
        D2D1_POINT_2F b = D2D1::Point2F(width - 34.0f, height - 34.0f);
        m_rt->DrawLine(a, b, m_panelBrush.Get(), 5.0f);
        m_rt->DrawLine(a, b, m_textBrush.Get(), 2.0f);
    }

    void UpdateFpsSample()
    {
        if (!m_showFpsCounter)
            return;

        const ULONGLONG now = GetTickCount64();
        if (m_fpsLastSampleMs == 0)
            m_fpsLastSampleMs = now;

        ++m_fpsFrameCount;
        const ULONGLONG elapsed = now - m_fpsLastSampleMs;
        if (elapsed >= 500) {
            m_fpsValue = static_cast<double>(m_fpsFrameCount) * 1000.0 / static_cast<double>(elapsed);
            m_fpsFrameCount = 0;
            m_fpsLastSampleMs = now;
        }
    }

    void DrawFpsCounter()
    {
        if (!m_showFpsCounter || !m_rt || !m_overlayUi.EnsureResources(m_rt.Get(), g_dwriteFactory.Get()))
            return;

        wchar_t buffer[48]{};
        swprintf_s(buffer, L"FPS %.0f", m_fpsValue);
        const float width = 78.0f;
        const float top = m_showToolbarPanel ? BuildToolbarPanelRect().bottom + 8.0f : 18.0f;
        const D2D1_RECT_F rect = D2D1::RectF(18.0f, top, 18.0f + width, top + 28.0f);
        m_overlayUi.DrawGlassPanel(rect, 8.0f);
        m_overlayUi.DrawLabel(buffer, m_overlayUi.ControlFormat(), D2D1::RectF(rect.left + 9.0f, rect.top + 6.0f, rect.right - 8.0f, rect.bottom - 5.0f));
    }

    void TraceSlowPaintStage(const wchar_t* stage, ULONGLONG elapsedMs, bool interactive) const
    {
        if (elapsedMs < 80)
            return;

        wchar_t buffer[256]{};
        swprintf_s(
            buffer,
            L"ERC Tools map slow paint: %s took %llu ms (zoom=%d, world=%d, interactive=%d)\n",
            stage,
            static_cast<unsigned long long>(elapsedMs),
            m_zoom,
            m_displayWorldMap ? 1 : 0,
            interactive ? 1 : 0);
        OutputDebugStringW(buffer);
    }


    bool GetSceneCachePlacement(const ViewState& view, bool allowScaled, D2D1_RECT_F& dest, double& scale) const
    {
        if (!m_sceneBitmap || m_sceneBitmapWidth <= 0 || m_sceneBitmapHeight <= 0)
            return false;
        if (m_sceneViewportWidth != view.width || m_sceneViewportHeight != view.height)
            return false;

        const int zoomDelta = m_zoom - m_sceneBitmapZoom;
        if (zoomDelta != 0) {
            if (!allowScaled)
                return false;
            if (std::abs(zoomDelta) > kSceneCacheMaxScaledZoomDelta)
                return false;
        }

        scale = std::ldexp(1.0, zoomDelta);
        if (!std::isfinite(scale) || scale <= 0.0)
            return false;

        const double cacheCenterX = std::ldexp(m_sceneBitmapCenterWorld.x, zoomDelta);
        const double cacheCenterY = std::ldexp(m_sceneBitmapCenterWorld.y, zoomDelta);

        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        double dx = cacheCenterX - view.centerWorld.x;
        if (dx > worldSize * 0.5)
            dx -= worldSize;
        else if (dx < -worldSize * 0.5)
            dx += worldSize;

        const double dy = cacheCenterY - view.centerWorld.y;
        const float scaledWidth = static_cast<float>(m_sceneBitmapWidth * scale);
        const float scaledHeight = static_cast<float>(m_sceneBitmapHeight * scale);
        const float centerX = static_cast<float>(view.width * 0.5 + dx);
        const float centerY = static_cast<float>(view.height * 0.5 + dy);
        dest = D2D1::RectF(
            centerX - scaledWidth * 0.5f,
            centerY - scaledHeight * 0.5f,
            centerX + scaledWidth * 0.5f,
            centerY + scaledHeight * 0.5f);
        return true;
    }

    bool SceneCacheCoversView(const ViewState& view, const D2D1_RECT_F& dest) const
    {
        return dest.left <= 0.0f &&
            dest.top <= 0.0f &&
            dest.right >= static_cast<float>(view.width) &&
            dest.bottom >= static_cast<float>(view.height);
    }

    bool SceneCacheResultCoversView(const SceneCacheBuildResult& result, const ViewState& view) const
    {
        if (result.cacheWidth <= 0 ||
            result.cacheHeight <= 0 ||
            result.viewportWidth != view.width ||
            result.viewportHeight != view.height ||
            result.zoom != m_zoom)
        {
            return false;
        }

        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        double dx = result.centerWorld.x - view.centerWorld.x;
        if (dx > worldSize * 0.5)
            dx -= worldSize;
        else if (dx < -worldSize * 0.5)
            dx += worldSize;

        const double dy = result.centerWorld.y - view.centerWorld.y;
        const float centerX = static_cast<float>(view.width * 0.5 + dx);
        const float centerY = static_cast<float>(view.height * 0.5 + dy);
        const D2D1_RECT_F dest = D2D1::RectF(
            centerX - result.cacheWidth * 0.5f,
            centerY - result.cacheHeight * 0.5f,
            centerX + result.cacheWidth * 0.5f,
            centerY + result.cacheHeight * 0.5f);
        return SceneCacheCoversView(view, dest);
    }

    static bool BuildWorkerBoundaryGeometry(
        ID2D1Factory* factory,
        const std::vector<GeoPoint>& points,
        ComPtr<ID2D1PathGeometry>& geometry,
        double& geometryMinX,
        double& geometryMaxX,
        double& geometryMinY,
        double& geometryMaxY)
    {
        if (!factory || points.size() < 3)
            return false;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(factory->CreatePathGeometry(&geom)))
            return false;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return false;

        sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

        constexpr double baseWorldSize = 256.0;
        constexpr double halfBaseWorld = baseWorldSize * 0.5;

        WorldPoint first = GeoToWorld(points[0].lat, points[0].lon, 0);
        double previousX = first.x;
        geometryMinX = geometryMaxX = first.x;
        geometryMinY = geometryMaxY = first.y;
        sink->BeginFigure(
            D2D1::Point2F(static_cast<float>(first.x), static_cast<float>(first.y)),
            D2D1_FIGURE_BEGIN_FILLED);

        const bool hasExplicitClose = points.size() > 1 && SameGeoPoint(points.front(), points.back());
        for (size_t i = 1; i < points.size(); ++i) {
            WorldPoint p = GeoToWorld(points[i].lat, points[i].lon, 0);
            while (p.x - previousX > halfBaseWorld)
                p.x -= baseWorldSize;
            while (p.x - previousX < -halfBaseWorld)
                p.x += baseWorldSize;
            previousX = p.x;

            geometryMinX = MinValue(geometryMinX, p.x);
            geometryMaxX = MaxValue(geometryMaxX, p.x);
            geometryMinY = MinValue(geometryMinY, p.y);
            geometryMaxY = MaxValue(geometryMaxY, p.y);
            sink->AddLine(D2D1::Point2F(static_cast<float>(p.x), static_cast<float>(p.y)));
        }

        sink->EndFigure(hasExplicitClose ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);
        if (FAILED(sink->Close()))
            return false;

        geometry = geom;
        return true;
    }

    static void DrawWorkerBoundaryRingFull(
        ID2D1RenderTarget* rt,
        ID2D1Factory* factory,
        ID2D1Brush* fillBrush,
        ID2D1Brush* strokeBrush,
        const std::vector<GeoPoint>& points,
        const ViewState& view,
        int zoom)
    {
        if (!rt || !factory || points.size() < 3)
            return;

        ComPtr<ID2D1PathGeometry> geometry;
        double geometryMinX = 0.0;
        double geometryMaxX = 0.0;
        double geometryMinY = 0.0;
        double geometryMaxY = 0.0;
        if (!BuildWorkerBoundaryGeometry(
            factory,
            points,
            geometry,
            geometryMinX,
            geometryMaxX,
            geometryMinY,
            geometryMaxY))
        {
            return;
        }

        const double scale = static_cast<double>(1 << zoom);
        const double worldSize = 256.0 * scale;
        const double viewLeft = view.centerWorld.x - view.width * 0.5;
        const double viewRight = view.centerWorld.x + view.width * 0.5;
        const double viewTop = view.centerWorld.y - view.height * 0.5;
        const double viewBottom = view.centerWorld.y + view.height * 0.5;
        const double geomTop = geometryMinY * scale;
        const double geomBottom = geometryMaxY * scale;
        if (geomBottom < viewTop - 4.0 || geomTop > viewBottom + 4.0)
            return;

        D2D1_MATRIX_3X2_F oldTransform{};
        rt->GetTransform(&oldTransform);

        const int minCopy = static_cast<int>(std::floor((viewLeft - geometryMaxX * scale) / worldSize)) - 1;
        const int maxCopy = static_cast<int>(std::ceil((viewRight - geometryMinX * scale) / worldSize)) + 1;
        for (int copy = minCopy; copy <= maxCopy; ++copy) {
            const double shiftX = static_cast<double>(copy) * worldSize;
            const double geomLeft = geometryMinX * scale + shiftX;
            const double geomRight = geometryMaxX * scale + shiftX;
            if (geomRight < viewLeft - 4.0 || geomLeft > viewRight + 4.0)
                continue;

            const double dx = -view.centerWorld.x + view.width * 0.5 + shiftX;
            const double dy = -view.centerWorld.y + view.height * 0.5;
            const D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F(
                static_cast<float>(scale), 0.0f,
                0.0f, static_cast<float>(scale),
                static_cast<float>(dx), static_cast<float>(dy));
            rt->SetTransform(transform);

            if (fillBrush)
                rt->FillGeometry(geometry.Get(), fillBrush);
            if (strokeBrush) {
                const float strokeWidth = static_cast<float>(2.0 / MaxValue(scale, 1.0));
                rt->DrawGeometry(geometry.Get(), strokeBrush, strokeWidth);
            }
        }

        rt->SetTransform(oldTransform);
    }

    static std::unique_ptr<SceneCacheBuildResult> RenderSceneCacheOnWorker(std::unique_ptr<SceneCacheBuildRequest> request)
    {
        auto result = std::make_unique<SceneCacheBuildResult>();
        if (!request)
            return result;

        result->zoom = request->zoom;
        result->cacheWidth = request->cacheWidth;
        result->cacheHeight = request->cacheHeight;
        result->viewportWidth = request->viewportWidth;
        result->viewportHeight = request->viewportHeight;
        result->displayWorldMap = request->displayWorldMap;
        result->centerWorld = request->centerWorld;

        HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(coHr);

        ComPtr<ID2D1Factory> factory;
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr)) {
            ComPtr<IWICImagingFactory> wicFactory;
            hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory));
            if (SUCCEEDED(hr) && wicFactory) {
                ComPtr<IWICBitmap> wicBitmap;
                hr = wicFactory->CreateBitmap(
                    static_cast<UINT>(request->cacheWidth),
                    static_cast<UINT>(request->cacheHeight),
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapCacheOnLoad,
                    &wicBitmap);
                if (SUCCEEDED(hr) && wicBitmap) {
                    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                        D2D1_RENDER_TARGET_TYPE_DEFAULT,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                        96.0f,
                        96.0f);

                    ComPtr<ID2D1RenderTarget> rt;
                    hr = factory->CreateWicBitmapRenderTarget(wicBitmap.Get(), props, &rt);
                    if (SUCCEEDED(hr) && rt) {
                        ComPtr<ID2D1SolidColorBrush> fillBrush;
                        ComPtr<ID2D1SolidColorBrush> strokeBrush;
                        ComPtr<ID2D1SolidColorBrush> roadBrush;
                        ComPtr<ID2D1SolidColorBrush> roadCasingBrush;
                        rt->CreateSolidColorBrush(D2D1::ColorF(0.30f, 0.55f, 0.25f, 0.18f), &fillBrush);
                        rt->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.28f, 0.12f, 0.92f), &strokeBrush);
                        if (request->includeRoadDepictions) {
                            rt->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.97f, 0.84f, 0.88f), &roadBrush);
                            rt->CreateSolidColorBrush(D2D1::ColorF(0.11f, 0.19f, 0.24f, 0.70f), &roadCasingBrush);
                        }

                        rt->BeginDraw();
                        rt->SetTransform(D2D1::Matrix3x2F::Identity());
                        rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

                        for (const SceneCacheRingSnapshot& ring : request->rings) {
                            DrawWorkerBoundaryRingFull(
                                rt.Get(),
                                factory.Get(),
                                fillBrush.Get(),
                                strokeBrush.Get(),
                                ring.points,
                                request->boundaryView,
                                request->zoom);
                        }

                        if (request->includeRoadDepictions) {
                            DrawWorkerRoadDepictionLines(
                                rt.Get(),
                                roadBrush.Get(),
                                roadCasingBrush.Get(),
                                request->boundaryView,
                                request->zoom,
                                request->roadRoutes,
                                request->hiddenRoadDepictionIds);
                        }

                        hr = rt->EndDraw();
                        if (SUCCEEDED(hr)) {
                            WICRect rect{};
                            rect.X = 0;
                            rect.Y = 0;
                            rect.Width = request->cacheWidth;
                            rect.Height = request->cacheHeight;

                            ComPtr<IWICBitmapLock> lock;
                            hr = wicBitmap->Lock(&rect, WICBitmapLockRead, &lock);
                            if (SUCCEEDED(hr) && lock) {
                                UINT stride = 0;
                                UINT dataSize = 0;
                                BYTE* data = nullptr;
                                hr = lock->GetStride(&stride);
                                if (SUCCEEDED(hr))
                                    hr = lock->GetDataPointer(&dataSize, &data);
                                if (SUCCEEDED(hr) && data && stride >= static_cast<UINT>(request->cacheWidth * 4)) {
                                    const size_t rowBytes = static_cast<size_t>(request->cacheWidth) * 4;
                                    result->pixels.resize(rowBytes * static_cast<size_t>(request->cacheHeight));
                                    for (int y = 0; y < request->cacheHeight; ++y) {
                                        std::memcpy(
                                            result->pixels.data() + rowBytes * static_cast<size_t>(y),
                                            data + static_cast<size_t>(stride) * static_cast<size_t>(y),
                                            rowBytes);
                                    }
                                    result->success = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (coInitialized)
            CoUninitialize();
        return result;
    }

    static std::unique_ptr<SceneTileBuildResult> RenderSceneTileOnWorker(std::unique_ptr<SceneTileBuildRequest> request)
    {
        auto result = std::make_unique<SceneTileBuildResult>();
        if (!request)
            return result;

        result->key = request->key;
        result->generation = request->generation;
        result->tileSize = request->tileSize;

        HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(coHr);

        ComPtr<ID2D1Factory> factory;
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr)) {
            ComPtr<IWICImagingFactory> wicFactory;
            hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory));
            if (SUCCEEDED(hr) && wicFactory) {
                ComPtr<IWICBitmap> wicBitmap;
                hr = wicFactory->CreateBitmap(
                    static_cast<UINT>(request->tileSize),
                    static_cast<UINT>(request->tileSize),
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapCacheOnLoad,
                    &wicBitmap);
                if (SUCCEEDED(hr) && wicBitmap) {
                    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                        D2D1_RENDER_TARGET_TYPE_DEFAULT,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                        96.0f,
                        96.0f);

                    ComPtr<ID2D1RenderTarget> rt;
                    hr = factory->CreateWicBitmapRenderTarget(wicBitmap.Get(), props, &rt);
                    if (SUCCEEDED(hr) && rt) {
                        ComPtr<ID2D1SolidColorBrush> fillBrush;
                        ComPtr<ID2D1SolidColorBrush> strokeBrush;
                        ComPtr<ID2D1SolidColorBrush> roadBrush;
                        ComPtr<ID2D1SolidColorBrush> roadCasingBrush;
                        rt->CreateSolidColorBrush(D2D1::ColorF(0.30f, 0.55f, 0.25f, 0.18f), &fillBrush);
                        rt->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.28f, 0.12f, 0.92f), &strokeBrush);
                        if (request->includeRoadDepictions) {
                            rt->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.97f, 0.84f, 0.88f), &roadBrush);
                            rt->CreateSolidColorBrush(D2D1::ColorF(0.11f, 0.19f, 0.24f, 0.70f), &roadCasingBrush);
                        }

                        rt->BeginDraw();
                        rt->SetTransform(D2D1::Matrix3x2F::Identity());
                        rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

                        for (const SceneCacheRingSnapshot& ring : request->rings) {
                            DrawWorkerBoundaryRingFull(
                                rt.Get(),
                                factory.Get(),
                                fillBrush.Get(),
                                strokeBrush.Get(),
                                ring.points,
                                request->boundaryView,
                                request->key.z);
                        }

                        if (request->includeRoadDepictions) {
                            DrawWorkerRoadDepictionLines(
                                rt.Get(),
                                roadBrush.Get(),
                                roadCasingBrush.Get(),
                                request->boundaryView,
                                request->key.z,
                                request->roadRoutes,
                                request->hiddenRoadDepictionIds);
                        }

                        hr = rt->EndDraw();
                        if (SUCCEEDED(hr)) {
                            WICRect rect{};
                            rect.X = 0;
                            rect.Y = 0;
                            rect.Width = request->tileSize;
                            rect.Height = request->tileSize;

                            ComPtr<IWICBitmapLock> lock;
                            hr = wicBitmap->Lock(&rect, WICBitmapLockRead, &lock);
                            if (SUCCEEDED(hr) && lock) {
                                UINT stride = 0;
                                UINT dataSize = 0;
                                BYTE* data = nullptr;
                                hr = lock->GetStride(&stride);
                                if (SUCCEEDED(hr))
                                    hr = lock->GetDataPointer(&dataSize, &data);
                                if (SUCCEEDED(hr) && data && stride >= static_cast<UINT>(request->tileSize * 4)) {
                                    const size_t rowBytes = static_cast<size_t>(request->tileSize) * 4;
                                    result->pixels.resize(rowBytes * static_cast<size_t>(request->tileSize));
                                    for (int y = 0; y < request->tileSize; ++y) {
                                        std::memcpy(
                                            result->pixels.data() + rowBytes * static_cast<size_t>(y),
                                            data + static_cast<size_t>(stride) * static_cast<size_t>(y),
                                            rowBytes);
                                    }
                                    result->success = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (coInitialized)
            CoUninitialize();
        return result;
    }

    std::unique_ptr<SceneCacheBuildRequest> BuildAsyncSceneCacheRequest(const ViewState& view)
    {
        auto request = std::make_unique<SceneCacheBuildRequest>();
        request->zoom = m_zoom;
        request->cacheWidth = MaxValue(1, view.width + kSceneCachePanMarginPixels * 2);
        request->cacheHeight = MaxValue(1, view.height + kSceneCachePanMarginPixels * 2);
        request->viewportWidth = view.width;
        request->viewportHeight = view.height;
        request->displayWorldMap = m_displayWorldMap;
        request->includeRoadDepictions = !m_displayWorldMap && m_showRoadDepictions;
        request->roadRoutes = m_roadDepictionRoutes;
        request->hiddenRoadDepictionIds = m_normalizedHiddenRoadDepictionIds;

        const ViewState cacheView = BuildViewStateForSize(request->cacheWidth, request->cacheHeight);
        request->centerWorld = cacheView.centerWorld;
        request->boundaryView = BuildViewStateForSize(request->cacheWidth, request->cacheHeight, kBoundaryDrawMarginPixels);

        if (m_displayWorldMap) {
            request->rings.reserve(128);
            for (BoundaryRing& ring : m_worldBoundaryRings) {
                if (!RingIntersectsView(ring, request->boundaryView))
                    continue;

                BoundaryLod* lod = WorldBoundaryLodForZoom(ring);
                BoundaryRenderSource source = (lod && lod->points.size() >= 3)
                    ? MakeBoundaryRenderSource(*lod)
                    : MakeBoundaryRenderSource(ring);
                if (!BoundarySourceIntersectsView(source, request->boundaryView))
                    continue;

                const std::vector<GeoPoint>& points = (lod && lod->points.size() >= 3) ? lod->points : ring.points;
                if (points.size() >= 3)
                    request->rings.push_back({ points });
            }
        }
        else {
            request->rings.reserve(m_ukBoundaryRings.size());
            for (const BoundaryRing& ring : m_ukBoundaryRings) {
                if (RingIntersectsView(ring, request->boundaryView) && ring.points.size() >= 3)
                    request->rings.push_back({ ring.points });
            }
        }

        return request;
    }

    std::vector<SceneTileDrawItem> BuildSceneTileDrawItems(const ViewState& view, int marginTiles) const
    {
        std::vector<SceneTileDrawItem> items;
        if (!HasSceneTileSources() || m_zoom < 0)
            return items;

        const double viewLeft = view.centerWorld.x - view.width * 0.5;
        const double viewRight = view.centerWorld.x + view.width * 0.5;
        const double viewTop = view.centerWorld.y - view.height * 0.5;
        const double viewBottom = view.centerWorld.y + view.height * 0.5;
        const int tileCount = SceneTileCountForZoom(m_zoom);
        const int xStart = static_cast<int>(std::floor(viewLeft / kSceneTileSize)) - marginTiles;
        const int xEnd = static_cast<int>(std::floor((viewRight - 0.001) / kSceneTileSize)) + marginTiles;
        const int yStart = ClampValue(static_cast<int>(std::floor(viewTop / kSceneTileSize)) - marginTiles, 0, tileCount - 1);
        const int yEnd = ClampValue(static_cast<int>(std::floor((viewBottom - 0.001) / kSceneTileSize)) + marginTiles, 0, tileCount - 1);

        const size_t reserveCount =
            static_cast<size_t>(MaxValue(0, xEnd - xStart + 1)) *
            static_cast<size_t>(MaxValue(0, yEnd - yStart + 1));
        items.reserve(reserveCount);

        for (int y = yStart; y <= yEnd; ++y) {
            for (int x = xStart; x <= xEnd; ++x) {
                SceneTileDrawItem item;
                item.drawTileX = x;
                item.drawTileY = y;
                item.key = SceneTileKey{ m_zoom, NormalizeSceneTileX(x, m_zoom), y, m_displayWorldMap };
                item.dest = D2D1::RectF(
                    static_cast<float>(static_cast<double>(x) * kSceneTileSize - viewLeft),
                    static_cast<float>(static_cast<double>(y) * kSceneTileSize - viewTop),
                    static_cast<float>((static_cast<double>(x) + 1.0) * kSceneTileSize - viewLeft),
                    static_cast<float>((static_cast<double>(y) + 1.0) * kSceneTileSize - viewTop));
                items.push_back(item);
            }
        }

        return items;
    }

    bool HasSceneTileSources() const
    {
        if (m_displayWorldMap)
            return !m_worldBoundaryRings.empty();
        return !m_ukBoundaryRings.empty();
    }

    bool ShouldUseSceneTileCache() const
    {
        if (!HasSceneTileSources())
            return false;

        return true;
    }

    bool IsSceneTileCacheReady(const ViewState& view)
    {
        if (!ShouldUseSceneTileCache())
            return false;

        const std::vector<SceneTileDrawItem> items = BuildSceneTileDrawItems(view, 0);
        if (items.empty())
            return false;

        const ULONGLONG now = GetTickCount64();
        for (const SceneTileDrawItem& item : items) {
            auto it = m_sceneTiles.find(item.key);
            if (it == m_sceneTiles.end() || !it->second.bitmap)
                return false;
            it->second.lastUsedMs = now;
        }

        return true;
    }

    bool DrawSceneTileCache(
        const ViewState& view,
        bool requireComplete = true,
        bool* allReadyOut = nullptr,
        bool* anyDrawnOut = nullptr)
    {
        if (allReadyOut)
            *allReadyOut = false;
        if (anyDrawnOut)
            *anyDrawnOut = false;

        if (!m_rt || !ShouldUseSceneTileCache())
            return false;

        const std::vector<SceneTileDrawItem> items = BuildSceneTileDrawItems(view, 0);
        if (items.empty())
            return false;

        const ULONGLONG now = GetTickCount64();
        bool allReady = true;
        bool anyDrawn = false;
        for (const SceneTileDrawItem& item : items) {
            auto it = m_sceneTiles.find(item.key);
            if (it == m_sceneTiles.end() || !it->second.bitmap) {
                allReady = false;
                if (requireComplete)
                    return false;
                continue;
            }

            it->second.lastUsedMs = now;
            m_rt->DrawBitmap(
                it->second.bitmap.Get(),
                item.dest,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            anyDrawn = true;
        }

        if (allReadyOut)
            *allReadyOut = allReady;
        if (anyDrawnOut)
            *anyDrawnOut = anyDrawn;

        if (requireComplete)
            return allReady;
        return anyDrawn;
    }

    std::unique_ptr<SceneTileBuildRequest> BuildSceneTileRequest(const SceneTileKey& key)
    {
        auto request = std::make_unique<SceneTileBuildRequest>();
        request->key = key;
        request->generation = m_sceneTileGeneration;
        request->tileSize = kSceneTileSize;
        request->includeRoadDepictions = !key.world && m_showRoadDepictions;
        request->roadRoutes = m_roadDepictionRoutes;
        request->hiddenRoadDepictionIds = m_normalizedHiddenRoadDepictionIds;

        const double tileLeft = static_cast<double>(key.x) * kSceneTileSize;
        const double tileTop = static_cast<double>(key.y) * kSceneTileSize;
        request->tileCenterWorld = {
            tileLeft + kSceneTileSize * 0.5,
            tileTop + kSceneTileSize * 0.5
        };
        request->boundaryView = BuildViewStateFromCenter(
            kSceneTileSize,
            kSceneTileSize,
            key.z,
            request->tileCenterWorld,
            64.0);

        request->rings.reserve(16);
        if (key.world) {
            for (BoundaryRing& ring : m_worldBoundaryRings) {
                if (!RingIntersectsView(ring, request->boundaryView))
                    continue;

                BoundaryLod* lod = WorldBoundaryLodForZoom(ring);
                BoundaryRenderSource source = (lod && lod->points.size() >= 3)
                    ? MakeBoundaryRenderSource(*lod)
                    : MakeBoundaryRenderSource(ring);
                if (!BoundarySourceIntersectsView(source, request->boundaryView))
                    continue;

                const std::vector<GeoPoint>& points = (lod && lod->points.size() >= 3) ? lod->points : ring.points;
                if (points.size() >= 3)
                    request->rings.push_back({ points });
            }
        }
        else {
            for (const BoundaryRing& ring : m_ukBoundaryRings) {
                if (RingIntersectsView(ring, request->boundaryView) && ring.points.size() >= 3)
                    request->rings.push_back({ ring.points });
            }
        }

        return request;
    }

    void QueueVisibleSceneTiles(const ViewState& view)
    {
        if (!ShouldUseSceneTileCache())
            return;

        std::vector<SceneTileDrawItem> items = BuildSceneTileDrawItems(view, kSceneTilePrefetchMarginTiles);
        if (items.empty())
            return;

        const D2D1_RECT_F viewport = D2D1::RectF(
            0.0f,
            0.0f,
            static_cast<float>(view.width),
            static_cast<float>(view.height));
        const float centerX = static_cast<float>(view.width) * 0.5f;
        const float centerY = static_cast<float>(view.height) * 0.5f;
        std::sort(items.begin(), items.end(), [viewport, centerX, centerY](const auto& a, const auto& b) {
            const float aArea = RectIntersectionArea(a.dest, viewport);
            const float bArea = RectIntersectionArea(b.dest, viewport);
            if (std::abs(aArea - bArea) > 0.5f)
                return aArea > bArea;

            return RectCenterDistanceSq(a.dest, centerX, centerY) <
                RectCenterDistanceSq(b.dest, centerX, centerY);
            });

        const ULONGLONG now = GetTickCount64();
        int requested = 0;
        std::unordered_set<SceneTileKey, SceneTileKeyHash> seen;
        for (const SceneTileDrawItem& item : items) {
            if (!seen.insert(item.key).second)
                continue;

            SceneTileEntry& entry = m_sceneTiles[item.key];
            entry.lastUsedMs = now;
            if (entry.bitmap || entry.loading)
                continue;
            if (entry.failed && now < entry.lastAttemptMs + 2500)
                continue;
            if (m_sceneTileBuildsInFlight >= kMaxSceneTileBuildsInFlight ||
                requested >= kMaxSceneTileRequestsPerPaint)
            {
                break;
            }

            auto request = BuildSceneTileRequest(item.key);
            entry.loading = true;
            entry.failed = false;
            entry.lastAttemptMs = now;
            ++m_sceneTileBuildsInFlight;
            ++requested;

            HWND hwnd = m_hwnd;
            ScheduleMapTask([hwnd, request = std::move(request)]() mutable {
                auto result = RenderSceneTileOnWorker(std::move(request));
                SceneTileBuildResult* rawResult = result.release();
                if (!PostMessageW(hwnd, WM_APP_SCENE_TILE_READY, 0, reinterpret_cast<LPARAM>(rawResult)))
                    delete rawResult;
                });
        }
    }

    void AdoptSceneTileResult(std::unique_ptr<SceneTileBuildResult> result)
    {
        if (!result || result->generation != m_sceneTileGeneration)
            return;

        if (m_sceneTileBuildsInFlight > 0)
            --m_sceneTileBuildsInFlight;

        auto it = m_sceneTiles.find(result->key);
        if (it == m_sceneTiles.end())
            it = m_sceneTiles.emplace(result->key, SceneTileEntry{}).first;

        it->second.loading = false;
        it->second.lastUsedMs = GetTickCount64();

        if (!result->success || result->pixels.empty()) {
            it->second.failed = true;
            it->second.lastAttemptMs = GetTickCount64();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        EnsureDeviceResources();
        if (!m_rt)
            return;

        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f);
        ComPtr<ID2D1Bitmap> bitmap;
        HRESULT hr = m_rt->CreateBitmap(
            D2D1::SizeU(static_cast<UINT32>(result->tileSize), static_cast<UINT32>(result->tileSize)),
            result->pixels.data(),
            static_cast<UINT32>(result->tileSize * 4),
            props,
            &bitmap);
        if (FAILED(hr) || !bitmap) {
            it->second.failed = true;
            it->second.lastAttemptMs = GetTickCount64();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        it->second.bitmap = bitmap;
        it->second.failed = false;
        PruneSceneTileCache();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void PruneSceneTileCache()
    {
        if (m_sceneTiles.size() <= kMaxSceneTileCacheEntries)
            return;

        std::vector<std::pair<SceneTileKey, ULONGLONG>> candidates;
        candidates.reserve(m_sceneTiles.size());
        for (const auto& [key, entry] : m_sceneTiles) {
            if (!entry.loading)
                candidates.push_back({ key, entry.lastUsedMs });
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
            });

        size_t toRemove = m_sceneTiles.size() - kMaxSceneTileCacheEntries;
        for (const auto& candidate : candidates) {
            if (toRemove == 0)
                break;
            m_sceneTiles.erase(candidate.first);
            --toRemove;
        }
    }

    void ClearSceneTileCache()
    {
        ++m_sceneTileGeneration;
        m_sceneTiles.clear();
        m_sceneTileBuildsInFlight = 0;
    }

    bool QueueSceneCacheRebuild(const ViewState& view)
    {
        if (m_sceneCacheBuildInFlight)
            return true;

        auto request = BuildAsyncSceneCacheRequest(view);
        if (!request)
            return false;

        m_sceneCacheBuildInFlight = true;
        HWND hwnd = m_hwnd;
        ScheduleMapTask([hwnd, request = std::move(request)]() mutable {
            auto result = RenderSceneCacheOnWorker(std::move(request));
            SceneCacheBuildResult* rawResult = result.release();
            if (!PostMessageW(hwnd, WM_APP_SCENE_CACHE_READY, 0, reinterpret_cast<LPARAM>(rawResult)))
                delete rawResult;
            });
        return true;
    }

    void AdoptAsyncSceneCacheResult(std::unique_ptr<SceneCacheBuildResult> result)
    {
        m_sceneCacheBuildInFlight = false;
        if (!result || !result->success || result->pixels.empty()) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        EnsureDeviceResources();
        if (!m_rt)
            return;

        const ViewState view = BuildViewState();
        if (result->viewportWidth != view.width ||
            result->viewportHeight != view.height ||
            result->zoom != m_zoom ||
            result->displayWorldMap != m_displayWorldMap ||
            !SceneCacheResultCoversView(*result, view))
        {
            m_sceneCacheDirty = true;
            QueueSceneCacheRebuild(view);
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f);
        ComPtr<ID2D1Bitmap> bitmap;
        HRESULT hr = m_rt->CreateBitmap(
            D2D1::SizeU(static_cast<UINT32>(result->cacheWidth), static_cast<UINT32>(result->cacheHeight)),
            result->pixels.data(),
            static_cast<UINT32>(result->cacheWidth * 4),
            props,
            &bitmap);
        if (FAILED(hr) || !bitmap) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        m_sceneBitmap = bitmap;
        m_sceneBitmapBack.Reset();
        m_sceneBitmapWidth = result->cacheWidth;
        m_sceneBitmapHeight = result->cacheHeight;
        m_sceneViewportWidth = result->viewportWidth;
        m_sceneViewportHeight = result->viewportHeight;
        m_sceneCacheDirty = false;
        m_sceneCacheRefreshPending = false;
        m_sceneCacheAllowDirtyUntilMs = 0;
        m_sceneCacheLastRebuildMs = GetTickCount64();
        KillTimer(m_hwnd, kSceneCacheRefreshTimer);
        m_sceneBitmapZoom = result->zoom;
        m_sceneBitmapCenterWorld = result->centerWorld;
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    bool RebuildSceneCache(const ViewState& view)
    {
        if (!m_rt)
            return false;

        const int cacheW = MaxValue(1, view.width + kSceneCachePanMarginPixels * 2);
        const int cacheH = MaxValue(1, view.height + kSceneCachePanMarginPixels * 2);

        ComPtr<ID2D1BitmapRenderTarget> cacheTarget;
        D2D1_SIZE_F desiredSize = D2D1::SizeF(static_cast<float>(cacheW), static_cast<float>(cacheH));
        D2D1_SIZE_U desiredPixels = D2D1::SizeU(static_cast<UINT32>(cacheW), static_cast<UINT32>(cacheH));
        D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED);
        HRESULT hr = m_rt->CreateCompatibleRenderTarget(
            &desiredSize,
            &desiredPixels,
            &pixelFormat,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
            &cacheTarget);
        if (FAILED(hr) || !cacheTarget)
            return false;

        ComPtr<ID2D1RenderTarget> previousTarget = m_rt;
        ComPtr<ID2D1RenderTarget> cacheRenderTarget;
        cacheTarget.As(&cacheRenderTarget);
        m_rt = cacheRenderTarget;

        const ViewState cacheView = BuildViewStateForSize(cacheW, cacheH);
        const ViewState cacheOverlayView = BuildViewStateForSize(cacheW, cacheH, 220.0);
        const ViewState cacheBoundaryView = BuildViewStateForSize(cacheW, cacheH, kBoundaryDrawMarginPixels);

        m_rt->BeginDraw();
        m_rt->SetTransform(D2D1::Matrix3x2F::Identity());
        m_rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        DrawMapGeographyBase(cacheOverlayView, cacheBoundaryView);
        hr = m_rt->EndDraw();

        ComPtr<ID2D1Bitmap> bitmap;
        if (SUCCEEDED(hr))
            hr = cacheTarget->GetBitmap(&bitmap);

        m_rt = previousTarget;

        if (FAILED(hr) || !bitmap) {
            InvalidateSceneCache();
            return false;
        }

        m_sceneBitmap = bitmap;
        m_sceneBitmapBack.Reset();
        m_sceneBitmapWidth = cacheW;
        m_sceneBitmapHeight = cacheH;
        m_sceneViewportWidth = view.width;
        m_sceneViewportHeight = view.height;
        m_sceneCacheDirty = false;
        m_sceneCacheRefreshPending = false;
        m_sceneCacheAllowDirtyUntilMs = 0;
        m_sceneCacheLastRebuildMs = GetTickCount64();
        KillTimer(m_hwnd, kSceneCacheRefreshTimer);
        m_sceneBitmapZoom = m_zoom;
        m_sceneBitmapCenterWorld = cacheView.centerWorld;
        return true;
    }

    bool IsSceneCacheCurrent(const ViewState& view, bool allowDirty = false, bool allowScaled = false) const
    {
        if (!m_rt || !m_sceneBitmap || m_sceneBitmapWidth <= 0 || m_sceneBitmapHeight <= 0)
            return false;
        if (m_sceneCacheDirty && !allowDirty)
            return false;

        D2D1_RECT_F dest{};
        double scale = 1.0;
        return GetSceneCachePlacement(view, allowScaled, dest, scale) && SceneCacheCoversView(view, dest);
    }

    bool IsOverlayUiDragActive() const
    {
        return m_notificationUiMouseDown ||
            m_draggingToolbarPanel ||
            m_draggingUsersPanel ||
            m_draggingPrivateChatPanel ||
            m_draggingNotificationHistoryScrollbar ||
            m_draggingNotificationHistoryContent;
    }

    bool DrawCachedScene(const ViewState& view, bool allowScaled, D2D1_RECT_F* destOut = nullptr, bool requireFullCoverage = true)
    {
        if (!m_rt || !m_sceneBitmap || m_sceneBitmapWidth <= 0 || m_sceneBitmapHeight <= 0)
            return false;

        D2D1_RECT_F dest{};
        double scale = 1.0;
        if (!GetSceneCachePlacement(view, allowScaled, dest, scale))
            return false;
        if (requireFullCoverage && !SceneCacheCoversView(view, dest))
            return false;

        const D2D1_RECT_F viewport = D2D1::RectF(
            0.0f,
            0.0f,
            static_cast<float>(view.width),
            static_cast<float>(view.height));
        const D2D1_RECT_F drawDest = D2D1::RectF(
            MaxValue(viewport.left, dest.left),
            MaxValue(viewport.top, dest.top),
            MinValue(viewport.right, dest.right),
            MinValue(viewport.bottom, dest.bottom));
        if (drawDest.right <= drawDest.left || drawDest.bottom <= drawDest.top)
            return false;

        const D2D1_RECT_F sourceRect = D2D1::RectF(
            ClampValue(static_cast<float>((drawDest.left - dest.left) / scale), 0.0f, static_cast<float>(m_sceneBitmapWidth)),
            ClampValue(static_cast<float>((drawDest.top - dest.top) / scale), 0.0f, static_cast<float>(m_sceneBitmapHeight)),
            ClampValue(static_cast<float>((drawDest.right - dest.left) / scale), 0.0f, static_cast<float>(m_sceneBitmapWidth)),
            ClampValue(static_cast<float>((drawDest.bottom - dest.top) / scale), 0.0f, static_cast<float>(m_sceneBitmapHeight)));
        if (sourceRect.right <= sourceRect.left || sourceRect.bottom <= sourceRect.top)
            return false;

        const D2D1_BITMAP_INTERPOLATION_MODE interpolation =
            (m_zoom == m_sceneBitmapZoom)
            ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
            : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        m_rt->DrawBitmap(
            m_sceneBitmap.Get(),
            drawDest,
            1.0f,
            interpolation,
            sourceRect);
        if (destOut)
            *destOut = dest;
        return true;
    }

    void DrawMapGeographyBase(const ViewState& overlayView, const ViewState& boundaryView)
    {
        if (!m_displayWorldMap) {
            DrawUkBoundary(boundaryView);
            DrawRoadDepictionLines(overlayView);
        }
        else
            DrawWorldBoundary(boundaryView);
    }

    void DrawMapStaticLabels(const ViewState& overlayView)
    {
        if (m_displayWorldMap)
            return;

        DrawRoadDepictionLabels(overlayView);
        DrawCityAnchors(overlayView);
    }

    void DrawWorldBoundaryPreview(const ViewState& view)
    {
        if (m_worldBoundaryRings.empty()) {
            DrawWorldBaseMap(view);
            return;
        }

        struct VisibleWorldPreviewRing
        {
            BoundaryRing* ring = nullptr;
            BoundaryLod* lod = nullptr;
        };

        std::vector<VisibleWorldPreviewRing> visibleRings;
        std::vector<BoundaryRenderSource> visibleSources;
        visibleRings.reserve(64);
        visibleSources.reserve(64);

        for (BoundaryRing& ring : m_worldBoundaryRings) {
            if (!RingIntersectsView(ring, view))
                continue;

            BoundaryLod* lod = WorldBoundaryLodForZoom(ring);
            BoundaryRenderSource source = (lod && lod->points.size() >= 3)
                ? MakeBoundaryRenderSource(*lod)
                : MakeBoundaryRenderSource(ring);
            if (!BoundarySourceIntersectsView(source, view))
                continue;

            visibleSources.push_back(source);
            visibleRings.push_back({ &ring, lod });
        }

        DrawHighZoomBoundaryFill(view, visibleSources);

        for (VisibleWorldPreviewRing& item : visibleRings) {
            if (item.lod && item.lod->points.size() >= 3)
                DrawBoundaryRingVisibleStroke(*item.lod, view);
            else if (item.ring)
                DrawBoundaryRingVisibleStroke(*item.ring, view);
        }
    }

    void DrawMapGeographyPreview(const ViewState& overlayView, const ViewState& boundaryView)
    {
        if (!m_displayWorldMap) {
            DrawUkBoundary(boundaryView);
            DrawRoadDepictionLines(overlayView);
            return;
        }

        DrawWorldBoundaryPreview(boundaryView);
    }

    void DrawSceneBase(
        const ViewState& tileView,
        const ViewState& overlayView,
        const ViewState& boundaryView,
        bool interactive = false,
        bool requestMissingTiles = true)
    {
        DrawTilesForView(tileView, interactive, nullptr, requestMissingTiles);
        DrawMapGeographyBase(overlayView, boundaryView);
    }

    void DrawDynamicSceneOverlays(const ViewState& overlayView)
    {
        DrawNotificationPolygons(overlayView);
        DrawEarthquakes(overlayView);
        DrawWeatherSystems(overlayView);
        DrawWeatherWarnings(overlayView);
        DrawFloods(overlayView);
        DrawNotes(overlayView);
        DrawMarkers(overlayView);
    }

    std::vector<D2D1_RECT_F> BuildExposedSceneStrips(const ViewState& view, const D2D1_RECT_F& cachedDest) const
    {
        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float left = ClampValue(cachedDest.left, 0.0f, width);
        const float top = ClampValue(cachedDest.top, 0.0f, height);
        const float right = ClampValue(cachedDest.right, 0.0f, width);
        const float bottom = ClampValue(cachedDest.bottom, 0.0f, height);

        std::vector<D2D1_RECT_F> strips;
        strips.reserve(4);

        auto addStrip = [&strips](const D2D1_RECT_F& strip) {
            if (strip.right > strip.left && strip.bottom > strip.top)
                strips.push_back(strip);
            };

        addStrip(D2D1::RectF(0.0f, 0.0f, left, height));
        addStrip(D2D1::RectF(right, 0.0f, width, height));
        addStrip(D2D1::RectF(left, 0.0f, right, top));
        addStrip(D2D1::RectF(left, bottom, right, height));

        return strips;
    }

    void DrawTilesInClip(const D2D1_RECT_F& clip)
    {
        if (!m_rt || clip.right <= clip.left || clip.bottom <= clip.top)
            return;

        m_rt->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        DrawTiles(true, &clip);
        m_rt->PopAxisAlignedClip();
    }

    void DrawExposedCachedSceneTiles(const std::vector<D2D1_RECT_F>& strips)
    {
        for (const D2D1_RECT_F& strip : strips)
            DrawTilesInClip(strip);
    }

    void DrawSceneOverlaysInClip(const D2D1_RECT_F& clip, const ViewState& overlayView, const ViewState& boundaryView)
    {
        if (!m_rt || clip.right <= clip.left || clip.bottom <= clip.top)
            return;

        const bool hadClip = m_hasOverlayClip;
        const D2D1_RECT_F previousClip = m_overlayClip;
        m_hasOverlayClip = true;
        m_overlayClip = clip;

        m_rt->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        DrawDynamicSceneOverlays(overlayView);
        m_rt->PopAxisAlignedClip();

        m_hasOverlayClip = hadClip;
        m_overlayClip = previousClip;
    }

    void DrawExposedCachedSceneEdges(const std::vector<D2D1_RECT_F>& strips, const ViewState& overlayView, const ViewState& boundaryView)
    {
        // The cached scene is only the previous viewport. Draw full overlays just
        // into newly exposed strips so panning reveals boundary/fill/markers ahead
        // of the cursor without paying to redraw the whole map each frame.
        for (const D2D1_RECT_F& strip : strips)
            DrawSceneOverlaysInClip(strip, overlayView, boundaryView);
    }

    void OnPaint()
    {
        PAINTSTRUCT ps{};
        BeginPaint(m_hwnd, &ps);

        EnsureDeviceResources();
        if (m_rt) {
            const ULONGLONG paintStartMs = GetTickCount64();
            const bool interactive = m_interactivePan;
            m_interactiveTileRequestsThisFrame = 0;
            const ViewState view = BuildViewState();
            const ViewState overlayView = BuildViewState(220.0);
            const ViewState boundaryView = BuildViewState(kBoundaryDrawMarginPixels);
            bool drewCachedScene = false;
            D2D1_RECT_F cachedSceneDest{};
            const bool allowStaleSceneForOverlayDrag = IsOverlayUiDragActive();
            const bool allowDeferredDirtyScene =
                m_sceneCacheDirty &&
                m_sceneCacheAllowDirtyUntilMs != 0 &&
                GetTickCount64() < m_sceneCacheAllowDirtyUntilMs;
            const bool allowDirtyScene = interactive || allowStaleSceneForOverlayDrag || allowDeferredDirtyScene;
            const bool allowScaledScene = false;
            const bool useViewportSceneCache = !m_displayWorldMap;
            const bool cacheCurrent =
                useViewportSceneCache &&
                IsSceneCacheCurrent(view, allowDirtyScene, allowScaledScene);
            const bool useSceneTileCache = ShouldUseSceneTileCache();
            if (useSceneTileCache)
                QueueVisibleSceneTiles(view);
            bool sceneTileCacheReady =
                useSceneTileCache &&
                !m_displayWorldMap &&
                IsSceneTileCacheReady(view);
            bool drawPreviewWhileCacheBuilds = false;

            if (useViewportSceneCache && !interactive && !cacheCurrent && !sceneTileCacheReady) {
                const ULONGLONG stageStartMs = GetTickCount64();
                QueueSceneCacheRebuild(view);
                TraceSlowPaintStage(L"QueueSceneCacheRebuild", GetTickCount64() - stageStartMs, interactive);
            }

            m_rt->BeginDraw();
            m_rt->Clear(D2D1::ColorF(kMapWaterR, kMapWaterG, kMapWaterB, 1.0f));
            m_tileBitmapDecodesThisFrame = 0;
            m_pendingTileBitmapDecode = false;
            ULONGLONG stageStartMs = GetTickCount64();
            DrawTiles(interactive);
            TraceSlowPaintStage(L"DrawTiles", GetTickCount64() - stageStartMs, interactive);

            stageStartMs = GetTickCount64();
            if (useSceneTileCache) {
                bool anySceneTileDrawn = false;
                const bool requireCompleteSceneTiles = !m_displayWorldMap;
                drewCachedScene = DrawSceneTileCache(
                    view,
                    requireCompleteSceneTiles,
                    &sceneTileCacheReady,
                    &anySceneTileDrawn);

                if (m_displayWorldMap && !drewCachedScene)
                    drewCachedScene = true;
            }
            else if (useViewportSceneCache && IsSceneCacheCurrent(view, allowDirtyScene, allowScaledScene)) {
                drewCachedScene = DrawCachedScene(view, allowScaledScene, &cachedSceneDest);
            }
            else if (useViewportSceneCache && m_sceneBitmap && m_zoom == m_sceneBitmapZoom) {
                drewCachedScene = DrawCachedScene(view, allowScaledScene, &cachedSceneDest, false);
            }
            TraceSlowPaintStage(L"DrawCachedScene", GetTickCount64() - stageStartMs, interactive);

            if (!drewCachedScene || drawPreviewWhileCacheBuilds) {
                stageStartMs = GetTickCount64();
                if (drawPreviewWhileCacheBuilds)
                    DrawMapGeographyPreview(overlayView, boundaryView);
                else
                    DrawMapGeographyBase(overlayView, boundaryView);
                TraceSlowPaintStage(L"DrawMapGeography", GetTickCount64() - stageStartMs, interactive);
            }
            DrawMapStaticLabels(overlayView);
            stageStartMs = GetTickCount64();
            DrawDynamicSceneOverlays(overlayView);
            TraceSlowPaintStage(L"DrawDynamicSceneOverlays", GetTickCount64() - stageStartMs, interactive);

            stageStartMs = GetTickCount64();
            DrawMapChrome();
            DrawAlertOverlay(overlayView);
            DrawNoteInterface(view);
            DrawNotificationInterface(view);
            DrawUsersPanel(view);
            DrawPrivateChat(view);
            DrawResponderChat(view);
            DrawOverlayContextMenus();
            UpdateFpsSample();
            DrawFpsCounter();
            TraceSlowPaintStage(L"DrawMapUi", GetTickCount64() - stageStartMs, interactive);

            stageStartMs = GetTickCount64();
            HRESULT hr = m_rt->EndDraw();
            TraceSlowPaintStage(L"EndDraw", GetTickCount64() - stageStartMs, interactive);
            if (hr == D2DERR_RECREATE_TARGET)
                DiscardDeviceResources();
            else if (m_pendingTileBitmapDecode && !interactive)
                InvalidateRect(m_hwnd, nullptr, FALSE);

            TraceSlowPaintStage(L"TotalPaint", GetTickCount64() - paintStartMs, interactive);
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

    void DrawWorldPolygon(const ViewState& view, const GeoPoint* pts, size_t count)
    {
        if (!m_rt || !g_d2dFactory || !pts || count < 3)
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        sink->BeginFigure(GeoToScreen(view, pts[0].lat, pts[0].lon), D2D1_FIGURE_BEGIN_FILLED);
        for (size_t i = 1; i < count; ++i)
            sink->AddLine(GeoToScreen(view, pts[i].lat, pts[i].lon));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        if (FAILED(sink->Close()))
            return;

        if (m_outlineFillBrush)
            m_rt->FillGeometry(geom.Get(), m_outlineFillBrush.Get());
        if (m_outlineStrokeBrush)
            m_rt->DrawGeometry(geom.Get(), m_outlineStrokeBrush.Get(), 1.5f);
    }

    void DrawWorldBaseMap(const ViewState& view)
    {
        static const GeoPoint northAmerica[] = {
            { 71.0, -168.0 }, { 72.0, -135.0 }, { 64.0, -90.0 }, { 58.0, -58.0 },
            { 47.0, -52.0 }, { 31.0, -82.0 }, { 15.0, -83.0 }, { 8.0, -79.0 },
            { 15.0, -103.0 }, { 32.0, -124.0 }, { 49.0, -126.0 }, { 58.0, -150.0 }
        };
        static const GeoPoint southAmerica[] = {
            { 13.0, -81.0 }, { 10.0, -61.0 }, { -5.0, -43.0 }, { -21.0, -39.0 },
            { -35.0, -53.0 }, { -55.0, -69.0 }, { -37.0, -73.0 }, { -18.0, -76.0 },
            { 2.0, -79.0 }
        };
        static const GeoPoint greenland[] = {
            { 83.0, -73.0 }, { 82.0, -18.0 }, { 72.0, -12.0 }, { 59.0, -43.0 },
            { 63.0, -58.0 }, { 73.0, -62.0 }
        };
        static const GeoPoint eurasia[] = {
            { 72.0, -10.0 }, { 70.0, 35.0 }, { 74.0, 95.0 }, { 67.0, 160.0 },
            { 52.0, 178.0 }, { 34.0, 140.0 }, { 22.0, 106.0 }, { 6.0, 95.0 },
            { 9.0, 77.0 }, { 29.0, 67.0 }, { 13.0, 45.0 }, { 30.0, 33.0 },
            { 37.0, 15.0 }, { 42.0, -8.0 }, { 56.0, -11.0 }
        };
        static const GeoPoint africa[] = {
            { 36.0, -17.0 }, { 32.0, 35.0 }, { 12.0, 50.0 }, { -11.0, 42.0 },
            { -35.0, 20.0 }, { -30.0, 16.0 }, { -7.0, 9.0 }, { 7.0, -14.0 }
        };
        static const GeoPoint australia[] = {
            { -11.0, 113.0 }, { -12.0, 153.0 }, { -28.0, 154.0 }, { -39.0, 145.0 },
            { -35.0, 115.0 }, { -22.0, 112.0 }
        };
        static const GeoPoint antarctica[] = {
            { -62.0, -180.0 }, { -66.0, -120.0 }, { -70.0, -45.0 }, { -68.0, 35.0 },
            { -66.0, 115.0 }, { -63.0, 180.0 }, { -82.0, 180.0 }, { -82.0, -180.0 }
        };

        DrawWorldPolygon(view, northAmerica, _countof(northAmerica));
        DrawWorldPolygon(view, southAmerica, _countof(southAmerica));
        DrawWorldPolygon(view, greenland, _countof(greenland));
        DrawWorldPolygon(view, eurasia, _countof(eurasia));
        DrawWorldPolygon(view, africa, _countof(africa));
        DrawWorldPolygon(view, australia, _countof(australia));
        DrawWorldPolygon(view, antarctica, _countof(antarctica));
    }

    static bool LongitudeRangesIntersect(const ViewState& view, double minLon, double maxLon)
    {
        if (view.allLongitudes)
            return true;

        minLon = NormalizeLongitude(minLon);
        maxLon = NormalizeLongitude(maxLon);

        if (view.wrapsLongitude)
            return maxLon >= view.minLon || minLon <= view.maxLon;

        return maxLon >= view.minLon && minLon <= view.maxLon;
    }

    static bool RingIntersectsView(const BoundaryRing& ring, const ViewState& view)
    {
        if (ring.points.size() < 3)
            return false;

        if (ring.maxLat < view.minLat || ring.minLat > view.maxLat)
            return false;

        return LongitudeRangesIntersect(view, ring.minLon, ring.maxLon);
    }

    template <typename BoundaryLike>
    static BoundaryRenderSource MakeBoundaryRenderSource(const BoundaryLike& ring)
    {
        return BoundaryRenderSource{
            &ring.segmentsByMinLat,
            ring.minLat,
            ring.maxLat,
            ring.minLon,
            ring.maxLon
        };
    }

    static bool BoundarySourceIntersectsView(const BoundaryRenderSource& source, const ViewState& view)
    {
        if (!source.segments || source.segments->empty())
            return false;

        if (source.maxLat < view.minLat || source.minLat > view.maxLat)
            return false;

        return LongitudeRangesIntersect(view, source.minLon, source.maxLon);
    }

    static bool IsGeoPointInBoundarySource(const BoundaryRenderSource& source, double lat, double lon)
    {
        if (!source.segments || source.segments->empty() ||
            lat < source.minLat || lat > source.maxLat ||
            lon < source.minLon || lon > source.maxLon)
        {
            return false;
        }

        bool inside = false;
        for (const BoundarySegment& segment : *source.segments) {
            if (segment.minLat > lat)
                break;
            if (segment.maxLat <= lat || segment.maxLon < lon)
                continue;

            const GeoPoint& a = segment.a;
            const GeoPoint& b = segment.b;
            if (((a.lat > lat) != (b.lat > lat)) &&
                (lon < (b.lon - a.lon) * (lat - a.lat) / (b.lat - a.lat) + a.lon))
            {
                inside = !inside;
            }
        }

        return inside;
    }

    void DrawHighZoomBoundaryFill(const ViewState& view, const std::vector<BoundaryRenderSource>& sources)
    {
        if (!m_rt || !m_outlineFillBrush || sources.empty())
            return;

        // At close zoom levels, filling the whole viewport when the centre is on
        // land paints nearby sea green. Instead, fill scanline spans inside the
        // visible boundary. Respect clipped edge renders too, so panning can draw
        // newly exposed strips without reprocessing or repainting the whole map.
        const float clipLeft = m_hasOverlayClip ? ClampValue(m_overlayClip.left, 0.0f, static_cast<float>(view.width)) : 0.0f;
        const float clipRight = m_hasOverlayClip ? ClampValue(m_overlayClip.right, 0.0f, static_cast<float>(view.width)) : static_cast<float>(view.width);
        const int yStart = m_hasOverlayClip ? ClampValue(static_cast<int>(std::floor(m_overlayClip.top)), 0, view.height) : 0;
        const int yEnd = m_hasOverlayClip ? ClampValue(static_cast<int>(std::ceil(m_overlayClip.bottom)), 0, view.height) : view.height;
        if (clipRight <= clipLeft || yEnd <= yStart)
            return;

        std::vector<float> intersections;
        intersections.reserve(16);
        const double centerLon = NormalizeLongitude(m_centerLon);

        for (int y = yStart; y < yEnd; ++y) {
            const double sampleY = static_cast<double>(y) + 0.5;
            const double worldY = view.centerWorld.y + (sampleY - view.height * 0.5);
            const GeoPoint sampleGeo = WorldToGeo(view.centerWorld.x, worldY, m_zoom);
            const double lat = sampleGeo.lat;

            intersections.clear();
            bool centreInsideRing = false;

            for (const BoundaryRenderSource& source : sources) {
                if (!source.segments || source.segments->empty() || lat < source.minLat || lat > source.maxLat)
                    continue;

                if (!centreInsideRing && IsGeoPointInBoundarySource(source, lat, centerLon))
                    centreInsideRing = true;

                for (const BoundarySegment& segment : *source.segments) {
                    if (segment.minLat > lat)
                        break;
                    if (segment.maxLat <= lat || std::abs(segment.b.lat - segment.a.lat) < 1e-12)
                        continue;

                    const double t = (lat - segment.a.lat) / (segment.b.lat - segment.a.lat);
                    const double lon = segment.a.lon + (segment.b.lon - segment.a.lon) * t;
                    intersections.push_back(GeoToScreen(view, lat, lon).x);
                }
            }

            if (intersections.empty()) {
                if (centreInsideRing) {
                    m_rt->FillRectangle(
                        D2D1::RectF(clipLeft, static_cast<float>(y), clipRight, static_cast<float>(y + 1)),
                        m_outlineFillBrush.Get());
                }
                continue;
            }

            std::sort(intersections.begin(), intersections.end());

            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                float left = MaxValue(intersections[i], clipLeft);
                float right = MinValue(intersections[i + 1], clipRight);
                if (right <= left)
                    continue;

                m_rt->FillRectangle(
                    D2D1::RectF(left, static_cast<float>(y), right, static_cast<float>(y + 1)),
                    m_outlineFillBrush.Get());
            }
        }
    }

    BoundaryLod* WorldBoundaryLodForZoom(BoundaryRing& ring)
    {
        if (ring.worldLods.size() < 4)
            return nullptr;
        if (m_zoom <= 3)
            return &ring.worldLods[3];
        if (m_zoom <= 5)
            return &ring.worldLods[2];
        if (m_zoom <= 9)
            return &ring.worldLods[1];
        return &ring.worldLods[0];
    }

    template <typename BoundaryLike>
    bool EnsureBoundaryFillGeometry(BoundaryLike& ring)
    {
        if (!g_d2dFactory || ring.points.size() < 3)
            return false;
        if (ring.fillGeometry)
            return true;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return false;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return false;

        sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

        constexpr double baseWorldSize = 256.0;
        constexpr double halfBaseWorld = baseWorldSize * 0.5;

        WorldPoint first = GeoToWorld(ring.points[0].lat, ring.points[0].lon, 0);
        double previousX = first.x;
        ring.geometryMinX = ring.geometryMaxX = first.x;
        ring.geometryMinY = ring.geometryMaxY = first.y;
        sink->BeginFigure(D2D1::Point2F(static_cast<float>(first.x), static_cast<float>(first.y)), D2D1_FIGURE_BEGIN_FILLED);

        const bool hasExplicitClose = ring.points.size() > 1 && SameGeoPoint(ring.points.front(), ring.points.back());
        const size_t pointCount = ring.points.size();

        for (size_t i = 1; i < pointCount; ++i) {
            WorldPoint p = GeoToWorld(ring.points[i].lat, ring.points[i].lon, 0);
            while (p.x - previousX > halfBaseWorld)
                p.x -= baseWorldSize;
            while (p.x - previousX < -halfBaseWorld)
                p.x += baseWorldSize;
            previousX = p.x;
            ring.geometryMinX = MinValue(ring.geometryMinX, p.x);
            ring.geometryMaxX = MaxValue(ring.geometryMaxX, p.x);
            ring.geometryMinY = MinValue(ring.geometryMinY, p.y);
            ring.geometryMaxY = MaxValue(ring.geometryMaxY, p.y);
            sink->AddLine(D2D1::Point2F(static_cast<float>(p.x), static_cast<float>(p.y)));
        }

        sink->EndFigure(hasExplicitClose ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);

        if (FAILED(sink->Close()))
            return false;

        ring.fillGeometry = geom;
        ring.fillGeometryZoom = 0;
        return true;
    }

    template <typename BoundaryLike>
    void DrawBoundaryRingFull(BoundaryLike& ring, const ViewState& view)
    {
        if (!m_rt || !EnsureBoundaryFillGeometry(ring))
            return;

        const double scale = static_cast<double>(1 << m_zoom);
        const double worldSize = 256.0 * scale;
        const double viewLeft = view.centerWorld.x - view.width * 0.5;
        const double viewRight = view.centerWorld.x + view.width * 0.5;
        const double viewTop = view.centerWorld.y - view.height * 0.5;
        const double viewBottom = view.centerWorld.y + view.height * 0.5;
        const double geomTop = ring.geometryMinY * scale;
        const double geomBottom = ring.geometryMaxY * scale;
        if (geomBottom < viewTop - 4.0 || geomTop > viewBottom + 4.0)
            return;

        D2D1_MATRIX_3X2_F oldTransform{};
        m_rt->GetTransform(&oldTransform);

        const int minCopy = static_cast<int>(std::floor((viewLeft - ring.geometryMaxX * scale) / worldSize)) - 1;
        const int maxCopy = static_cast<int>(std::ceil((viewRight - ring.geometryMinX * scale) / worldSize)) + 1;
        for (int copy = minCopy; copy <= maxCopy; ++copy) {
            const double shiftX = static_cast<double>(copy) * worldSize;
            const double geomLeft = ring.geometryMinX * scale + shiftX;
            const double geomRight = ring.geometryMaxX * scale + shiftX;
            if (geomRight < viewLeft - 4.0 || geomLeft > viewRight + 4.0)
                continue;

            const double dx = -view.centerWorld.x + view.width * 0.5 + shiftX;
            const double dy = -view.centerWorld.y + view.height * 0.5;
            const D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F(
                static_cast<float>(scale), 0.0f,
                0.0f, static_cast<float>(scale),
                static_cast<float>(dx), static_cast<float>(dy));
            m_rt->SetTransform(transform);

            if (m_outlineFillBrush)
                m_rt->FillGeometry(ring.fillGeometry.Get(), m_outlineFillBrush.Get());

            if (m_outlineStrokeBrush) {
                const float strokeWidth = static_cast<float>(2.0 / MaxValue(scale, 1.0));
                m_rt->DrawGeometry(ring.fillGeometry.Get(), m_outlineStrokeBrush.Get(), strokeWidth);
            }
        }

        m_rt->SetTransform(oldTransform);
    }

    template <typename BoundaryLike>
    void DrawBoundaryRingVisibleStroke(const BoundaryLike& ring, const ViewState& view)
    {
        if (!m_rt || !m_outlineStrokeBrush || ring.segmentsByMinLat.empty())
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        bool hasFigure = false;
        for (const BoundarySegment& segment : ring.segmentsByMinLat) {
            if (segment.minLat > view.maxLat)
                break;
            if (segment.maxLat < view.minLat)
                continue;

            if (!LongitudeRangesIntersect(view, segment.minLon, segment.maxLon))
                continue;

            const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
            const double halfWorld = worldSize * 0.5;
            WorldPoint worldA = GeoToWorld(segment.a.lat, segment.a.lon, m_zoom);
            WorldPoint worldB = GeoToWorld(segment.b.lat, segment.b.lon, m_zoom);
            while (worldA.x - view.centerWorld.x > halfWorld)
                worldA.x -= worldSize;
            while (worldA.x - view.centerWorld.x < -halfWorld)
                worldA.x += worldSize;
            while (worldB.x - worldA.x > halfWorld)
                worldB.x -= worldSize;
            while (worldB.x - worldA.x < -halfWorld)
                worldB.x += worldSize;

            const D2D1_POINT_2F a = D2D1::Point2F(
                static_cast<float>((worldA.x - view.centerWorld.x) + view.width * 0.5),
                static_cast<float>((worldA.y - view.centerWorld.y) + view.height * 0.5));
            const D2D1_POINT_2F b = D2D1::Point2F(
                static_cast<float>((worldB.x - view.centerWorld.x) + view.width * 0.5),
                static_cast<float>((worldB.y - view.centerWorld.y) + view.height * 0.5));
            if (m_hasOverlayClip) {
                D2D1_RECT_F segmentRect = D2D1::RectF(
                    MinValue(a.x, b.x) - 2.0f,
                    MinValue(a.y, b.y) - 2.0f,
                    MaxValue(a.x, b.x) + 2.0f,
                    MaxValue(a.y, b.y) + 2.0f);
                if (!RectsIntersect(segmentRect, m_overlayClip))
                    continue;
            }

            sink->BeginFigure(a, D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(b);
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            hasFigure = true;
        }

        HRESULT closeHr = sink->Close();
        if (FAILED(closeHr) || !hasFigure)
            return;

        m_rt->DrawGeometry(geom.Get(), m_outlineStrokeBrush.Get(), 2.0f);
    }

    void DrawUkBoundary(const ViewState& view)
    {
        if (m_ukBoundaryRings.empty())
            return;

        // Full detailed polygon fills are best for broad overview zooms, but become
        // expensive at the mid/detail zoom where coastline complexity is crowded.
        // From that point on, use the clipped scanline fill plus visible stroke
        // path; it preserves the same land/sea result while avoiding full-ring
        // geometry rebuilds during pan and repaint.
        const bool fullBoundary = !m_hasOverlayClip && !m_interactivePan && m_zoom <= kFullBoundaryMaxZoom;

        std::vector<BoundaryRenderSource> visibleSources;
        if (!fullBoundary)
            visibleSources.reserve(m_ukBoundaryRings.size());

        for (const BoundaryRing& ring : m_ukBoundaryRings) {
            if (RingIntersectsView(ring, view))
                visibleSources.push_back(MakeBoundaryRenderSource(ring));
        }

        if (!fullBoundary) {
            // At close zoom levels the fill renderer is already clipped to the
            // actual visible boundary spans. Run it whenever a high-zoom boundary
            // is visible rather than gating on the viewport centre; otherwise
            // panning over coastline can randomly drop all land tint as soon as
            // the centre point crosses water.
            DrawHighZoomBoundaryFill(view, visibleSources);
        }

        for (auto& ring : m_ukBoundaryRings) {
            if (!RingIntersectsView(ring, view))
                continue;

            if (fullBoundary)
                DrawBoundaryRingFull(ring, view);
            else
                DrawBoundaryRingVisibleStroke(ring, view);
        }
    }

    void DrawWorldBoundary(const ViewState& view)
    {
        if (m_worldBoundaryRings.empty()) {
            DrawWorldBaseMap(view);
            return;
        }

        struct VisibleWorldRing
        {
            BoundaryRing* ring = nullptr;
            BoundaryLod* lod = nullptr;
        };
        const bool broadWorldView = m_zoom <= kWorldCachedGeographyMaxZoom;
        const bool fillBoundary =
            !m_hasOverlayClip &&
            (broadWorldView || (!m_interactivePan && m_zoom <= kWorldGeometryFillMaxZoom));
        std::vector<VisibleWorldRing> visibleRings;
        std::vector<BoundaryRenderSource> visibleSources;
        visibleRings.reserve(64);
        visibleSources.reserve(64);

        for (BoundaryRing& ring : m_worldBoundaryRings) {
            if (!RingIntersectsView(ring, view))
                continue;

            BoundaryLod* lod = WorldBoundaryLodForZoom(ring);
            BoundaryRenderSource source = (lod && lod->points.size() >= 3)
                ? MakeBoundaryRenderSource(*lod)
                : MakeBoundaryRenderSource(ring);
            if (!BoundarySourceIntersectsView(source, view))
                continue;

            visibleRings.push_back({ &ring, lod });
            visibleSources.push_back(source);
        }

        if (!fillBoundary)
            DrawHighZoomBoundaryFill(view, visibleSources);

        for (VisibleWorldRing& item : visibleRings) {
            if (item.lod && item.lod->points.size() >= 3) {
                if (fillBoundary)
                    DrawBoundaryRingFull(*item.lod, view);
                else
                    DrawBoundaryRingVisibleStroke(*item.lod, view);
            }
            else if (item.ring) {
                if (fillBoundary)
                    DrawBoundaryRingFull(*item.ring, view);
                else
                    DrawBoundaryRingVisibleStroke(*item.ring, view);
            }
        }
    }

    HWND m_hwnd = nullptr;
    std::vector<TrafficAlert> m_alerts;
    std::vector<MapNote> m_notes;
    std::vector<ChatMessage> m_chatMessages;
    std::vector<PrivateMessage> m_privateMessages;
    std::vector<OnlineUser> m_onlineUsers;
    std::vector<GeoPolygon> m_notificationPolygons;
    std::vector<GeoPoint> m_draftPolygon;
    std::vector<EarthquakeEvent> m_earthquakes;
    std::vector<WeatherSystemEvent> m_weatherSystems;
    std::vector<WeatherWarningEvent> m_weatherWarnings;
    std::vector<FloodEvent> m_floods;
    std::wstring m_selectedId;
    SelectCallback m_onSelect;
    NoteCreateCallback m_onNoteCreate;
    NoteUpdateCallback m_onNoteUpdate;
    NoteDeleteCallback m_onNoteDelete;
    PolygonPointCallback m_onPolygonPoint;
    PolygonPointMoveCallback m_onPolygonPointMove;
    PolygonPointDeleteCallback m_onPolygonPointDelete;
    PolygonClearCallback m_onPolygonClear;
    RefreshCallback m_onRefresh;
    NotificationHistoryClearCallback m_onNotificationHistoryClear;
    NotificationHistoryActivateCallback m_onNotificationHistoryActivate;
    NotificationHistoryDeleteCallback m_onNotificationHistoryDelete;
    ChatSendCallback m_onChatSend;
    PrivateChatSendCallback m_onPrivateChatSend;
    ChatClearCallback m_onChatClear;
    ChatMessageActionCallback m_onChatMessageAction;
    UserActionCallback m_onUserAction;
    PanelCloseCallback m_onPanelClose;
    MapDisplayModeCallback m_onMapDisplayMode;

    int m_zoom = kDefaultZoom;
    double m_centerLat = kDefaultCenterLat;
    double m_centerLon = kDefaultCenterLon;

    POINT m_mouseDown{};
    POINT m_lastMouse{};
    POINT m_hoverPoint{ -10000, -10000 };
    bool m_dragging = false;
    bool m_interactivePan = false;
    bool m_notificationUiMouseDown = false;
    bool m_leftButtonDown = false;
    bool m_addNoteMode = false;
    bool m_polygonCaptureActive = false;
    bool m_draggingPolygonPoint = false;
    size_t m_activeNotificationPolygonIndex = static_cast<size_t>(-1);
    size_t m_draggingPolygonIndex = static_cast<size_t>(-1);
    size_t m_draggingPolygonPointIndex = static_cast<size_t>(-1);
    bool m_trackingMouseLeave = false;
    int m_interactiveTileRequestsThisFrame = 0;
    std::wstring m_hoveredAlertId;
    std::wstring m_hoveredEarthquakeId;
    std::wstring m_hoveredWeatherSystemId;
    std::wstring m_hoveredWeatherWarningId;
    std::wstring m_hoveredFloodId;
    NoteEditorMode m_noteEditorMode = NoteEditorMode::None;
    size_t m_noteEditorIndex = static_cast<size_t>(-1);
    std::wstring m_noteEditorText;
    size_t m_noteEditorCursor = 0;
    double m_noteEditorLat = 0.0;
    double m_noteEditorLon = 0.0;
    OverlayInputFocus m_overlayInputFocus = OverlayInputFocus::None;
    AppNotification m_activeNotification;
    std::vector<AppNotification> m_notificationHistory;
    bool m_hasActiveNotification = false;
    bool m_activeNotificationClosing = false;
    float m_activeNotificationProgress = 0.0f;
    float m_activeNotificationAnimationStart = 0.0f;
    float m_activeNotificationAnimationTarget = 0.0f;
    ULONGLONG m_activeNotificationAnimationStartMs = 0;
    bool m_activeNotificationAnimating = false;
    bool m_showNotificationHistory = false;
    bool m_notificationHistoryCollapsed = false;
    float m_notificationHistoryOpenProgress = 0.0f;
    float m_notificationHistoryAnimationStart = 0.0f;
    float m_notificationHistoryAnimationTarget = 0.0f;
    ULONGLONG m_notificationHistoryAnimationStartMs = 0;
    bool m_notificationHistoryAnimating = false;
    bool m_showEarthquakeOverlayLabels = false;
    bool m_showIncidentOverlayLabels = false;
    bool m_showWeatherSystemOverlayLabels = false;
    bool m_showWeatherWarningOverlayLabels = false;
    bool m_showWeatherWarningPolygons = true;
    bool m_showFloodOverlayLabels = false;
    bool m_showAreaLabels = true;
    bool m_showRoadDepictions = false;
    std::shared_ptr<const std::vector<RoadDepictionRoute>> m_roadDepictionRoutes =
        std::make_shared<std::vector<RoadDepictionRoute>>(BuiltInRoadDepictionRoutes());
    std::unordered_set<std::wstring> m_hiddenRoadDepictionIds;
    std::unordered_set<std::wstring> m_normalizedHiddenRoadDepictionIds;
    bool m_displayWorldMap = false;
    bool m_showFpsCounter = false;
    bool m_showToolbarPanel = true;
    bool m_notificationContextMenuVisible = false;
    size_t m_notificationContextMenuIndex = static_cast<size_t>(-1);
    D2D1_RECT_F m_notificationContextMenuRect{};
    bool m_chatContextMenuVisible = false;
    size_t m_chatContextMenuIndex = static_cast<size_t>(-1);
    D2D1_RECT_F m_chatContextMenuRect{};
    bool m_userContextMenuVisible = false;
    size_t m_userContextMenuIndex = static_cast<size_t>(-1);
    D2D1_RECT_F m_userContextMenuRect{};
    bool m_draggingNotificationHistoryScrollbar = false;
    bool m_draggingNotificationHistoryContent = false;
    bool m_responderChatCollapsed = false;
    bool m_responderChatInputFocused = false;
    bool m_canClearResponderChat = false;
    float m_responderChatOpenProgress = 1.0f;
    float m_responderChatAnimationStart = 1.0f;
    float m_responderChatAnimationTarget = 1.0f;
    ULONGLONG m_responderChatAnimationStartMs = 0;
    bool m_responderChatAnimating = false;
    std::wstring m_responderChatDraft;
    bool m_showUsersPanel = false;
    bool m_usersPanelCollapsed = false;
    float m_usersPanelOpenProgress = 0.0f;
    float m_usersPanelAnimationStart = 0.0f;
    float m_usersPanelAnimationTarget = 0.0f;
    ULONGLONG m_usersPanelAnimationStartMs = 0;
    bool m_usersPanelAnimating = false;
    bool m_draggingUsersPanel = false;
    float m_usersPanelOffsetX = 0.0f;
    float m_usersPanelOffsetY = 0.0f;
    bool m_privateChatVisible = false;
    bool m_privateChatInputFocused = false;
    bool m_draggingPrivateChatPanel = false;
    OnlineUser m_privateChatUser;
    std::wstring m_privateChatDraft;
    float m_privateChatOffsetX = 0.0f;
    float m_privateChatOffsetY = 0.0f;
    bool m_draggingToolbarPanel = false;
    float m_toolbarPanelOffsetX = 0.0f;
    float m_toolbarPanelOffsetY = 0.0f;
    float m_notificationHistoryScroll = 0.0f;
    float m_notificationHistoryScrollbarDragOffset = 0.0f;
    D2D1_RECT_F m_lastActiveNotificationRect{};
    D2D1_RECT_F m_lastNotificationHistoryRect{};
    bool m_hasLastActiveNotificationRect = false;
    bool m_hasLastNotificationHistoryRect = false;
    MapOverlayUiRenderer m_overlayUi;
    ULONGLONG m_fpsLastSampleMs = 0;
    int m_fpsFrameCount = 0;
    double m_fpsValue = 0.0;
    int m_tileBitmapDecodesThisFrame = 0;
    bool m_pendingTileBitmapDecode = false;

    ComPtr<ID2D1HwndRenderTarget> m_hwndRt;
    ComPtr<ID2D1RenderTarget> m_rt;
    ComPtr<ID2D1SolidColorBrush> m_severeBrush;
    ComPtr<ID2D1SolidColorBrush> m_moderateBrush;
    ComPtr<ID2D1SolidColorBrush> m_minorBrush;
    ComPtr<ID2D1SolidColorBrush> m_unknownBrush;
    ComPtr<ID2D1SolidColorBrush> m_selectedBrush;
    ComPtr<ID2D1SolidColorBrush> m_placeholderBrush;
    ComPtr<ID2D1SolidColorBrush> m_borderBrush;
    ComPtr<ID2D1SolidColorBrush> m_outlineFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_outlineStrokeBrush;
    ComPtr<ID2D1SolidColorBrush> m_panelBrush;
    ComPtr<ID2D1SolidColorBrush> m_textBrush;
    ComPtr<ID2D1SolidColorBrush> m_noteBrush;
    ComPtr<ID2D1SolidColorBrush> m_laneTileBrush;
    ComPtr<ID2D1SolidColorBrush> m_polygonFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_polygonStrokeBrush;
    ComPtr<ID2D1SolidColorBrush> m_draftFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_draftStrokeBrush;
    ComPtr<ID2D1SolidColorBrush> m_earthquakeBrush;
    ComPtr<ID2D1SolidColorBrush> m_weatherSystemBrush;
    ComPtr<ID2D1SolidColorBrush> m_forecastRingBrush;
    ComPtr<ID2D1SolidColorBrush> m_weatherWarningBrush;
    ComPtr<ID2D1SolidColorBrush> m_weatherWarningFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_floodBrush;
    ComPtr<ID2D1SolidColorBrush> m_roadBrush;
    ComPtr<ID2D1SolidColorBrush> m_roadCasingBrush;
    ComPtr<ID2D1StrokeStyle> m_forecastErrorStrokeStyle;
    ComPtr<IDWriteTextFormat> m_noteTextFormat;
    ComPtr<ID2D1Bitmap> m_sceneBitmap;
    ComPtr<ID2D1Bitmap> m_sceneBitmapBack;
    bool m_sceneCacheDirty = false;
    bool m_sceneCacheRefreshPending = false;
    bool m_sceneCacheBuildInFlight = false;
    ULONGLONG m_sceneCacheAllowDirtyUntilMs = 0;
    ULONGLONG m_sceneCacheLastRebuildMs = 0;
    int m_sceneBitmapWidth = 0;
    int m_sceneBitmapHeight = 0;
    int m_sceneViewportWidth = 0;
    int m_sceneViewportHeight = 0;
    int m_sceneBitmapZoom = kDefaultZoom;
    WorldPoint m_sceneBitmapCenterWorld{};
    bool m_hasOverlayClip = false;
    D2D1_RECT_F m_overlayClip{};
    std::vector<BoundaryRing> m_ukBoundaryRings;
    std::vector<BoundaryRing> m_worldBoundaryRings;
    std::unordered_map<SceneTileKey, SceneTileEntry, SceneTileKeyHash> m_sceneTiles;
    uint64_t m_sceneTileGeneration = 1;
    int m_sceneTileBuildsInFlight = 0;

    std::mutex m_tileMutex;
    std::unordered_map<TileKey, std::shared_ptr<TileEntry>, TileKeyHash> m_tiles;
    std::unordered_map<std::wstring, ComPtr<ID2D1Bitmap>> m_laneBitmaps;
};

MapView::MapView() : m_impl(std::make_unique<Impl>())
{
}

MapView::~MapView() = default;

MapView::MapView(MapView&&) noexcept = default;
MapView& MapView::operator=(MapView&&) noexcept = default;

bool MapView::Create(HWND parent, int x, int y, int w, int h)
{
    return m_impl->Create(parent, x, y, w, h);
}

HWND MapView::Hwnd() const
{
    return m_impl->Hwnd();
}

void MapView::SetSelectCallback(SelectCallback cb)
{
    m_impl->SetSelectCallback(std::move(cb));
}

void MapView::SetNoteCreateCallback(NoteCreateCallback cb)
{
    m_impl->SetNoteCreateCallback(std::move(cb));
}

void MapView::SetNoteUpdateCallback(NoteUpdateCallback cb)
{
    m_impl->SetNoteUpdateCallback(std::move(cb));
}

void MapView::SetNoteDeleteCallback(NoteDeleteCallback cb)
{
    m_impl->SetNoteDeleteCallback(std::move(cb));
}

void MapView::SetPolygonPointCallback(PolygonPointCallback cb)
{
    m_impl->SetPolygonPointCallback(std::move(cb));
}

void MapView::SetPolygonPointMoveCallback(PolygonPointMoveCallback cb)
{
    m_impl->SetPolygonPointMoveCallback(std::move(cb));
}

void MapView::SetPolygonPointDeleteCallback(PolygonPointDeleteCallback cb)
{
    m_impl->SetPolygonPointDeleteCallback(std::move(cb));
}

void MapView::SetPolygonClearCallback(PolygonClearCallback cb)
{
    m_impl->SetPolygonClearCallback(std::move(cb));
}

void MapView::SetRefreshCallback(RefreshCallback cb)
{
    m_impl->SetRefreshCallback(std::move(cb));
}

void MapView::SetNotificationHistoryClearCallback(NotificationHistoryClearCallback cb)
{
    m_impl->SetNotificationHistoryClearCallback(std::move(cb));
}

void MapView::SetNotificationHistoryActivateCallback(NotificationHistoryActivateCallback cb)
{
    m_impl->SetNotificationHistoryActivateCallback(std::move(cb));
}

void MapView::SetNotificationHistoryDeleteCallback(NotificationHistoryDeleteCallback cb)
{
    m_impl->SetNotificationHistoryDeleteCallback(std::move(cb));
}

void MapView::SetChatSendCallback(ChatSendCallback cb)
{
    m_impl->SetChatSendCallback(std::move(cb));
}

void MapView::SetPrivateChatSendCallback(PrivateChatSendCallback cb)
{
    m_impl->SetPrivateChatSendCallback(std::move(cb));
}

void MapView::SetChatClearCallback(ChatClearCallback cb)
{
    m_impl->SetChatClearCallback(std::move(cb));
}

void MapView::SetChatMessageActionCallback(ChatMessageActionCallback cb)
{
    m_impl->SetChatMessageActionCallback(std::move(cb));
}

void MapView::SetUserActionCallback(UserActionCallback cb)
{
    m_impl->SetUserActionCallback(std::move(cb));
}

void MapView::SetPanelCloseCallback(PanelCloseCallback cb)
{
    m_impl->SetPanelCloseCallback(std::move(cb));
}

void MapView::SetMapDisplayModeCallback(MapDisplayModeCallback cb)
{
    m_impl->SetMapDisplayModeCallback(std::move(cb));
}

void MapView::SetChatClearEnabled(bool enabled)
{
    m_impl->SetChatClearEnabled(enabled);
}

void MapView::SetAlerts(const std::vector<TrafficAlert>& alerts)
{
    m_impl->SetAlerts(alerts);
}

void MapView::SetIncidentOverlayVisible(bool visible)
{
    m_impl->SetIncidentOverlayVisible(visible);
}

void MapView::SetNotes(const std::vector<MapNote>& notes)
{
    m_impl->SetNotes(notes);
}

void MapView::SetChatMessages(const std::vector<ChatMessage>& messages)
{
    m_impl->SetChatMessages(messages);
}

void MapView::SetPrivateMessages(const std::vector<PrivateMessage>& messages)
{
    m_impl->SetPrivateMessages(messages);
}

void MapView::SetOnlineUsers(const std::vector<OnlineUser>& users)
{
    m_impl->SetOnlineUsers(users);
}

void MapView::OpenPrivateChat(const OnlineUser& user)
{
    m_impl->OpenPrivateChat(user);
}

void MapView::SetUsersVisible(bool visible)
{
    m_impl->SetUsersVisible(visible);
}

void MapView::SetNotificationPolygons(const std::vector<GeoPolygon>& polygons)
{
    m_impl->SetNotificationPolygons(polygons);
}

void MapView::SetActiveNotificationPolygonIndex(size_t index)
{
    m_impl->SetActiveNotificationPolygonIndex(index);
}

void MapView::SetDraftPolygon(const std::vector<GeoPoint>& points)
{
    m_impl->SetDraftPolygon(points);
}

void MapView::SetPolygonCaptureActive(bool active)
{
    m_impl->SetPolygonCaptureActive(active);
}

void MapView::SetEarthquakes(const std::vector<EarthquakeEvent>& earthquakes)
{
    m_impl->SetEarthquakes(earthquakes);
}

void MapView::SetEarthquakeOverlayVisible(bool visible)
{
    m_impl->SetEarthquakeOverlayVisible(visible);
}

void MapView::SetWeatherSystems(const std::vector<WeatherSystemEvent>& systems)
{
    m_impl->SetWeatherSystems(systems);
}

void MapView::SetWeatherSystemOverlayVisible(bool visible)
{
    m_impl->SetWeatherSystemOverlayVisible(visible);
}

void MapView::SetWeatherWarnings(const std::vector<WeatherWarningEvent>& warnings)
{
    m_impl->SetWeatherWarnings(warnings);
}

void MapView::SetWeatherWarningOverlayVisible(bool visible)
{
    m_impl->SetWeatherWarningOverlayVisible(visible);
}

void MapView::SetWeatherWarningPolygonsVisible(bool visible)
{
    m_impl->SetWeatherWarningPolygonsVisible(visible);
}

void MapView::SetFloods(const std::vector<FloodEvent>& floods)
{
    m_impl->SetFloods(floods);
}

void MapView::SetFloodOverlayVisible(bool visible)
{
    m_impl->SetFloodOverlayVisible(visible);
}

void MapView::SetAreaLabelsVisible(bool visible)
{
    m_impl->SetAreaLabelsVisible(visible);
}

void MapView::SetRoadDepictionsVisible(bool visible)
{
    m_impl->SetRoadDepictionsVisible(visible);
}

bool MapView::LoadRoadDepictionsFromFile(
    const std::filesystem::path& path,
    std::wstring* errorOut,
    const std::unordered_set<std::wstring>* allowedLabels)
{
    return m_impl->LoadRoadDepictionsFromFile(path, errorOut, allowedLabels);
}

std::vector<std::wstring> MapView::RoadDepictionLabels() const
{
    return m_impl->RoadDepictionLabels();
}

void MapView::SetHiddenRoadDepictions(const std::unordered_set<std::wstring>& hiddenRoadLabels)
{
    m_impl->SetHiddenRoadDepictions(hiddenRoadLabels);
}

void MapView::SetDisplayWorldMap(bool visible)
{
    m_impl->SetDisplayWorldMap(visible);
}

void MapView::SetFpsCounterVisible(bool visible)
{
    m_impl->SetFpsCounterVisible(visible);
}

void MapView::SetToolbarVisible(bool visible)
{
    m_impl->SetToolbarVisible(visible);
}

void MapView::SetActiveNotification(const AppNotification& notification)
{
    m_impl->SetActiveNotification(notification);
}

void MapView::ClearActiveNotification()
{
    m_impl->ClearActiveNotification();
}

void MapView::SetNotificationHistory(const std::vector<AppNotification>& notifications)
{
    m_impl->SetNotificationHistory(notifications);
}

void MapView::SetNotificationHistoryVisible(bool visible)
{
    m_impl->SetNotificationHistoryVisible(visible);
}

void MapView::SetSelectedId(const std::wstring& id)
{
    m_impl->SetSelectedId(id);
}

void MapView::CenterOnAlert(const std::wstring& id)
{
    m_impl->CenterOnAlert(id);
}

void MapView::FitToPoints(const std::vector<GeoPoint>& points, int singlePointZoom)
{
    m_impl->FitToPoints(points, singlePointZoom);
}

void MapView::FitToAlerts()
{
    m_impl->FitToAlerts();
}

bool MapView::LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut)
{
    return m_impl->LoadUkBoundaryFromFile(path, errorOut);
}

bool MapView::LoadWorldBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut)
{
    return m_impl->LoadWorldBoundaryFromFile(path, errorOut);
}
