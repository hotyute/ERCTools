// =================================================================================
// FILE: weather_intensity.h
// =================================================================================

#pragma once
#include "common.h"

int WeatherSystemCategoryRank(const std::wstring& category);
int WeatherSystemWindCategoryRank(double windKnots);
int WeatherSystemEffectiveCategoryRank(const std::wstring& category, double windKnots);
std::wstring WeatherSystemCategoryRankName(int rank);
