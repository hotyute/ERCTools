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
    std::vector<BYTE> bytes;
    ComPtr<ID2D1Bitmap> bitmap;
};

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
        Invalidate();
    }

    void SetNotes(const std::vector<MapNote>& notes)
    {
        m_notes = notes;
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
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.05f, 0.10f, 0.18f, 0.72f), &m_panelBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.90f), &m_textBrush);
        m_rt->CreateSolidColorBrush(D2D1::ColorF(0.40f, 0.20f, 0.95f, 0.95f), &m_noteBrush);
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



    void DrawCityAnchors()
    {
        static const GeoPoint cities[] = {
            { 51.5074, -0.1278 }, { 52.4862, -1.8904 }, { 53.4808, -2.2426 },
            { 53.8008, -1.5491 }, { 55.9533, -3.1883 }, { 51.4816, -3.1791 },
            { 50.8198, -1.0880 }, { 54.9783, -1.6178 }
        };

        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        const int width = static_cast<int>(rc.right - rc.left);
        const int height = static_cast<int>(rc.bottom - rc.top);

        for (const GeoPoint& city : cities) {
            D2D1_POINT_2F p = GeoToScreen(city.lat, city.lon);
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

        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        int width = static_cast<int>(rc.right - rc.left);
        int height = static_cast<int>(rc.bottom - rc.top);

        for (const auto& note : m_notes) {
            D2D1_POINT_2F p = GeoToScreen(note.latitude, note.longitude);
            if (p.x < -80.0f || p.y < -80.0f || p.x > width + 80.0f || p.y > height + 80.0f)
                continue;

            D2D1_ROUNDED_RECT bubble = D2D1::RoundedRect(
                D2D1::RectF(p.x + 10.0f, p.y - 42.0f, p.x + 210.0f, p.y + 18.0f),
                10.0f,
                10.0f);
            m_rt->FillRoundedRectangle(bubble, m_panelBrush.Get());
            m_rt->DrawRoundedRectangle(bubble, m_noteBrush.Get(), 1.5f);
            m_rt->FillEllipse(D2D1::Ellipse(p, 8.0f, 8.0f), m_noteBrush.Get());
            m_rt->DrawLine(p, D2D1::Point2F(p.x + 12.0f, p.y - 6.0f), m_noteBrush.Get(), 2.0f);
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
            DrawCityAnchors();
            DrawNotes();
            DrawMarkers();
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
    std::vector<std::vector<GeoPoint>> m_ukBoundaryRings;

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
