// =================================================================================
// FILE: app_state.h
// =================================================================================


#pragma once
#include "common.h"

inline constexpr UINT WM_APP_FEED_READY = WM_APP + 1;
inline constexpr UINT WM_APP_TILE_READY = WM_APP + 2;
inline constexpr UINT WM_APP_BOUNDARY_READY = WM_APP + 3;
inline constexpr UINT WM_APP_SERVER_READY = WM_APP + 4;
inline constexpr UINT WM_APP_EARTHQUAKE_READY = WM_APP + 5;

extern const int kMinZoom;
extern const int kMaxZoom;
extern const int kDefaultZoom;

extern const double kDefaultCenterLat;
extern const double kDefaultCenterLon;
extern const double kMaxMercatorLat;
extern const double kPi;

extern const wchar_t kMainClassName[];
extern const wchar_t kMapClassName[];
extern const wchar_t kUkBoundarySourceUrl[];

extern std::atomic_bool g_boundaryDownloadInProgress;
extern std::atomic_bool g_appQuitting;
extern std::atomic_bool g_fetchInProgress;
extern ComPtr<ID2D1Factory> g_d2dFactory;
extern ComPtr<IWICImagingFactory> g_wicFactory;
extern ComPtr<IDWriteFactory> g_dwriteFactory;
