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

struct WeatherSystemEvent
{
    std::wstring id;
    std::wstring name;
    std::wstring basin;
    std::wstring category;
    std::wstring windText;
    std::wstring forecastCategory;
    std::wstring forecastWindText;
    std::wstring updatedText;
    double windKnots = 0.0;
    double forecastWindKnots = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double forecastLatitude = 0.0;
    double forecastLongitude = 0.0;
    bool hasLocation = false;
    bool hasForecastLocation = false;
};

struct WeatherWarningEvent
{
    std::wstring id;
    std::wstring colour;
    std::wstring type;
    std::wstring headline;
    std::wstring area;
    std::wstring validFrom;
    std::wstring validTo;
    std::wstring issuedText;
    std::wstring detail;
    std::vector<GeoPoint> polygon;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

struct FloodEvent
{
    std::wstring id;
    std::wstring severity;
    std::wstring area;
    std::wstring region;
    std::wstring riverOrSea;
    std::wstring message;
    std::wstring timeRaised;
    std::wstring timeChanged;
    int severityLevel = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

struct AppNotificationLink
{
    std::wstring text;
    std::wstring sourceType;
    std::wstring sourceId;
};

struct AppNotification
{
    std::wstring title;
    std::wstring body;
    std::wstring timestamp;
    std::wstring sourceType;
    std::wstring sourceId;
    std::vector<AppNotificationLink> links;
};

struct ChatMessage
{
    std::wstring author;
    std::wstring position;
    std::wstring text;
    std::wstring timestamp;
};

struct OnlineUser
{
    std::wstring displayName;
    std::wstring username;
    std::wstring position;
    std::wstring pod;
    std::wstring lastSeen;
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
