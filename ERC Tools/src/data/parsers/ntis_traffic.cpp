// =================================================================================
// FILE: ntis_traffic.cpp
// =================================================================================

#include "data/parsers/ntis_traffic.h"

#include "data/parsers/parsing.h"
#include "core/util.h"

bool ParseNtisTrafficSnapshot(
    const std::string& text,
    bool includePlanned,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    alertsOut.clear();
    statusOut.clear();
    errorOut.clear();

    if (text.empty()) {
        errorOut = L"The NTIS event snapshot is empty.";
        return false;
    }

    try {
        const json root = json::parse(text);
        if (!root.is_object() || !root.contains("alerts") || !root["alerts"].is_array()) {
            errorOut = L"The NTIS event snapshot does not contain an alerts array.";
            return false;
        }
        if (root.value("refreshInProgress", false)) {
            errorOut = L"The NTIS event snapshot is still being rebuilt.";
            return false;
        }

        std::unordered_set<std::wstring> seenIds;
        for (const json& item : root["alerts"]) {
            if (!item.is_object())
                continue;
            if (!includePlanned && item.value("planned", false))
                continue;

            TrafficAlert alert = ParseAlertObject(item);
            if (alert.id.empty() || !seenIds.insert(alert.id).second)
                continue;
            alertsOut.push_back(std::move(alert));
        }

        const unsigned long long generation = root.value("generation", 0ull);
        statusOut = L"National Highways NTIS: " + std::to_wstring(alertsOut.size()) +
            L" current incident(s)";
        if (generation > 0)
            statusOut += L" (snapshot " + std::to_wstring(generation) + L")";
        statusOut += L".";
        return true;
    }
    catch (const std::exception& error) {
        errorOut = L"Could not parse the NTIS event snapshot: " + Utf8ToWide(error.what());
        return false;
    }
}
