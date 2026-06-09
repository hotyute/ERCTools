// =================================================================================
// FILE: road_data.cpp
// =================================================================================

#include "road_data.h"
#include "util.h"

namespace
{
    void SetPointBounds(
        const std::vector<GeoPoint>& points,
        double& minLat,
        double& maxLat,
        double& minLon,
        double& maxLon)
    {
        if (points.empty()) {
            minLat = maxLat = minLon = maxLon = 0.0;
            return;
        }

        minLat = maxLat = points.front().lat;
        minLon = maxLon = points.front().lon;
        for (const GeoPoint& point : points) {
            minLat = MinValue(minLat, point.lat);
            maxLat = MaxValue(maxLat, point.lat);
            minLon = MinValue(minLon, point.lon);
            maxLon = MaxValue(maxLon, point.lon);
        }
    }

    std::vector<GeoPoint> SimplifyRoadLine(const std::vector<GeoPoint>& source, double toleranceDegrees)
    {
        if (source.size() < 4 || toleranceDegrees <= 0.0)
            return source;

        const double toleranceSq = toleranceDegrees * toleranceDegrees;
        std::vector<GeoPoint> simplified;
        simplified.reserve(source.size());
        simplified.push_back(source.front());

        GeoPoint lastKept = source.front();
        for (size_t i = 1; i + 1 < source.size(); ++i) {
            const GeoPoint& pt = source[i];
            const double dLat = pt.lat - lastKept.lat;
            const double dLon = pt.lon - lastKept.lon;
            if (dLat * dLat + dLon * dLon >= toleranceSq) {
                simplified.push_back(pt);
                lastKept = pt;
            }
        }

        simplified.push_back(source.back());
        return simplified.size() >= 2 ? simplified : source;
    }

    void PrepareRouteRenderData(RoadDepictionRoute& route)
    {
        route.normalizedLabel = ToLower(Trim(route.label));

        SetPointBounds(route.points, route.minLat, route.maxLat, route.minLon, route.maxLon);

        route.farPoints = SimplifyRoadLine(route.points, 0.018);
        SetPointBounds(route.farPoints, route.farMinLat, route.farMaxLat, route.farMinLon, route.farMaxLon);

        route.midPoints = SimplifyRoadLine(route.points, 0.006);
        SetPointBounds(route.midPoints, route.midMinLat, route.midMaxLat, route.midMinLon, route.midMaxLon);

        route.nearPoints = SimplifyRoadLine(route.points, 0.0015);
        SetPointBounds(route.nearPoints, route.nearMinLat, route.nearMaxLat, route.nearMinLon, route.nearMaxLon);
    }

    RoadDepictionRoute MakeRoute(const std::wstring& label, std::vector<GeoPoint> points)
    {
        RoadDepictionRoute route;
        route.label = label;
        route.points = std::move(points);
        PrepareRouteRenderData(route);
        return route;
    }

    bool RoadLabelAllowed(const std::wstring& label, const std::unordered_set<std::wstring>* allowedLabels)
    {
        if (!allowedLabels || allowedLabels->empty())
            return true;
        return allowedLabels->find(label) != allowedLabels->end() ||
            allowedLabels->find(ToLower(label)) != allowedLabels->end();
    }

    std::wstring PickRoadProperty(const json& properties, std::initializer_list<const char*> keys)
    {
        if (!properties.is_object())
            return {};

        for (const char* key : keys) {
            auto it = properties.find(key);
            if (it == properties.end())
                continue;

            std::wstring value = Trim(JsonValueToText(*it));
            if (!value.empty() && ToLower(value) != L"null")
                return value;
        }
        return {};
    }

    std::wstring RoadLabelFromFeature(const json& feature)
    {
        const json* properties = nullptr;
        auto propsIt = feature.find("properties");
        if (propsIt != feature.end())
            properties = &*propsIt;
        else if (feature.is_object())
            properties = &feature;

        if (!properties)
            return {};

        std::wstring label = PickRoadProperty(*properties, {
            "roadNumber",
            "road_number",
            "roadClassificationNumber",
            "road_classification_number",
            "RoadNumber",
            "Road_Number",
            "roadName",
            "road_name",
            "roadName1",
            "road_name_1",
            "name1",
            "name",
            "street",
            "identifier",
            "localIdentifier"
            });

        label = Trim(label);
        if (label.empty())
            return {};

        while (label.find(L"  ") != std::wstring::npos)
            label.replace(label.find(L"  "), 2, L" ");
        return label;
    }

    bool TryReadCoordinate(const json& coord, GeoPoint& pointOut)
    {
        if (!coord.is_array() || coord.size() < 2)
            return false;
        if (!coord[0].is_number() || !coord[1].is_number())
            return false;

        const double lon = coord[0].get<double>();
        const double lat = coord[1].get<double>();
        if (!std::isfinite(lat) || !std::isfinite(lon))
            return false;
        if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
            return false;

        pointOut = { lat, lon };
        return true;
    }

    std::vector<GeoPoint> ReadLineString(const json& coordinates)
    {
        std::vector<GeoPoint> points;
        if (!coordinates.is_array())
            return points;

        points.reserve(coordinates.size());
        for (const json& coord : coordinates) {
            GeoPoint point;
            if (TryReadCoordinate(coord, point))
                points.push_back(point);
        }

        if (points.size() < 2)
            points.clear();
        return points;
    }

    void CollectGeometryLines(const json& geometry, std::vector<std::vector<GeoPoint>>& linesOut)
    {
        if (!geometry.is_object())
            return;

        const std::string type = PickString(geometry, { "type" }).empty()
            ? std::string()
            : WideToUtf8(PickString(geometry, { "type" }));
        auto coordsIt = geometry.find("coordinates");

        if (type == "LineString" && coordsIt != geometry.end()) {
            std::vector<GeoPoint> line = ReadLineString(*coordsIt);
            if (!line.empty())
                linesOut.push_back(std::move(line));
            return;
        }

        if (type == "MultiLineString" && coordsIt != geometry.end() && coordsIt->is_array()) {
            for (const json& lineCoords : *coordsIt) {
                std::vector<GeoPoint> line = ReadLineString(lineCoords);
                if (!line.empty())
                    linesOut.push_back(std::move(line));
            }
            return;
        }

        if (type == "GeometryCollection") {
            auto geometriesIt = geometry.find("geometries");
            if (geometriesIt != geometry.end() && geometriesIt->is_array()) {
                for (const json& child : *geometriesIt)
                    CollectGeometryLines(child, linesOut);
            }
        }
    }

    void CollectFeatureLines(
        const json& feature,
        std::vector<RoadDepictionRoute>& routesOut,
        const std::unordered_set<std::wstring>* allowedLabels)
    {
        std::wstring label = RoadLabelFromFeature(feature);
        if (label.empty())
            return;
        if (!RoadLabelAllowed(label, allowedLabels))
            return;

        auto geometryIt = feature.find("geometry");
        if (geometryIt == feature.end())
            return;

        std::vector<std::vector<GeoPoint>> lines;
        CollectGeometryLines(*geometryIt, lines);
        for (auto& line : lines) {
            if (line.size() >= 2)
                routesOut.push_back(MakeRoute(label, std::move(line)));
        }
    }

    void CollectRoadRoutesRecursive(
        const json& node,
        std::vector<RoadDepictionRoute>& routesOut,
        const std::unordered_set<std::wstring>* allowedLabels)
    {
        if (node.is_object()) {
            std::wstring type = PickString(node, { "type" });
            if (type == L"Feature") {
                CollectFeatureLines(node, routesOut, allowedLabels);
                return;
            }

            auto featuresIt = node.find("features");
            if (featuresIt != node.end() && featuresIt->is_array()) {
                for (const json& feature : *featuresIt)
                    CollectRoadRoutesRecursive(feature, routesOut, allowedLabels);
                return;
            }

            auto geometriesIt = node.find("geometries");
            if (geometriesIt != node.end() && geometriesIt->is_array()) {
                for (const json& geometry : *geometriesIt)
                    CollectRoadRoutesRecursive(geometry, routesOut, allowedLabels);
            }
            return;
        }

        if (node.is_array()) {
            for (const json& item : node)
                CollectRoadRoutesRecursive(item, routesOut, allowedLabels);
        }
    }
}

std::vector<RoadDepictionRoute> BuiltInRoadDepictionRoutes()
{
    std::vector<RoadDepictionRoute> routes;
    routes.reserve(9);
    routes.push_back(MakeRoute(L"M25", { {51.66, -0.45}, {51.69, -0.05}, {51.60, 0.29}, {51.36, 0.20}, {51.25, -0.16}, {51.34, -0.55}, {51.55, -0.57}, {51.66, -0.45} }));
    routes.push_back(MakeRoute(L"M1", { {51.55, -0.42}, {52.04, -0.76}, {52.59, -1.13}, {53.02, -1.30}, {53.46, -1.39}, {53.80, -1.55} }));
    routes.push_back(MakeRoute(L"M6", { {52.49, -1.89}, {52.74, -2.02}, {53.01, -2.18}, {53.39, -2.60}, {53.75, -2.72}, {54.05, -2.80}, {54.89, -2.94}, {55.00, -3.06} }));
    routes.push_back(MakeRoute(L"M4", { {51.50, -0.42}, {51.45, -1.00}, {51.45, -1.49}, {51.56, -2.24}, {51.54, -3.05}, {51.62, -3.94} }));
    routes.push_back(MakeRoute(L"M5", { {52.49, -1.89}, {52.19, -2.22}, {51.86, -2.24}, {51.45, -2.59}, {51.02, -3.10}, {50.72, -3.53} }));
    routes.push_back(MakeRoute(L"M62", { {53.41, -2.99}, {53.48, -2.24}, {53.67, -1.50}, {53.74, -0.33}, {53.77, -0.10} }));
    routes.push_back(MakeRoute(L"A1(M)", { {51.88, -0.21}, {52.14, -0.32}, {52.57, -0.25}, {53.14, -0.67}, {53.53, -1.12}, {54.34, -1.43}, {54.97, -1.62} }));
    routes.push_back(MakeRoute(L"A14", { {52.40, -1.00}, {52.38, -0.72}, {52.33, -0.19}, {52.25, 0.15}, {52.06, 1.16} }));
    routes.push_back(MakeRoute(L"A27", { {50.82, -1.09}, {50.84, -0.78}, {50.82, -0.37}, {50.83, -0.14}, {50.82, 0.14}, {50.77, 0.29} }));
    return routes;
}

bool LoadRoadDepictionRoutesFromGeoJson(
    const std::filesystem::path& path,
    std::vector<RoadDepictionRoute>& routesOut,
    std::wstring* errorOut,
    const std::unordered_set<std::wstring>* allowedLabels)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (errorOut)
            *errorOut = L"Could not open road depictions file: " + path.wstring();
        return false;
    }

    json root;
    try {
        in >> root;
    }
    catch (const std::exception& e) {
        if (errorOut)
            *errorOut = L"Road depictions JSON parse failed: " + Utf8ToWide(e.what());
        return false;
    }

    std::vector<RoadDepictionRoute> routes;
    CollectRoadRoutesRecursive(root, routes, allowedLabels);
    if (routes.empty()) {
        if (errorOut)
            *errorOut = L"No named LineString or MultiLineString roads found in GeoJSON.";
        return false;
    }

    std::sort(routes.begin(), routes.end(), [](const auto& a, const auto& b) {
        if (a.label != b.label)
            return a.label < b.label;
        if (a.minLat != b.minLat)
            return a.minLat < b.minLat;
        return a.minLon < b.minLon;
        });

    routesOut = std::move(routes);
    return true;
}
