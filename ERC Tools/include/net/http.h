// =================================================================================
// FILE: http.h
// =================================================================================

#pragma once
#include "core/common.h"

bool HttpGetText(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut);
bool HttpGetTextWithTimeout(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut, DWORD timeoutMs);
bool HttpGetBinary(const std::wstring& inputUrl, std::vector<BYTE>& bodyOut, std::wstring& errorOut);
bool HttpPostJsonText(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, std::wstring& errorOut);
bool HttpPostJsonTextStatus(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, DWORD& statusOut, std::wstring& errorOut);
bool HttpPutJsonText(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, std::wstring& errorOut);
bool HttpPatchJsonText(const std::wstring& inputUrl, const std::string& jsonBody, std::string& bodyOut, std::wstring& errorOut);
bool HttpDeleteText(const std::wstring& inputUrl, std::string& bodyOut, std::wstring& errorOut);
bool HttpGetTextWithHeaders(const std::wstring& inputUrl, const std::wstring& extraHeaders, std::string& bodyOut, std::wstring& errorOut);
bool HttpPostJsonTextWithHeaders(const std::wstring& inputUrl, const std::string& jsonBody, const std::wstring& extraHeaders, std::string& bodyOut, std::wstring& errorOut);
bool HttpPutJsonTextWithHeaders(const std::wstring& inputUrl, const std::string& jsonBody, const std::wstring& extraHeaders, std::string& bodyOut, std::wstring& errorOut);
bool HttpPatchJsonTextWithHeaders(const std::wstring& inputUrl, const std::string& jsonBody, const std::wstring& extraHeaders, std::string& bodyOut, std::wstring& errorOut);
bool HttpDeleteTextWithHeaders(const std::wstring& inputUrl, const std::wstring& extraHeaders, std::string& bodyOut, std::wstring& errorOut);
bool IsTrafficEnglandAlertsPageUrl(const std::wstring& inputUrl);
std::wstring BuildTrafficEnglandAlertsApiUrl(size_t start, size_t step, bool unplannedOnly = true, const std::wstring& order = L"Road");
