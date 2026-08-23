#include "data/parsers/traffic_scotland.h"

#include "net/http.h"
#include "data/parsers/parsing.h"
#include "core/util.h"

namespace
{
struct ScotlandListItem
{
    std::wstring sid;
    std::wstring location;
    std::wstring direction;
    std::wstring type;
    std::wstring startTime;
    std::wstring description;
};

struct ScotlandDetail
{
    std::wstring updatedText;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasLocation = false;
};

struct ScotlandDetailCacheEntry
{
    ScotlandDetail detail;
    ULONGLONG fetchedAt = 0;
};

constexpr ULONGLONG kScotlandDetailCacheMs = 60ULL * 1000ULL;

std::wstring ExtractParagraphValue(
    const std::wstring& block,
    const std::wstring& label,
    size_t startAt = 0)
{
    const std::wstring marker = L">" + label + L"</span>";
    size_t markerPos = block.find(marker, startAt);
    if (markerPos == std::wstring::npos)
        return L"";
    size_t valueStart = markerPos + marker.size();
    size_t valueEnd = block.find(L"</p>", valueStart);
    if (valueEnd == std::wstring::npos)
        return L"";
    return StripHtmlTags(block.substr(valueStart, valueEnd - valueStart));
}

std::wstring ExtractRoad(const std::wstring& location)
{
    std::wsmatch match;
    if (std::regex_search(location, match, std::wregex(LR"(\b((?:M|A)\d+(?:\(M\))?)\b)", std::regex_constants::icase)))
        return match[1].str();
    return L"";
}

std::wstring SeverityFor(const ScotlandListItem& item)
{
    const std::wstring combined = ToLower(item.type + L" " + item.description);
    if (combined.find(L"closed") != std::wstring::npos ||
        combined.find(L"collision") != std::wstring::npos ||
        combined.find(L"accident") != std::wstring::npos)
        return L"Severe";
    if (combined.find(L"queue") != std::wstring::npos ||
        combined.find(L"restricted") != std::wstring::npos ||
        combined.find(L"breakdown") != std::wstring::npos)
        return L"Moderate";
    if (combined.find(L"roadwork") != std::wstring::npos)
        return L"Minor";
    return L"Unknown";
}

std::vector<ScotlandListItem> ParseList(const std::wstring& html)
{
    std::vector<ScotlandListItem> items;
    const std::wstring headingMarker = L"<h2 class=\"qsf\">";
    size_t headingPos = 0;
    while ((headingPos = html.find(headingMarker, headingPos)) != std::wstring::npos) {
        const size_t nextHeading = html.find(headingMarker, headingPos + headingMarker.size());
        const size_t blockEnd = nextHeading == std::wstring::npos ? html.size() : nextHeading;
        const std::wstring block = html.substr(headingPos, blockEnd - headingPos);

        const size_t headingValueStart = headingPos + headingMarker.size();
        const size_t headingValueEnd = html.find(L"</h2>", headingValueStart);
        if (headingValueEnd == std::wstring::npos || headingValueEnd >= blockEnd)
            break;

        ScotlandListItem item;
        item.location = StripHtmlTags(html.substr(headingValueStart, headingValueEnd - headingValueStart));
        item.direction = ExtractParagraphValue(block, L"Direction:");
        item.type = ExtractParagraphValue(block, L"Incident type:");
        item.startTime = ExtractParagraphValue(block, L"Start time:");

        const size_t startLabel = block.find(L">Start time:</span>");
        const size_t startParagraphEnd = startLabel == std::wstring::npos
            ? std::wstring::npos
            : block.find(L"</p>", startLabel);
        if (startParagraphEnd != std::wstring::npos) {
            const size_t descriptionStart = block.find(L"<p>", startParagraphEnd);
            const size_t descriptionEnd = descriptionStart == std::wstring::npos
                ? std::wstring::npos
                : block.find(L"</p>", descriptionStart + 3);
            if (descriptionStart != std::wstring::npos && descriptionEnd != std::wstring::npos)
                item.description = StripHtmlTags(block.substr(descriptionStart + 3, descriptionEnd - descriptionStart - 3));
        }

        const std::wstring sidMarker = L"/more-details?sid=";
        const size_t sidStartMarker = block.find(sidMarker);
        if (sidStartMarker != std::wstring::npos) {
            const size_t sidStart = sidStartMarker + sidMarker.size();
            const size_t sidEnd = block.find(L'&', sidStart);
            if (sidEnd != std::wstring::npos)
                item.sid = HtmlDecode(block.substr(sidStart, sidEnd - sidStart));
        }
        if (!item.sid.empty() && !item.location.empty())
            items.push_back(std::move(item));

        if (nextHeading == std::wstring::npos)
            break;
        headingPos = nextHeading;
    }
    return items;
}

std::wstring ExtractDetailLabelValue(const std::wstring& html, const std::wstring& label)
{
    const std::wstring lowerHtml = ToLower(html);
    const size_t labelPos = lowerHtml.find(ToLower(label));
    if (labelPos == std::wstring::npos)
        return L"";

    const size_t labelEnd = lowerHtml.find(L"</span>", labelPos);
    if (labelEnd == std::wstring::npos)
        return L"";
    const size_t valueTag = lowerHtml.find(L"<span", labelEnd + 7);
    if (valueTag == std::wstring::npos)
        return L"";
    const size_t valueStart = lowerHtml.find(L'>', valueTag);
    if (valueStart == std::wstring::npos)
        return L"";
    const size_t valueEnd = lowerHtml.find(L"</span>", valueStart + 1);
    if (valueEnd == std::wstring::npos)
        return L"";
    return StripHtmlTags(html.substr(valueStart + 1, valueEnd - valueStart - 1));
}

bool ParseDetail(const std::string& body, ScotlandDetail& detailOut)
{
    detailOut = {};
    const std::wstring html = Utf8ToWide(body);
    detailOut.updatedText = ExtractDetailLabelValue(html, L"Last updated");

    std::wsmatch match;
    const std::wregex centerRegex(
        LR"(center\s*:\s*\{\s*lat\s*:\s*(-?\d+(?:\.\d+)?)\s*,\s*lng\s*:\s*(-?\d+(?:\.\d+)?)\s*\})",
        std::regex_constants::icase);
    if (std::regex_search(html, match, centerRegex) && match.size() >= 3) {
        try {
            detailOut.latitude = std::stod(match[1].str());
            detailOut.longitude = std::stod(match[2].str());
            detailOut.hasLocation = detailOut.latitude >= -90.0 && detailOut.latitude <= 90.0 &&
                detailOut.longitude >= -180.0 && detailOut.longitude <= 180.0;
        }
        catch (...) {
            detailOut.hasLocation = false;
        }
    }

    return detailOut.hasLocation || !detailOut.updatedText.empty();
}

bool FetchDetail(const std::wstring& sid, ScotlandDetail& detailOut)
{
    static std::mutex cacheMutex;
    static std::unordered_map<std::wstring, ScotlandDetailCacheEntry> cache;
    const ULONGLONG now = GetTickCount64();
    ScotlandDetail cachedDetail;
    bool hasCachedDetail = false;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto found = cache.find(sid);
        if (found != cache.end()) {
            cachedDetail = found->second.detail;
            hasCachedDetail = true;
            if (now - found->second.fetchedAt <= kScotlandDetailCacheMs) {
                detailOut = cachedDetail;
                return detailOut.hasLocation || !detailOut.updatedText.empty();
            }
        }
    }

    std::string body;
    std::wstring error;
    const std::wstring url = L"https://www.traffic.gov.scot/more-details?sid=" + sid + L"&type=incidents";
    ScotlandDetail fetchedDetail;
    if (!HttpGetText(url, body, error) || !ParseDetail(body, fetchedDetail)) {
        if (hasCachedDetail) {
            detailOut = cachedDetail;
            return detailOut.hasLocation || !detailOut.updatedText.empty();
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(cacheMutex);
    cache[sid] = { fetchedDetail, now };
    detailOut = std::move(fetchedDetail);
    return true;
}
}

bool FetchTrafficScotlandAlerts(
    const TrafficScotlandOptions& options,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    alertsOut.clear();
    statusOut.clear();
    errorOut.clear();
    if (!options.enabled)
        return true;

    std::string body;
    if (!HttpGetText(options.incidentsUrl, body, errorOut))
        return false;

    const std::vector<ScotlandListItem> items = ParseList(Utf8ToWide(body));
    if (items.empty()) {
        errorOut = L"Traffic Scotland returned no recognisable incident records.";
        return false;
    }

    alertsOut.resize(items.size());
    for (size_t i = 0; i < items.size(); ++i) {
        const ScotlandListItem& item = items[i];
        TrafficAlert& alert = alertsOut[i];
        alert.id = L"traffic-scotland:" + item.sid;
        alert.road = ExtractRoad(item.location);
        alert.region = L"Scotland";
        alert.title = item.type.empty() ? L"Traffic incident" : item.type;
        alert.eventType = item.type;
        alert.severity = SeverityFor(item);
        alert.updatedText = item.startTime;
        alert.description = L"Location: " + item.location;
        if (!item.direction.empty())
            alert.description += L"\r\nDirection: " + item.direction;
        if (!item.startTime.empty())
            alert.description += L"\r\nStart time: " + item.startTime;
        if (!item.description.empty())
            alert.description += L"\r\n" + item.description;

    }

    std::atomic_size_t nextIndex{ 0 };
    std::atomic_size_t located{ 0 };
    const size_t workerCount = std::min<size_t>(6, items.size());
    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (size_t worker = 0; worker < workerCount; ++worker) {
        workers.emplace_back([&]() {
            for (;;) {
                const size_t index = nextIndex.fetch_add(1);
                if (index >= items.size())
                    break;
                TrafficAlert& alert = alertsOut[index];
                ScotlandDetail detail;
                FetchDetail(items[index].sid, detail);
                alert.hasLocation = detail.hasLocation;
                if (detail.hasLocation) {
                    alert.latitude = detail.latitude;
                    alert.longitude = detail.longitude;
                }
                if (!detail.updatedText.empty()) {
                    alert.updatedText = detail.updatedText;
                    alert.description += L"\r\nLast updated: " + detail.updatedText;
                }
                if (alert.hasLocation)
                    located.fetch_add(1);
            }
            });
    }
    for (std::thread& worker : workers)
        worker.join();

    statusOut = L"Traffic Scotland: " + std::to_wstring(alertsOut.size()) +
        L" incident(s), " + std::to_wstring(located.load()) + L" mapped.";
    return true;
}

bool ParseTrafficScotlandAlertsFromBodies(
    const std::string& listBody,
    const std::unordered_map<std::wstring, std::string>& detailBodies,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut)
{
    alertsOut.clear();
    statusOut.clear();
    errorOut.clear();

    const std::vector<ScotlandListItem> items = ParseList(Utf8ToWide(listBody));
    if (items.empty()) {
        errorOut = L"Traffic Scotland returned no recognisable incident records.";
        return false;
    }

    alertsOut.resize(items.size());
    size_t located = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        const ScotlandListItem& item = items[i];
        TrafficAlert& alert = alertsOut[i];
        alert.id = L"traffic-scotland:" + item.sid;
        alert.road = ExtractRoad(item.location);
        alert.region = L"Scotland";
        alert.title = item.type.empty() ? L"Traffic incident" : item.type;
        alert.eventType = item.type;
        alert.severity = SeverityFor(item);
        alert.updatedText = item.startTime;
        alert.description = L"Location: " + item.location;
        if (!item.direction.empty())
            alert.description += L"\r\nDirection: " + item.direction;
        if (!item.startTime.empty())
            alert.description += L"\r\nStart time: " + item.startTime;
        if (!item.description.empty())
            alert.description += L"\r\n" + item.description;

        auto detail = detailBodies.find(item.sid);
        if (detail != detailBodies.end()) {
            ScotlandDetail parsedDetail;
            ParseDetail(detail->second, parsedDetail);
            alert.hasLocation = parsedDetail.hasLocation;
            if (parsedDetail.hasLocation) {
                alert.latitude = parsedDetail.latitude;
                alert.longitude = parsedDetail.longitude;
            }
            if (!parsedDetail.updatedText.empty()) {
                alert.updatedText = parsedDetail.updatedText;
                alert.description += L"\r\nLast updated: " + parsedDetail.updatedText;
            }
            if (alert.hasLocation)
                ++located;
        }
    }

    statusOut = L"Traffic Scotland: " + std::to_wstring(alertsOut.size()) +
        L" incident(s), " + std::to_wstring(located) + L" mapped.";
    return true;
}
