// =================================================================================
// FILE: login_dialog.cpp
// =================================================================================

#include "login_dialog.h"
#include "http.h"
#include "util.h"

namespace
{
constexpr const wchar_t* kLoginClassName = L"ERCToolsLoginWindow";

constexpr int IDC_LOGIN_DISPLAY_NAME = 7101;
constexpr int IDC_LOGIN_USERNAME = 7102;
constexpr int IDC_LOGIN_PASSWORD = 7103;
constexpr int IDC_LOGIN_POSITION = 7104;
constexpr int IDC_LOGIN_POD = 7105;
constexpr int IDC_LOGIN_STATUS = 7106;
constexpr int IDC_LOGIN_BUTTON = 7107;
constexpr int IDC_LOGIN_CANCEL = 7108;

static HMENU ControlMenuId(int id)
{
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

struct LoginContext
{
    HINSTANCE hInst = nullptr;
    HWND hwnd = nullptr;
    HWND displayNameEdit = nullptr;
    HWND usernameEdit = nullptr;
    HWND passwordEdit = nullptr;
    HWND positionCombo = nullptr;
    HWND podCombo = nullptr;
    HWND statusLabel = nullptr;
    ClientSession* session = nullptr;
    std::wstring serverBaseUrl = L"http://localhost:8080";
    bool done = false;
    bool accepted = false;
};

static std::wstring AppendApiPath(std::wstring base, const wchar_t* path)
{
    base = NormalizeUrl(base);
    while (!base.empty() && base.back() == L'/')
        base.pop_back();
    return base + path;
}

static std::wstring LoadConfiguredServerBaseUrl()
{
    std::ifstream in(GetSettingsPath(), std::ios::binary);
    if (!in)
        return L"http://localhost:8080";

    try {
        json root = json::parse(in);
        const json* settings = &root;
        auto settingsIt = root.find("settings");
        if (settingsIt != root.end() && settingsIt->is_object())
            settings = &(*settingsIt);

        auto it = settings->find("serverBaseUrl");
        if (it != settings->end() && it->is_string()) {
            std::wstring value = NormalizeUrl(Utf8ToWide(it->get<std::string>()));
            if (!value.empty())
                return value;
        }
    }
    catch (...) {
    }

    return L"http://localhost:8080";
}

static void SetChildFont(HWND hwnd, HFONT font)
{
    if (hwnd && font)
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

static HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font)
{
    HWND hwnd = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, w, h, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    SetChildFont(hwnd, font);
    return hwnd;
}

static void AddComboItem(HWND combo, const wchar_t* text)
{
    SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
}

static std::wstring ComboText(HWND combo)
{
    int index = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    if (index < 0)
        return L"";
    wchar_t buffer[128]{};
    SendMessageW(combo, CB_GETLBTEXT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(buffer));
    return buffer;
}

static bool ParseLoginResponse(const std::string& response, ClientSession& sessionOut, std::wstring& errorOut)
{
    try {
        json root = json::parse(response);
        if (!root.is_object()) {
            errorOut = L"Login response was not an object.";
            return false;
        }

        std::wstring token = PickString(root, { "token", "accessToken", "sessionToken" });
        if (token.empty()) {
            errorOut = PickString(root, { "error", "message" });
            if (errorOut.empty())
                errorOut = L"Login response did not include a token.";
            return false;
        }

        ClientSession session;
        session.authenticated = true;
        session.token = token;

        const json* user = &root;
        auto userIt = root.find("user");
        if (userIt != root.end() && userIt->is_object())
            user = &(*userIt);

        session.displayName = PickString(*user, { "displayName", "display_name", "name" });
        session.username = PickString(*user, { "username", "user" });
        session.position = PickString(*user, { "position", "role" });
        session.pod = PickString(*user, { "pod" });
        sessionOut = std::move(session);
        return true;
    }
    catch (const std::exception& e) {
        errorOut = L"Login response parse failed: " + Utf8ToWide(e.what());
        return false;
    }
}

static void AttemptLogin(LoginContext* ctx)
{
    if (!ctx || !ctx->session)
        return;

    std::wstring displayName = Trim(GetWindowTextString(ctx->displayNameEdit));
    std::wstring username = Trim(GetWindowTextString(ctx->usernameEdit));
    std::wstring password = GetWindowTextString(ctx->passwordEdit);
    std::wstring position = ComboText(ctx->positionCombo);
    std::wstring pod = ComboText(ctx->podCombo);

    if (displayName.empty() || username.empty() || password.empty() || position.empty() || pod.empty()) {
        SetWindowTextSafe(ctx->statusLabel, L"All fields are required.");
        return;
    }

    SetWindowTextSafe(ctx->statusLabel, L"Signing in...");
    EnableWindow(ctx->hwnd, FALSE);

    std::string body = "{";
    body += "\"displayName\":" + JsonEscape(displayName);
    body += ",\"username\":" + JsonEscape(username);
    body += ",\"password\":" + JsonEscape(password);
    body += ",\"position\":" + JsonEscape(position);
    body += ",\"pod\":" + JsonEscape(pod);
    body += ",\"clientVersion\":" + JsonEscape(kClientVersion);
    body += ",\"platform\":" + JsonEscape(kClientPlatform);
    body += "}";

    std::string response;
    std::wstring error;
    ClientSession session;
    bool ok = HttpPostJsonText(AppendApiPath(ctx->serverBaseUrl, L"/api/auth/login"), body, response, error) &&
        ParseLoginResponse(response, session, error);

    EnableWindow(ctx->hwnd, TRUE);
    SetForegroundWindow(ctx->hwnd);

    if (!ok) {
        if (error.empty())
            error = L"Login failed.";
        SetWindowTextSafe(ctx->statusLabel, error);
        return;
    }

    if (session.displayName.empty())
        session.displayName = displayName;
    if (session.username.empty())
        session.username = username;
    if (session.position.empty())
        session.position = position;
    if (session.pod.empty())
        session.pod = pod;

    *ctx->session = std::move(session);
    ctx->accepted = true;
    ctx->done = true;
    DestroyWindow(ctx->hwnd);
}

static void CreateLoginControls(LoginContext* ctx)
{
    HFONT font = CreateUiFont(10);
    HFONT titleFont = CreateUiFont(18, FW_SEMIBOLD);

    CreateLabel(ctx->hwnd, L"ERC Tools Login", 24, 20, 360, 34, titleFont);
    CreateLabel(ctx->hwnd, (L"Server: " + ctx->serverBaseUrl).c_str(), 24, 58, 420, 22, font);

    CreateLabel(ctx->hwnd, L"Display Name:", 24, 96, 130, 22, font);
    ctx->displayNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 154, 92, 270, 26, ctx->hwnd, ControlMenuId(IDC_LOGIN_DISPLAY_NAME), ctx->hInst, nullptr);

    CreateLabel(ctx->hwnd, L"Username:", 24, 134, 130, 22, font);
    ctx->usernameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 154, 130, 270, 26, ctx->hwnd, ControlMenuId(IDC_LOGIN_USERNAME), ctx->hInst, nullptr);

    CreateLabel(ctx->hwnd, L"Password:", 24, 172, 130, 22, font);
    ctx->passwordEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL, 154, 168, 270, 26, ctx->hwnd, ControlMenuId(IDC_LOGIN_PASSWORD), ctx->hInst, nullptr);

    CreateLabel(ctx->hwnd, L"Position:", 24, 210, 130, 22, font);
    ctx->positionCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 154, 206, 270, 160, ctx->hwnd, ControlMenuId(IDC_LOGIN_POSITION), ctx->hInst, nullptr);
    AddComboItem(ctx->positionCombo, L"Administrator");
    AddComboItem(ctx->positionCombo, L"Supervisor");
    AddComboItem(ctx->positionCombo, L"Manager");
    AddComboItem(ctx->positionCombo, L"ERC");
    SendMessageW(ctx->positionCombo, CB_SETCURSEL, 3, 0);

    CreateLabel(ctx->hwnd, L"Pod:", 24, 248, 130, 22, font);
    ctx->podCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 154, 244, 270, 220, ctx->hwnd, ControlMenuId(IDC_LOGIN_POD), ctx->hInst, nullptr);
    for (int i = 1; i <= 9; ++i) {
        std::wstring pod = L"Pod " + std::to_wstring(i);
        AddComboItem(ctx->podCombo, pod.c_str());
    }
    SendMessageW(ctx->podCombo, CB_SETCURSEL, 0, 0);

    ctx->statusLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 286, 400, 40, ctx->hwnd, ControlMenuId(IDC_LOGIN_STATUS), ctx->hInst, nullptr);
}

static LRESULT CALLBACK LoginWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* ctx = reinterpret_cast<LoginContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ctx = reinterpret_cast<LoginContext*>(cs->lpCreateParams);
        ctx->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    }

    switch (msg) {
    case WM_CREATE:
        CreateLoginControls(ctx);
        {
            HFONT font = CreateUiFont(10);
            for (HWND h : { ctx->displayNameEdit, ctx->usernameEdit, ctx->passwordEdit, ctx->positionCombo, ctx->podCombo, ctx->statusLabel })
                SetChildFont(h, font);
            HWND login = CreateWindowExW(0, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 222, 336, 96, 32, hwnd, ControlMenuId(IDC_LOGIN_BUTTON), ctx->hInst, nullptr);
            HWND cancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 328, 336, 96, 32, hwnd, ControlMenuId(IDC_LOGIN_CANCEL), ctx->hInst, nullptr);
            SetChildFont(login, font);
            SetChildFont(cancel, font);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_LOGIN_BUTTON && HIWORD(wParam) == BN_CLICKED) {
            AttemptLogin(ctx);
            return 0;
        }
        if (LOWORD(wParam) == IDC_LOGIN_CANCEL && HIWORD(wParam) == BN_CLICKED) {
            ctx->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_CLOSE:
        if (ctx)
            ctx->done = true;
        DestroyWindow(hwnd);
        return 0;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(22, 34, 49));
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH brush = CreateSolidBrush(RGB(246, 248, 251));
        return reinterpret_cast<LRESULT>(brush);
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}

bool ShowLoginDialog(HINSTANCE hInst, ClientSession& sessionOut)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = LoginWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(246, 248, 251));
    wc.lpszClassName = kLoginClassName;
    RegisterClassExW(&wc);

    LoginContext ctx;
    ctx.hInst = hInst;
    ctx.session = &sessionOut;
    ctx.serverBaseUrl = LoadConfiguredServerBaseUrl();

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kLoginClassName,
        L"ERC Tools Login",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        470,
        420,
        nullptr,
        nullptr,
        hInst,
        &ctx);

    if (!hwnd)
        return false;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (!ctx.done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            AttemptLogin(&ctx);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return ctx.accepted && sessionOut.authenticated;
}
