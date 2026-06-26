#pragma once

#include "core/models.h"

#include <unordered_map>

struct TrafficScotlandOptions
{
    bool enabled = true;
    std::wstring incidentsUrl = L"https://www.traffic.gov.scot/traffic-information/incidents";
};

bool ParseTrafficScotlandAlertsFromBodies(
    const std::string& listBody,
    const std::unordered_map<std::wstring, std::string>& detailBodies,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut);

bool FetchTrafficScotlandAlerts(
    const TrafficScotlandOptions& options,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut);
