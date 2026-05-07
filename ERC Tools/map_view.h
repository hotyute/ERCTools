#pragma once
#include "common.h"
#include "models.h"

class MapView
{
public:
    using SelectCallback = std::function<void(const std::wstring& id)>;

    MapView();
    ~MapView();

    MapView(const MapView&) = delete;
    MapView& operator=(const MapView&) = delete;
    MapView(MapView&&) noexcept;
    MapView& operator=(MapView&&) noexcept;

    bool Create(HWND parent, int x, int y, int w, int h);
    HWND Hwnd() const;
    void SetSelectCallback(SelectCallback cb);
    void SetAlerts(const std::vector<TrafficAlert>& alerts);
    void SetSelectedId(const std::wstring& id);
    void CenterOnAlert(const std::wstring& id);
    void FitToAlerts();
    bool LoadUkBoundaryFromFile(const std::filesystem::path& path, std::wstring* errorOut = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
