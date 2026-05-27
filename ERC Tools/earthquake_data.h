// =================================================================================
// FILE: earthquake_data.h
// =================================================================================

#pragma once
#include "common.h"
#include "models.h"

std::wstring EarthquakeTimeText(long long timeMs);
std::vector<EarthquakeEvent> ParseEarthquakeEvents(const std::string& body);
