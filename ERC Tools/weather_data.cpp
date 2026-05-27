// =================================================================================
// FILE: weather_data.cpp
// =================================================================================


#include "weather_data.h"
#include "util.h"

static void ReplaceAllTextLocal(std::wstring& text, const std::wstring& from, const std::wstring& to)
{
    if (from.empty())
        return;

    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::wstring DecodeBasicHtmlEntities(std::wstring text)
{
    ReplaceAllTextLocal(text, L"&amp;", L"&");
    ReplaceAllTextLocal(text, L"&lt;", L"<");
    ReplaceAllTextLocal(text, L"&gt;", L">");
    ReplaceAllTextLocal(text, L"&quot;", L"\"");
    ReplaceAllTextLocal(text, L"&#39;", L"'");
    ReplaceAllTextLocal(text, L"&nbsp;", L" ");
    return text;
}

static std::wstring StripTemplateHtmlTags(const std::wstring& html)
{
    std::wstring text = html;
    text = std::regex_replace(text, std::wregex(LR"(<\s*(?:br|hr)\b[^>]*>)", std::regex_constants::icase), L" ");
    text = std::regex_replace(text, std::wregex(LR"(</\s*(?:p|div|td|tr|li|h[1-6])\s*>)", std::regex_constants::icase), L" ");
    text = std::regex_replace(text, std::wregex(LR"(<[^>]+>)"), L" ");
    text = DecodeBasicHtmlEntities(text);
    std::wstring compact;
    compact.reserve(text.size());
    bool lastSpace = false;
    for (wchar_t ch : text) {
        bool isSpace = iswspace(ch) != 0;
        if (isSpace) {
            if (!lastSpace)
                compact.push_back(L' ');
            lastSpace = true;
        }
        else {
            compact.push_back(ch);
            lastSpace = false;
        }
    }
    return Trim(compact);
}

static bool IsValidMapCoordinate(double lat, double lon)
{
    return std::isfinite(lat) && std::isfinite(lon) &&
        lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

static bool TryParseHemisphereCoordinate(const std::wstring& text, double& valueOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    wchar_t hemisphere = 0;
    if (!value.empty()) {
        wchar_t last = value.back();
        if (last == L'n' || last == L's' || last == L'e' || last == L'w') {
            hemisphere = last;
            value.pop_back();
        }
    }

    value = Trim(value);
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed))
        return false;

    if (hemisphere == L's' || hemisphere == L'w')
        parsed = -parsed;
    valueOut = parsed;
    return true;
}

static bool TryParseKnots(const std::wstring& text, double& knotsOut)
{
    std::wstring value = ToLower(Trim(text));
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed))
        return false;

    knotsOut = parsed;
    return true;
}

static std::vector<std::wstring> ExtractHtmlTableCells(const std::wstring& rowHtml)
{
    std::vector<std::wstring> cells;
    std::wregex cellRe(LR"(<\s*t[hd]\b[^>]*>([\s\S]*?)</\s*t[hd]\s*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(rowHtml.begin(), rowHtml.end(), cellRe), end; it != end; ++it) {
        std::wstring cell = StripTemplateHtmlTags((*it)[1].str());
        if (!cell.empty())
            cells.push_back(std::move(cell));
    }
    return cells;
}

static std::wstring ExtractFirstHref(const std::wstring& html)
{
    std::wsmatch m;
    std::wregex hrefRe(LR"re(href\s*=\s*(?:"([^"]+)"|'([^']+)'|([^>\s]+)))re", std::regex_constants::icase);
    if (!std::regex_search(html, m, hrefRe))
        return L"";
    for (size_t i = 1; i < m.size(); ++i) {
        if (m[i].matched)
            return Trim(DecodeBasicHtmlEntities(m[i].str()));
    }
    return L"";
}

static int ParseLeadHours(const std::wstring& text)
{
    std::wsmatch m;
    std::wregex leadRe(LR"((\d+)\s*(?:hrs?|hours?))", std::regex_constants::icase);
    if (std::regex_search(text, m, leadRe) && m.size() > 1)
        return _wtoi(m[1].str().c_str());
    return 0;
}

struct ForecastImageArea
{
    double x = 0.0;
    double y = 0.0;
    double radiusPx = 0.0;
};

static std::vector<ForecastImageArea> ExtractForecastErrorImageAreas(const std::wstring& html)
{
    std::vector<ForecastImageArea> areas;
    std::wregex areaRe(LR"(<\s*area\b[^>]*>)", std::regex_constants::icase);
    std::wregex coordsRe(LR"re(coords\s*=\s*(?:"([^"]+)"|'([^']+)'|([^>\s]+)))re", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), areaRe), end; it != end; ++it) {
        std::wstring tag = (*it)[0].str();
        if (ToLower(tag).find(L"forecast error") == std::wstring::npos)
            continue;

        std::wsmatch m;
        if (!std::regex_search(tag, m, coordsRe))
            continue;

        std::wstring coords;
        for (size_t i = 1; i < m.size(); ++i) {
            if (m[i].matched) {
                coords = m[i].str();
                break;
            }
        }

        std::vector<double> values;
        std::wregex numberRe(LR"((-?\d+(?:\.\d+)?))");
        for (std::wsregex_iterator nit(coords.begin(), coords.end(), numberRe), nend; nit != nend; ++nit) {
            wchar_t* tail = nullptr;
            double value = std::wcstod((*nit)[1].str().c_str(), &tail);
            if (std::isfinite(value))
                values.push_back(value);
        }

        if (values.size() >= 3)
            areas.push_back({ values[0], values[1], values[2] });
    }
    return areas;
}

static double GreatCircleDistanceNm(double latA, double lonA, double latB, double lonB)
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kEarthRadiusNm = 3440.065;
    const double phi1 = latA * kPi / 180.0;
    const double phi2 = latB * kPi / 180.0;
    const double dPhi = (latB - latA) * kPi / 180.0;
    const double dLambda = (lonB - lonA) * kPi / 180.0;
    const double s1 = std::sin(dPhi * 0.5);
    const double s2 = std::sin(dLambda * 0.5);
    const double a = s1 * s1 + std::cos(phi1) * std::cos(phi2) * s2 * s2;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(MaxValue(0.0, 1.0 - a)));
    return kEarthRadiusNm * c;
}

static void ApplyForecastErrorRadii(std::vector<WeatherForecastPoint>& track, const std::wstring& html)
{
    if (track.empty())
        return;

    std::vector<ForecastImageArea> areas = ExtractForecastErrorImageAreas(html);
    const size_t count = MinValue(track.size(), areas.size());
    if (count == 0)
        return;

    std::vector<double> nmPerPixelSamples;
    for (size_t i = 1; i < count; ++i) {
        if (!track[i - 1].hasLocation || !track[i].hasLocation)
            continue;
        const double dx = areas[i].x - areas[i - 1].x;
        const double dy = areas[i].y - areas[i - 1].y;
        const double pixelDistance = std::sqrt(dx * dx + dy * dy);
        if (pixelDistance < 1.0)
            continue;
        const double nmDistance = GreatCircleDistanceNm(
            track[i - 1].latitude,
            track[i - 1].longitude,
            track[i].latitude,
            track[i].longitude);
        if (nmDistance > 1.0)
            nmPerPixelSamples.push_back(nmDistance / pixelDistance);
    }

    double nmPerPixel = 1.5;
    if (!nmPerPixelSamples.empty()) {
        std::sort(nmPerPixelSamples.begin(), nmPerPixelSamples.end());
        nmPerPixel = nmPerPixelSamples[nmPerPixelSamples.size() / 2];
    }
    nmPerPixel = ClampValue(nmPerPixel, 0.25, 12.0);

    for (size_t i = 0; i < count; ++i) {
        const double radiusNm = areas[i].radiusPx * nmPerPixel;
        if (std::isfinite(radiusNm) && radiusNm > 0.0)
            track[i].errorRadiusNm = radiusNm;
    }
}

static std::wstring ExtractWeatherSystemsStatusText(const std::wstring& htmlText)
{
    std::wstring text = StripTemplateHtmlTags(htmlText);
    std::wsmatch m;
    std::wregex statusRe(LR"(Tropical Storm Tracker:\s*(.*?GMT))", std::regex_constants::icase);
    if (std::regex_search(text, m, statusRe) && m.size() > 1)
        return Trim(m[1].str());
    return L"";
}

std::vector<WeatherSystemEvent> ParseWeatherSystemEvents(const std::string& body, std::wstring& statusTextOut)
{
    std::vector<WeatherSystemEvent> systems;
    std::wstring html = Utf8ToWide(body);
    statusTextOut = ExtractWeatherSystemsStatusText(html);

    std::wregex rowRe(LR"(<\s*tr\b[^>]*>([\s\S]*?)</\s*tr\s*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), rowRe), end; it != end; ++it) {
        std::wstring rowHtml = (*it)[1].str();
        std::vector<std::wstring> cells = ExtractHtmlTableCells(rowHtml);
        if (cells.size() < 10)
            continue;

        std::wstring first = ToLower(Trim(cells[0]));
        if (first == L"system" || first == L"current data")
            continue;

        WeatherSystemEvent system;
        system.name = Trim(cells[0]);
        system.basin = Trim(cells[1]);
        system.windText = Trim(cells[4]);
        system.category = Trim(cells[5]);
        system.forecastWindText = Trim(cells[8]);
        system.forecastCategory = Trim(cells[9]);
        system.updatedText = statusTextOut;
        system.detailPath = ExtractFirstHref(rowHtml);

        double lat = 0.0;
        double lon = 0.0;
        if (TryParseHemisphereCoordinate(cells[2], lat) &&
            TryParseHemisphereCoordinate(cells[3], lon) &&
            IsValidMapCoordinate(lat, lon))
        {
            system.latitude = lat;
            system.longitude = lon;
            system.hasLocation = true;
        }

        double forecastLat = 0.0;
        double forecastLon = 0.0;
        if (TryParseHemisphereCoordinate(cells[6], forecastLat) &&
            TryParseHemisphereCoordinate(cells[7], forecastLon) &&
            IsValidMapCoordinate(forecastLat, forecastLon))
        {
            system.forecastLatitude = forecastLat;
            system.forecastLongitude = forecastLon;
            system.hasForecastLocation = true;
        }

        TryParseKnots(system.windText, system.windKnots);
        TryParseKnots(system.forecastWindText, system.forecastWindKnots);

        if (system.hasLocation) {
            WeatherForecastPoint current;
            current.leadHours = 0;
            current.category = system.category;
            current.windText = system.windText;
            current.windKnots = system.windKnots;
            current.latitude = system.latitude;
            current.longitude = system.longitude;
            current.hasLocation = true;
            system.forecastTrack.push_back(std::move(current));
        }
        if (system.hasForecastLocation) {
            WeatherForecastPoint forecast;
            forecast.leadHours = 24;
            forecast.category = system.forecastCategory;
            forecast.windText = system.forecastWindText;
            forecast.windKnots = system.forecastWindKnots;
            forecast.latitude = system.forecastLatitude;
            forecast.longitude = system.forecastLongitude;
            forecast.hasLocation = true;
            system.forecastTrack.push_back(std::move(forecast));
        }

        system.id = system.name + L"|" + system.basin;
        if (!system.updatedText.empty())
            system.id += L"|" + system.updatedText;

        if (!system.name.empty())
            systems.push_back(std::move(system));
    }

    return systems;
}

std::vector<WeatherForecastPoint> ParseWeatherSystemForecastTrack(const std::string& body)
{
    std::vector<WeatherForecastPoint> track;
    std::wstring html = Utf8ToWide(body);

    std::wregex rowRe(LR"(<\s*tr\b[^>]*>([\s\S]*?)</\s*tr\s*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), rowRe), end; it != end; ++it) {
        std::vector<std::wstring> cells = ExtractHtmlTableCells((*it)[1].str());
        if (cells.size() < 6)
            continue;

        std::wstring first = ToLower(Trim(cells[0]));
        if (first == L"time" || first == L"gmt" || first == L"lat")
            continue;

        WeatherForecastPoint point;
        point.timeText = Trim(cells[0]);
        point.leadHours = ParseLeadHours(cells[1]);
        point.windText = Trim(cells[4]);
        point.category = Trim(cells[5]);
        TryParseKnots(point.windText, point.windKnots);

        if (TryParseHemisphereCoordinate(cells[2], point.latitude) &&
            TryParseHemisphereCoordinate(cells[3], point.longitude) &&
            IsValidMapCoordinate(point.latitude, point.longitude))
        {
            point.hasLocation = true;
        }

        if (point.hasLocation)
            track.push_back(std::move(point));
    }

    std::sort(track.begin(), track.end(), [](const WeatherForecastPoint& a, const WeatherForecastPoint& b) {
        return a.leadHours < b.leadHours;
        });

    track.erase(std::unique(track.begin(), track.end(), [](const WeatherForecastPoint& a, const WeatherForecastPoint& b) {
        return a.leadHours == b.leadHours &&
            std::abs(a.latitude - b.latitude) < 0.001 &&
            std::abs(a.longitude - b.longitude) < 0.001;
        }), track.end());

    ApplyForecastErrorRadii(track, html);
    return track;
}

static std::vector<std::wstring> HtmlToReadableLines(std::wstring html)
{
    html = std::regex_replace(html, std::wregex(LR"(<\s*br\s*/?\s*>)", std::regex_constants::icase), L"\n");
    html = std::regex_replace(html, std::wregex(LR"(</\s*(?:p|div|section|article|li|ul|ol|tr|td|th|h[1-6])\s*>)", std::regex_constants::icase), L"\n");
    html = std::regex_replace(html, std::wregex(LR"(<[^>]+>)"), L" ");
    html = DecodeBasicHtmlEntities(html);

    std::vector<std::wstring> lines;
    std::wstringstream stream(html);
    std::wstring line;
    while (std::getline(stream, line)) {
        line = Trim(std::regex_replace(line, std::wregex(LR"(\s+)"), L" "));
        if (!line.empty())
            lines.push_back(std::move(line));
    }
    return lines;
}

static bool TextContainsNoCase(const std::wstring& text, const std::wstring& needle)
{
    return ToLower(text).find(ToLower(needle)) != std::wstring::npos;
}

static bool TryApproximateUkAreaCentroid(const std::wstring& areaText, double& latOut, double& lonOut)
{
    struct AreaCentroid
    {
        const wchar_t* key;
        double lat;
        double lon;
    };

    static const AreaCentroid kAreas[] = {
        { L"scotland", 56.4907, -4.2026 },
        { L"northern ireland", 54.6079, -6.7080 },
        { L"wales", 52.1307, -3.7837 },
        { L"north east england", 54.9783, -1.6178 },
        { L"north west england", 53.4808, -2.2426 },
        { L"yorkshire", 53.8008, -1.5491 },
        { L"humber", 53.7444, -0.3326 },
        { L"east midlands", 52.9548, -1.1581 },
        { L"west midlands", 52.4862, -1.8904 },
        { L"east of england", 52.2405, 0.7110 },
        { L"london", 51.5074, -0.1278 },
        { L"south east england", 51.2787, -0.5217 },
        { L"south west england", 51.4545, -2.5879 },
        { L"cornwall", 50.2660, -5.0527 },
        { L"devon", 50.7156, -3.5309 },
        { L"somerset", 51.1051, -2.9262 },
        { L"gloucestershire", 51.8642, -2.2382 },
        { L"oxfordshire", 51.7520, -1.2577 },
        { L"buckinghamshire", 51.8072, -0.8128 },
        { L"cambridgeshire", 52.2053, 0.1218 },
        { L"lincolnshire", 53.2307, -0.5406 },
        { L"cheshire", 53.2326, -2.6103 },
        { L"manchester", 53.4808, -2.2426 },
        { L"merseyside", 53.4084, -2.9916 },
        { L"kent", 51.2787, 0.5217 },
        { L"sussex", 50.8225, -0.1372 },
        { L"essex", 51.7343, 0.4691 },
        { L"norfolk", 52.6309, 1.2974 },
        { L"suffolk", 52.1872, 0.9708 },
        { L"cumbria", 54.5772, -2.7975 },
        { L"northumberland", 55.2083, -2.0784 },
        { L"durham", 54.7753, -1.5849 },
        { L"leicestershire", 52.6369, -1.1398 },
        { L"nottinghamshire", 52.9548, -1.1581 },
        { L"derbyshire", 53.1047, -1.5624 },
        { L"staffordshire", 52.8050, -2.1164 },
        { L"warwickshire", 52.2823, -1.5849 },
        { L"worcestershire", 52.1920, -2.2200 },
        { L"herefordshire", 52.0564, -2.7160 },
        { L"shropshire", 52.7064, -2.7418 },
        { L"bristol", 51.4545, -2.5879 },
        { L"bath", 51.3758, -2.3599 }
    };

    std::wstring area = ToLower(areaText);
    for (const AreaCentroid& item : kAreas) {
        if (area.find(item.key) != std::wstring::npos) {
            latOut = item.lat;
            lonOut = item.lon;
            return true;
        }
    }

    return false;
}

static bool IsWeatherTypeLine(const std::wstring& line)
{
    static const wchar_t* kTypes[] = {
        L"rain", L"wind", L"snow", L"ice", L"fog", L"thunderstorm",
        L"lightning", L"extreme heat", L"rain & wind", L"rain and wind"
    };
    std::wstring value = ToLower(Trim(line));
    for (const wchar_t* type : kTypes) {
        if (value == type)
            return true;
    }
    return false;
}

static bool LooksLikeMetOfficeTime(const std::wstring& line)
{
    return std::regex_match(Trim(line), std::wregex(LR"(\d{1,2}:\d{2})"));
}

static std::wstring JoinLimitedText(const std::vector<std::wstring>& values, size_t maxItems)
{
    std::wstring text;
    const size_t count = MinValue(values.size(), maxItems);
    for (size_t i = 0; i < count; ++i) {
        if (!text.empty())
            text += L", ";
        text += values[i];
    }
    if (values.size() > count)
        text += L", ...";
    return text;
}

struct MetOfficeWarningGeometry
{
    std::wstring id;
    std::wstring colour;
    std::wstring type;
    std::vector<GeoPoint> polygon;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

static size_t FindJsonObjectEnd(const std::string& text, size_t openBrace)
{
    bool inString = false;
    bool escaped = false;
    int depth = 0;
    for (size_t i = openBrace; i < text.size(); ++i) {
        const char ch = text[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            }
            else if (ch == '\\') {
                escaped = true;
            }
            else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
        }
        else if (ch == '{') {
            ++depth;
        }
        else if (ch == '}') {
            --depth;
            if (depth == 0)
                return i;
        }
    }
    return std::string::npos;
}

static bool TryExtractCoordinatePair(const json& point, double& latOut, double& lonOut)
{
    if (!point.is_array() || point.size() < 2)
        return false;

    double lon = 0.0;
    double lat = 0.0;
    if (!TryGetDoubleFromJsonValue(point[0], lon) || !TryGetDoubleFromJsonValue(point[1], lat))
        return false;
    if (!IsValidMapCoordinate(lat, lon))
        return false;

    latOut = lat;
    lonOut = lon;
    return true;
}

static bool TryExtractPolygonRing(const json& coords, const std::wstring& type, std::vector<GeoPoint>& ringOut)
{
    const json* ring = nullptr;
    if (type == L"Polygon") {
        if (coords.is_array() && !coords.empty() && coords[0].is_array())
            ring = &coords[0];
    }
    else if (type == L"MultiPolygon") {
        if (coords.is_array() && !coords.empty() && coords[0].is_array() && !coords[0].empty() && coords[0][0].is_array())
            ring = &coords[0][0];
    }

    if (!ring)
        return false;

    std::vector<GeoPoint> points;
    for (const json& item : *ring) {
        double lat = 0.0;
        double lon = 0.0;
        if (TryExtractCoordinatePair(item, lat, lon))
            points.push_back({ lat, lon });
    }

    if (points.size() < 3)
        return false;

    ringOut = std::move(points);
    return true;
}

static bool TryPolygonCentroid(const std::vector<GeoPoint>& ring, double& latOut, double& lonOut)
{
    if (ring.size() < 3)
        return false;

    double signedArea = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    for (size_t i = 0; i < ring.size(); ++i) {
        const GeoPoint& a = ring[i];
        const GeoPoint& b = ring[(i + 1) % ring.size()];
        const double cross = a.lon * b.lat - b.lon * a.lat;
        signedArea += cross;
        cx += (a.lon + b.lon) * cross;
        cy += (a.lat + b.lat) * cross;
    }

    if (std::abs(signedArea) > 1e-9) {
        signedArea *= 0.5;
        lonOut = cx / (6.0 * signedArea);
        latOut = cy / (6.0 * signedArea);
        return IsValidMapCoordinate(latOut, lonOut);
    }

    double latSum = 0.0;
    double lonSum = 0.0;
    for (const GeoPoint& point : ring) {
        latSum += point.lat;
        lonSum += point.lon;
    }
    latOut = latSum / static_cast<double>(ring.size());
    lonOut = lonSum / static_cast<double>(ring.size());
    return IsValidMapCoordinate(latOut, lonOut);
}

static std::vector<MetOfficeWarningGeometry> ExtractMetOfficeWarningGeometries(const std::string& body)
{
    std::vector<MetOfficeWarningGeometry> geometries;
    const std::string marker = "polygonsGeoJson:";
    size_t markerPos = body.find(marker);
    if (markerPos == std::string::npos)
        return geometries;

    size_t openBrace = body.find('{', markerPos + marker.size());
    if (openBrace == std::string::npos)
        return geometries;

    size_t closeBrace = FindJsonObjectEnd(body, openBrace);
    if (closeBrace == std::string::npos || closeBrace <= openBrace)
        return geometries;

    try {
        json root = json::parse(body.substr(openBrace, closeBrace - openBrace + 1));
        auto featuresIt = root.find("features");
        if (featuresIt == root.end() || !featuresIt->is_array())
            return geometries;

        std::unordered_map<std::wstring, size_t> byId;
        for (const json& feature : *featuresIt) {
            if (!feature.is_object())
                continue;

            const json* props = nullptr;
            auto propsIt = feature.find("properties");
            if (propsIt != feature.end() && propsIt->is_object())
                props = &(*propsIt);
            if (!props)
                continue;

            std::wstring id = PickString(*props, { "id" });
            if (id.empty())
                continue;

            size_t index = 0;
            auto existing = byId.find(id);
            if (existing == byId.end()) {
                index = geometries.size();
                byId[id] = index;
                MetOfficeWarningGeometry geometry;
                geometry.id = id;
                geometry.colour = PickString(*props, { "warningLevel" });
                geometry.type = PickString(*props, { "weather" });
                geometries.push_back(std::move(geometry));
            }
            else {
                index = existing->second;
            }

            const json* geom = nullptr;
            auto geomIt = feature.find("geometry");
            if (geomIt != feature.end() && geomIt->is_object())
                geom = &(*geomIt);
            if (!geom)
                continue;

            std::wstring geomType = PickString(*geom, { "type" });
            auto coordsIt = geom->find("coordinates");
            if (coordsIt == geom->end())
                continue;

            MetOfficeWarningGeometry& geometry = geometries[index];
            if (geomType == L"Point") {
                double lat = 0.0;
                double lon = 0.0;
                if (TryExtractCoordinatePair(*coordsIt, lat, lon)) {
                    geometry.latitude = lat;
                    geometry.longitude = lon;
                    geometry.hasLocation = true;
                }
            }
            else if (geometry.polygon.empty()) {
                TryExtractPolygonRing(*coordsIt, geomType, geometry.polygon);
            }
        }

        for (MetOfficeWarningGeometry& geometry : geometries) {
            if (!geometry.hasLocation && TryPolygonCentroid(geometry.polygon, geometry.latitude, geometry.longitude))
                geometry.hasLocation = true;
        }
    }
    catch (...) {
        geometries.clear();
    }

    return geometries;
}

static bool WeatherWarningGeometryMatches(const MetOfficeWarningGeometry& geometry, const WeatherWarningEvent& event)
{
    const std::wstring geometryColour = ToLower(Trim(geometry.colour));
    const std::wstring eventColour = ToLower(Trim(event.colour));
    const std::wstring geometryType = ToLower(Trim(geometry.type));
    const std::wstring eventType = ToLower(Trim(event.type));
    return (geometryColour.empty() || eventColour.empty() || geometryColour == eventColour) &&
        (geometryType.empty() || eventType.empty() || geometryType == eventType);
}

static bool AssignMetOfficeWarningGeometry(
    WeatherWarningEvent& event,
    const std::vector<MetOfficeWarningGeometry>& geometries,
    std::vector<bool>& usedGeometries)
{
    auto assignAt = [&](size_t index) {
        const MetOfficeWarningGeometry& geometry = geometries[index];
        usedGeometries[index] = true;
        if (!geometry.id.empty())
            event.id = geometry.id;
        event.polygon = geometry.polygon;
        if (geometry.hasLocation) {
            event.latitude = geometry.latitude;
            event.longitude = geometry.longitude;
            event.hasLocation = true;
        }
        return true;
        };

    for (size_t i = 0; i < geometries.size(); ++i) {
        if (!usedGeometries[i] && WeatherWarningGeometryMatches(geometries[i], event))
            return assignAt(i);
    }

    for (size_t i = 0; i < geometries.size(); ++i) {
        if (!usedGeometries[i])
            return assignAt(i);
    }

    return false;
}

std::vector<WeatherWarningEvent> ParseWeatherWarningEvents(const std::string& body, std::wstring& statusTextOut)
{
    std::vector<WeatherWarningEvent> warnings;
    std::vector<MetOfficeWarningGeometry> geometries = ExtractMetOfficeWarningGeometries(body);
    std::vector<bool> usedGeometries(geometries.size(), false);
    std::vector<std::wstring> lines = HtmlToReadableLines(Utf8ToWide(body));
    statusTextOut.clear();

    if (lines.empty())
        return warnings;

    auto firstForecastDay = std::find_if(lines.begin(), lines.end(), [](const std::wstring& line) {
        return std::regex_search(line, std::wregex(LR"(\b(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s+\d{1,2}\s+\w{3}\b)", std::regex_constants::icase));
        });
    const size_t start = firstForecastDay == lines.end() ? 0 : static_cast<size_t>(std::distance(lines.begin(), firstForecastDay));

    for (size_t i = start; i + 1 < lines.size(); ++i) {
        std::wstring colour;
        std::wstring lower = ToLower(lines[i]);
        if (lower == L"red warning")
            colour = L"Red";
        else if (lower == L"amber warning")
            colour = L"Amber";
        else if (lower == L"yellow warning")
            colour = L"Yellow";
        else
            continue;

        size_t typeIndex = i + 1;
        while (typeIndex < lines.size() && (lines[typeIndex] == L"x" || lines[typeIndex] == L"\x00d7"))
            ++typeIndex;
        if (typeIndex >= lines.size() || !IsWeatherTypeLine(lines[typeIndex]))
            continue;

        WeatherWarningEvent event;
        event.colour = colour;
        event.type = lines[typeIndex];

        size_t cursor = typeIndex + 1;
        while (cursor < lines.size() && !LooksLikeMetOfficeTime(lines[cursor]))
            ++cursor;
        if (cursor < lines.size()) {
            event.validFrom = lines[cursor];
            if (cursor + 1 < lines.size())
                event.validFrom += L" " + lines[cursor + 1];
            size_t toCursor = cursor + 1;
            while (toCursor < lines.size() && !LooksLikeMetOfficeTime(lines[toCursor]))
                ++toCursor;
            if (toCursor < lines.size()) {
                event.validTo = lines[toCursor];
                if (toCursor + 1 < lines.size())
                    event.validTo += L" " + lines[toCursor + 1];
            }
            cursor = toCursor;
        }

        size_t headlineIndex = cursor;
        while (headlineIndex < lines.size()) {
            const std::wstring lowerHeadline = ToLower(lines[headlineIndex]);
            if (lines[headlineIndex].size() > 20 &&
                lowerHeadline.find(L"what should") == std::wstring::npos &&
                lowerHeadline.find(L"further detail") == std::wstring::npos &&
                lowerHeadline.find(L"why is the warning") == std::wstring::npos)
            {
                event.headline = lines[headlineIndex];
                break;
            }
            ++headlineIndex;
        }

        size_t regionHeaderIndex = lines.size();
        size_t blockEndIndex = lines.size();
        for (size_t j = headlineIndex + 1; j < lines.size(); ++j) {
            std::wstring itemLower = ToLower(lines[j]);
            if (itemLower == L"red warning" || itemLower == L"amber warning" || itemLower == L"yellow warning" ||
                TextContainsNoCase(lines[j], L"Warnings are in force") ||
                TextContainsNoCase(lines[j], L"Website feedback") ||
                TextContainsNoCase(lines[j], L"Follow alerts in the app"))
            {
                blockEndIndex = j;
                break;
            }
            if (TextContainsNoCase(lines[j], L"Regions and local authorities affected")) {
                regionHeaderIndex = j;
                continue;
            }
            if (TextContainsNoCase(lines[j], L"Issued") && j + 1 < lines.size()) {
                event.issuedText = lines[j + 1];
                continue;
            }
        }

        std::vector<std::wstring> areas;
        if (regionHeaderIndex < blockEndIndex) {
            for (size_t j = regionHeaderIndex + 1; j < blockEndIndex; ++j) {
                if (areas.size() >= 24)
                    break;
                if (TextContainsNoCase(lines[j], L"Issued") || TextContainsNoCase(lines[j], L"Warnings are in force"))
                    break;
                if (
                !LooksLikeMetOfficeTime(lines[j]) &&
                lines[j].find(L"UTC") == std::wstring::npos &&
                lines[j].size() >= 4 &&
                lines[j].size() <= 64 &&
                lines[j].find(L".") == std::wstring::npos &&
                lines[j].find(L":") == std::wstring::npos)
                {
                    static const wchar_t* ignored[] = {
                        L"today", L"tomorrow", L"what should i expect?", L"what should i do?",
                        L"further detail", L"issued", L"give us feedback about this warning"
                    };
                    bool skip = false;
                    for (const wchar_t* value : ignored) {
                        if (ToLower(lines[j]) == value) {
                            skip = true;
                            break;
                        }
                    }
                    if (!skip)
                        areas.push_back(lines[j]);
                }
            }
        }

        event.area = JoinLimitedText(areas, 10);
        if (event.headline.empty())
            event.headline = event.colour + L" warning for " + event.type;
        event.detail = event.headline;
        if (!event.area.empty())
            event.detail += L" Affected: " + event.area;

        AssignMetOfficeWarningGeometry(event, geometries, usedGeometries);
        if (!event.hasLocation) {
            const std::wstring mapText = event.area.empty() ? event.headline : event.area;
            event.hasLocation = TryApproximateUkAreaCentroid(mapText, event.latitude, event.longitude);
        }
        if (event.id.empty())
            event.id = event.colour + L"|" + event.type + L"|" + event.validFrom + L"|" + event.validTo + L"|" + event.headline;
        warnings.push_back(std::move(event));
    }

    if (warnings.empty()) {
        auto noWarnings = std::find_if(lines.begin() + static_cast<std::ptrdiff_t>(start), lines.end(), [](const std::wstring& line) {
            return TextContainsNoCase(line, L"No warnings");
            });
        if (noWarnings != lines.end())
            statusTextOut = L"No weather warnings found.";
    }

    return warnings;
}

std::vector<FloodEvent> ParseFloodEvents(const std::string& body, std::wstring& statusTextOut)
{
    std::vector<FloodEvent> floods;
    statusTextOut.clear();

    json root = json::parse(body);
    if (!root.is_object())
        return floods;

    auto itemsIt = root.find("items");
    if (itemsIt == root.end())
        return floods;

    const json* items = &(*itemsIt);
    if (items->is_object()) {
        json single = json::array();
        single.push_back(*items);
        root["__single_items"] = std::move(single);
        items = &root["__single_items"];
    }
    if (!items->is_array())
        return floods;

    for (const json& item : *items) {
        if (!item.is_object())
            continue;

        FloodEvent event;
        event.id = PickString(item, { "@id", "id" });
        event.area = PickString(item, { "description", "label" });
        event.region = PickString(item, { "eaRegionName", "eaAreaName", "county" });
        event.severity = PickString(item, { "severity" });
        event.message = PickString(item, { "message" });
        event.timeRaised = PickString(item, { "timeRaised" });
        event.timeChanged = PickString(item, { "timeMessageChanged", "timeSeverityChanged" });

        double value = 0.0;
        if (PickDouble(item, { "lat", "latitude" }, value))
            event.latitude = value;
        if (PickDouble(item, { "long", "lon", "lng", "longitude" }, value))
            event.longitude = value;

        auto levelIt = item.find("severityLevel");
        if (levelIt != item.end() && levelIt->is_number_integer())
            event.severityLevel = levelIt->get<int>();

        auto areaIt = item.find("floodArea");
        if (areaIt != item.end() && areaIt->is_object()) {
            if (event.region.empty())
                event.region = PickString(*areaIt, { "county", "eaRegionName", "eaAreaName" });
            event.riverOrSea = PickString(*areaIt, { "riverOrSea" });
            if (event.area.empty())
                event.area = PickString(*areaIt, { "description", "label" });
            if (!std::isfinite(event.latitude) || !std::isfinite(event.longitude) ||
                (event.latitude == 0.0 && event.longitude == 0.0))
            {
                if (PickDouble(*areaIt, { "lat", "latitude" }, value))
                    event.latitude = value;
                if (PickDouble(*areaIt, { "long", "lon", "lng", "longitude" }, value))
                    event.longitude = value;
            }
        }

        event.hasLocation = IsValidMapCoordinate(event.latitude, event.longitude);
        if (!event.hasLocation) {
            std::wstring areaText = event.area + L" " + event.region + L" " + event.riverOrSea;
            event.hasLocation = TryApproximateUkAreaCentroid(areaText, event.latitude, event.longitude);
        }

        if (event.id.empty())
            event.id = event.area + L"|" + event.severity + L"|" + event.timeChanged;
        if (!event.area.empty() || !event.severity.empty())
            floods.push_back(std::move(event));
    }

    if (floods.empty())
        statusTextOut = L"No flood warnings found.";
    return floods;
}
