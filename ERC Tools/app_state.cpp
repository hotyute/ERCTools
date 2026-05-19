// =================================================================================
// FILE: app_state.cpp
// =================================================================================


#include "app_state.h"

const int kMinZoom = 2;
const int kMaxZoom = 19;
const int kDefaultZoom = 6;

const double kDefaultCenterLat = 53.0;
const double kDefaultCenterLon = -1.5;
const double kMaxMercatorLat = 85.05112878;
const double kPi = 3.14159265358979323846;

const wchar_t kMainClassName[] = L"TrafficEnglandNativeMainWindow";
const wchar_t kMapClassName[] = L"TrafficEnglandNativeMapView";
const wchar_t kUkBoundarySourceUrl[] =
L"https://github.com/wmgeolab/geoBoundaries/raw/main/releaseData/gbOpen/GBR/ADM0/geoBoundaries-GBR-ADM0.geojson";

std::atomic_bool g_boundaryDownloadInProgress{ false };
std::atomic_bool g_appQuitting{ false };
std::atomic_bool g_fetchInProgress{ false };
std::atomic_bool g_weatherSystemsFetchInProgress{ false };
ComPtr<ID2D1Factory> g_d2dFactory;
ComPtr<IWICImagingFactory> g_wicFactory;
ComPtr<IDWriteFactory> g_dwriteFactory;
