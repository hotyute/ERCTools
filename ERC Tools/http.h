#pragma once
#include "common.h"

bool HttpGetText(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut);
bool HttpGetBinary(const std::wstring& inputUrl, std::vector<BYTE>& bodyOut, std::wstring& errorOut);
bool HttpPostJsonText(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, std::wstring& errorOut);
bool IsTrafficEnglandAlertsPageUrl(const std::wstring& inputUrl);
std::wstring BuildTrafficEnglandAlertsApiUrl(size_t start, size_t step, bool unplannedOnly = true, const std::wstring& order = L"Road");
