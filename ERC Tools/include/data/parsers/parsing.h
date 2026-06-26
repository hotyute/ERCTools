// =================================================================================
// FILE: parsing.h
// =================================================================================

#pragma once
#include "core/common.h"
#include "core/models.h"

std::wstring HtmlDecode(const std::wstring& text);
std::wstring StripHtmlTags(std::wstring text);
std::vector<TrafficAlert> ParseHtmlTrafficAlerts(const std::wstring& html);
bool ExtractRingFromCoords(const json& coords, std::vector<GeoPoint>& ring);
void CollectBoundaryRingsFromGeometry(const json& geom, std::vector<std::vector<GeoPoint>>& rings);
void CollectBoundaryRingsFromNode(const json& node, std::vector<std::vector<GeoPoint>>& rings);
TrafficAlert ParseAlertObject(const json& obj);
std::vector<TrafficAlert> ParseTrafficAlerts(const std::string& text, std::wstring& errorOut);
std::vector<TrafficAlert> SampleAlerts();
