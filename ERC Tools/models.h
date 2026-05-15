// =================================================================================
// FILE: models.h
// =================================================================================


#pragma once
#include "common.h"

struct GeoPoint
{
    double lat = 0.0;
    double lon = 0.0;
};

struct WorldPoint
{
    double x = 0.0;
    double y = 0.0;
};

struct TrafficAlert
{
    std::wstring id;
    std::wstring title;
    std::wstring description;
    std::wstring road;
    std::wstring region;
    std::wstring severity;
    std::wstring eventType;
    std::wstring updatedText;
    std::vector<std::wstring> laneImageUrls;
    std::vector<bool> laneClosedStates;
    int lanesClosed = 0;
    int lanesTotal = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

struct GeoPolygon
{
    std::wstring name;
    std::wstring roadFilter;
    bool allRoads = true;
    std::vector<GeoPoint> points;
};

struct EarthquakeEvent
{
    std::wstring id;
    std::wstring place;
    std::wstring timeText;
    double magnitude = 0.0;
    long long timeMs = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    double depthKm = 0.0;
    bool hasLocation = false;
};

struct AppNotification
{
    std::wstring title;
    std::wstring body;
    std::wstring timestamp;
};

struct ChatMessage
{
    std::wstring author;
    std::wstring text;
    std::wstring timestamp;
};

struct MapNote
{
    std::wstring id;
    std::wstring author;
    std::wstring text;
    std::wstring timestamp;
    double latitude = 0.0;
    double longitude = 0.0;
};
