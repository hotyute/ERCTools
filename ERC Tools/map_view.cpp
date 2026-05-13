// =================================================================================
// FILE: map_view.cpp
// =================================================================================

#include "map_view.h"
#include "app_state.h"
#include "http.h"
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

constexpr int kMaxConcurrentTileDownloads = 6;
constexpr size_t kMaxTileCacheEntries = 768;
constexpr UINT_PTR kInteractionIdleTimer = 1;
constexpr UINT kInteractionIdleMs = 120;
constexpr float kMapWaterR = 0.80f;
constexpr float kMapWaterG = 0.91f;
constexpr float kMapWaterB = 0.98f;
constexpr int kMaxFallbackTileZoomDelta = 5;
constexpr double kBoundaryDrawMarginPixels = 512.0;
// Reuse the cached scene for panning only. Scaling the cached composite during
// wheel zoom subtly changes translucent fill colours, especially at very close
// zoom levels, so zoom frames are rendered live while still using lightweight
// tile loading/fallbacks.
constexpr double kMinCachedSceneScale = 1.0;
constexpr double kMaxCachedSceneScale = 1.0;
std::atomic<int> g_activeTileDownloads{ 0 };

// ============================================================
// MapView
// ============================================================

class MapView::Impl
{
public:
    using SelectCallback = std::function<void(const std::wstring&)>;
    using NoteLocationCallback = std::function<void(double lat, double lon)>;

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

    void SetNoteLocationCallback(NoteLocationCallback cb)
    {
        m_onNoteLocation = std::move(cb);
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
        InvalidateSceneCache();
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
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

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
            SetCapture(m_hwnd);
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

        case WM_LBUTTONUP:
            OnLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONDBLCLK:
            OnDoubleClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
        double bestDist = 14.0;
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

        if (!m_dragging) {
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
        if (!m_onNoteLocation)
            return;

        GeoPoint geo = ScreenToGeo(x, y);
        m_onNoteLocation(geo.lat, geo.lon);
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
        m_noteTextFormat.Reset();
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

            --g_activeTileDownloads;

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

    void DrawMarkers()
    {
        const ViewState view = BuildViewState(32.0);
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

    void DrawTiles(bool interactive)
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
                auto entry = interactive ? FindTile(key) : GetOrCreateTile(key);
                if (entry) {
                    std::lock_guard<std::mutex> lk(entry->mutex);
                    entry->lastUsedMs = GetTickCount64();
                }

                D2D1_RECT_F dest = D2D1::RectF(
                    static_cast<float>(tx * 256.0 - originX),
                    static_cast<float>(ty * 256.0 - originY),
                    static_cast<float>(tx * 256.0 - originX + 256.0),
                    static_cast<float>(ty * 256.0 - originY + 256.0));

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
                    if (!interactive)
                        RequestTile(key);

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



    void DrawCityAnchors()
    {
        static const GeoPoint cities[] = {
            { 51.5074, -0.1278 }, { 52.4862, -1.8904 }, { 53.4808, -2.2426 },
            { 53.8008, -1.5491 }, { 55.9533, -3.1883 }, { 51.4816, -3.1791 },
            { 50.8198, -1.0880 }, { 54.9783, -1.6178 }
        };

        const ViewState view = BuildViewState(16.0);
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

    void DrawNotes()
    {
        if (!m_rt)
            return;

        const ViewState view = BuildViewState(220.0);
        int width = view.width;
        int height = view.height;

        for (const auto& note : m_notes) {
            if (!IsGeoPointInView(view, note.latitude, note.longitude))
                continue;

            D2D1_POINT_2F p = GeoToScreen(view, note.latitude, note.longitude);
            if (p.x < -80.0f || p.y < -80.0f || p.x > width + 80.0f || p.y > height + 80.0f)
                continue;

            std::wstring text = note.text;
            if (text.size() > 120)
                text = text.substr(0, 117) + L"...";

            D2D1_RECT_F textRect = D2D1::RectF(p.x + 24.0f, p.y - 36.0f, p.x + 204.0f, p.y + 12.0f);
            D2D1_ROUNDED_RECT bubble = D2D1::RoundedRect(
                D2D1::RectF(p.x + 10.0f, p.y - 44.0f, p.x + 214.0f, p.y + 20.0f),
                10.0f,
                10.0f);
            m_rt->FillRoundedRectangle(bubble, m_panelBrush.Get());
            m_rt->DrawRoundedRectangle(bubble, m_noteBrush.Get(), 1.5f);
            m_rt->FillEllipse(D2D1::Ellipse(p, 8.0f, 8.0f), m_noteBrush.Get());
            m_rt->DrawLine(p, D2D1::Point2F(p.x + 12.0f, p.y - 6.0f), m_noteBrush.Get(), 2.0f);
            if (m_noteTextFormat && !text.empty())
                m_rt->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), m_noteTextFormat.Get(), textRect, m_textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }

    void DrawMapChrome()
    {
        if (!m_rt)
            return;

        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);

        m_rt->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(18.0f, 18.0f, 330.0f, 92.0f), 16.0f, 16.0f),
            m_panelBrush.Get());
        m_rt->DrawRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(18.0f, 18.0f, 330.0f, 92.0f), 16.0f, 16.0f),
            m_textBrush.Get(),
            1.0f);

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

        const int zoomDelta = m_zoom - m_sceneBitmapZoom;
        if (zoomDelta != 0)
            return false;

        const double scale = 1.0;
        if (scale < kMinCachedSceneScale || scale > kMaxCachedSceneScale)
            return false;

        const double worldSize = 256.0 * static_cast<double>(1 << m_zoom);
        double dx = m_sceneBitmapCenterWorld.x * scale - view.centerWorld.x;
        if (dx > worldSize * 0.5)
            dx -= worldSize;
        else if (dx < -worldSize * 0.5)
            dx += worldSize;

        const double dy = m_sceneBitmapCenterWorld.y * scale - view.centerWorld.y;
        if (std::abs(dx) > view.width * 0.75 || std::abs(dy) > view.height * 0.75)
            return false;

        const float scaledWidth = static_cast<float>(m_sceneBitmapWidth * scale);
        const float scaledHeight = static_cast<float>(m_sceneBitmapHeight * scale);
        const float centerX = static_cast<float>(view.width * 0.5 + dx);
        const float centerY = static_cast<float>(view.height * 0.5 + dy);
        const D2D1_RECT_F dest = D2D1::RectF(
            centerX - scaledWidth * 0.5f,
            centerY - scaledHeight * 0.5f,
            centerX + scaledWidth * 0.5f,
            centerY + scaledHeight * 0.5f);

        m_rt->DrawBitmap(m_sceneBitmap.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        if (destOut)
            *destOut = dest;
        return true;
    }

    void DrawSceneOverlays()
    {
        DrawUkBoundary();
        DrawCityAnchors();
        DrawNotes();
        DrawMarkers();
    }

    void DrawSceneOverlaysInClip(const D2D1_RECT_F& clip)
    {
        if (!m_rt || clip.right <= clip.left || clip.bottom <= clip.top)
            return;

        m_rt->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        DrawSceneOverlays();
        m_rt->PopAxisAlignedClip();
    }

    void DrawExposedCachedSceneEdges(const ViewState& view, const D2D1_RECT_F& cachedDest)
    {
        if (!m_rt)
            return;

        const float width = static_cast<float>(view.width);
        const float height = static_cast<float>(view.height);
        const float left = ClampValue(cachedDest.left, 0.0f, width);
        const float top = ClampValue(cachedDest.top, 0.0f, height);
        const float right = ClampValue(cachedDest.right, 0.0f, width);
        const float bottom = ClampValue(cachedDest.bottom, 0.0f, height);

        // The cached scene is only the previous viewport. Draw full overlays just
        // into newly exposed strips so panning reveals boundary/fill/markers ahead
        // of the cursor without paying to redraw the whole map each frame.
        DrawSceneOverlaysInClip(D2D1::RectF(0.0f, 0.0f, left, height));
        DrawSceneOverlaysInClip(D2D1::RectF(right, 0.0f, width, height));
        DrawSceneOverlaysInClip(D2D1::RectF(left, 0.0f, right, top));
        DrawSceneOverlaysInClip(D2D1::RectF(left, bottom, right, height));
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
            const ViewState view = BuildViewState();
            bool drewCachedScene = false;
            D2D1_RECT_F cachedSceneDest{};

            if (interactive && m_sceneBitmap && m_zoom == m_sceneBitmapZoom) {
                DrawTiles(true);
                drewCachedScene = DrawCachedScene(view, &cachedSceneDest);
                if (drewCachedScene)
                    DrawExposedCachedSceneEdges(view, cachedSceneDest);
            }

            if (!drewCachedScene) {
                DrawTiles(interactive);
                DrawSceneOverlays();

                if (!interactive) {
                    m_rt->Flush();
                    UpdateSceneCache(view);
                }
            }

            DrawMapChrome();

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
        // visible boundary. This keeps the stable high-zoom tint without leaking
        // it outside the coastline.
        constexpr int kFillBandHeight = 6;
        std::vector<float> intersections;
        intersections.reserve(16);

        for (int y = 0; y < view.height; y += kFillBandHeight) {
            const int bandBottom = MinValue(y + kFillBandHeight, view.height);
            const double sampleY = (static_cast<double>(y) + bandBottom) * 0.5;
            const double worldY = view.centerWorld.y + (sampleY - view.height * 0.5);
            const GeoPoint sampleGeo = WorldToGeo(view.centerWorld.x, worldY, m_zoom);
            const double lat = sampleGeo.lat;

            intersections.clear();
            bool centreInsideRing = false;

            for (const BoundaryRing& ring : m_ukBoundaryRings) {
                if (ring.segmentsByMinLat.empty() || lat < ring.minLat || lat > ring.maxLat)
                    continue;

                if (!centreInsideRing && IsGeoPointInRing(ring, lat, NormalizeLongitude(m_centerLon)))
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
                        D2D1::RectF(0.0f, static_cast<float>(y), static_cast<float>(view.width), static_cast<float>(bandBottom)),
                        m_outlineFillBrush.Get());
                }
                continue;
            }

            std::sort(intersections.begin(), intersections.end());

            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                float left = MaxValue(intersections[i], 0.0f);
                float right = MinValue(intersections[i + 1], static_cast<float>(view.width));
                if (right <= left)
                    continue;

                m_rt->FillRectangle(
                    D2D1::RectF(left, static_cast<float>(y), right, static_cast<float>(bandBottom)),
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

    void DrawUkBoundary()
    {
        if (m_ukBoundaryRings.empty())
            return;

        const ViewState view = BuildViewState(kBoundaryDrawMarginPixels);
        // Keep the boundary fill path stable while panning/zooming. Switching from
        // full polygon fill to viewport fill solely because the user is interacting
        // changes the apparent tint and causes obvious flicker at mid zoom levels.
        const bool fullBoundary = m_zoom < 10;

        if (!fullBoundary) {
            // At close zoom levels the visible outline is drawn segment-by-segment
            // for performance. Keep the land tint stable by applying the same
            // translucent fill over the viewport when the view centre is inside
            // the boundary, instead of letting it flicker off during interaction.
            const double centerLon = NormalizeLongitude(m_centerLon);
            for (const auto& ring : m_ukBoundaryRings) {
                if (RingIntersectsView(ring, view) && IsGeoPointInRing(ring, m_centerLat, centerLon)) {
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
    std::wstring m_selectedId;
    SelectCallback m_onSelect;
    NoteLocationCallback m_onNoteLocation;

    int m_zoom = kDefaultZoom;
    double m_centerLat = kDefaultCenterLat;
    double m_centerLon = kDefaultCenterLon;

    POINT m_mouseDown{};
    POINT m_lastMouse{};
    bool m_dragging = false;
    bool m_interactivePan = false;

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
    ComPtr<IDWriteTextFormat> m_noteTextFormat;
    ComPtr<ID2D1Bitmap> m_sceneBitmap;
    int m_sceneBitmapWidth = 0;
    int m_sceneBitmapHeight = 0;
    int m_sceneBitmapZoom = kDefaultZoom;
    WorldPoint m_sceneBitmapCenterWorld{};
    std::vector<BoundaryRing> m_ukBoundaryRings;

    std::mutex m_tileMutex;
    std::unordered_map<TileKey, std::shared_ptr<TileEntry>, TileKeyHash> m_tiles;
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

void MapView::SetNoteLocationCallback(NoteLocationCallback cb)
{
    m_impl->SetNoteLocationCallback(std::move(cb));
}

void MapView::SetAlerts(const std::vector<TrafficAlert>& alerts)
{
    m_impl->SetAlerts(alerts);
}

void MapView::SetNotes(const std::vector<MapNote>& notes)
{
    m_impl->SetNotes(notes);
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
