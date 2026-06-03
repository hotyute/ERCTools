#include "populated_places.h"
#include "util.h"

namespace
{
    constexpr double kEarthRadiusMiles = 3958.7613;
    constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

    double LongitudeDelta(double a, double b)
    {
        double delta = std::abs(a - b);
        return delta > 180.0 ? 360.0 - delta : delta;
    }

    bool TryJsonDouble(const json& value, double& out)
    {
        if (value.is_number()) {
            out = value.get<double>();
            return std::isfinite(out);
        }

        if (value.is_string()) {
            if (!TryGetDoubleFromJsonValue(value, out))
                return false;
            return std::isfinite(out);
        }

        return false;
    }

    std::wstring PickPlaceString(const json& props, std::initializer_list<const char*> keys)
    {
        if (!props.is_object())
            return {};

        for (const char* key : keys) {
            auto it = props.find(key);
            if (it == props.end() || !it->is_string())
                continue;
            std::wstring value = Trim(Utf8ToWide(it->get<std::string>()));
            if (!value.empty())
                return value;
        }
        return {};
    }
}

std::vector<PopulatedPlace> ParsePopulatedPlacesGeoJson(const std::string& body, std::wstring* errorOut)
{
    std::vector<PopulatedPlace> out;
    try {
        json root = json::parse(body);
        auto featuresIt = root.find("features");
        if (featuresIt == root.end() || !featuresIt->is_array()) {
            if (errorOut)
                *errorOut = L"Populated places GeoJSON does not contain a features array.";
            return out;
        }

        out.reserve(featuresIt->size());
        for (const auto& feature : *featuresIt) {
            const json& props = feature.value("properties", json::object());
            const json& geometry = feature.value("geometry", json::object());

            double lon = 0.0;
            double lat = 0.0;
            bool hasCoords = false;
            auto coordsIt = geometry.find("coordinates");
            if (coordsIt != geometry.end() && coordsIt->is_array() && coordsIt->size() >= 2) {
                hasCoords = TryJsonDouble((*coordsIt)[0], lon) && TryJsonDouble((*coordsIt)[1], lat);
            }

            if (!hasCoords) {
                auto latIt = props.find("latitude");
                auto lonIt = props.find("longitude");
                hasCoords = latIt != props.end() && lonIt != props.end() &&
                    TryJsonDouble(*latIt, lat) &&
                    TryJsonDouble(*lonIt, lon);
            }

            if (!hasCoords || !std::isfinite(lat) || !std::isfinite(lon) ||
                lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
            {
                continue;
            }

            PopulatedPlace place;
            place.name = PickPlaceString(props, { "nameascii", "name", "namealt" });
            place.country = PickPlaceString(props, { "adm0name", "sov0name", "country" });
            place.latitude = lat;
            place.longitude = lon;

            double pop = 0.0;
            auto popIt = props.find("pop_max");
            if (popIt == props.end())
                popIt = props.find("pop_min");
            if (popIt != props.end() && TryJsonDouble(*popIt, pop))
                place.population = pop;

            if (place.name.empty())
                place.name = L"Populated place";

            out.push_back(std::move(place));
        }

        if (out.empty() && errorOut)
            *errorOut = L"Populated places GeoJSON parsed successfully, but no usable point features were found.";
    }
    catch (const std::exception& e) {
        if (errorOut)
            *errorOut = L"Populated places parse failed: " + Utf8ToWide(e.what());
        out.clear();
    }

    return out;
}

bool LoadPopulatedPlacesFromFile(const std::filesystem::path& path, std::vector<PopulatedPlace>& placesOut, std::wstring* errorOut)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (errorOut)
            *errorOut = L"Populated places cache is not available.";
        return false;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string body = buffer.str();
    if (body.empty()) {
        if (errorOut)
            *errorOut = L"Populated places cache is empty.";
        return false;
    }

    std::vector<PopulatedPlace> parsed = ParsePopulatedPlacesGeoJson(body, errorOut);
    if (parsed.empty())
        return false;

    placesOut = std::move(parsed);
    return true;
}

bool SavePopulatedPlacesToFile(const std::filesystem::path& path, const std::string& body, std::wstring* errorOut)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        if (errorOut)
            *errorOut = L"Could not open populated places cache for writing.";
        return false;
    }

    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    if (!out.good()) {
        if (errorOut)
            *errorOut = L"Could not write populated places cache.";
        return false;
    }

    return true;
}

double HaversineMiles(double lat1, double lon1, double lat2, double lon2)
{
    const double p1 = lat1 * kDegreesToRadians;
    const double p2 = lat2 * kDegreesToRadians;
    const double dp = (lat2 - lat1) * kDegreesToRadians;
    double dl = (lon2 - lon1);
    if (dl > 180.0)
        dl -= 360.0;
    else if (dl < -180.0)
        dl += 360.0;
    dl *= kDegreesToRadians;

    const double a = std::sin(dp / 2.0) * std::sin(dp / 2.0) +
        std::cos(p1) * std::cos(p2) * std::sin(dl / 2.0) * std::sin(dl / 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(MaxValue(0.0, 1.0 - a)));
    return kEarthRadiusMiles * c;
}

bool FindNearestPopulatedPlace(
    const std::vector<PopulatedPlace>& places,
    double latitude,
    double longitude,
    double maxDistanceMiles,
    PopulatedPlace* nearestOut,
    double* distanceMilesOut)
{
    if (places.empty() || maxDistanceMiles <= 0.0 ||
        !std::isfinite(latitude) || !std::isfinite(longitude))
    {
        return false;
    }

    const double latDelta = maxDistanceMiles / 69.0;
    const double cosLat = std::cos(latitude * kDegreesToRadians);
    const double lonScale = 69.0 * MaxValue(0.2, std::abs(cosLat));
    const double lonDelta = maxDistanceMiles / lonScale;

    bool found = false;
    double best = maxDistanceMiles + 0.0001;
    const PopulatedPlace* bestPlace = nullptr;

    for (const PopulatedPlace& place : places) {
        if (std::abs(place.latitude - latitude) > latDelta + 0.0001)
            continue;
        if (LongitudeDelta(place.longitude, longitude) > lonDelta + 0.0001)
            continue;

        const double distance = HaversineMiles(latitude, longitude, place.latitude, place.longitude);
        if (distance <= best) {
            best = distance;
            bestPlace = &place;
            found = true;
        }
    }

    if (!found)
        return false;

    if (nearestOut && bestPlace)
        *nearestOut = *bestPlace;
    if (distanceMilesOut)
        *distanceMilesOut = best;
    return true;
}
