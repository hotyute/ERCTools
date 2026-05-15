// =================================================================================
// FILE: main_view.h
// =================================================================================


#pragma once
#include "common.h"
#include "models.h"

class MapView
{
public:
    using SelectCallback = std::function<void(const std::wstring& id)>;
    using NoteCreateCallback = std::function<void(const std::wstring& text, double lat, double lon)>;
    using NoteUpdateCallback = std::function<void(size_t index, const std::wstring& text)>;
    using NoteDeleteCallback = std::function<void(size_t index)>;
    using PolygonPointCallback = std::function<void(double lat, double lon)>;
    using RefreshCallback = std::function<void()>;

    MapView();
    ~MapView();

    MapView(const MapView&) = delete;
    MapView& operator=(const MapView&) = delete;
    MapView(MapView&&) noexcept;
    MapView& operator=(MapView&&) noexcept;

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND Hwnd() const;
    void SetSelectCallback(SelectCallback cb);
    void SetNoteCreateCallback(NoteCreateCallback cb);
    void SetNoteUpdateCallback(NoteUpdateCallback cb);
    void SetNoteDeleteCallback(NoteDeleteCallback cb);
    void SetPolygonPointCallback(PolygonPointCallback cb);
    void SetRefreshCallback(RefreshCallback cb);
    void SetAlerts(const std::vector<TrafficAlert>& alerts);
    void SetNotes(const std::vector<MapNote>& notes);
    void SetNotificationPolygons(const std::vector<GeoPolygon>& polygons);
    void SetDraftPolygon(const std::vector<GeoPoint>& points);
    void SetPolygonCaptureActive(bool active);
    void SetEarthquakes(const std::vector<EarthquakeEvent>& earthquakes);
    void SetActiveNotification(const AppNotification& notification);
    void ClearActiveNotification();
    void SetNotificationHistory(const std::vector<AppNotification>& notifications);
    void SetNotificationHistoryVisible(bool visible);
    void SetSelectedId(const std::wstring& id);
    void CenterOnAlert(const std::wstring& id);
    void FitToAlerts();
    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
