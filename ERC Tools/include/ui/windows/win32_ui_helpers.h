// =================================================================================
// FILE: win32_ui_helpers.h
// =================================================================================

#pragma once
#include "core/common.h"

inline constexpr COLORREF kUiBackground = RGB(246, 248, 251);
inline constexpr COLORREF kUiSurface = RGB(255, 255, 255);
inline constexpr COLORREF kUiText = RGB(22, 34, 49);
inline constexpr COLORREF kUiMutedText = RGB(86, 99, 115);
inline constexpr COLORREF kUiSelection = RGB(226, 240, 255);

int MaxInt(int a, int b);
int MinInt(int a, int b);
LONG MaxLong(LONG a, LONG b);
HMENU ControlId(int id);
void EnableChildTabStops(HWND parent);
HBRUSH ModernWindowBrush();
HBRUSH ModernInputBrush();
LRESULT HandleModernCtlColor(UINT msg, WPARAM wParam);
void ApplyModernEditChrome(HWND hwnd);
SIZE MeasureControlText(HWND hwnd, int wrapWidth = 0);
int PreferredControlWidth(HWND hwnd, int padding, int minimum = 0, int maximum = 0);
int PreferredControlHeight(HWND hwnd, int padding, int minimum = 0, int wrapWidth = 0);
void SizeControlToText(HWND hwnd, int horizontalPadding, int verticalPadding, int minimumWidth = 0, int maximumWidth = 0, int minimumHeight = 0);
int AutoLabelWidth(HWND hwnd, int maximumWidth = 0);
int AutoLabelHeight(HWND hwnd, int minimumHeight = 22, int wrapWidth = 0);
void SizeLabelToText(HWND hwnd, int maximumWidth = 0);
void MoveLabelToText(HWND hwnd, int x, int y, int maximumWidth = 0);
bool IsClassName(HWND hwnd, const wchar_t* expected);
void AutoFitWindowToChildren(HWND hwnd, int padding = 28);
