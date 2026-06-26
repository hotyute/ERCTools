// =================================================================================
// FILE: weather_data.h
// =================================================================================


#pragma once
#include "core/common.h"
#include "core/models.h"

std::vector<WeatherSystemEvent> ParseWeatherSystemEvents(const std::string& body, std::wstring& statusTextOut);
std::vector<WeatherForecastPoint> ParseWeatherSystemForecastTrack(const std::string& body);
std::vector<WeatherWarningEvent> ParseWeatherWarningEvents(const std::string& body, std::wstring& statusTextOut);
std::vector<FloodEvent> ParseFloodEvents(const std::string& body, std::wstring& statusTextOut);
