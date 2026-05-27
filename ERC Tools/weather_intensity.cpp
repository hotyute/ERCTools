// =================================================================================
// FILE: weather_intensity.cpp
// =================================================================================

#include "weather_intensity.h"
#include "util.h"

int WeatherSystemCategoryRank(const std::wstring& category)
{
    std::wstring cat = ToLower(Trim(category));
    if (cat.empty())
        return -1;

    if (cat.find(L"5") != std::wstring::npos)
        return 6;
    if (cat.find(L"4") != std::wstring::npos)
        return 5;
    if (cat.find(L"3") != std::wstring::npos)
        return 4;
    if (cat.find(L"2") != std::wstring::npos)
        return 3;
    if (cat.find(L"1") != std::wstring::npos ||
        cat.find(L"hurricane") != std::wstring::npos ||
        cat.find(L"typhoon") != std::wstring::npos ||
        cat.find(L"cyclone") != std::wstring::npos)
    {
        return 2;
    }
    if (cat == L"ts" || cat.find(L"storm") != std::wstring::npos)
        return 1;
    if (cat == L"td" || cat.find(L"depression") != std::wstring::npos)
        return 0;
    return -1;
}

std::wstring WeatherSystemCategoryRankName(int rank)
{
    switch (rank) {
    case 0: return L"TD";
    case 1: return L"TS";
    case 2: return L"Cat 1";
    case 3: return L"Cat 2";
    case 4: return L"Cat 3";
    case 5: return L"Cat 4";
    case 6: return L"Cat 5";
    default: return L"All";
    }
}
