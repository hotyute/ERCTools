// =================================================================================
// FILE: login_dialog.cpp
// =================================================================================

#include "login_dialog.h"
#include "http.h"
#include "util.h"

namespace
{
constexpr const wchar_t* kLoginClassName = L"ERCToolsLoginWindow";

constexpr int IDC_LOGIN_USERNAME = 7102;
constexpr int IDC_LOGIN_PASSWORD = 7103;
constexpr int IDC_LOGIN_POSITION = 7104;
constexpr int IDC_LOGIN_POD = 7105;
constexpr int IDC_LOGIN_STATUS = 7106;
constexpr int IDC_LOGIN_BUTTON = 7107;
constexpr int IDC_LOGIN_CANCEL = 7108;
constexpr int IDC_LOGIN_OFFLINE = 7109;
constexpr int IDC_LOGIN_REMEMBER = 7110;

static HMENU ControlMenuId(int id)
{
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

struct LoginContext
{
    HINSTANCE hInst = nullptr;
    HWND hwnd = nullptr;
    HWND usernameEdit = nullptr;
    HWND passwordEdit = nullptr;
    HWND positionCombo = nullptr;
    HWND podCombo = nullptr;
    HWND rememberCheck = nullptr;
    HWND statusLabel = nullptr;
    ClientSession* session = nullptr;
    std::wstring serverBaseUrl = L"http://213.254.181.35:8081";
    bool done = false;
    bool accepted = false;
};

struct RememberedLogin
{
    bool remember = false;
    std::wstring username;
    std::wstring password;
    std::wstring position;
    std::wstring pod;
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
        return L"http://213.254.181.35:8081";

    try {
        json root = json::parse(in);
        const json* settings = &root;
        auto settingsIt = root.find("settings");
        if (settingsIt != root.end() && settingsIt->is_object())
            settings = &(*settingsIt);

        auto it = settings->find("serverBaseUrl");
        if (it != settings->end() && it->is_string()) {
            std::wstring value = NormalizeUrl(Utf8ToWide(it->get<std::string>()));
            std::wstring lower = ToLower(value);
            if (lower == L"http://localhost:8080" || lower == L"https://localhost:8080" ||
                lower == L"http://213.254.181.35:8080" || lower == L"https://213.254.181.35:8080")
            {
                value = L"http://213.254.181.35:8081";
            }
            if (!value.empty())
                return value;
        }
    }
    catch (...) {
    }

    return L"http://213.254.181.35:8081";
}

static RememberedLogin LoadRememberedLogin()
{
    RememberedLogin remembered;
    std::ifstream in(GetSettingsPath(), std::ios::binary);
    if (!in)
        return remembered;

    try {
        json root = json::parse(in);
        if (!root.is_object())
            return remembered;
        auto it = root.find("loginRemember");
        if (it == root.end() || !it->is_object())
            return remembered;

        remembered.remember = it->value("enabled", false);
        if (!remembered.remember)
            return remembered;

        auto read = [&](const char* key) -> std::wstring {
            auto valueIt = it->find(key);
            if (valueIt != it->end() && valueIt->is_string())
                return Utf8ToWide(valueIt->get<std::string>());
            return L"";
            };
        remembered.username = read("username");
        remembered.password = read("password");
        remembered.position = read("position");
        remembered.pod = read("pod");
    }
    catch (...) {
    }
    return remembered;
}

static void SaveRememberedLogin(const RememberedLogin& remembered)
{
    try {
        json root = json::object();
        {
            std::ifstream in(GetSettingsPath(), std::ios::binary);
            if (in) {
                try {
                    root = json::parse(in);
                    if (!root.is_object())
                        root = json::object();
                }
                catch (...) {
                    root = json::object();
                }
            }
        }

        root["loginRemember"] = {
            { "enabled", remembered.remember }
        };
        if (remembered.remember) {
            root["loginRemember"]["username"] = WideToUtf8(remembered.username);
            root["loginRemember"]["password"] = WideToUtf8(remembered.password);
            root["loginRemember"]["position"] = WideToUtf8(remembered.position);
            root["loginRemember"]["pod"] = WideToUtf8(remembered.pod);
        }

        std::ofstream out(GetSettingsPath(), std::ios::binary | std::ios::trunc);
        if (out)
            out << root.dump();
    }
    catch (...) {
    }
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

static void SetComboSelection(HWND combo, const std::wstring& text, int fallback)
{
    int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < count; ++i) {
        wchar_t buffer[128]{};
        SendMessageW(combo, CB_GETLBTEXT, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(buffer));
        if (text == buffer) {
            SendMessageW(combo, CB_SETCURSEL, i, 0);
            return;
        }
    }
    SendMessageW(combo, CB_SETCURSEL, fallback, 0);
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

static std::wstring LoginErrorMessageForCode(std::wstring code, DWORD status)
{
    code = ToLower(Trim(code));
    if (code == L"missing_fields")
        return L"Username, password, position and pod are required.";
    if (code == L"invalid_json")
        return L"The login request was not understood by the server.";
    if (code == L"invalid_credentials")
        return L"Invalid username or password.";
    if (code == L"account_disabled")
        return L"This account is disabled.";
    if (code == L"position_not_allowed")
        return L"This account cannot sign in as the selected position.";
    if (code == L"pod_in_use")
        return L"Selected pod is already in use.";
    if (code == L"database_error")
        return L"The server could not reach the account database.";
    if (code == L"session_create_failed")
        return L"Your credentials were accepted, but the server could not create a session.";
    if (code == L"missing_token")
        return L"The login response did not include a session token.";
    if (code == L"session_invalid")
        return L"Your login session is invalid or expired.";
    if (code == L"forbidden")
        return L"This account is not allowed to complete that action.";

    if (status == 401)
        return L"Login details were not accepted.";
    if (status == 403)
        return L"This account is not allowed to complete that action.";
    if (status >= 500)
        return L"The server could not complete login.";
    return L"";
}

static std::wstring LoginErrorMessageFromResponse(const std::string& response, DWORD status, const std::wstring& fallback)
{
    try {
        json root = json::parse(response.empty() ? "{}" : response);
        if (root.is_object()) {
            std::wstring code = PickString(root, { "code", "errorCode", "error_code" });
            std::wstring mapped = LoginErrorMessageForCode(code, status);
            if (!mapped.empty())
                return mapped;

            std::wstring message = PickString(root, { "error", "message" });
            if (!message.empty() && status < 500)
                return message;
        }
    }
    catch (...) {
    }

    std::wstring mapped = LoginErrorMessageForCode(L"", status);
    if (!mapped.empty())
        return mapped;
    return fallback.empty() ? L"Login failed." : fallback;
}

static void AttemptLogin(LoginContext* ctx)
{
    if (!ctx || !ctx->session)
        return;

    std::wstring username = Trim(GetWindowTextString(ctx->usernameEdit));
    std::wstring password = GetWindowTextString(ctx->passwordEdit);
    std::wstring position = ComboText(ctx->positionCombo);
    std::wstring pod = ComboText(ctx->podCombo);

    if (username.empty() || password.empty() || position.empty() || pod.empty()) {
        SetWindowTextSafe(ctx->statusLabel, L"Username, password, position and pod are required.");
        return;
    }

    SetWindowTextSafe(ctx->statusLabel, L"Signing in...");
    EnableWindow(ctx->hwnd, FALSE);

    std::string body = "{";
    body += "\"username\":" + JsonEscape(username);
    body += ",\"password\":" + JsonEscape(password);
    body += ",\"position\":" + JsonEscape(position);
    body += ",\"pod\":" + JsonEscape(pod);
    body += ",\"clientVersion\":" + JsonEscape(kClientVersion);
    body += ",\"platform\":" + JsonEscape(kClientPlatform);
    body += "}";

    std::string response;
    std::wstring error;
    ClientSession session;
    DWORD httpStatus = 0;
    bool httpOk = HttpPostJsonTextStatus(AppendApiPath(ctx->serverBaseUrl, L"/api/auth/login"), body, response, httpStatus, error);
    bool ok = httpOk && ParseLoginResponse(response, session, error);
    if (!httpOk || !ok)
        error = LoginErrorMessageFromResponse(response, httpStatus, error);

    EnableWindow(ctx->hwnd, TRUE);
    SetForegroundWindow(ctx->hwnd);

    if (!ok) {
        if (error.empty())
            error = L"Login failed.";
        SetWindowTextSafe(ctx->statusLabel, error);
        return;
    }

    if (session.displayName.empty())
        session.displayName = username;
    if (session.username.empty())
        session.username = username;
    if (session.position.empty())
        session.position = position;
    if (session.pod.empty())
        session.pod = pod;

    RememberedLogin remembered;
    remembered.remember = ctx->rememberCheck &&
        SendMessageW(ctx->rememberCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (remembered.remember) {
        remembered.username = username;
        remembered.password = password;
        remembered.position = position;
        remembered.pod = pod;
    }
    SaveRememberedLogin(remembered);

    *ctx->session = std::move(session);
    ctx->accepted = true;
    ctx->done = true;
    DestroyWindow(ctx->hwnd);
}

static void AcceptOfflineMode(LoginContext* ctx)
{
    if (!ctx || !ctx->session)
        return;

    ClientSession session;
    session.offlineMode = true;
    session.username = L"Offline";

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

    CreateLabel(ctx->hwnd, L"Username:", 24, 96, 130, 22, font);
    ctx->usernameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | ES_AUTOHSCROLL, 154, 92, 270, 26, ctx->hwnd, ControlMenuId(IDC_LOGIN_USERNAME), ctx->hInst, nullptr);

    CreateLabel(ctx->hwnd, L"Password:", 24, 134, 130, 22, font);
    ctx->passwordEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, 154, 130, 270, 26, ctx->hwnd, ControlMenuId(IDC_LOGIN_PASSWORD), ctx->hInst, nullptr);

    CreateLabel(ctx->hwnd, L"Position:", 24, 172, 130, 22, font);
    ctx->positionCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 154, 168, 270, 160, ctx->hwnd, ControlMenuId(IDC_LOGIN_POSITION), ctx->hInst, nullptr);
    AddComboItem(ctx->positionCombo, L"Administrator");
    AddComboItem(ctx->positionCombo, L"Supervisor");
    AddComboItem(ctx->positionCombo, L"Manager");
    AddComboItem(ctx->positionCombo, L"ERC");
    SendMessageW(ctx->positionCombo, CB_SETCURSEL, 3, 0);

    CreateLabel(ctx->hwnd, L"Pod:", 24, 210, 130, 22, font);
    ctx->podCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 154, 206, 270, 220, ctx->hwnd, ControlMenuId(IDC_LOGIN_POD), ctx->hInst, nullptr);
    for (int i = 1; i <= 9; ++i) {
        std::wstring pod = L"Pod " + std::to_wstring(i);
        AddComboItem(ctx->podCombo, pod.c_str());
    }
    SendMessageW(ctx->podCombo, CB_SETCURSEL, 0, 0);

    ctx->rememberCheck = CreateWindowExW(0, L"BUTTON", L"Remember username and password", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 154, 246, 270, 24, ctx->hwnd, ControlMenuId(IDC_LOGIN_REMEMBER), ctx->hInst, nullptr);
    ctx->statusLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 24, 278, 400, 42, ctx->hwnd, ControlMenuId(IDC_LOGIN_STATUS), ctx->hInst, nullptr);

    RememberedLogin remembered = LoadRememberedLogin();
    if (remembered.remember) {
        SetWindowTextSafe(ctx->usernameEdit, remembered.username);
        SetWindowTextSafe(ctx->passwordEdit, remembered.password);
        SetComboSelection(ctx->positionCombo, remembered.position, 3);
        SetComboSelection(ctx->podCombo, remembered.pod, 0);
        SendMessageW(ctx->rememberCheck, BM_SETCHECK, BST_CHECKED, 0);
    }
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
            for (HWND h : { ctx->usernameEdit, ctx->passwordEdit, ctx->positionCombo, ctx->podCombo, ctx->rememberCheck, ctx->statusLabel })
                SetChildFont(h, font);
            HWND offline = CreateWindowExW(0, L"BUTTON", L"Offline Mode", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 24, 338, 128, 32, hwnd, ControlMenuId(IDC_LOGIN_OFFLINE), ctx->hInst, nullptr);
            HWND login = CreateWindowExW(0, L"BUTTON", L"Login", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 222, 338, 96, 32, hwnd, ControlMenuId(IDC_LOGIN_BUTTON), ctx->hInst, nullptr);
            HWND cancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 328, 338, 96, 32, hwnd, ControlMenuId(IDC_LOGIN_CANCEL), ctx->hInst, nullptr);
            SetChildFont(offline, font);
            SetChildFont(login, font);
            SetChildFont(cancel, font);
            SendMessageW(hwnd, DM_SETDEFID, IDC_LOGIN_BUTTON, 0);
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
        if (LOWORD(wParam) == IDC_LOGIN_OFFLINE && HIWORD(wParam) == BN_CLICKED) {
            AcceptOfflineMode(ctx);
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
        430,
        nullptr,
        nullptr,
        hInst,
        &ctx);

    if (!hwnd)
        return false;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(ctx.usernameEdit);

    MSG msg{};
    while (!ctx.done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (IsWindow(hwnd) && IsDialogMessageW(hwnd, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return ctx.accepted && (sessionOut.authenticated || sessionOut.offlineMode);
}
