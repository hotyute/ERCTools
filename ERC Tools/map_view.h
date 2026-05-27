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
    using PolygonPointMoveCallback = std::function<void(size_t polygonIndex, size_t pointIndex, double lat, double lon)>;
    using PolygonPointDeleteCallback = std::function<void(size_t polygonIndex, size_t pointIndex)>;
    using PolygonClearCallback = std::function<void(size_t polygonIndex)>;
    using RefreshCallback = std::function<void()>;
    using NotificationHistoryClearCallback = std::function<void()>;
    using NotificationHistoryActivateCallback = std::function<void(const AppNotification&)>;
    using ChatSendCallback = std::function<void(const std::wstring& text)>;
    using ChatClearCallback = std::function<void()>;

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
    void SetPolygonPointMoveCallback(PolygonPointMoveCallback cb);
    void SetPolygonPointDeleteCallback(PolygonPointDeleteCallback cb);
    void SetPolygonClearCallback(PolygonClearCallback cb);
    void SetRefreshCallback(RefreshCallback cb);
    void SetNotificationHistoryClearCallback(NotificationHistoryClearCallback cb);
    void SetNotificationHistoryActivateCallback(NotificationHistoryActivateCallback cb);
    void SetChatSendCallback(ChatSendCallback cb);
    void SetChatClearCallback(ChatClearCallback cb);
    void SetChatClearEnabled(bool enabled);
    void SetAlerts(const std::vector<TrafficAlert>& alerts);
    void SetIncidentOverlayVisible(bool visible);
    void SetNotes(const std::vector<MapNote>& notes);
    void SetChatMessages(const std::vector<ChatMessage>& messages);
    void SetOnlineUsers(const std::vector<OnlineUser>& users);
    void SetUsersVisible(bool visible);
    void SetNotificationPolygons(const std::vector<GeoPolygon>& polygons);
    void SetActiveNotificationPolygonIndex(size_t index);
    void SetDraftPolygon(const std::vector<GeoPoint>& points);
    void SetPolygonCaptureActive(bool active);
    void SetEarthquakes(const std::vector<EarthquakeEvent>& earthquakes);
    void SetEarthquakeOverlayVisible(bool visible);
    void SetWeatherSystems(const std::vector<WeatherSystemEvent>& systems);
    void SetWeatherSystemOverlayVisible(bool visible);
    void SetWeatherWarnings(const std::vector<WeatherWarningEvent>& warnings);
    void SetWeatherWarningOverlayVisible(bool visible);
    void SetWeatherWarningPolygonsVisible(bool visible);
    void SetFloods(const std::vector<FloodEvent>& floods);
    void SetFloodOverlayVisible(bool visible);
    void SetAreaLabelsVisible(bool visible);
    void SetRoadDepictionsVisible(bool visible);
    void SetDisplayWorldMap(bool visible);
    void SetActiveNotification(const AppNotification& notification);
    void ClearActiveNotification();
    void SetNotificationHistory(const std::vector<AppNotification>& notifications);
    void SetNotificationHistoryVisible(bool visible);
    void SetSelectedId(const std::wstring& id);
    void CenterOnAlert(const std::wstring& id);
    void FitToAlerts();
    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);
    bool LoadWorldBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
