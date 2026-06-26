// =================================================================================
// FILE: earthquake_data.h
// =================================================================================

#pragma once
#include "core/common.h"
#include "core/models.h"

std::wstring EarthquakeTimeText(long long timeMs);
std::vector<EarthquakeEvent> ParseEarthquakeEvents(const std::string& body);
