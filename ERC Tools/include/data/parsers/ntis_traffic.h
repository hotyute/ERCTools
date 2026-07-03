// =================================================================================
// FILE: ntis_traffic.h
// =================================================================================

#pragma once

#include "core/common.h"
#include "core/models.h"

bool ParseNtisTrafficSnapshot(
    const std::string& text,
    bool includePlanned,
    std::vector<TrafficAlert>& alertsOut,
    std::wstring& statusOut,
    std::wstring& errorOut);
