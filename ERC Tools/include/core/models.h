// =================================================================================
// FILE: models.h
// =================================================================================


#pragma once
#include "core/common.h"

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
    bool excluded = false;
    bool unresolved = false;
    bool networkResolved = false;
    bool trafficEnglandEligible = true;
    bool trafficEnglandUnplanned = true;
    bool trafficEnglandVisible = true;
};

struct IncidentExclusion
{
    std::wstring key;
    std::wstring sourceId;
    std::wstring source;
    std::wstring road;
    std::wstring summary;
    std::wstring addedBy;
    std::wstring addedAt;
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
    bool excluded = false;
};

struct WeatherForecastPoint
{
    std::wstring timeText;
    std::wstring category;
    std::wstring windText;
    int leadHours = 0;
    double windKnots = 0.0;
    double errorRadiusNm = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
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
    std::wstring detailPath;
    std::vector<WeatherForecastPoint> forecastTrack;
    double windKnots = 0.0;
    double forecastWindKnots = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double forecastLatitude = 0.0;
    double forecastLongitude = 0.0;
    bool hasLocation = false;
    bool hasForecastLocation = false;
    bool excluded = false;
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
    bool excluded = false;
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
    bool excluded = false;
};

struct AppNotificationLink
{
    std::wstring text;
    std::wstring sourceType;
    std::wstring sourceId;
    bool excluded = false;
};

struct AppNotification
{
    std::wstring title;
    std::wstring body;
    std::wstring timestamp;
    std::wstring sourceType;
    std::wstring sourceId;
    std::vector<AppNotificationLink> links;
    bool excluded = false;
};

struct ChatMessage
{
    std::wstring id;
    std::wstring author;
    std::wstring username;
    std::wstring position;
    std::wstring text;
    std::wstring timestamp;
};

struct OnlineUser
{
    std::wstring id;
    std::wstring displayName;
    std::wstring username;
    std::wstring position;
    std::wstring pod;
    std::wstring lastSeen;
};

struct PrivateMessage
{
    std::wstring id;
    std::wstring senderUsername;
    std::wstring senderDisplayName;
    std::wstring senderPosition;
    std::wstring recipientUsername;
    std::wstring recipientDisplayName;
    std::wstring recipientPosition;
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
