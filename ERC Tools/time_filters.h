// =================================================================================
// FILE: time_filters.h
// =================================================================================

#pragma once
#include "common.h"

bool TryParseDurationMinutes(const std::wstring& text, double& minutesOut);
int PeriodHoursFromText(const std::wstring& text, int fallbackHours = 24);
long long PeriodStartTimeMs(const std::wstring& text);
bool TryParseDoubleText(const std::wstring& text, double& valueOut);
bool TryParseDateTimeFilter(const std::wstring& text, long long& timeMsOut);
