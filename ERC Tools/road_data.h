// =================================================================================
// FILE: road_data.h
// =================================================================================

#pragma once
#include "common.h"
#include "models.h"

struct RoadDepictionRoute
{
    std::wstring label;
    std::vector<GeoPoint> points;
    double minLat = 0.0;
    double maxLat = 0.0;
    double minLon = 0.0;
    double maxLon = 0.0;
};

std::vector<RoadDepictionRoute> BuiltInRoadDepictionRoutes();
bool LoadRoadDepictionRoutesFromGeoJson(
    const std::filesystem::path& path,
    std::vector<RoadDepictionRoute>& routesOut,
    std::wstring* errorOut = nullptr,
    const std::unordered_set<std::wstring>* allowedLabels = nullptr);
