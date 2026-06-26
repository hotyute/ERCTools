// =================================================================================
// FILE: parsing.cpp
// =================================================================================


#include "data/parsers/parsing.h"
#include "core/util.h"
#include "app/app_state.h"

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




static bool IsGenericAlertTitle(const std::wstring& title)
{
    std::wstring normalized = ToLower(Trim(title));
    return normalized.empty() ||
        normalized == L"traffic alert" ||
        normalized == L"traffic alerts" ||
        normalized == L"title alert" ||
        normalized == L"alert";
}

static std::wstring NormalizeAlertDescription(const std::wstring& description)
{
    if (description.find(L'<') != std::wstring::npos && description.find(L'>') != std::wstring::npos)
        return StripHtmlTags(description);

    return Trim(description);
}

static json::const_iterator FindJsonKeyInsensitive(const json& obj, const char* key);
static bool TryGetArrayByKeys(const json& obj, std::initializer_list<const char*> keys, const json*& arrOut);

static std::wstring ExtractLabeledAlertField(const std::wstring& description, const wchar_t* label)
{
    std::wstring normalized = NormalizeAlertDescription(description);
    if (normalized.empty())
        return L"";

    std::wstring pattern = LR"((?:^|[\r\n])\s*)";
    pattern += label;
    pattern += LR"(\s*:\s*([^\r\n]+))";

    std::wsmatch m;
    std::wregex lineRe(pattern, std::regex_constants::icase);
    if (std::regex_search(normalized, m, lineRe) && m.size() > 1)
        return Trim(m[1].str());

    // Be tolerant of descriptions whose <br> separators were already collapsed to spaces.
    static const wchar_t* kKnownLabels =
        L"From Location|To Location|Location|Reason|Status|Time To Clear|Return To Normal|Lanes Closed|Delay";
    std::wstring inlinePattern = LR"((?:^|\s))";
    inlinePattern += label;
    inlinePattern += LR"(\s*:\s*(.*?)(?=\s+(?:)";
    inlinePattern += kKnownLabels;
    inlinePattern += LR"()\s*:|$))";

    std::wregex inlineRe(inlinePattern, std::regex_constants::icase);
    if (std::regex_search(normalized, m, inlineRe) && m.size() > 1)
        return Trim(m[1].str());

    return L"";
}


static std::wstring MakeTrafficEnglandAbsoluteUrl(std::wstring url)
{
    url = Trim(HtmlDecode(url));
    if (url.empty())
        return L"";
    if (url.rfind(L"//", 0) == 0)
        return L"https:" + url;
    std::wstring lower = ToLower(url);
    if (lower.rfind(L"http://", 0) == 0 || lower.rfind(L"https://", 0) == 0)
        return url;
    if (!url.empty() && url.front() != L'/')
        url.insert(url.begin(), L'/');
    return L"https://www.trafficengland.com" + url;
}


static std::wstring ExtractHtmlAttribute(const std::wstring& tag, const wchar_t* attributeName)
{
    std::wstring pattern = attributeName;
    pattern += LR"(\s*=\s*(['\"])(.*?)\1)";
    std::wregex attrRe(pattern, std::regex_constants::icase);
    std::wsmatch m;
    if (std::regex_search(tag, m, attrRe) && m.size() > 2)
        return HtmlDecode(m[2].str());

    pattern = attributeName;
    pattern += LR"(\s*=\s*([^\s>]+))";
    std::wregex unquotedAttrRe(pattern, std::regex_constants::icase);
    if (std::regex_search(tag, m, unquotedAttrRe) && m.size() > 1)
        return HtmlDecode(m[1].str());

    return L"";
}

static bool IsHardShoulderText(const std::wstring& text)
{
    std::wstring lower = ToLower(text);
    return lower.find(L"hard shoulder") != std::wstring::npos ||
        lower.find(L"hard-shoulder") != std::wstring::npos ||
        lower.find(L"hard_shoulder") != std::wstring::npos ||
        lower.find(L"hardshoulder") != std::wstring::npos;
}

static bool ExtractLaneClosureCountsFromImages(const std::wstring& html, int& closedOut, int& totalOut)
{
    closedOut = 0;
    totalOut = 0;
    if (html.find(L'<') == std::wstring::npos)
        return false;

    std::wregex imgTagRe(LR"(<\s*img\b[^>]*>)", std::regex_constants::icase);
    for (std::wsregex_iterator it(html.begin(), html.end(), imgTagRe), end; it != end; ++it) {
        std::wstring tag = (*it)[0].str();
        std::wstring alt = ToLower(ExtractHtmlAttribute(tag, L"alt"));
        std::wstring src = ToLower(ExtractHtmlAttribute(tag, L"src"));
        const bool looksLikeLaneImage =
            alt.find(L"lane") != std::wstring::npos ||
            src.find(L"normal-closed") != std::wstring::npos ||
            src.find(L"normal-open") != std::wstring::npos ||
            src.find(L"lane") != std::wstring::npos;
        if (!looksLikeLaneImage)
            continue;
        if (IsHardShoulderText(alt + L" " + src))
            continue;

        ++totalOut;
        if (alt.find(L"closed") != std::wstring::npos ||
            src.find(L"closed") != std::wstring::npos ||
            src.find(L"redx") != std::wstring::npos ||
            src.find(L"red_x") != std::wstring::npos ||
            src.find(L"red-x") != std::wstring::npos)
        {
            ++closedOut;
        }
    }

    return totalOut > 0;
}

static std::vector<std::wstring> ExtractImageUrls(const std::wstring& html)
{
    std::vector<std::wstring> urls;
    if (html.find(L'<') == std::wstring::npos)
        return urls;

    std::wregex imgRe(LR"(<\s*img\b[^>]*\bsrc\s*=\s*(['\"]?)([^'\"\s>]+)\1[^>]*>)", std::regex_constants::icase);
    std::vector<std::wstring> laneUrls;
    for (std::wsregex_iterator it(html.begin(), html.end(), imgRe), end; it != end; ++it) {
        std::wstring url = MakeTrafficEnglandAbsoluteUrl((*it)[2].str());
        if (url.empty() || std::find(urls.begin(), urls.end(), url) != urls.end())
            continue;

        std::wstring lowerUrl = ToLower(url);
        if (IsHardShoulderText(lowerUrl))
            continue;
        if (lowerUrl.find(L"lane") != std::wstring::npos ||
            lowerUrl.find(L"closed") != std::wstring::npos ||
            lowerUrl.find(L"arrow") != std::wstring::npos ||
            lowerUrl.find(L"redx") != std::wstring::npos ||
            lowerUrl.find(L"red_x") != std::wstring::npos)
        {
            laneUrls.push_back(url);
        }
        urls.push_back(std::move(url));
    }
    return laneUrls.empty() ? urls : laneUrls;
}


static std::wstring BuildLaneClosureLine(int closed, int total)
{
    if (total <= 0)
        return L"";
    closed = ClampValue(closed, 0, total);
    return L"Lanes Closed : " + std::to_wstring(closed) + L" of " + std::to_wstring(total);
}

static void AppendLaneClosureLineIfMissing(TrafficAlert& alert)
{
    std::wstring line = BuildLaneClosureLine(alert.lanesClosed, alert.lanesTotal);
    if (line.empty())
        return;

    std::wregex existingLineRe(LR"((^|\r?\n)\s*Lanes\s+Closed\s*:\s*[^\r\n]*)", std::regex_constants::icase);
    std::wsmatch m;
    if (std::regex_search(alert.description, m, existingLineRe)) {
        std::wstring replacement = m[1].str() + line;
        alert.description.replace(static_cast<size_t>(m.position(0)), static_cast<size_t>(m.length(0)), replacement);
        return;
    }

    if (!alert.description.empty())
        alert.description += L"\r\n";
    alert.description += line;
}

static void ApplyLaneImageMetadata(TrafficAlert& alert, const std::wstring& html)
{
    std::vector<std::wstring> imageUrls = ExtractImageUrls(html);
    if (!imageUrls.empty())
        alert.laneImageUrls = std::move(imageUrls);

    int imageClosed = 0;
    int imageTotal = 0;
    if (ExtractLaneClosureCountsFromImages(html, imageClosed, imageTotal) && imageClosed > 0) {
        alert.lanesClosed = imageClosed;
        alert.lanesTotal = imageTotal;
        AppendLaneClosureLineIfMissing(alert);
    }
    else if (alert.lanesTotal == 0 && alert.lanesClosed > 0 && !alert.laneImageUrls.empty()) {
        alert.lanesTotal = static_cast<int>(alert.laneImageUrls.size());
        AppendLaneClosureLineIfMissing(alert);
    }
}

static bool ExtractLaneClosureCounts(const std::wstring& text, int& closedOut, int& totalOut)
{
    closedOut = 0;
    totalOut = 0;

    std::wstring lanesText = ExtractLabeledAlertField(text, L"Lanes Closed");
    if (lanesText.empty())
        lanesText = text;

    std::wsmatch m;
    std::wregex ofRe(LR"((\d+)\s*(?:of|/)\s*(\d+))", std::regex_constants::icase);
    if (std::regex_search(lanesText, m, ofRe) && m.size() > 2) {
        closedOut = _wtoi(m[1].str().c_str());
        totalOut = _wtoi(m[2].str().c_str());
        return closedOut > 0 && totalOut >= closedOut;
    }

    std::wregex closedRe(LR"((\d+)\s+lanes?\s+(?:is\s+|are\s+)?closed)", std::regex_constants::icase);
    if (std::regex_search(lanesText, m, closedRe) && m.size() > 1) {
        closedOut = _wtoi(m[1].str().c_str());
        totalOut = MaxValue(4, closedOut);
        return closedOut > 0;
    }

    std::wstring lower = ToLower(lanesText);
    if (lower.find(L"lane") != std::wstring::npos && lower.find(L"closed") != std::wstring::npos) {
        if (lower.find(L"one") != std::wstring::npos) closedOut = 1;
        else if (lower.find(L"two") != std::wstring::npos) closedOut = 2;
        else if (lower.find(L"three") != std::wstring::npos) closedOut = 3;
        else if (lower.find(L"four") != std::wstring::npos) closedOut = 4;
        if (closedOut > 0) {
            totalOut = MaxValue(4, closedOut);
            return true;
        }
    }

    return false;
}

static bool IsClosedLaneStatus(const std::wstring& status)
{
    std::wstring lower = ToLower(Trim(status));
    return lower.find(L"closed") != std::wstring::npos ||
        lower.find(L"blocked") != std::wstring::npos ||
        lower.find(L"closure") != std::wstring::npos;
}

static bool TryGetBoolByKeys(const json& obj, std::initializer_list<const char*> keys, bool& out)
{
    if (!obj.is_object())
        return false;

    for (const char* key : keys) {
        auto it = FindJsonKeyInsensitive(obj, key);
        if (it == obj.end())
            continue;

        if (it->is_boolean()) {
            out = it->get<bool>();
            return true;
        }

        if (it->is_number_integer()) {
            out = it->get<int>() != 0;
            return true;
        }

        std::wstring text = ToLower(Trim(JsonValueToText(*it)));
        if (text == L"true" || text == L"yes" || text == L"1") {
            out = true;
            return true;
        }
        if (text == L"false" || text == L"no" || text == L"0") {
            out = false;
            return true;
        }
    }

    return false;
}

static void ApplyLaneClosureTextMetadata(TrafficAlert& alert, const std::wstring& text)
{
    int textClosed = 0;
    int textTotal = 0;
    if (ExtractLaneClosureCounts(text, textClosed, textTotal)) {
        alert.lanesClosed = textClosed;
        alert.lanesTotal = textTotal;
        alert.laneClosedStates.clear();
        AppendLaneClosureLineIfMissing(alert);
    }
}

static void ApplyStructuredLaneMetadata(TrafficAlert& alert, const json& props, const json& obj)
{
    std::wstring laneDescription = PickString(props, {
        "laneClosureDescription", "lane closure description", "lanesClosed", "lanes closed",
        "closedLanes", "closed lanes", "laneClosures", "lane closures"
        });
    if (laneDescription.empty())
        laneDescription = PickString(obj, {
            "laneClosureDescription", "lane closure description", "lanesClosed", "lanes closed",
            "closedLanes", "closed lanes", "laneClosures", "lane closures"
            });

    if (!laneDescription.empty())
        ApplyLaneClosureTextMetadata(alert, laneDescription);

    bool fullClosure = false;
    bool hasFullClosure = TryGetBoolByKeys(props, { "fullClosure", "full closure" }, fullClosure) ||
        TryGetBoolByKeys(obj, { "fullClosure", "full closure" }, fullClosure);

    const json* lanes = nullptr;
    if (!TryGetArrayByKeys(props, { "eventLanes", "event lanes", "lanes", "laneStates", "lane states" }, lanes))
        TryGetArrayByKeys(obj, { "eventLanes", "event lanes", "lanes", "laneStates", "lane states" }, lanes);

    if (lanes && !lanes->empty()) {
        std::vector<bool> laneClosedStates;
        int closed = 0;

        for (const json& lane : *lanes) {
            std::wstring status = PickString(lane, {
                "laneStatus", "lane status", "status", "state", "closureStatus", "closure status"
                });
            std::wstring name = PickString(lane, {
                "laneName", "lane name", "name", "label", "displayName", "display name"
                });

            if (status.empty() && lane.is_string())
                status = JsonValueToText(lane);
            if (status.empty() && name.empty())
                continue;
            if (IsHardShoulderText(name + L" " + status))
                continue;

            bool isClosed = IsClosedLaneStatus(status);
            laneClosedStates.push_back(isClosed);
            if (isClosed)
                ++closed;
        }

        if (!laneClosedStates.empty() && (closed > 0 || (hasFullClosure && fullClosure))) {
            alert.laneClosedStates = std::move(laneClosedStates);
            alert.lanesTotal = static_cast<int>(alert.laneClosedStates.size());
            alert.lanesClosed = fullClosure ? alert.lanesTotal : closed;
        }
    }

    if (hasFullClosure) {
        if (fullClosure && alert.lanesTotal > 0) {
            alert.lanesClosed = alert.lanesTotal;
            alert.laneClosedStates.assign(static_cast<size_t>(alert.lanesTotal), true);
        }
    }

    AppendLaneClosureLineIfMissing(alert);
}

static bool LooksLikeTrafficEnglandDescription(const std::wstring& text)
{
    std::wstring normalized = NormalizeAlertDescription(text);
    return !ExtractLabeledAlertField(normalized, L"Reason").empty() ||
        !ExtractLabeledAlertField(normalized, L"Location").empty() ||
        !ExtractLabeledAlertField(normalized, L"From Location").empty() ||
        !ExtractLabeledAlertField(normalized, L"To Location").empty() ||
        !ExtractLabeledAlertField(normalized, L"Status").empty();
}

static std::wstring ExtractReasonTitle(const std::wstring& description)
{
    if (description.empty())
        return L"";

    std::wstring reason = ExtractLabeledAlertField(description, L"Reason");
    if (!reason.empty())
        return reason;

    std::wstring normalized = NormalizeAlertDescription(description);
    size_t lineEnd = normalized.find_first_of(L"\r\n");
    return Trim(normalized.substr(0, lineEnd));
}

static std::wstring BuildLabeledLine(const wchar_t* label, const std::wstring& value)
{
    if (value.empty())
        return L"";

    std::wstring line = label;
    line += L" : ";
    line += value;
    return line;
}

static void AppendDescriptionLine(std::wstring& description, const std::wstring& line)
{
    if (line.empty())
        return;

    if (!description.empty())
        description += L"\r\n";
    description += line;
}

static std::wstring BuildTrafficEnglandDescriptionFromFields(const json& props, const json& obj)
{
    auto pick = [&](std::initializer_list<const char*> keys) {
        std::wstring value = PickString(props, keys);
        if (value.empty())
            value = PickString(obj, keys);
        return NormalizeAlertDescription(value);
        };

    std::wstring description;
    AppendDescriptionLine(description, BuildLabeledLine(L"From Location", pick({
        "fromLocation", "from location", "from", "origin", "startLocation", "start location"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"To Location", pick({
        "toLocation", "to location", "to", "destination", "endLocation", "end location"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Location", pick({
        "location", "eventLocation", "event location", "locationDescription", "location description",
        "where", "whereIsIt", "where is it"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Reason", pick({
        "reason", "eventReason", "event reason", "reasonDescription", "reason description",
        "cause", "eventSubType", "event sub type", "incidentType", "incident type"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Status", pick({
        "status", "eventStatus", "event status", "currentStatus", "current status"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Time To Clear", pick({
        "timeToClear", "time to clear", "expectedClearTime", "expected clear time", "clearTime", "clear time"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Return To Normal", pick({
        "returnToNormal", "return to normal", "returnToNormalTime", "return to normal time",
        "normalTime", "normal time"
        })));
    AppendDescriptionLine(description, BuildLabeledLine(L"Lanes Closed", pick({
        "laneClosureDescription", "lane closure description", "lanesClosed", "lanes closed",
        "closedLanes", "closed lanes", "laneClosures", "lane closures"
        })));

    return description;
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



static std::wstring JsonValueToAlertCellText(const json& value)
{
    if (value.is_object()) {
        std::wstring picked = PickString(value, {
            "display", "text", "html", "value", "data", "rendered", "filter", "sort"
            });
        if (!picked.empty())
            return picked;

        std::wstring joined;
        for (auto it = value.begin(); it != value.end(); ++it) {
            std::wstring part = JsonValueToAlertCellText(*it);
            if (part.empty())
                continue;
            if (!joined.empty())
                joined += L" ";
            joined += part;
        }
        return joined;
    }

    if (value.is_array()) {
        std::wstring joined;
        for (const auto& item : value) {
            std::wstring part = JsonValueToAlertCellText(item);
            if (part.empty())
                continue;
            if (!joined.empty())
                joined += L" ";
            joined += part;
        }
        return joined;
    }

    return JsonValueToText(value);
}

static bool BuildHtmlAlertFromCells(const std::vector<std::wstring>& cells, size_t& idCounter, TrafficAlert& alertOut)
{
    if (cells.empty())
        return false;

    size_t descriptionIndex = cells.size();
    for (size_t i = 0; i < cells.size(); ++i) {
        if (LooksLikeTrafficEnglandDescription(cells[i])) {
            descriptionIndex = i;
            break;
        }
    }

    if (descriptionIndex == cells.size() && cells.size() >= 4)
        descriptionIndex = 3;

    if (descriptionIndex == cells.size())
        return false;

    std::wstring description = NormalizeAlertDescription(cells[descriptionIndex]);
    if (description.empty())
        return false;

    std::wstring road = descriptionIndex > 0 ? Trim(cells[0]) : ExtractLabeledAlertField(description, L"Location");
    if (road.empty())
        road = ExtractLabeledAlertField(description, L"From Location");
    std::wstring type = descriptionIndex > 1 ? Trim(cells[1]) : L"";
    std::wstring severity = descriptionIndex > 2 ? Trim(cells[2]) : L"";

    if (ToLower(road) == L"road" && ToLower(type) == L"type")
        return false;

    TrafficAlert a;
    a.id = L"html-" + std::to_wstring(++idCounter);
    a.road = road;
    a.description = description;
    std::wstring reason = ExtractReasonTitle(description);
    a.title = reason.empty() ? (type.empty() ? L"Traffic alert" : type) : reason;
    a.severity = severity.empty() ? L"Unknown" : severity;
    a.updatedText = L"";
    a.region = L"";
    a.hasLocation = false;
    ExtractLaneClosureCounts(a.description, a.lanesClosed, a.lanesTotal);
    AppendLaneClosureLineIfMissing(a);

    alertOut = std::move(a);
    return true;
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

        TrafficAlert a;
        if (BuildHtmlAlertFromCells(cells, idCounter, a)) {
            ApplyLaneImageMetadata(a, rowHtml);
            out.push_back(std::move(a));
        }
    }

    if (out.empty()) {
        std::vector<std::wstring> cells;
        for (std::wsregex_iterator cellIt(html.begin(), html.end(), cellRe), cellEnd;
            cellIt != cellEnd;
            ++cellIt)
        {
            cells.push_back(StripHtmlTags((*cellIt)[1].str()));
        }

        TrafficAlert a;
        if (BuildHtmlAlertFromCells(cells, idCounter, a)) {
            ApplyLaneImageMetadata(a, html);
            out.push_back(std::move(a));
        }
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
    auto propertiesIt = obj.find("properties");
    if (propertiesIt == obj.end())
        propertiesIt = obj.find("Properties");
    if (propertiesIt != obj.end() && propertiesIt->is_object())
        props = &(*propertiesIt);

    a.id = PickString(*props, { "id", "incidentId", "alertId", "uuid", "eventId", "eventID", "event_id" });
    if (a.id.empty())
        a.id = PickString(obj, { "id", "incidentId", "alertId", "uuid", "eventId", "eventID", "event_id" });

    std::wstring reason = PickString(*props, {
        "reason", "eventReason", "event reason", "reasonDescription", "reason description",
        "cause", "eventSubType", "event sub type", "incidentType", "incident type"
        });
    if (reason.empty())
        reason = PickString(obj, {
            "reason", "eventReason", "event reason", "reasonDescription", "reason description",
            "cause", "eventSubType", "event sub type", "incidentType", "incident type"
            });
    reason = NormalizeAlertDescription(reason);

    a.title = PickString(*props, {
        "title", "headline", "summary", "name", "eventType", "event_type",
        "event type", "alertType", "alert type", "type"
        });
    if (a.title.empty())
        a.title = PickString(obj, {
            "title", "headline", "summary", "name", "eventType", "event_type",
            "event type", "alertType", "alert type", "type"
            });
    a.title = NormalizeAlertDescription(a.title);
    if (!reason.empty() && IsGenericAlertTitle(a.title))
        a.title = reason;

    std::wstring rawDescription = PickString(*props, {
        "description", "details", "detail", "message", "fullText", "full text",
        "eventDescription", "event_description", "event description", "comment", "comments",
        "disseminationText", "dissemination text", "publicDescription", "public description",
        "gdp", "gdpFormatted", "formatDesc", "formattedDescription", "formatted description",
        "additionalDescription", "additional description", "additionalDescriptionFormatted",
        "additional description formatted", "popup", "popupContent", "popup content",
        "content", "html", "info", "information"
        });
    if (rawDescription.empty())
        rawDescription = PickString(obj, {
            "description", "details", "detail", "message", "fullText", "full text",
            "eventDescription", "event_description", "event description", "comment", "comments",
            "disseminationText", "dissemination text", "publicDescription", "public description",
            "gdp", "gdpFormatted", "formatDesc", "formattedDescription", "formatted description",
            "additionalDescription", "additional description", "additionalDescriptionFormatted",
            "additional description formatted", "popup", "popupContent", "popup content",
            "content", "html", "info", "information"
            });
    a.description = NormalizeAlertDescription(rawDescription);
    if (a.description.empty())
        a.description = BuildTrafficEnglandDescriptionFromFields(*props, obj);
    if (a.description.empty() && !reason.empty())
        a.description = reason;
    if (reason.empty())
        reason = ExtractLabeledAlertField(a.description, L"Reason");
    if (!reason.empty() && IsGenericAlertTitle(a.title))
        a.title = reason;

    a.road = PickString(*props, { "road", "roadName", "route", "roadNumber", "road_number" });
    if (a.road.empty())
        a.road = PickString(obj, { "road", "roadName", "route", "roadNumber", "road_number" });

    a.region = PickString(*props, { "region", "area", "county", "district", "location", "eventLocation", "event location" });
    if (a.region.empty())
        a.region = PickString(obj, { "region", "area", "county", "district", "location", "eventLocation", "event location" });

    a.severity = PickString(*props, { "severity", "impact", "level", "priority", "severityId", "severity_id" });
    if (a.severity.empty())
        a.severity = PickString(obj, { "severity", "impact", "level", "priority", "severityId", "severity_id" });

    a.eventType = PickString(*props, {
        "teEventType", "trafficEnglandEventType", "traffic england event type",
        "eventType", "event_type", "event type", "alertType", "alert type", "type"
        });
    if (a.eventType.empty())
        a.eventType = PickString(obj, {
            "teEventType", "trafficEnglandEventType", "traffic england event type",
            "eventType", "event_type", "event type", "alertType", "alert type", "type"
            });
    a.eventType = NormalizeAlertDescription(a.eventType);

    a.updatedText = PickDateText(*props, { "updated", "lastUpdated", "lastUpdatedTime", "lastUpdatedTimestamp", "timestamp", "created", "published", "last_update" });
    if (a.updatedText.empty())
        a.updatedText = PickDateText(obj, { "updated", "lastUpdated", "lastUpdatedTime", "lastUpdatedTimestamp", "timestamp", "created", "published", "last_update" });

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

    if (IsGenericAlertTitle(a.title)) {
        std::wstring descriptionReason = ExtractReasonTitle(a.description);
        a.title = descriptionReason.empty() ? L"Traffic alert" : descriptionReason;
    }

    if (a.severity.empty())
        a.severity = L"Unknown";

    ExtractLaneClosureCounts(a.description, a.lanesClosed, a.lanesTotal);
    ApplyLaneImageMetadata(a, rawDescription);
    ApplyStructuredLaneMetadata(a, *props, obj);
    AppendLaneClosureLineIfMissing(a);

    return a;
}


static std::string NormalizeJsonKeyName(std::string key)
{
    std::string out;
    out.reserve(key.size());

    for (unsigned char ch : key) {
        if (std::isalnum(ch))
            out.push_back(static_cast<char>(std::tolower(ch)));
    }

    return out;
}

static json::const_iterator FindJsonKeyInsensitive(const json& obj, const char* key)
{
    if (!obj.is_object())
        return obj.end();

    auto it = obj.find(key);
    if (it != obj.end())
        return it;

    std::string wanted = NormalizeJsonKeyName(key);

    for (auto candidate = obj.begin(); candidate != obj.end(); ++candidate) {
        if (NormalizeJsonKeyName(candidate.key()) == wanted)
            return candidate;
    }

    return obj.end();
}

static bool TryGetArrayByKeys(const json& obj, std::initializer_list<const char*> keys, const json*& arrOut)
{
    for (const char* key : keys) {
        auto it = FindJsonKeyInsensitive(obj, key);
        if (it != obj.end() && it->is_array()) {
            arrOut = &(*it);
            return true;
        }
    }

    return false;
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

        size_t jsonRowIdCounter = 0;

        auto addItem = [&](const json& item)
            {
                if (item.is_object()) {
                    out.push_back(ParseAlertObject(item));
                    return;
                }

                if (item.is_array()) {
                    std::vector<std::wstring> cells;
                    std::wstring rawRowHtml;
                    for (const auto& cell : item) {
                        std::wstring rawValue = JsonValueToAlertCellText(cell);
                        if (!rawValue.empty()) {
                            if (!rawRowHtml.empty())
                                rawRowHtml += L" ";
                            rawRowHtml += rawValue;
                        }

                        std::wstring value = NormalizeAlertDescription(rawValue);
                        if (!value.empty())
                            cells.push_back(value);
                    }

                    TrafficAlert a;
                    if (BuildHtmlAlertFromCells(cells, jsonRowIdCounter, a)) {
                        ApplyLaneImageMetadata(a, rawRowHtml);
                        out.push_back(std::move(a));
                    }
                }
            };

        auto addArray = [&](const json& arr)
            {
                recognized = true;
                for (const auto& item : arr)
                    addItem(item);
            };

        auto addArrayByKeys = [&](const json& obj, std::initializer_list<const char*> keys)
            {
                const json* arr = nullptr;
                if (TryGetArrayByKeys(obj, keys, arr)) {
                    addArray(*arr);
                    return true;
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
            else if (FindJsonKeyInsensitive(root, "data") != root.end() &&
                FindJsonKeyInsensitive(root, "data")->is_object() &&
                addArrayByKeys(*FindJsonKeyInsensitive(root, "data"), { "events", "alerts", "items", "results", "rows" }))
            {
                // Recognized and added above.
            }
            else if (FindJsonKeyInsensitive(root, "geometry") != root.end() ||
                FindJsonKeyInsensitive(root, "latitude") != root.end() ||
                FindJsonKeyInsensitive(root, "lat") != root.end() ||
                FindJsonKeyInsensitive(root, "title") != root.end() ||
                FindJsonKeyInsensitive(root, "description") != root.end() ||
                FindJsonKeyInsensitive(root, "headline") != root.end() ||
                FindJsonKeyInsensitive(root, "eventType") != root.end() ||
                FindJsonKeyInsensitive(root, "roadNumber") != root.end() ||
                FindJsonKeyInsensitive(root, "reason") != root.end())
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

    TrafficAlert a1;
    a1.id = L"sample-1";
    a1.title = L"Queueing traffic on the M1";
    a1.description = L"Slow traffic northbound due to congestion.";
    a1.road = L"M1";
    a1.region = L"East Midlands";
    a1.severity = L"Moderate";
    a1.updatedText = TimeTToText(now - 300);
    a1.latitude = 52.0570;
    a1.longitude = -0.7550;
    a1.hasLocation = true;
    out.push_back(std::move(a1));

    TrafficAlert a2;
    a2.id = L"sample-2";
    a2.title = L"Road Traffic Collision";
    a2.description = L"Location : The M25 anticlockwise between J5 and J1A\r\nReason : Road Traffic Collision\r\nLanes Closed : 2 of 4";
    a2.road = L"M25";
    a2.region = L"Greater London";
    a2.severity = L"Severe";
    a2.eventType = L"INCIDENT";
    a2.updatedText = TimeTToText(now - 420);
    a2.lanesClosed = 2;
    a2.lanesTotal = 4;
    a2.latitude = 51.6090;
    a2.longitude = -0.4300;
    a2.hasLocation = true;
    out.push_back(std::move(a2));

    TrafficAlert a3;
    a3.id = L"sample-3";
    a3.title = L"Incident on A1";
    a3.description = L"Delays likely near the junction.";
    a3.road = L"A1";
    a3.region = L"North East";
    a3.severity = L"Minor";
    a3.updatedText = TimeTToText(now - 180);
    a3.latitude = 54.9700;
    a3.longitude = -1.6170;
    a3.hasLocation = true;
    out.push_back(std::move(a3));

    return out;
}
