// =================================================================================
// FILE: time_filters.cpp
// =================================================================================

#include "time_filters.h"
#include "util.h"

static std::wstring CompactDurationText(std::wstring value)
{
    for (wchar_t& ch : value) {
        if (ch == L'-' || ch == L',' || ch == L';')
            ch = L' ';
    }

    std::wstring compact;
    compact.reserve(value.size());
    bool lastSpace = false;
    for (wchar_t ch : value) {
        if (iswspace(ch)) {
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

static bool TryParseDurationNumberToken(std::wstring token, double& valueOut)
{
    token = ToLower(Trim(token));
    if (token.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(token.c_str(), &end);
    if (end != token.c_str() && Trim(end ? end : L"").empty() && std::isfinite(parsed) && parsed >= 0.0) {
        valueOut = parsed;
        return true;
    }

    if (token == L"a" || token == L"an")
        token = L"one";

    static const std::unordered_map<std::wstring, double> words = {
        { L"zero", 0.0 }, { L"one", 1.0 }, { L"two", 2.0 }, { L"three", 3.0 },
        { L"four", 4.0 }, { L"five", 5.0 }, { L"six", 6.0 }, { L"seven", 7.0 },
        { L"eight", 8.0 }, { L"nine", 9.0 }, { L"ten", 10.0 }, { L"eleven", 11.0 },
        { L"twelve", 12.0 }
    };
    auto it = words.find(token);
    if (it == words.end())
        return false;
    valueOut = it->second;
    return true;
}

static bool DurationUnitIsHour(const std::wstring& unit)
{
    return unit == L"h" || unit == L"hr" || unit == L"hrs" || unit == L"hour" || unit == L"hours";
}

static bool DurationUnitIsMinute(const std::wstring& unit)
{
    return unit == L"m" || unit == L"min" || unit == L"mins" || unit == L"minute" || unit == L"minutes";
}

bool TryParseDurationMinutes(const std::wstring& text, double& minutesOut)
{
    std::wstring value = CompactDurationText(ToLower(Trim(text)));
    if (value.empty())
        return false;

    double total = 0.0;
    bool matched = false;
    std::wsmatch m;

    std::wregex hourHalfRe(
        LR"(\b(\d+(?:\.\d+)?|a|an|one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)\s+(?:and\s+)?(?:a\s+)?half\s+hours?\b)",
        std::regex_constants::icase);
    if (std::regex_search(value, m, hourHalfRe) && m.size() > 1) {
        double amount = 0.0;
        if (TryParseDurationNumberToken(m[1].str(), amount)) {
            total += (amount + 0.5) * 60.0;
            matched = true;
            value = std::regex_replace(value, hourHalfRe, L" ");
        }
    }

    std::wregex numericHourAndHalfRe(
        LR"(\b(\d+(?:\.\d+)?|a|an|one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)\s+hours?\s+(?:and\s+)?(?:a\s+)?half\b)",
        std::regex_constants::icase);
    if (std::regex_search(value, m, numericHourAndHalfRe) && m.size() > 1) {
        double amount = 0.0;
        if (TryParseDurationNumberToken(m[1].str(), amount)) {
            total += (amount + 0.5) * 60.0;
            matched = true;
            value = std::regex_replace(value, numericHourAndHalfRe, L" ");
        }
    }

    std::wregex hourAndHalfRe(
        LR"(\bhours?\s+(?:and\s+)?(?:a\s+)?half\b)",
        std::regex_constants::icase);
    if (std::regex_search(value, hourAndHalfRe)) {
        total += 90.0;
        matched = true;
        value = std::regex_replace(value, hourAndHalfRe, L" ");
    }

    std::wregex halfHourRe(LR"(\bhalf\s+(?:an?\s+)?hours?\b|\bhalf\s+hour\b)", std::regex_constants::icase);
    if (std::regex_search(value, halfHourRe)) {
        total += 30.0;
        matched = true;
        value = std::regex_replace(value, halfHourRe, L" ");
    }

    std::wregex segmentRe(
        LR"(\b(\d+(?:\.\d+)?|a|an|one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)\s*(hours?|hrs?|hr|h|minutes?|mins?|min|m)\b)",
        std::regex_constants::icase);
    for (std::wsregex_iterator it(value.begin(), value.end(), segmentRe), endIt; it != endIt; ++it) {
        if (it->size() < 3)
            continue;
        double amount = 0.0;
        if (!TryParseDurationNumberToken((*it)[1].str(), amount))
            continue;
        std::wstring unit = ToLower((*it)[2].str());
        if (DurationUnitIsHour(unit))
            total += amount * 60.0;
        else if (DurationUnitIsMinute(unit))
            total += amount;
        else
            continue;
        matched = true;
    }

    if (!matched || !std::isfinite(total) || total < 0.0)
        return false;

    minutesOut = total;
    return true;
}

int PeriodHoursFromText(const std::wstring& text, int fallbackHours)
{
    if (IsAllPeriodText(text))
        return 0;

    double minutes = 0.0;
    if (TryParseDurationMinutes(text, minutes) && minutes > 0.0)
        return std::max(1, static_cast<int>(std::round(minutes / 60.0)));
    return fallbackHours;
}

bool IsAllPeriodText(const std::wstring& text)
{
    std::wstring value = ToLower(Trim(text));
    return value.empty() || value == L"all";
}

long long PeriodStartTimeMs(const std::wstring& text)
{
    int hours = PeriodHoursFromText(text, 24);
    if (hours <= 0)
        return 0;
    std::time_t now = std::time(nullptr);
    return (static_cast<long long>(now) - static_cast<long long>(hours) * 60LL * 60LL) * 1000LL;
}

bool TryParseDoubleText(const std::wstring& text, double& valueOut)
{
    std::wstring value = Trim(text);
    if (value.empty())
        return false;

    wchar_t* end = nullptr;
    double parsed = std::wcstod(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(parsed) || !Trim(end ? end : L"").empty())
        return false;

    valueOut = parsed;
    return true;
}

bool TryParseDateTimeFilter(const std::wstring& text, long long& timeMsOut)
{
    std::wstring value = Trim(text);
    if (value.empty()) {
        timeMsOut = 0;
        return true;
    }

    std::tm tm{};
    int y = 0, mon = 0, d = 0, h = 0, min = 0;
    int count = swscanf_s(value.c_str(), L"%d-%d-%d %d:%d", &y, &mon, &d, &h, &min);
    if (count < 3)
        count = swscanf_s(value.c_str(), L"%d/%d/%d %d:%d", &y, &mon, &d, &h, &min);
    if (count < 3)
        return false;

    tm.tm_year = y - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = d;
    tm.tm_hour = count >= 4 ? h : 0;
    tm.tm_min = count >= 5 ? min : 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;

    std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1))
        return false;

    timeMsOut = static_cast<long long>(t) * 1000LL;
    return true;
}
