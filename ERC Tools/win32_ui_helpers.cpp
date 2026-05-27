// =================================================================================
// FILE: win32_ui_helpers.cpp
// =================================================================================

#include "win32_ui_helpers.h"
#include "util.h"

int MaxInt(int a, int b)
{
    return a > b ? a : b;
}

int MinInt(int a, int b)
{
    return a < b ? a : b;
}

LONG MaxLong(LONG a, LONG b)
{
    return a > b ? a : b;
}

HMENU ControlId(int id)
{
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

static bool IsTabStopCandidate(HWND hwnd)
{
    if (!hwnd || !IsWindowEnabled(hwnd))
        return false;

    wchar_t className[64]{};
    if (!GetClassNameW(hwnd, className, static_cast<int>(_countof(className))))
        return false;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (lstrcmpiW(className, L"Button") == 0)
        return (style & BS_TYPEMASK) != BS_GROUPBOX;
    if (lstrcmpiW(className, L"Edit") == 0)
        return true;
    if (lstrcmpiW(className, L"ComboBox") == 0)
        return true;
    if (lstrcmpiW(className, L"ListBox") == 0)
        return true;
    if (lstrcmpiW(className, L"SysListView32") == 0)
        return true;
    return false;
}

static BOOL CALLBACK EnableChildTabStopsProc(HWND child, LPARAM)
{
    if (IsTabStopCandidate(child)) {
        LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
        if ((style & WS_TABSTOP) == 0)
            SetWindowLongPtrW(child, GWL_STYLE, style | WS_TABSTOP);
    }
    return TRUE;
}

void EnableChildTabStops(HWND parent)
{
    if (parent)
        EnumChildWindows(parent, EnableChildTabStopsProc, 0);
}

HBRUSH ModernWindowBrush()
{
    static HBRUSH brush = CreateSolidBrush(kUiBackground);
    return brush;
}

HBRUSH ModernInputBrush()
{
    static HBRUSH brush = CreateSolidBrush(kUiSurface);
    return brush;
}

LRESULT HandleModernCtlColor(UINT msg, WPARAM wParam)
{
    HDC hdc = reinterpret_cast<HDC>(wParam);
    if (!hdc)
        return FALSE;

    SetTextColor(hdc, kUiText);
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN) {
        SetBkColor(hdc, kUiBackground);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(ModernWindowBrush());
    }

    SetBkColor(hdc, kUiSurface);
    SetBkMode(hdc, OPAQUE);
    return reinterpret_cast<LRESULT>(ModernInputBrush());
}

void ApplyModernEditChrome(HWND hwnd)
{
    if (!hwnd)
        return;
    SendMessageW(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
}

SIZE MeasureControlText(HWND hwnd, int wrapWidth)
{
    SIZE size{};
    if (!hwnd)
        return size;

    std::wstring text = GetWindowTextString(hwnd);
    if (text.empty())
        return size;

    HDC dc = GetDC(hwnd);
    if (!dc)
        return size;

    HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;

    if (wrapWidth > 0) {
        RECT textRect{ 0, 0, wrapWidth, 0 };
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &textRect, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);
        size.cx = textRect.right - textRect.left;
        size.cy = textRect.bottom - textRect.top;
    }
    else {
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    }

    if (oldFont)
        SelectObject(dc, oldFont);
    ReleaseDC(hwnd, dc);
    return size;
}

int PreferredControlWidth(HWND hwnd, int padding, int minimum, int maximum)
{
    SIZE textSize = MeasureControlText(hwnd);
    int width = MaxInt(minimum, static_cast<int>(textSize.cx) + padding);
    if (maximum > 0)
        width = MinInt(width, maximum);
    return width;
}

int PreferredControlHeight(HWND hwnd, int padding, int minimum, int wrapWidth)
{
    SIZE textSize = MeasureControlText(hwnd, wrapWidth > padding ? wrapWidth - padding : 0);
    return MaxInt(minimum, static_cast<int>(textSize.cy) + padding);
}

void SizeControlToText(HWND hwnd, int horizontalPadding, int verticalPadding, int minimumWidth, int maximumWidth, int minimumHeight)
{
    if (!hwnd)
        return;

    const int width = PreferredControlWidth(hwnd, horizontalPadding, minimumWidth, maximumWidth);
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        width,
        PreferredControlHeight(hwnd, verticalPadding, minimumHeight, width),
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

int AutoLabelWidth(HWND hwnd, int maximumWidth)
{
    return PreferredControlWidth(hwnd, 4, 0, maximumWidth);
}

int AutoLabelHeight(HWND hwnd, int minimumHeight, int wrapWidth)
{
    return PreferredControlHeight(hwnd, 6, minimumHeight, wrapWidth);
}

void SizeLabelToText(HWND hwnd, int maximumWidth)
{
    SizeControlToText(hwnd, 4, 6, 0, maximumWidth, 22);
}

void MoveLabelToText(HWND hwnd, int x, int y, int maximumWidth)
{
    const int width = AutoLabelWidth(hwnd, maximumWidth);
    MoveWindow(hwnd, x, y, width, AutoLabelHeight(hwnd, 22, width), TRUE);
}

bool IsClassName(HWND hwnd, const wchar_t* expected)
{
    wchar_t className[64]{};
    if (!GetClassNameW(hwnd, className, static_cast<int>(_countof(className))))
        return false;
    return _wcsicmp(className, expected) == 0;
}

static int MaxAutoLayoutClientWidth(HWND hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{ sizeof(mi) };
    if (GetMonitorInfoW(monitor, &mi))
        return MaxInt(320, (mi.rcWork.right - mi.rcWork.left) - 96);
    return 900;
}

static bool IsTextButtonStyle(DWORD style)
{
    switch (style & BS_TYPEMASK) {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
    case BS_CHECKBOX:
    case BS_AUTOCHECKBOX:
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
    case BS_OWNERDRAW:
        return true;
    default:
        return false;
    }
}

static bool IsPushButtonStyle(DWORD style)
{
    switch (style & BS_TYPEMASK) {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
    case BS_OWNERDRAW:
        return true;
    default:
        return false;
    }
}

static bool IsTextStaticStyle(DWORD style)
{
    switch (style & SS_TYPEMASK) {
    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_SIMPLE:
    case SS_LEFTNOWORDWRAP:
        return true;
    default:
        return false;
    }
}

static BOOL CALLBACK AutoSizeTextChildEnumProc(HWND child, LPARAM param)
{
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0)
        return TRUE;

    int textLength = GetWindowTextLengthW(child);
    if (textLength <= 0)
        return TRUE;

    HWND dialog = reinterpret_cast<HWND>(param);
    HWND parent = GetParent(child);
    if (!dialog || !parent)
        return TRUE;

    RECT screenRect{};
    if (!GetWindowRect(child, &screenRect))
        return TRUE;

    POINT topLeft{ screenRect.left, screenRect.top };
    ScreenToClient(dialog, &topLeft);

    RECT parentRect = screenRect;
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT*>(&parentRect), 2);
    const int currentW = parentRect.right - parentRect.left;
    const int currentH = parentRect.bottom - parentRect.top;

    const int maxRight = MaxAutoLayoutClientWidth(dialog) - 28;
    const int maxWidth = MaxInt(80, maxRight - topLeft.x);

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
    if (IsClassName(child, L"BUTTON") && IsTextButtonStyle(style)) {
        const bool checkLike = (style & BS_TYPEMASK) == BS_CHECKBOX ||
            (style & BS_TYPEMASK) == BS_AUTOCHECKBOX ||
            (style & BS_TYPEMASK) == BS_RADIOBUTTON ||
            (style & BS_TYPEMASK) == BS_AUTORADIOBUTTON;
        const int horizontalPadding = checkLike ? 36 : 34;
        const int desiredW = PreferredControlWidth(child, horizontalPadding, currentW, maxWidth);
        const int desiredH = PreferredControlHeight(child, 10, currentH, desiredW);
        if (desiredW != currentW || desiredH != currentH) {
            SetWindowPos(child, nullptr, 0, 0, desiredW, desiredH,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return TRUE;
    }

    if (IsClassName(child, L"STATIC") && IsTextStaticStyle(style)) {
        const int desiredW = PreferredControlWidth(child, 6, currentW, maxWidth);
        const int desiredH = PreferredControlHeight(child, 8, currentH, desiredW);
        if (desiredW != currentW || desiredH != currentH) {
            SetWindowPos(child, nullptr, 0, 0, desiredW, desiredH,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    return TRUE;
}

static void AutoSizeTextControls(HWND hwnd)
{
    if (!hwnd)
        return;
    EnumChildWindows(hwnd, AutoSizeTextChildEnumProc, reinterpret_cast<LPARAM>(hwnd));
}

struct AutoLayoutButtonInfo
{
    HWND hwnd = nullptr;
    RECT rect{};
};

struct AutoLayoutButtonCollectState
{
    HWND parent = nullptr;
    std::vector<AutoLayoutButtonInfo> buttons;
};

static BOOL CALLBACK AutoLayoutButtonEnumProc(HWND child, LPARAM param)
{
    auto* state = reinterpret_cast<AutoLayoutButtonCollectState*>(param);
    if (!state || GetParent(child) != state->parent)
        return TRUE;
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0)
        return TRUE;
    if (!IsClassName(child, L"BUTTON"))
        return TRUE;

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
    if (!IsPushButtonStyle(style))
        return TRUE;

    RECT rect{};
    if (!GetWindowRect(child, &rect))
        return TRUE;
    MapWindowPoints(HWND_DESKTOP, state->parent, reinterpret_cast<POINT*>(&rect), 2);
    state->buttons.push_back({ child, rect });
    return TRUE;
}

static void AutoLayoutButtonRows(HWND hwnd)
{
    if (!hwnd)
        return;

    AutoLayoutButtonCollectState state;
    state.parent = hwnd;
    EnumChildWindows(hwnd, AutoLayoutButtonEnumProc, reinterpret_cast<LPARAM>(&state));
    if (state.buttons.empty())
        return;

    std::sort(state.buttons.begin(), state.buttons.end(), [](const AutoLayoutButtonInfo& a, const AutoLayoutButtonInfo& b) {
        if (std::abs(a.rect.top - b.rect.top) > 8)
            return a.rect.top < b.rect.top;
        return a.rect.left < b.rect.left;
        });

    std::vector<std::vector<AutoLayoutButtonInfo>> rows;
    for (const AutoLayoutButtonInfo& button : state.buttons) {
        if (rows.empty() || std::abs(rows.back().front().rect.top - button.rect.top) > 8)
            rows.push_back({});
        rows.back().push_back(button);
    }

    const int gap = 8;
    const int leftPadding = 18;
    const int rightLimit = MaxAutoLayoutClientWidth(hwnd) - 28;
    for (std::vector<AutoLayoutButtonInfo>& row : rows) {
        if (row.empty())
            continue;

        std::sort(row.begin(), row.end(), [](const AutoLayoutButtonInfo& a, const AutoLayoutButtonInfo& b) {
            return a.rect.left < b.rect.left;
            });

        int rowLeft = row.front().rect.left;
        int rowTop = row.front().rect.top;
        int x = rowLeft;
        int y = rowTop;
        int rowHeight = 0;
        for (const AutoLayoutButtonInfo& button : row) {
            const int width = MaxInt(1, button.rect.right - button.rect.left);
            const int height = MaxInt(1, button.rect.bottom - button.rect.top);
            if (row.size() > 1 && x > rowLeft && x + width > rightLimit) {
                x = MaxInt(leftPadding, rowLeft);
                y += rowHeight + gap;
                rowHeight = 0;
            }

            SetWindowPos(button.hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
            x += width + gap;
            rowHeight = MaxInt(rowHeight, height);
        }
    }
}

static BOOL CALLBACK AutoFitChildEnumProc(HWND child, LPARAM param)
{
    RECT childRect{};
    if ((GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) == 0 || !GetWindowRect(child, &childRect))
        return TRUE;

    HWND parent = GetParent(child);
    POINT topLeft{ childRect.left, childRect.top };
    POINT bottomRight{ childRect.right, childRect.bottom };
    ScreenToClient(parent, &topLeft);
    ScreenToClient(parent, &bottomRight);

    RECT* bounds = reinterpret_cast<RECT*>(param);
    bounds->right = MaxLong(bounds->right, bottomRight.x);
    bounds->bottom = MaxLong(bounds->bottom, bottomRight.y);
    return TRUE;
}

void AutoFitWindowToChildren(HWND hwnd, int padding)
{
    if (!hwnd)
        return;

    AutoSizeTextControls(hwnd);
    AutoLayoutButtonRows(hwnd);

    RECT childBounds{ 0, 0, 0, 0 };
    EnumChildWindows(hwnd, AutoFitChildEnumProc, reinterpret_cast<LPARAM>(&childBounds));
    if (childBounds.right <= 0 || childBounds.bottom <= 0)
        return;

    int desiredClientW = MaxInt(260, childBounds.right + padding);
    int desiredClientH = MaxInt(160, childBounds.bottom + padding);

    RECT windowRect{ 0, 0, desiredClientW, desiredClientH };
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&windowRect, style, GetMenu(hwnd) != nullptr, exStyle);

    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}
