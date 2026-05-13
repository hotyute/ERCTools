// =================================================================================
// FILE: util.h
// =================================================================================


#pragma once
#include "common.h"
#include "models.h"

template<typename T>
T ClampValue(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

template<typename T>
T MinValue(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T MaxValue(T a, T b)
{
    return a > b ? a : b;
}

void OpenConsole();
void ConsoleLog(const std::wstring& text);
std::filesystem::path GetBoundaryCachePath();
std::filesystem::path GetLaneImageCachePath(const std::wstring& imageUrl);
std::filesystem::path GetSettingsPath();
bool SaveBinaryToFile(const std::filesystem::path& path, const std::vector<BYTE>& bytes);
std::wstring Trim(const std::wstring& s);
std::wstring ToLower(std::wstring s);
std::wstring Utf8ToWide(const std::string& s);
std::string WideToUtf8(const std::wstring& s);
std::string JsonEscape(const std::wstring& s);
std::wstring WinErrorText(DWORD err);
std::wstring GetWindowTextString(HWND hWnd);
void SetWindowTextSafe(HWND hWnd, const std::wstring& text);
std::wstring TimeTToText(std::time_t t);
std::wstring JsonValueToText(const json& v);
bool TryGetDoubleFromJsonValue(const json& v, double& out);
std::wstring PickString(const json& obj, std::initializer_list<const char*> keys);
bool PickDouble(const json& obj, std::initializer_list<const char*> keys, double& out);
std::wstring PickDateText(const json& obj, std::initializer_list<const char*> keys);
std::wstring BuildSeverityDisplay(const std::wstring& severity);
std::wstring SeverityBucket(const std::wstring& severity);
std::wstring BuildAlertSummary(const TrafficAlert& a);
std::wstring BuildAlertDetails(const TrafficAlert& a);
std::wstring NormalizeUrl(std::wstring url);
void EnableModernWindowFrame(HWND hwnd);
HFONT CreateUiFont(int pointSize = 10, int weight = FW_NORMAL);
void ApplyExplorerTheme(HWND hwnd);
