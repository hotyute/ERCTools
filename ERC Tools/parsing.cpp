#include "parsing.h"
#include "util.h"
#include "app_state.h"

std::wstring HtmlDecode(const std::wstring& text)
{
    std::wstring out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'&') {
            size_t sem = text.find(L';', i + 1);
            if (sem != std::wstring::npos && (sem - i) <= 12) {
                std::wstring ent = text.substr(i + 1, sem - i - 1);
                std::wstring low = ToLower(ent);

                if (low == L"amp") out.push_back(L'&');
                else if (low == L"lt") out.push_back(L'<');
                else if (low == L"gt") out.push_back(L'>');
                else if (low == L"quot") out.push_back(L'"');
                else if (low == L"apos" || low == L"#39") out.push_back(L'\'');
                else if (low == L"nbsp") out.push_back(L' ');
                else if (low.size() > 2 && low[0] == L'#') {
                    int code = 0;
                    if (low.size() > 3 && low[1] == L'x') {
                        code = static_cast<int>(wcstol(low.c_str() + 2, nullptr, 16));
                    }
                    else {
                        code = static_cast<int>(wcstol(low.c_str() + 1, nullptr, 10));
                    }

                    if (code > 0)
                        out.push_back(static_cast<wchar_t>(code));
                }
                else {
                    out += L"&";
                    out += ent;
                    out += L";";
                }

                i = sem;
                continue;
            }
        }

        out.push_back(text[i]);
    }

    return out;
}

std::wstring StripHtmlTags(std::wstring text)
{
    text = std::regex_replace(
        text,
        std::wregex(LR"(<\s*br\s*/?\s*>)", std::regex_constants::icase),
        L"\n");

    text = std::regex_replace(
        text,
        std::wregex(LR"(<[^>]+>)", std::regex_constants::icase),
        L" ");

    text = HtmlDecode(text);

    std::wstring out;
    out.reserve(text.size());

    bool lastSpace = false;

    for (wchar_t ch : text) {
        if (ch == L'\r')
            continue;

        if (ch == L'\n') {
            if (!out.empty() && out.back() != L'\n')
                out.push_back(L'\n');
            lastSpace = false;
            continue;
        }

        if (iswspace(ch)) {
            if (!lastSpace) {
                out.push_back(L' ');
                lastSpace = true;
            }
        }
        else {
            out.push_back(ch);
            lastSpace = false;
        }
    }

    return Trim(out);
}


static bool IsValidLatLon(double lat, double lon)
{
    return std::isfinite(lat) && std::isfinite(lon) &&
        lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

static bool ConvertBritishNationalGridToLatLon(double easting, double northing, double& latOut, double& lonOut)
{
    if (!std::isfinite(easting) || !std::isfinite(northing) ||
        easting < 0.0 || easting > 700000.0 || northing < 0.0 || northing > 1300000.0)
        return false;

    constexpr double a = 6377563.396;
    constexpr double b = 6356256.909;
    constexpr double f0 = 0.9996012717;
    const double lat0 = 49.0 * kPi / 180.0;
    const double lon0 = -2.0 * kPi / 180.0;
    constexpr double n0 = -100000.0;
    constexpr double e0 = 400000.0;
    constexpr double e2 = 1.0 - (b * b) / (a * a);
    constexpr double n = (a - b) / (a + b);

    double lat = lat0;
    double m = 0.0;
    do {
        lat = (northing - n0 - m) / (a * f0) + lat;
        double ma = (1.0 + n + 5.0 / 4.0 * n * n + 5.0 / 4.0 * n * n * n) * (lat - lat0);
        double mb = (3.0 * n + 3.0 * n * n + 21.0 / 8.0 * n * n * n) * std::sin(lat - lat0) * std::cos(lat + lat0);
        double mc = (15.0 / 8.0 * n * n + 15.0 / 8.0 * n * n * n) * std::sin(2.0 * (lat - lat0)) * std::cos(2.0 * (lat + lat0));
        double md = 35.0 / 24.0 * n * n * n * std::sin(3.0 * (lat - lat0)) * std::cos(3.0 * (lat + lat0));
        m = b * f0 * (ma - mb + mc - md);
    } while (std::abs(northing - n0 - m) >= 0.00001);

    const double sinLat = std::sin(lat);
    const double cosLat = std::cos(lat);
    const double tanLat = std::tan(lat);
    const double nu = a * f0 / std::sqrt(1.0 - e2 * sinLat * sinLat);
    const double rho = a * f0 * (1.0 - e2) / std::pow(1.0 - e2 * sinLat * sinLat, 1.5);
    const double eta2 = nu / rho - 1.0;
    const double dE = easting - e0;

    const double vii = tanLat / (2.0 * rho * nu);
    const double viii = tanLat / (24.0 * rho * std::pow(nu, 3.0)) * (5.0 + 3.0 * tanLat * tanLat + eta2 - 9.0 * tanLat * tanLat * eta2);
    const double ix = tanLat / (720.0 * rho * std::pow(nu, 5.0)) * (61.0 + 90.0 * tanLat * tanLat + 45.0 * std::pow(tanLat, 4.0));
    const double x = 1.0 / (cosLat * nu);
    const double xi = 1.0 / (6.0 * cosLat * std::pow(nu, 3.0)) * (nu / rho + 2.0 * tanLat * tanLat);
    const double xii = 1.0 / (120.0 * cosLat * std::pow(nu, 5.0)) * (5.0 + 28.0 * tanLat * tanLat + 24.0 * std::pow(tanLat, 4.0));
    const double xiia = 1.0 / (5040.0 * cosLat * std::pow(nu, 7.0)) * (61.0 + 662.0 * tanLat * tanLat + 1320.0 * std::pow(tanLat, 4.0) + 720.0 * std::pow(tanLat, 6.0));

    lat = lat - vii * dE * dE + viii * std::pow(dE, 4.0) - ix * std::pow(dE, 6.0);
    double lon = lon0 + x * dE - xi * std::pow(dE, 3.0) + xii * std::pow(dE, 5.0) - xiia * std::pow(dE, 7.0);

    // Airy 1830 / OSGB36 to WGS84 Helmert transform.
    double h = 0.0;
    double sinPhi = std::sin(lat), cosPhi = std::cos(lat), sinLam = std::sin(lon), cosLam = std::cos(lon);
    double nuA = a / std::sqrt(1.0 - e2 * sinPhi * sinPhi);
    double x1 = (nuA + h) * cosPhi * cosLam;
    double y1 = (nuA + h) * cosPhi * sinLam;
    double z1 = ((1.0 - e2) * nuA + h) * sinPhi;

    constexpr double tx = 446.448, ty = -125.157, tz = 542.060;
    constexpr double sppm = 20.4894;
    const double rx = 0.1502 * kPi / (180.0 * 3600.0);
    const double ry = 0.2470 * kPi / (180.0 * 3600.0);
    const double rz = 0.8421 * kPi / (180.0 * 3600.0);
    double scale = 1.0 + sppm * 1e-6;
    double x2 = tx + x1 * scale - y1 * rz + z1 * ry;
    double y2 = ty + x1 * rz + y1 * scale - z1 * rx;
    double z2 = tz - x1 * ry + y1 * rx + z1 * scale;

    constexpr double a2 = 6378137.000;
    constexpr double b2 = 6356752.3141;
    constexpr double e22 = 1.0 - (b2 * b2) / (a2 * a2);
    double p = std::sqrt(x2 * x2 + y2 * y2);
    double phi = std::atan2(z2, p * (1.0 - e22));
    double phiPrev;
    do {
        phiPrev = phi;
        double nu2 = a2 / std::sqrt(1.0 - e22 * std::sin(phi) * std::sin(phi));
        phi = std::atan2(z2 + e22 * nu2 * std::sin(phi), p);
    } while (std::abs(phi - phiPrev) > 1e-12);

    latOut = phi * 180.0 / kPi;
    lonOut = std::atan2(y2, x2) * 180.0 / kPi;
    return IsValidLatLon(latOut, lonOut);
}

static bool NormalizeCoordinatePair(double first, double second, double& latOut, double& lonOut, bool geoJsonOrder)
{
    double candidateLat = geoJsonOrder ? second : first;
    double candidateLon = geoJsonOrder ? first : second;
    if (IsValidLatLon(candidateLat, candidateLon)) {
        latOut = candidateLat;
        lonOut = candidateLon;
        return true;
    }

    if (geoJsonOrder) {
        if (ConvertBritishNationalGridToLatLon(first, second, latOut, lonOut))
            return true;
        if (ConvertBritishNationalGridToLatLon(second, first, latOut, lonOut))
            return true;
    }
    else {
        if (ConvertBritishNationalGridToLatLon(second, first, latOut, lonOut))
            return true;
        if (ConvertBritishNationalGridToLatLon(first, second, latOut, lonOut))
            return true;
    }

    return false;
}

std::vector<TrafficAlert> ParseHtmlTrafficAlerts(const std::wstring& html)
{
    std::vector<TrafficAlert> out;

    std::wregex rowRe(LR"(<tr\b[^>]*>([\s\S]*?)</tr>)", std::regex_constants::icase);
    std::wregex cellRe(LR"(<t[dh]\b[^>]*>([\s\S]*?)</t[dh]>)", std::regex_constants::icase);

    size_t idCounter = 0;

    for (std::wsregex_iterator rowIt(html.begin(), html.end(), rowRe), rowEnd;
        rowIt != rowEnd;
        ++rowIt)
    {
        std::wstring rowHtml = (*rowIt)[1].str();

        std::vector<std::wstring> cells;
        for (std::wsregex_iterator cellIt(rowHtml.begin(), rowHtml.end(), cellRe), cellEnd;
            cellIt != cellEnd;
            ++cellIt)
        {
            cells.push_back(StripHtmlTags((*cellIt)[1].str()));
        }

        if (cells.size() < 4)
            continue;

        std::wstring road = Trim(cells[0]);
        std::wstring type = Trim(cells[1]);
        std::wstring severity = Trim(cells[2]);
        std::wstring description = Trim(cells[3]);

        // Skip the table header row
        if (ToLower(road) == L"road" && ToLower(type) == L"type")
            continue;

        if (road.empty() || description.empty())
            continue;

        TrafficAlert a;
        a.id = L"html-" + std::to_wstring(++idCounter);
        a.road = road;
        a.title = type.empty() ? L"Traffic alert" : type;
        a.severity = severity.empty() ? L"Unknown" : severity;
        a.description = description;
        a.updatedText = L"";
        a.region = L"";
        a.hasLocation = false;

        out.push_back(std::move(a));
    }

    return out;
}

bool ExtractRingFromCoords(const json& coords, std::vector<GeoPoint>& ring)
{
    ring.clear();

    if (!coords.is_array())
        return false;

    for (const auto& pos : coords) {
        if (!pos.is_array() || pos.size() < 2)
            continue;

        double first = 0.0;
        double second = 0.0;
        double lat = 0.0;
        double lon = 0.0;

        if (!TryGetDoubleFromJsonValue(pos[0], first))
            continue;
        if (!TryGetDoubleFromJsonValue(pos[1], second))
            continue;
        if (!NormalizeCoordinatePair(first, second, lat, lon, true))
            continue;

        ring.push_back({ lat, lon });
    }

    if (ring.size() >= 3) {
        const GeoPoint& a = ring.front();
        const GeoPoint& b = ring.back();
        if (std::abs(a.lat - b.lat) < 1e-12 && std::abs(a.lon - b.lon) < 1e-12)
            ring.pop_back();
    }

    return ring.size() >= 3;
}

void CollectBoundaryRingsFromGeometry(const json& geom, std::vector<std::vector<GeoPoint>>& rings)
{
    if (!geom.is_object())
        return;

    const std::string type = geom.value("type", "");
    auto coordIt = geom.find("coordinates");
    if (coordIt == geom.end() || !coordIt->is_array())
        return;

    if (type == "Polygon") {
        for (const auto& ringCoords : *coordIt) {
            std::vector<GeoPoint> ring;
            if (ExtractRingFromCoords(ringCoords, ring))
                rings.push_back(std::move(ring));
        }
    }
    else if (type == "MultiPolygon") {
        for (const auto& polygon : *coordIt) {
            if (!polygon.is_array())
                continue;

            for (const auto& ringCoords : polygon) {
                std::vector<GeoPoint> ring;
                if (ExtractRingFromCoords(ringCoords, ring))
                    rings.push_back(std::move(ring));
            }
        }
    }
}

void CollectBoundaryRingsFromNode(const json& node, std::vector<std::vector<GeoPoint>>& rings)
{
    if (node.is_array()) {
        for (const auto& item : node)
            CollectBoundaryRingsFromNode(item, rings);
        return;
    }

    if (!node.is_object())
        return;

    const std::string type = node.value("type", "");

    if (type == "FeatureCollection") {
        auto it = node.find("features");
        if (it != node.end() && it->is_array()) {
            for (const auto& feature : *it)
                CollectBoundaryRingsFromNode(feature, rings);
        }
        return;
    }

    if (type == "Feature") {
        auto it = node.find("geometry");
        if (it != node.end() && it->is_object())
            CollectBoundaryRingsFromGeometry(*it, rings);
        return;
    }

    if (node.contains("geometry") && node["geometry"].is_object())
        CollectBoundaryRingsFromGeometry(node["geometry"], rings);
    else
        CollectBoundaryRingsFromGeometry(node, rings);
}

TrafficAlert ParseAlertObject(const json& obj)
{
    TrafficAlert a;

    const json* props = &obj;
    if (obj.contains("properties") && obj["properties"].is_object())
        props = &obj["properties"];

    a.id = PickString(*props, { "id", "incidentId", "alertId", "uuid", "eventId", "eventID", "event_id" });
    if (a.id.empty())
        a.id = PickString(obj, { "id", "incidentId", "alertId", "uuid", "eventId", "eventID", "event_id" });

    a.title = PickString(*props, { "title", "headline", "summary", "name", "eventType", "type", "event_type" });
    if (a.title.empty())
        a.title = PickString(obj, { "title", "headline", "summary", "name", "eventType", "type", "event_type" });

    a.description = PickString(*props, { "description", "details", "message", "fullText", "eventDescription", "event_description", "comment" });
    if (a.description.empty())
        a.description = PickString(obj, { "description", "details", "message", "fullText", "eventDescription", "event_description", "comment" });

    a.road = PickString(*props, { "road", "roadName", "route", "roadNumber", "road_number" });
    if (a.road.empty())
        a.road = PickString(obj, { "road", "roadName", "route", "roadNumber", "road_number" });

    a.region = PickString(*props, { "region", "area", "county", "district", "location" });
    if (a.region.empty())
        a.region = PickString(obj, { "region", "area", "county", "district", "location" });

    a.severity = PickString(*props, { "severity", "impact", "level", "priority", "severityId", "severity_id" });
    if (a.severity.empty())
        a.severity = PickString(obj, { "severity", "impact", "level", "priority", "severityId", "severity_id" });

    a.updatedText = PickDateText(*props, { "updated", "lastUpdated", "lastUpdatedTime", "timestamp", "created", "published", "last_update" });
    if (a.updatedText.empty())
        a.updatedText = PickDateText(obj, { "updated", "lastUpdated", "lastUpdatedTime", "timestamp", "created", "published", "last_update" });

    double lat = 0.0;
    double lon = 0.0;
    bool coordinatePairIsGeoJson = false;
    bool hasLat = PickDouble(*props, { "latitude", "lat", "y", "latitudeDecimal" }, lat);
    bool hasLon = PickDouble(*props, { "longitude", "lon", "lng", "long", "x", "longitudeDecimal" }, lon);

    if (!(hasLat && hasLon)) {
        hasLat = PickDouble(obj, { "latitude", "lat", "y", "latitudeDecimal" }, lat);
        hasLon = PickDouble(obj, { "longitude", "lon", "lng", "long", "x", "longitudeDecimal" }, lon);
    }

    if (!(hasLat && hasLon) && obj.contains("geometry") && obj["geometry"].is_object()) {
        const json& geom = obj["geometry"];
        std::wstring geomType = PickString(geom, { "type" });
        if (ToLower(geomType) == L"point" &&
            geom.contains("coordinates") &&
            geom["coordinates"].is_array())
        {
            const json& coords = geom["coordinates"];
            if (coords.size() >= 2) {
                if (TryGetDoubleFromJsonValue(coords[0], lon) && TryGetDoubleFromJsonValue(coords[1], lat)) {
                    hasLat = true;
                    hasLon = true;
                    coordinatePairIsGeoJson = true;
                }
            }
        }
    }

    if (hasLat && hasLon) {
        double fixedLat = 0.0;
        double fixedLon = 0.0;
        double first = coordinatePairIsGeoJson ? lon : lat;
        double second = coordinatePairIsGeoJson ? lat : lon;
        if (NormalizeCoordinatePair(first, second, fixedLat, fixedLon, coordinatePairIsGeoJson)) {
            a.latitude = fixedLat;
            a.longitude = fixedLon;
            a.hasLocation = true;
        }
    }

    static std::atomic<unsigned long long> s_idCounter{ 0 };
    if (a.id.empty())
        a.id = L"alert-" + std::to_wstring(++s_idCounter);

    if (a.title.empty())
        a.title = L"Traffic alert";

    if (a.severity.empty())
        a.severity = L"Unknown";

    return a;
}

std::vector<TrafficAlert> ParseTrafficAlerts(const std::string& text, std::wstring& errorOut)
{
    errorOut.clear();

    if (text.empty()) {
        errorOut = L"Empty response.";
        return {};
    }

    // Try JSON / GeoJSON first
    try {
        json root = json::parse(text);

        std::vector<TrafficAlert> out;
        bool recognized = false;

        auto addIfObject = [&](const json& item)
            {
                if (item.is_object())
                    out.push_back(ParseAlertObject(item));
            };

        auto addArray = [&](const json& arr)
            {
                recognized = true;
                for (const auto& item : arr)
                    addIfObject(item);
            };

        auto addArrayByKeys = [&](const json& obj, std::initializer_list<const char*> keys)
            {
                for (const char* key : keys) {
                    if (obj.contains(key) && obj[key].is_array()) {
                        addArray(obj[key]);
                        return true;
                    }
                }
                return false;
            };

        if (root.is_array()) {
            addArray(root);
        }
        else if (root.is_object()) {
            if (addArrayByKeys(root, { "features", "alerts", "data", "events", "items", "results", "rows", "aaData" })) {
                // Recognized and added above.
            }
            else if (root.contains("data") && root["data"].is_object() &&
                addArrayByKeys(root["data"], { "events", "alerts", "items", "results", "rows" }))
            {
                // Recognized and added above.
            }
            else if (root.contains("geometry") ||
                root.contains("latitude") ||
                root.contains("lat") ||
                root.contains("title") ||
                root.contains("description") ||
                root.contains("headline") ||
                root.contains("eventType") ||
                root.contains("roadNumber"))
            {
                recognized = true;
                out.push_back(ParseAlertObject(root));
            }
        }

        if (recognized && !out.empty())
            return out;

        if (recognized && out.empty())
            errorOut = L"JSON parsed, but no alerts were found.";
    }
    catch (...) {
        // Not JSON, so try HTML below
    }

    // HTML table fallback
    std::wstring html = Utf8ToWide(text);
    std::vector<TrafficAlert> htmlAlerts = ParseHtmlTrafficAlerts(html);

    if (!htmlAlerts.empty())
        return htmlAlerts;

    if (errorOut.empty())
        errorOut = L"Could not parse the response as JSON or as an HTML alerts table.";

    return {};
}

std::vector<TrafficAlert> SampleAlerts()
{
    std::vector<TrafficAlert> out;
    std::time_t now = std::time(nullptr);

    out.push_back({
        L"sample-1",
        L"Queueing traffic on the M1",
        L"Slow traffic northbound due to congestion.",
        L"M1",
        L"East Midlands",
        L"Moderate",
        TimeTToText(now - 300),
        52.0570, -0.7550, true
        });

    out.push_back({
        L"sample-2",
        L"Lane closed on the M25",
        L"One lane is closed for roadworks.",
        L"M25",
        L"Greater London",
        L"Severe",
        TimeTToText(now - 420),
        51.6090, -0.4300, true
        });

    out.push_back({
        L"sample-3",
        L"Incident on A1",
        L"Delays likely near the junction.",
        L"A1",
        L"North East",
        L"Minor",
        TimeTToText(now - 180),
        54.9700, -1.6170, true
        });

    return out;
}
