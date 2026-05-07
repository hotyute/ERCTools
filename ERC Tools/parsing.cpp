#include "parsing.h"
#include "util.h"

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
bool ExtractRingFromCoords(const json& coords, std::vector<GeoPoint>& ring)
{
    ring.clear();

    if (!coords.is_array())
        return false;

    for (const auto& pos : coords) {
        if (!pos.is_array() || pos.size() < 2)
            continue;

        double lon = 0.0;
        double lat = 0.0;

        if (!TryGetDoubleFromJsonValue(pos[0], lon))
            continue;
        if (!TryGetDoubleFromJsonValue(pos[1], lat))
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
                TryGetDoubleFromJsonValue(coords[0], lon);
                TryGetDoubleFromJsonValue(coords[1], lat);
                hasLat = true;
                hasLon = true;
            }
        }
    }

    if (hasLat && hasLon) {
        a.latitude = lat;
        a.longitude = lon;
        a.hasLocation = true;
    }

    std::atomic<unsigned long long> s_idCounter{ 0 };
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
