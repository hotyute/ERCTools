// =================================================================================
// FILE: earthquake_data.cpp
// =================================================================================

#include "data/parsers/earthquake_data.h"
#include "core/util.h"

std::wstring EarthquakeTimeText(long long timeMs)
{
    if (timeMs <= 0)
        return L"";

    std::time_t t = static_cast<std::time_t>(timeMs / 1000LL);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buf[64]{};
    wcsftime(buf, _countof(buf), L"%Y-%m-%d %H:%M", &tm);
    return buf;
}

std::vector<EarthquakeEvent> ParseEarthquakeEvents(const std::string& body)
{
    std::vector<EarthquakeEvent> out;
    json root = json::parse(body);
    if (!root.is_object())
        return out;

    auto featuresIt = root.find("features");
    if (featuresIt == root.end() || !featuresIt->is_array())
        return out;

    for (const json& feature : *featuresIt) {
        if (!feature.is_object())
            continue;

        const json* propsPtr = nullptr;
        const json* geomPtr = nullptr;
        auto propsIt = feature.find("properties");
        if (propsIt != feature.end() && propsIt->is_object())
            propsPtr = &(*propsIt);
        auto geomIt = feature.find("geometry");
        if (geomIt != feature.end() && geomIt->is_object())
            geomPtr = &(*geomIt);
        const json& props = propsPtr ? *propsPtr : feature;
        const json& geom = geomPtr ? *geomPtr : feature;

        EarthquakeEvent event;
        event.id = PickString(feature, { "id" });
        event.place = PickString(props, { "place", "title" });
        PickDouble(props, { "mag", "magnitude" }, event.magnitude);
        event.timeMs = props.value("time", 0LL);
        event.timeText = EarthquakeTimeText(event.timeMs);

        auto coordsIt = geom.find("coordinates");
        if (coordsIt != geom.end() && coordsIt->is_array() && coordsIt->size() >= 2) {
            TryGetDoubleFromJsonValue((*coordsIt)[0], event.longitude);
            TryGetDoubleFromJsonValue((*coordsIt)[1], event.latitude);
            if (coordsIt->size() >= 3)
                TryGetDoubleFromJsonValue((*coordsIt)[2], event.depthKm);
            event.hasLocation = std::isfinite(event.latitude) && std::isfinite(event.longitude);
        }

        if (event.id.empty())
            event.id = event.place + L"|" + std::to_wstring(event.timeMs);
        out.push_back(std::move(event));
    }

    return out;
}
