// =================================================================================
// FILE: road_data.h
// =================================================================================

#pragma once
#include "core/common.h"
#include "core/models.h"

struct RoadDepictionRoute
{
    std::wstring label;
    std::wstring normalizedLabel;
    std::vector<GeoPoint> points;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
    std::vector<GeoPoint> farPoints;
    double farMinLat = 0.0;
    double farMaxLat = 0.0;
    double farMinLon = 0.0;
    double farMaxLon = 0.0;
    std::vector<GeoPoint> midPoints;
    double midMinLat = 0.0;
    double midMaxLat = 0.0;
    double midMinLon = 0.0;
    double midMaxLon = 0.0;
    std::vector<GeoPoint> nearPoints;
    double nearMinLat = 0.0;
    double nearMaxLat = 0.0;
    double nearMinLon = 0.0;
    double nearMaxLon = 0.0;
};

std::vector<RoadDepictionRoute> BuiltInRoadDepictionRoutes();
bool LoadRoadDepictionRoutesFromGeoJson(
    const std::filesystem::path& path,
    std::vector<RoadDepictionRoute>& routesOut,
    std::wstring* errorOut = nullptr,
    const std::unordered_set<std::wstring>* allowedLabels = nullptr);
