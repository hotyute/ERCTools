// =================================================================================
// FILE: map_view.cpp
// =================================================================================

#include "map_view.h"
#include "app_state.h"
#include "http.h"
#include "map_overlay_ui.h"
#include "parsing.h"
#include "util.h"

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

struct BoundaryRing
{
    std::vector<GeoPoint> points;
    std::vector<BoundarySegment> segmentsByMinLat;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
};

// Tile/cache tuning.
constexpr int kMaxConcurrentTileDownloads = 6;
constexpr size_t kMaxTileCacheEntries = 768;
constexpr int kMaxFallbackTileZoomDelta = 5;
constexpr int kMaxInteractiveTileRequestsPerFrame = 2;

// Interaction timing.
constexpr UINT_PTR kInteractionIdleTimer = 1;
constexpr UINT kInteractionIdleMs = 120;

// Map styling.
constexpr float kMapWaterR = 0.80f;
constexpr float kMapWaterG = 0.91f;
constexpr float kMapWaterB = 0.98f;
constexpr float kOverlayUiMargin = 12.0f;
constexpr float kOverlayUiPadding = 14.0f;
constexpr float kOverlayUiGap = 10.0f;
constexpr float kNotificationScrollStep = 56.0f;
constexpr float kNoteBubbleWidth = 204.0f;
constexpr float kNoteBubbleHeight = 64.0f;
constexpr float kNoteEditorWidth = 300.0f;
constexpr float kNoteEditorMinHeight = 144.0f;

// Boundary rendering.
constexpr double kBoundaryDrawMarginPixels = 512.0;
constexpr int kFullBoundaryMaxZoom = 7;

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

    void SetAlerts(const std::vector<TrafficAlert>& alerts)
    {
        m_alerts = alerts;
        InvalidateSceneCache();
        Invalidate();
    }

    void SetNotes(const std::vector<MapNote>& notes)
    {
        if (NotesEqual(m_notes, notes))
            return;

        m_notes = notes;
        if (m_noteEditorMode == NoteEditorMode::Edit && m_noteEditorIndex >= m_notes.size())
            CancelNoteEditor();
        InvalidateSceneCache();
        Invalidate();
    }

    void SetNotificationPolygons(const std::vector<GeoPolygon>& polygons)
    {
        m_notificationPolygons = polygons;
        InvalidateSceneCache();
        Invalidate();
    }

    void SetActiveNotificationPolygonIndex(size_t index)
    {
        m_activeNotificationPolygonIndex = index;
        InvalidateSceneCache();
        Invalidate();
    }

    void SetDraftPolygon(const std::vector<GeoPoint>& points)
    {
        m_draftPolygon = points;
        InvalidateSceneCache();
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
        InvalidateSceneCache();
        Invalidate();
    }

    void SetActiveNotification(const AppNotification& notification)
    {
        m_activeNotification = notification;
        m_hasActiveNotification = !notification.title.empty() || !notification.body.empty();
        Invalidate();
    }

    void ClearActiveNotification()
    {
        if (!m_hasActiveNotification)
            return;

        m_hasActiveNotification = false;
        m_activeNotification = {};
        Invalidate();
    }

    void SetNotificationHistory(const std::vector<AppNotification>& notifications)
    {
        m_notificationHistory = notifications;
        m_notificationHistoryScroll = ClampNotificationHistoryScroll(m_notificationHistoryScroll);
        Invalidate();
    }

    void SetNotificationHistoryVisible(bool visible)
    {
        if (m_showNotificationHistory == visible)
            return;

        m_showNotificationHistory = visible;
        m_notificationHistoryScroll = ClampNotificationHistoryScroll(m_notificationHistoryScroll);
        Invalidate();
    }

    void SetSelectedId(const std::wstring& id)
    {
        m_selectedId = id;
        InvalidateSceneCache();
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

        m_ukBoundaryRings.clear();
        m_ukBoundaryRings.reserve(rings.size());
        for (auto& ring : rings) {
            if (ring.empty())
                continue;

            BoundaryRing cached;
            cached.minLat = cached.maxLat = ring[0].lat;
            cached.minLon = cached.maxLon = ring[0].lon;
            for (const GeoPoint& pt : ring) {
                cached.minLat = MinValue(cached.minLat, pt.lat);
                cached.maxLat = MaxValue(cached.maxLat, pt.lat);
                cached.minLon = MinValue(cached.minLon, pt.lon);
                cached.maxLon = MaxValue(cached.maxLon, pt.lon);
            }
            cached.segmentsByMinLat.reserve(ring.size());
            for (size_t i = 0; i < ring.size(); ++i) {
                BoundarySegment segment;
                segment.a = ring[i];
                segment.b = ring[(i + 1) % ring.size()];
                segment.minLat = MinValue(segment.a.lat, segment.b.lat);
                segment.maxLat = MaxValue(segment.a.lat, segment.b.lat);
                segment.minLon = MinValue(segment.a.lon, segment.b.lon);
                segment.maxLon = MaxValue(segment.a.lon, segment.b.lon);
                cached.segmentsByMinLat.push_back(segment);
            }
            std::sort(cached.segmentsByMinLat.begin(), cached.segmentsByMinLat.end(), [](const auto& a, const auto& b) {
                return a.minLat < b.minLat;
                });

            cached.points = std::move(ring);
            m_ukBoundaryRings.push_back(std::move(cached));
        }

        InvalidateSceneCache();
        Invalidate();
        return true;
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
            if (m_rt) {
                UINT w = static_cast<UINT>(std::max<LONG>(1L, LOWORD(lParam)));
                UINT h = static_cast<UINT>(std::max<LONG>(1L, HIWORD(lParam)));
                m_rt->Resize(D2D1::SizeU(w, h));
                InvalidateSceneCache();
            }
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
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_LBUTTONDOWN:
            SetFocus(m_hwnd);
            if (HandleNotificationPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HitNotificationInterface(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
                SetCapture(m_hwnd);
                m_notificationUiMouseDown = true;
                return 0;
            }
            EnsureDeviceResources();
            if (m_rt && HandleNotePointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
            if (HandlePolygonPointPointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
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
            if (!m_hoveredAlertId.empty()) {
                m_hoveredAlertId.clear();
                Invalidate();
            }
            return 0;

        case WM_LBUTTONUP:
            OnLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_RBUTTONDOWN:
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
            if (HandleNoteEditorKeyDown(wParam))
                return 0;
            break;

        case WM_CHAR:
            if (HandleNoteEditorChar(wParam))
                return 0;
            break;

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

        ViewState view;
        view.width = std::max(1, static_cast<int>(rc.right - rc.left));
        view.height = std::max(1, static_cast<int>(rc.bottom - rc.top));
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

    D2D1_POINT_2F GeoToScreen(const ViewState& view, double lat, double lon) const
    {
        WorldPoint p = GeoToWorld(lat, lon, m_zoom);

        float x = static_cast<float>((p.x - view.centerWorld.x) + view.width * 0.5);
        float y = static_cast<float>((p.y - view.centerWorld.y) + view.height * 0.5);

        return D2D1::Point2F(x, y);
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

    static bool RectsIntersect(const D2D1_RECT_F& a, const D2D1_RECT_F& b)
    {
        return a.left < b.right && a.right > b.left &&
            a.top < b.bottom && a.bottom > b.top;
    }

    static int PositiveModulo(int value, int modulus)
    {
        int result = value % modulus;
        return result < 0 ? result + modulus : result;
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

    void InvalidateSceneCache()
    {
        m_sceneBitmap.Reset();
        m_sceneBitmapWidth = 0;
        m_sceneBitmapHeight = 0;
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

    D2D1_RECT_F BuildNoteBubbleRect(const ViewState& view, D2D1_POINT_2F anchor, float width, float height) const
    {
        D2D1_RECT_F rect = D2D1::RectF(anchor.x + 10.0f, anchor.y - height + 20.0f, anchor.x + 10.0f + width, anchor.y + 20.0f);
        if (rect.right > view.width - 8.0f)
            rect = D2D1::RectF(anchor.x - width - 12.0f, anchor.y - height + 20.0f, anchor.x - 12.0f, anchor.y + 20.0f);
        if (rect.top < 8.0f)
            rect = D2D1::RectF(rect.left, anchor.y + 18.0f, rect.right, anchor.y + 18.0f + height);
        return ClampRectToView(rect, view);
    }

    D2D1_RECT_F BuildAddNoteButtonRect() const
    {
        return D2D1::RectF(132.0f, 18.0f, 224.0f, 50.0f);
    }

    D2D1_RECT_F BuildAddNotePromptRect() const
    {
        return D2D1::RectF(234.0f, 18.0f, 436.0f, 50.0f);
    }

    D2D1_RECT_F BuildRefreshButtonRect() const
    {
        return D2D1::RectF(18.0f, 18.0f, 120.0f, 50.0f);
    }

    D2D1_RECT_F BuildNoteCloseRect(const D2D1_RECT_F& bubble) const
    {
        return D2D1::RectF(bubble.right - 24.0f, bubble.top + 6.0f, bubble.right - 6.0f, bubble.top + 24.0f);
    }

    D2D1_RECT_F BuildNoteEditorRect(const ViewState& view) const
    {
        D2D1_POINT_2F anchor = GeoToScreen(view, m_noteEditorLat, m_noteEditorLon);
        float textHeight = 46.0f;
        if (m_overlayUi.BodyFormat())
            textHeight = MaxValue(46.0f, m_overlayUi.MeasureTextHeight(m_noteEditorText.empty() ? L" " : m_noteEditorText, m_overlayUi.BodyFormat(), kNoteEditorWidth - 32.0f));
        const float height = ClampValue(94.0f + textHeight, kNoteEditorMinHeight, 320.0f);
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
        SetFocus(m_hwnd);
        Invalidate();
    }

    void CancelNoteEditor()
    {
        m_noteEditorMode = NoteEditorMode::None;
        m_noteEditorIndex = static_cast<size_t>(-1);
        m_noteEditorText.clear();
        m_noteEditorCursor = 0;
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
            D2D1_RECT_F rect = BuildNoteBubbleRect(view, anchor, kNoteBubbleWidth, kNoteBubbleHeight);
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
        if (PointInRect(x, y, BuildRefreshButtonRect()))
            return true;
        if (PointInRect(x, y, BuildAddNoteButtonRect()))
            return true;
        if (m_addNoteMode && PointInRect(x, y, BuildAddNotePromptRect()))
            return true;
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

        const D2D1_RECT_F addButton = BuildAddNoteButtonRect();
        const D2D1_RECT_F refreshButton = BuildRefreshButtonRect();
        if (PointInRect(x, y, refreshButton)) {
            if (m_onRefresh)
                m_onRefresh();
            return true;
        }

        if (PointInRect(x, y, addButton)) {
            m_addNoteMode = !m_addNoteMode;
            if (m_addNoteMode)
                CancelNoteEditor();
            Invalidate();
            return true;
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
                InvalidateSceneCache();
                Invalidate();
            }
            return true;
        }

        GeoPoint geo = ScreenToGeo(x, y);
        if (PointInGeoPolygon(geo.lat, geo.lon, m_notificationPolygons[m_activeNotificationPolygonIndex].points)) {
            m_notificationPolygons[m_activeNotificationPolygonIndex].points.clear();
            if (m_onPolygonClear)
                m_onPolygonClear(m_activeNotificationPolygonIndex);
            InvalidateSceneCache();
            Invalidate();
            return true;
        }

        return false;
    }

    void OnMouseMove(int x, int y, UINT buttons)
    {
        if (m_draggingPolygonPoint && (buttons & MK_LBUTTON) && GetCapture() == m_hwnd) {
            if (m_draggingPolygonIndex < m_notificationPolygons.size() &&
                m_draggingPolygonPointIndex < m_notificationPolygons[m_draggingPolygonIndex].points.size())
            {
                GeoPoint geo = ScreenToGeo(x, y);
                m_notificationPolygons[m_draggingPolygonIndex].points[m_draggingPolygonPointIndex] = geo;
                if (m_onPolygonPointMove)
                    m_onPolygonPointMove(m_draggingPolygonIndex, m_draggingPolygonPointIndex, geo.lat, geo.lon);
                InvalidateSceneCache();
                Invalidate();
            }
            return;
        }

        if (m_notificationUiMouseDown || HitNotificationInterface(x, y) || HitNoteInterface(x, y)) {
            if (!m_hoveredAlertId.empty()) {
                m_hoveredAlertId.clear();
                Invalidate();
            }
            return;
        }

        if (!m_trackingMouseLeave) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            if (TrackMouseEvent(&tme))
                m_trackingMouseLeave = true;
        }

        std::wstring hoveredId = HitTestAlert(x, y);
        if (hoveredId != m_hoveredAlertId) {
            m_hoveredAlertId = std::move(hoveredId);
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

        if (m_notificationUiMouseDown || HitNotificationInterface(x, y) || HitNoteInterface(x, y)) {
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
        if (HitNotificationInterface(x, y) || HitNoteInterface(x, y))
            return;

        GeoPoint geo = ScreenToGeo(x, y);
        StartDraftNoteAt(geo.lat, geo.lon);
    }

    void OnMouseWheel(int screenX, int screenY, short delta)
    {
        POINT pt{ screenX, screenY };
        ScreenToClient(m_hwnd, &pt);

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

        if (FAILED(g_d2dFactory->CreateHwndRenderTarget(rtProps, hwndProps, &m_rt)))
            return;

        // Keep Direct2D drawing units aligned with Win32 mouse/client coordinates.
        // Otherwise high-DPI scaling can make ScreenToGeo and GeoToScreen disagree,
        // which places newly-created notes away from the double-clicked map point.
        m_rt->SetDpi(96.0f, 96.0f);

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

            m_rt->DrawBitmap(parentBmp.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, src);
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
        for (const GeoPoint& pt : points) {
            if (IsGeoPointInView(view, pt.lat, pt.lon))
                return true;
        }
        return false;
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

            for (const EarthquakeEvent& event : m_earthquakes) {
                if (!event.hasLocation || !IsGeoPointInView(view, event.latitude, event.longitude))
                    continue;

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
            return;
        }

        for (const EarthquakeEvent& event : m_earthquakes) {
            if (!event.hasLocation || !IsGeoPointInView(view, event.latitude, event.longitude))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, event.latitude, event.longitude);
            float radius = static_cast<float>(ClampValue(4.0 + event.magnitude * 2.2, 5.0, 22.0));
            m_rt->FillEllipse(D2D1::Ellipse(p, radius, radius), m_earthquakeBrush.Get());
            m_rt->DrawEllipse(D2D1::Ellipse(p, radius, radius), m_borderBrush.Get(), 1.25f);
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
        bool hasHistory = false;
    };

    static bool PointInRect(int x, int y, const D2D1_RECT_F& rect)
    {
        return static_cast<float>(x) >= rect.left && static_cast<float>(x) <= rect.right &&
            static_cast<float>(y) >= rect.top && static_cast<float>(y) <= rect.bottom;
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
            layout.historyRect = D2D1::RectF(
                width - kOverlayUiMargin - historyW,
                kOverlayUiMargin,
                width - kOverlayUiMargin,
                kOverlayUiMargin + MaxValue(120.0f, usableH));
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

    D2D1_RECT_F NotificationHistoryClearRect(const D2D1_RECT_F& panelRect) const
    {
        return D2D1::RectF(
            panelRect.right - kOverlayUiPadding - 70.0f,
            panelRect.top + kOverlayUiPadding - 2.0f,
            panelRect.right - kOverlayUiPadding,
            panelRect.top + kOverlayUiPadding + 25.0f);
    }

    float NotificationItemHeight(const AppNotification& notification, float width) const
    {
        width = MaxValue(1.0f, width);
        const float timeH = notification.timestamp.empty()
            ? 0.0f
            : MaxValue(14.0f, m_overlayUi.MeasureTextHeight(notification.timestamp, m_overlayUi.SmallFormat(), width));
        const float titleH = MaxValue(18.0f, m_overlayUi.MeasureTextHeight(notification.title, m_overlayUi.TitleFormat(), width));
        const float bodyH = notification.body.empty()
            ? 0.0f
            : MaxValue(18.0f, m_overlayUi.MeasureTextHeight(notification.body, m_overlayUi.BodyFormat(), width));

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

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (m_showNotificationHistory && PointInRect(x, y, layout.historyRect))
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

    bool HandleNotificationPointerDown(int x, int y)
    {
        if (!m_showNotificationHistory)
            return false;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (!layout.hasHistory || !PointInRect(x, y, layout.historyRect))
            return false;

        if (PointInRect(x, y, NotificationHistoryClearRect(layout.historyRect))) {
            m_notificationHistoryScroll = 0.0f;
            if (m_onNotificationHistoryClear)
                m_onNotificationHistoryClear();
            Invalidate();
            return true;
        }

        return false;
    }

    bool TryScrollNotificationHistoryAt(int x, int y, short delta)
    {
        if (!m_showNotificationHistory)
            return false;

        const NotificationLayout layout = BuildNotificationLayout(BuildViewState());
        if (!PointInRect(x, y, layout.historyRect))
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
        if (rect.right - rect.left < 120.0f || rect.bottom - rect.top < 120.0f)
            return;

        m_lastNotificationHistoryRect = rect;
        m_hasLastNotificationHistoryRect = true;
        m_overlayUi.DrawGlassPanel(rect, 12.0f);

        D2D1_RECT_F titleRect = D2D1::RectF(
            rect.left + kOverlayUiPadding,
            rect.top + kOverlayUiPadding - 1.0f,
            rect.right - kOverlayUiPadding - 78.0f,
            rect.top + kOverlayUiPadding + 22.0f);
        m_overlayUi.DrawLabel(L"Notification History", m_overlayUi.TitleFormat(), titleRect);
        m_overlayUi.DrawButton(OverlayButton{ 0, L"Clear", NotificationHistoryClearRect(rect), !m_notificationHistory.empty(), false, false });

        std::wstring countText = m_notificationHistory.empty()
            ? L"No notifications yet"
            : std::to_wstring(m_notificationHistory.size()) + L" recent notification(s)";
        D2D1_RECT_F countRect = D2D1::RectF(
            rect.left + kOverlayUiPadding,
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
                        D2D1_RECT_F timeRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + 15.0f);
                        m_overlayUi.DrawLabel(notification.timestamp, m_overlayUi.SmallFormat(), timeRect, m_overlayUi.MutedTextBrush());
                        itemY += 17.0f;
                    }

                    const float titleH = MaxValue(18.0f, m_overlayUi.MeasureTextHeight(notification.title, m_overlayUi.TitleFormat(), contentW));
                    D2D1_RECT_F itemTitleRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + titleH + 2.0f);
                    m_overlayUi.DrawLabel(notification.title, m_overlayUi.TitleFormat(), itemTitleRect);
                    itemY += titleH + 3.0f;

                    if (!notification.body.empty()) {
                        const float bodyH = MaxValue(18.0f, m_overlayUi.MeasureTextHeight(notification.body, m_overlayUi.BodyFormat(), contentW));
                        D2D1_RECT_F bodyRect = D2D1::RectF(contentRect.left, itemY, contentRect.right, itemY + bodyH + 2.0f);
                        m_overlayUi.DrawLabel(notification.body, m_overlayUi.BodyFormat(), bodyRect, m_overlayUi.TextBrush());
                    }

                    m_overlayUi.DrawSeparator(contentRect.left, contentRect.right, y + itemH - 5.0f);
                }
                y += itemH;
            }
        }
        m_rt->PopAxisAlignedClip();

        D2D1_RECT_F scrollTrack = D2D1::RectF(rect.right - 12.0f, contentRect.top, rect.right - 7.0f, contentRect.bottom);
        m_overlayUi.DrawScrollbar(scrollTrack, contentH, viewportH, m_notificationHistoryScroll);
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
        const float textH = hasLaneOverlay ? 64.0f : 44.0f;
        const float panelPad = 8.0f;
        const float iconsW = hasLaneOverlay ? (total * icon + (total - 1) * gap) : 0.0f;
        const float panelW = MaxValue(238.0f, iconsW + panelPad * 2.0f);
        const float panelH = hasLaneOverlay ? (textH + icon + panelPad * 2.0f + 4.0f) : (textH + panelPad * 2.0f);

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

        std::wstring roadTitle = alert->road.empty() ? alert->region : alert->road;
        if (roadTitle.empty())
            roadTitle = L"Traffic alert";
        std::wstring alertTitle = alert->title.empty() ? BuildSeverityDisplay(alert->severity) : alert->title;
        std::wstring laneTitle;
        if (hasLaneOverlay)
            laneTitle = L"Lanes closed: " + std::to_wstring(closed) + L" of " + std::to_wstring(total);
        if (m_noteTextFormat) {
            D2D1_RECT_F roadRect = D2D1::RectF(left + panelPad, top + panelPad - 1.0f, left + panelW - panelPad, top + panelPad + 20.0f);
            D2D1_RECT_F titleRect = D2D1::RectF(left + panelPad, top + panelPad + 19.0f, left + panelW - panelPad, top + panelPad + 42.0f);
            m_rt->DrawTextW(roadTitle.c_str(), static_cast<UINT32>(roadTitle.size()), m_noteTextFormat.Get(), roadRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            m_rt->DrawTextW(alertTitle.c_str(), static_cast<UINT32>(alertTitle.size()), m_noteTextFormat.Get(), titleRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            if (hasLaneOverlay) {
                D2D1_RECT_F laneRect = D2D1::RectF(left + panelPad, top + panelPad + 42.0f, left + panelW - panelPad, top + panelPad + textH);
                m_rt->DrawTextW(laneTitle.c_str(), static_cast<UINT32>(laneTitle.size()), m_noteTextFormat.Get(), laneRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!hasLaneOverlay)
            return;

        float x = left + panelPad;
        const float y = top + panelPad + textH + 4.0f;
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
        }
    }

    void DrawTiles(bool interactive, const D2D1_RECT_F* clip = nullptr)
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        int width = std::max(1, static_cast<int>(rc.right - rc.left));
        int height = std::max(1, static_cast<int>(rc.bottom - rc.top));

        const D2D1_RECT_F viewport = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        if (clip && !RectsIntersect(*clip, viewport))
            return;

        const float drawLeft = clip ? ClampValue(clip->left, 0.0f, viewport.right) : viewport.left;
        const float drawTop = clip ? ClampValue(clip->top, 0.0f, viewport.bottom) : viewport.top;
        const float drawRight = clip ? ClampValue(clip->right, 0.0f, viewport.right) : viewport.right;
        const float drawBottom = clip ? ClampValue(clip->bottom, 0.0f, viewport.bottom) : viewport.bottom;
        if (drawRight <= drawLeft || drawBottom <= drawTop)
            return;

        WorldPoint centerWorld = GeoToWorld(m_centerLat, m_centerLon, m_zoom);
        double originX = centerWorld.x - width * 0.5;
        double originY = centerWorld.y - height * 0.5;

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
                    else if (!interactive && entry->ready && !entry->bytes.empty()) {
                        bytesCopy = entry->bytes;
                    }
                }

                if (!interactive && entry && !bmp && !bytesCopy.empty()) {
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
                    const bool drewFallback = TryDrawFallbackTile(key, dest);
                    if (!interactive) {
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



    void DrawCityAnchors(const ViewState& view)
    {
        static const GeoPoint cities[] = {
            { 51.5074, -0.1278 }, { 52.4862, -1.8904 }, { 53.4808, -2.2426 },
            { 53.8008, -1.5491 }, { 55.9533, -3.1883 }, { 51.4816, -3.1791 },
            { 50.8198, -1.0880 }, { 54.9783, -1.6178 }
        };

        const int width = view.width;
        const int height = view.height;

        for (const GeoPoint& city : cities) {
            if (!IsGeoPointInView(view, city.lat, city.lon))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, city.lat, city.lon);
            if (p.x < -16.0f || p.y < -16.0f || p.x > width + 16.0f || p.y > height + 16.0f)
                continue;

            m_rt->FillEllipse(D2D1::Ellipse(p, 4.0f, 4.0f), m_textBrush.Get());
            m_rt->DrawEllipse(D2D1::Ellipse(p, 8.0f, 8.0f), m_panelBrush.Get(), 2.0f);
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

            std::wstring text = note.text;
            if (text.size() > 120)
                text = text.substr(0, 117) + L"...";

            D2D1_RECT_F bubbleRect = BuildNoteBubbleRect(view, p, kNoteBubbleWidth, kNoteBubbleHeight);
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
        OverlayButton closeButton;
        closeButton.text = L"x";
        closeButton.bounds = closeRect;
        m_overlayUi.DrawButton(closeButton);

        OverlayTextBox textBox;
        textBox.text = m_noteEditorText;
        textBox.placeholder = L"Type note text here...";
        textBox.bounds = D2D1::RectF(rect.left + 12.0f, rect.top + 42.0f, rect.right - 12.0f, rect.bottom - 54.0f);
        textBox.focused = true;
        m_overlayUi.DrawTextBox(textBox);

        const bool showCaret = (GetTickCount64() / 550) % 2 == 0;
        if (showCaret && m_noteEditorText.empty()) {
            m_rt->DrawLine(
                D2D1::Point2F(textBox.bounds.left + 10.0f, textBox.bounds.top + 10.0f),
                D2D1::Point2F(textBox.bounds.left + 10.0f, textBox.bounds.bottom - 10.0f),
                m_overlayUi.AccentBrush(),
                1.4f);
        }

        OverlayButton saveButton;
        saveButton.text = L"Save";
        saveButton.bounds = D2D1::RectF(rect.right - 156.0f, rect.bottom - 42.0f, rect.right - 84.0f, rect.bottom - 12.0f);
        m_overlayUi.DrawButton(saveButton);

        OverlayButton cancelButton;
        cancelButton.text = L"Cancel";
        cancelButton.bounds = D2D1::RectF(rect.right - 78.0f, rect.bottom - 42.0f, rect.right - 12.0f, rect.bottom - 12.0f);
        m_overlayUi.DrawButton(cancelButton);
    }

    void DrawNoteToolbar()
    {
        if (!m_rt)
            return;

        OverlayButton refreshButton;
        refreshButton.text = L"Refresh";
        refreshButton.bounds = BuildRefreshButtonRect();
        m_overlayUi.DrawButton(refreshButton);

        OverlayButton addButton;
        addButton.text = m_addNoteMode ? L"Adding" : L"+ Note";
        addButton.bounds = BuildAddNoteButtonRect();
        addButton.hot = m_addNoteMode;
        m_overlayUi.DrawButton(addButton);

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


    void UpdateSceneCache(const ViewState& view)
    {
        if (!m_rt)
            return;

        const D2D1_SIZE_U pixelSize = m_rt->GetPixelSize();
        if (pixelSize.width == 0 || pixelSize.height == 0)
            return;

        if (!m_sceneBitmap ||
            m_sceneBitmapWidth != static_cast<int>(pixelSize.width) ||
            m_sceneBitmapHeight != static_cast<int>(pixelSize.height))
        {
            m_sceneBitmap.Reset();
            D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(m_rt->GetPixelFormat());
            if (FAILED(m_rt->CreateBitmap(pixelSize, nullptr, 0, &props, &m_sceneBitmap)))
                return;

            m_sceneBitmapWidth = static_cast<int>(pixelSize.width);
            m_sceneBitmapHeight = static_cast<int>(pixelSize.height);
        }

        D2D1_POINT_2U destPoint = D2D1::Point2U(0, 0);
        D2D1_RECT_U srcRect = D2D1::RectU(0, 0, pixelSize.width, pixelSize.height);
        if (FAILED(m_sceneBitmap->CopyFromRenderTarget(&destPoint, m_rt.Get(), &srcRect))) {
            InvalidateSceneCache();
            return;
        }

        m_sceneBitmapZoom = m_zoom;
        m_sceneBitmapCenterWorld = view.centerWorld;
    }

    bool DrawCachedScene(const ViewState& view, D2D1_RECT_F* destOut = nullptr)
    {
        if (!m_rt || !m_sceneBitmap || m_sceneBitmapWidth <= 0 || m_sceneBitmapHeight <= 0)
            return false;

        // Reuse the cached scene for same-zoom panning only. Scaling the cached
        // composite during wheel zoom changes translucent fill colours, especially
        // at close zoom levels, so zoom frames are rendered live.
        if (m_zoom != m_sceneBitmapZoom)
            return false;

        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        double dx = m_sceneBitmapCenterWorld.x - view.centerWorld.x;
        if (dx > worldSize * 0.5)
            dx -= worldSize;
        else if (dx < -worldSize * 0.5)
            dx += worldSize;

        const double dy = m_sceneBitmapCenterWorld.y - view.centerWorld.y;
        if (std::abs(dx) > view.width * 0.75 || std::abs(dy) > view.height * 0.75)
            return false;

        const float scaledWidth = static_cast<float>(m_sceneBitmapWidth);
        const float scaledHeight = static_cast<float>(m_sceneBitmapHeight);
        const float centerX = static_cast<float>(view.width * 0.5 + dx);
        const float centerY = static_cast<float>(view.height * 0.5 + dy);
        const D2D1_RECT_F dest = D2D1::RectF(
            centerX - scaledWidth * 0.5f,
            centerY - scaledHeight * 0.5f,
            centerX + scaledWidth * 0.5f,
            centerY + scaledHeight * 0.5f);

        m_rt->DrawBitmap(m_sceneBitmap.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        if (destOut)
            *destOut = dest;
        return true;
    }

    void DrawSceneOverlays(const ViewState& overlayView, const ViewState& boundaryView)
    {
        DrawUkBoundary(boundaryView);
        DrawCityAnchors(overlayView);
        DrawNotificationPolygons(overlayView);
        DrawEarthquakes(overlayView);
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
        DrawSceneOverlays(overlayView, boundaryView);
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
            m_rt->BeginDraw();
            m_rt->Clear(D2D1::ColorF(kMapWaterR, kMapWaterG, kMapWaterB, 1.0f));

            const bool interactive = m_interactivePan;
            m_interactiveTileRequestsThisFrame = 0;
            const ViewState view = BuildViewState();
            const ViewState overlayView = BuildViewState(220.0);
            const ViewState boundaryView = BuildViewState(kBoundaryDrawMarginPixels);
            bool drewCachedScene = false;
            D2D1_RECT_F cachedSceneDest{};

            if (interactive && m_sceneBitmap && m_zoom == m_sceneBitmapZoom) {
                drewCachedScene = DrawCachedScene(view, &cachedSceneDest);
                if (drewCachedScene) {
                    const std::vector<D2D1_RECT_F> exposedStrips = BuildExposedSceneStrips(view, cachedSceneDest);
                    DrawExposedCachedSceneTiles(exposedStrips);
                    DrawExposedCachedSceneEdges(exposedStrips, overlayView, boundaryView);
                }
            }

            if (!drewCachedScene) {
                DrawTiles(interactive);
                DrawSceneOverlays(overlayView, boundaryView);

                if (!interactive) {
                    m_rt->Flush();
                    UpdateSceneCache(view);
                }
            }

            DrawMapChrome();
            DrawAlertOverlay(overlayView);
            DrawNoteInterface(view);
            DrawNotificationInterface(view);

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

    static bool IsGeoPointInRing(const BoundaryRing& ring, double lat, double lon)
    {
        if (ring.segmentsByMinLat.empty() ||
            lat < ring.minLat || lat > ring.maxLat ||
            lon < ring.minLon || lon > ring.maxLon)
        {
            return false;
        }

        bool inside = false;
        for (const BoundarySegment& segment : ring.segmentsByMinLat) {
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

    void DrawHighZoomBoundaryFill(const ViewState& view)
    {
        if (!m_rt || !m_outlineFillBrush)
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

            for (const BoundaryRing& ring : m_ukBoundaryRings) {
                if (ring.segmentsByMinLat.empty() || lat < ring.minLat || lat > ring.maxLat)
                    continue;

                if (!centreInsideRing && IsGeoPointInRing(ring, lat, centerLon))
                    centreInsideRing = true;

                for (const BoundarySegment& segment : ring.segmentsByMinLat) {
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

    void DrawBoundaryRingFull(const BoundaryRing& ring)
    {
        if (!m_rt || ring.points.size() < 3)
            return;

        ComPtr<ID2D1PathGeometry> geom;
        if (FAILED(g_d2dFactory->CreatePathGeometry(&geom)))
            return;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geom->Open(&sink)))
            return;

        sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

        sink->BeginFigure(
            GeoToScreen(ring.points[0].lat, ring.points[0].lon),
            D2D1_FIGURE_BEGIN_FILLED);

        for (size_t i = 1; i < ring.points.size(); ++i)
            sink->AddLine(GeoToScreen(ring.points[i].lat, ring.points[i].lon));

        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        if (FAILED(sink->Close()))
            return;

        if (m_outlineFillBrush)
            m_rt->FillGeometry(geom.Get(), m_outlineFillBrush.Get());

        if (m_outlineStrokeBrush)
            m_rt->DrawGeometry(geom.Get(), m_outlineStrokeBrush.Get(), 2.0f);
    }

    void DrawBoundaryRingVisibleStroke(const BoundaryRing& ring, const ViewState& view)
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

            sink->BeginFigure(GeoToScreen(view, segment.a.lat, segment.a.lon), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(GeoToScreen(view, segment.b.lat, segment.b.lon));
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
        const bool fullBoundary = m_zoom <= kFullBoundaryMaxZoom;

        if (!fullBoundary) {
            // At close zoom levels the fill renderer is already clipped to the
            // actual visible boundary spans. Run it whenever a high-zoom boundary
            // is visible rather than gating on the viewport centre; otherwise
            // panning over coastline can randomly drop all land tint as soon as
            // the centre point crosses water.
            for (const auto& ring : m_ukBoundaryRings) {
                if (RingIntersectsView(ring, view)) {
                    DrawHighZoomBoundaryFill(view);
                    break;
                }
            }
        }

        for (const auto& ring : m_ukBoundaryRings) {
            if (!RingIntersectsView(ring, view))
                continue;

            if (fullBoundary)
                DrawBoundaryRingFull(ring);
            else
                DrawBoundaryRingVisibleStroke(ring, view);
        }
    }

    HWND m_hwnd = nullptr;
    std::vector<TrafficAlert> m_alerts;
    std::vector<MapNote> m_notes;
    std::vector<GeoPolygon> m_notificationPolygons;
    std::vector<GeoPoint> m_draftPolygon;
    std::vector<EarthquakeEvent> m_earthquakes;
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

    int m_zoom = kDefaultZoom;
    double m_centerLat = kDefaultCenterLat;
    double m_centerLon = kDefaultCenterLon;

    POINT m_mouseDown{};
    POINT m_lastMouse{};
    bool m_dragging = false;
    bool m_interactivePan = false;
    bool m_notificationUiMouseDown = false;
    bool m_addNoteMode = false;
    bool m_polygonCaptureActive = false;
    bool m_draggingPolygonPoint = false;
    size_t m_activeNotificationPolygonIndex = static_cast<size_t>(-1);
    size_t m_draggingPolygonIndex = static_cast<size_t>(-1);
    size_t m_draggingPolygonPointIndex = static_cast<size_t>(-1);
    bool m_trackingMouseLeave = false;
    int m_interactiveTileRequestsThisFrame = 0;
    std::wstring m_hoveredAlertId;
    NoteEditorMode m_noteEditorMode = NoteEditorMode::None;
    size_t m_noteEditorIndex = static_cast<size_t>(-1);
    std::wstring m_noteEditorText;
    size_t m_noteEditorCursor = 0;
    double m_noteEditorLat = 0.0;
    double m_noteEditorLon = 0.0;
    AppNotification m_activeNotification;
    std::vector<AppNotification> m_notificationHistory;
    bool m_hasActiveNotification = false;
    bool m_showNotificationHistory = false;
    float m_notificationHistoryScroll = 0.0f;
    D2D1_RECT_F m_lastActiveNotificationRect{};
    D2D1_RECT_F m_lastNotificationHistoryRect{};
    bool m_hasLastActiveNotificationRect = false;
    bool m_hasLastNotificationHistoryRect = false;
    MapOverlayUiRenderer m_overlayUi;

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
    ComPtr<ID2D1SolidColorBrush> m_panelBrush;
    ComPtr<ID2D1SolidColorBrush> m_textBrush;
    ComPtr<ID2D1SolidColorBrush> m_noteBrush;
    ComPtr<ID2D1SolidColorBrush> m_laneTileBrush;
    ComPtr<ID2D1SolidColorBrush> m_polygonFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_polygonStrokeBrush;
    ComPtr<ID2D1SolidColorBrush> m_draftFillBrush;
    ComPtr<ID2D1SolidColorBrush> m_draftStrokeBrush;
    ComPtr<ID2D1SolidColorBrush> m_earthquakeBrush;
    ComPtr<IDWriteTextFormat> m_noteTextFormat;
    ComPtr<ID2D1Bitmap> m_sceneBitmap;
    int m_sceneBitmapWidth = 0;
    int m_sceneBitmapHeight = 0;
    int m_sceneBitmapZoom = kDefaultZoom;
    WorldPoint m_sceneBitmapCenterWorld{};
    bool m_hasOverlayClip = false;
    D2D1_RECT_F m_overlayClip{};
    std::vector<BoundaryRing> m_ukBoundaryRings;

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

void MapView::SetAlerts(const std::vector<TrafficAlert>& alerts)
{
    m_impl->SetAlerts(alerts);
}

void MapView::SetNotes(const std::vector<MapNote>& notes)
{
    m_impl->SetNotes(notes);
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

void MapView::FitToAlerts()
{
    m_impl->FitToAlerts();
}

bool MapView::LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut)
{
    return m_impl->LoadUkBoundaryFromFile(path, errorOut);
}
