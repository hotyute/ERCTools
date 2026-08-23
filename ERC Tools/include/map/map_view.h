// =================================================================================
// FILE: main_view.h
// =================================================================================


#pragma once
#include "core/common.h"
#include "core/models.h"

struct MapOverlayPositions
{
    float mapControlsX = 0.0f;
    float mapControlsY = 0.0f;
    float countdownX = 0.0f;
    float countdownY = 0.0f;
    float usersX = 0.0f;
    float usersY = 0.0f;
    float privateChatX = 0.0f;
    float privateChatY = 0.0f;
    float responderChatX = 0.0f;
    float responderChatY = 0.0f;
    float fpsX = 0.0f;
    float fpsY = 0.0f;
    float commsX = 0.0f;
    float commsY = 0.0f;
    float roadIncidentsX = 0.0f;
    float roadIncidentsY = 0.0f;
    float notificationHistoryX = 0.0f;
    float notificationHistoryY = 0.0f;
    bool roadIncidentsCollapsed = false;
    bool notificationHistoryCollapsed = false;
    bool usersCollapsed = false;
    bool responderChatCollapsed = false;
};

class MapView
{
public:
    using SelectCallback = std::function<void(const std::wstring& id)>;
    using EventSelectCallback = std::function<void(const std::wstring& sourceType, const std::wstring& id)>;
    using EventActionCallback = std::function<void(
        const std::wstring& sourceType,
        const std::wstring& id,
        const std::wstring& action)>;
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
    using NotificationHistoryActionCallback = std::function<void(const AppNotification&, const std::wstring& action)>;
    using ChatSendCallback = std::function<void(const std::wstring& text)>;
    using PrivateChatSendCallback = std::function<void(const std::wstring& recipientUsername, const std::wstring& text)>;
    using PrivateChatStateCallback = std::function<void(const std::wstring& recipientUsername, bool open)>;
    using CountdownPresetsChangedCallback = std::function<void(const std::array<std::wstring, 3>& presets)>;
    using ChatClearCallback = std::function<void()>;
    using ChatMessageActionCallback = std::function<void(const ChatMessage& message, const std::wstring& action)>;
    using UserActionCallback = std::function<void(const OnlineUser& user, const std::wstring& action)>;
    using PanelCloseCallback = std::function<void(const std::wstring& panelName)>;
    using MapDisplayModeCallback = std::function<void(bool displayWorldMap)>;
    using IrelandVisibilityCallback = std::function<void(bool visible)>;
    using OverlayPositionsChangedCallback = std::function<void(const MapOverlayPositions& positions)>;
    using RoadIncidentFilterCallback = std::function<void(const std::wstring& searchText, int severityIndex)>;

    MapView();
    ~MapView();

    MapView(const MapView&) = delete;
    MapView& operator=(const MapView&) = delete;
    MapView(MapView&&) noexcept;
    MapView& operator=(MapView&&) noexcept;

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND Hwnd() const;
    void SetSelectCallback(SelectCallback cb);
    void SetEventSelectCallback(EventSelectCallback cb);
    void SetEventActionCallback(EventActionCallback cb);
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
    void SetNotificationHistoryDeleteCallback(NotificationHistoryDeleteCallback cb);
    void SetNotificationHistoryActionCallback(NotificationHistoryActionCallback cb);
    void SetChatSendCallback(ChatSendCallback cb);
    void SetPrivateChatSendCallback(PrivateChatSendCallback cb);
    void SetPrivateChatStateCallback(PrivateChatStateCallback cb);
    void SetCountdownPresetsChangedCallback(CountdownPresetsChangedCallback cb);
    void SetChatClearCallback(ChatClearCallback cb);
    void SetChatMessageActionCallback(ChatMessageActionCallback cb);
    void SetUserActionCallback(UserActionCallback cb);
    void SetPanelCloseCallback(PanelCloseCallback cb);
    void SetMapDisplayModeCallback(MapDisplayModeCallback cb);
    void SetIrelandVisibilityCallback(IrelandVisibilityCallback cb);
    void SetOverlayPositionsChangedCallback(OverlayPositionsChangedCallback cb);
    void SetRoadIncidentFilterCallback(RoadIncidentFilterCallback cb);
    void SetChatClearEnabled(bool enabled);
    void SetAlerts(const std::vector<TrafficAlert>& alerts);
    void SetIncidentOverlayVisible(bool visible);
    void SetRoadIncidentPanelAlerts(const std::vector<TrafficAlert>& alerts);
    void SetRoadIncidentPanelFilter(const std::wstring& searchText, int severityIndex);
    void SetRoadIncidentPanelVisible(bool visible);
    void SetNotes(const std::vector<MapNote>& notes);
    void SetChatMessages(const std::vector<ChatMessage>& messages);
    void SetPrivateMessages(const std::vector<PrivateMessage>& messages);
    void SetPrivateMessageUnreadCounts(const std::unordered_map<std::wstring, size_t>& unreadCounts);
    void SetOnlineUsers(const std::vector<OnlineUser>& users);
    void OpenPrivateChat(const OnlineUser& user);
    void SetUsersVisible(bool visible);
    void SetNotificationPolygons(const std::vector<GeoPolygon>& polygons);
    void SetNotificationPolygonsVisible(bool visible);
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
    bool LoadRoadDepictionsFromFile(
        const std::filesystem::path& path,
        std::wstring* errorOut = nullptr,
        const std::unordered_set<std::wstring>* allowedLabels = nullptr);
    std::vector<std::wstring> RoadDepictionLabels() const;
    void SetHiddenRoadDepictions(const std::unordered_set<std::wstring>& hiddenRoadLabels);
    void SetDisplayWorldMap(bool visible);
    void SetIrelandVisible(bool visible);
    void SetFpsCounterVisible(bool visible);
    void SetToolbarVisible(bool visible);
    void SetCountdownVisible(bool visible);
    void SetCommsIndicatorVisible(bool visible);
    void SetOverlayPositions(const MapOverlayPositions& positions);
    MapOverlayPositions OverlayPositions() const;
    void SetCountdownPresets(const std::array<std::wstring, 3>& presets);
    void SetNotificationAvoidanceEnabled(bool enabled);
    void SetActiveNotification(const AppNotification& notification);
    void ClearActiveNotification();
    void SetNotificationHistory(const std::vector<AppNotification>& notifications);
    void SetNotificationHistoryVisible(bool visible);
    void SetSelectedId(const std::wstring& id);
    void CenterOnAlert(const std::wstring& id);
    void FitToPoints(const std::vector<GeoPoint>& points, int singlePointZoom = 8);
    void FitToAlerts();
    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);
    bool LoadWorldBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
