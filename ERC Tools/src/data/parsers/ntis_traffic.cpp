// =================================================================================
// FILE: ntis_traffic.cpp
// =================================================================================

#include "data/parsers/ntis_traffic.h"

#include "data/parsers/parsing.h"
#include "core/util.h"

namespace
{
std::wstring TrafficEnglandUpdatedText(const std::wstring& value)
{
    if (value.size() >= 19 &&
        value[4] == L'-' && value[7] == L'-' &&
        (value[10] == L'T' || value[10] == L' ') &&
        value[13] == L':' && value[16] == L':')
    {
        return value.substr(0, 10) + L" " + value.substr(11, 8);
    }
    return value;
}
}

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
        if (root.value("sourceStale", false)) {
            statusOut = L"National Highways NTIS event feed is stale; no incidents are being shown.";
            const std::wstring sourceUpdated = TrafficEnglandUpdatedText(
                Utf8ToWide(root.value("sourceUpdatedAt", std::string())));
            if (!sourceUpdated.empty())
                statusOut += L" Last complete snapshot: " + sourceUpdated + L".";
            return true;
        }

        std::unordered_set<std::wstring> seenIds;
        for (const json& item : root["alerts"]) {
            if (!item.is_object())
                continue;
            TrafficAlert alert = ParseAlertObject(item);
            alert.updatedText = TrafficEnglandUpdatedText(alert.updatedText);
            if (!includePlanned && !alert.trafficEnglandUnplanned)
                continue;
            if (alert.id.empty() || !seenIds.insert(alert.id).second)
                continue;
            alertsOut.push_back(std::move(alert));
        }

        const unsigned long long generation = root.value("generation", 0ull);
        const size_t publicCount = root.value("trafficEnglandPublicCount", alertsOut.size());
        const size_t currentRecordCount = root.value("currentRecordCount", alertsOut.size());
        statusOut = L"National Highways NTIS: " + std::to_wstring(publicCount) +
            L" Traffic England-compatible public incident(s)";
        if (currentRecordCount != publicCount)
            statusOut += L" from " + std::to_wstring(currentRecordCount) + L" current NTIS record(s)";
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
