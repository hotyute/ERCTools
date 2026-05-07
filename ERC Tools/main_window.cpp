#include "main_window.h"
#include "app_state.h"
#include "http.h"
#include "map_view.h"
#include "parsing.h"
#include "util.h"


constexpr int IDC_URL_EDIT = 1001;
constexpr int IDC_REFRESH_BTN = 1002;
constexpr int IDC_SEARCH_EDIT = 1003;
constexpr int IDC_SEVERITY_COMBO = 1004;
constexpr int IDC_LISTVIEW = 1005;
constexpr int IDC_DETAILS_EDIT = 1006;
constexpr int IDC_DOWNLOAD_BOUNDARY_BTN = 1007;
constexpr int IDC_HEADER_LABEL = 1008;
constexpr int IDC_URL_LABEL = 1009;
constexpr int IDC_SEARCH_LABEL = 1010;
constexpr int IDC_SEVERITY_LABEL = 1011;
constexpr int IDC_STATUS_BAR = 1012;
constexpr int IDC_SERVER_EDIT = 1013;
constexpr int IDC_SERVER_LABEL = 1014;
constexpr int IDC_CHAT_HISTORY = 1015;
constexpr int IDC_CHAT_EDIT = 1016;
constexpr int IDC_CHAT_SEND_BTN = 1017;
constexpr int IDC_NOTE_EDIT = 1018;
constexpr int IDC_NOTE_BTN = 1019;
constexpr int IDC_NOTE_LABEL = 1020;

struct FeedResult
{
    bool ok = false;
    std::wstring error;
    std::vector<TrafficAlert> alerts;
};

struct BoundaryDownloadResult
{
    bool ok = false;
    std::wstring error;
    std::filesystem::path filePath;
};

enum class ServerAction
{
    Poll,
    SendChat,
    SendNote
};

struct ServerResult
{
    ServerAction action = ServerAction::Poll;
    bool ok = false;
    std::wstring error;
    std::vector<ChatMessage> chat;
    std::vector<MapNote> notes;
};

static std::wstring AppendPath(std::wstring base, const wchar_t* path)
{
    base = NormalizeUrl(base);
    while (!base.empty() && base.back() == L'/')
        base.pop_back();
    return base + path;
}

static std::vector<ChatMessage> ParseChatMessages(const json& root)
{
    std::vector<ChatMessage> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("chat") && root["chat"].is_array())
        arr = &root["chat"];
    else if (root.contains("messages") && root["messages"].is_array())
        arr = &root["messages"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        ChatMessage msg;
        msg.author = PickString(item, { "author", "user", "name" });
        msg.text = PickString(item, { "text", "message", "body" });
        msg.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        if (!msg.text.empty())
            out.push_back(std::move(msg));
    }

    return out;
}

static std::vector<MapNote> ParseMapNotes(const json& root)
{
    std::vector<MapNote> out;
    const json* arr = nullptr;
    if (root.is_array())
        arr = &root;
    else if (root.contains("notes") && root["notes"].is_array())
        arr = &root["notes"];

    if (!arr)
        return out;

    for (const auto& item : *arr) {
        if (!item.is_object())
            continue;

        MapNote note;
        note.id = PickString(item, { "id", "noteId" });
        note.author = PickString(item, { "author", "user", "name" });
        note.text = PickString(item, { "text", "note", "body" });
        note.timestamp = PickString(item, { "timestamp", "time", "createdAt" });
        bool hasLat = PickDouble(item, { "lat", "latitude" }, note.latitude);
        bool hasLon = PickDouble(item, { "lon", "lng", "longitude" }, note.longitude);
        if (hasLat && hasLon && !note.text.empty())
            out.push_back(std::move(note));
    }

    return out;
}


static bool FetchTrafficEnglandAlerts(std::vector<TrafficAlert>& alertsOut, std::wstring& errorOut)
{
    alertsOut.clear();
    errorOut.clear();

    constexpr size_t kPageSize = 100;
    constexpr size_t kMaxPages = 20;

    for (size_t page = 0; page < kMaxPages; ++page) {
        const size_t start = page * kPageSize;
        std::string body;
        std::wstring httpError;
        if (!HttpGetText(BuildTrafficEnglandAlertsApiUrl(start, kPageSize), body, httpError)) {
            errorOut = L"Traffic England alerts API failed: " + httpError;
            return false;
        }

        std::wstring parseError;
        std::vector<TrafficAlert> pageAlerts = ParseTrafficAlerts(body, parseError);
        if (pageAlerts.empty()) {
            if (page == 0) {
                errorOut = parseError.empty()
                    ? L"Traffic England alerts API returned no alerts."
                    : L"Traffic England alerts API could not be parsed: " + parseError;
                return false;
            }
            break;
        }

        const size_t returned = pageAlerts.size();
        alertsOut.insert(alertsOut.end(), pageAlerts.begin(), pageAlerts.end());

        if (returned < kPageSize)
            break;
    }

    return !alertsOut.empty();
}

class MainWindow
{
public:
    bool Create(HINSTANCE hInst)
    {
        m_hInst = hInst;
        if (!RegisterClass())
            return false;

        m_hwnd = CreateWindowExW(
            0,
            kMainClassName,
            L"Traffic England Alerts Map",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1600,
            950,
            nullptr,
            nullptr,
            hInst,
            this);

        return m_hwnd != nullptr;
    }

    int Run(int nCmdShow)
    {
        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

private:
    static constexpr const wchar_t* kBaseTitle = L"Traffic England Alerts Map";

    static bool RegisterClass()
    {
        static bool registered = false;
        if (registered)
            return true;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = MainWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kMainClassName;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        registered = true;
        return true;
    }

    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow* self = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        }
        else {
            self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
            return self->HandleMessage(msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            OnCreate();
            return 0;

        case WM_SIZE:
            Layout();
            return 0;

        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_NOTIFY:
            OnNotify(reinterpret_cast<NMHDR*>(lParam));
            return 0;

        case WM_TIMER:
            if (wParam == 1)
                RefreshFeedAsync();
            else if (wParam == 2)
                PollServerAsync();
            return 0;

        case WM_APP_FEED_READY:
            OnFeedReady(reinterpret_cast<FeedResult*>(lParam));
            return 0;

        case WM_APP_BOUNDARY_READY:
            OnBoundaryReady(reinterpret_cast<BoundaryDownloadResult*>(lParam));
            return 0;

        case WM_APP_SERVER_READY:
            OnServerReady(reinterpret_cast<ServerResult*>(lParam));
            return 0;

        case WM_DESTROY:
            g_appQuitting.store(true);
            KillTimer(m_hwnd, 1);
            KillTimer(m_hwnd, 2);
            if (m_font) {
                DeleteObject(m_font);
                m_font = nullptr;
            }
            if (m_headerFont) {
                DeleteObject(m_headerFont);
                m_headerFont = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void OnCreate()
    {
        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icc);

        EnableModernWindowFrame(m_hwnd);

        m_font = CreateUiFont(10);
        m_headerFont = CreateUiFont(16, FW_SEMIBOLD);

        m_headerLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Traffic England Alerts",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_HEADER_LABEL),
            m_hInst,
            nullptr);

        m_urlLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Alerts endpoint",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_URL_LABEL),
            m_hInst,
            nullptr);

        m_searchLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Search",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEARCH_LABEL),
            m_hInst,
            nullptr);

        m_severityLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"Severity",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEVERITY_LABEL),
            m_hInst,
            nullptr);

        m_serverLabel = CreateWindowExW(
            0, L"STATIC", L"Collaboration server", WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_SERVER_LABEL), m_hInst, nullptr);

        m_noteLabel = CreateWindowExW(
            0, L"STATIC", L"Map note (double-click map to choose a location)", WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_NOTE_LABEL), m_hInst, nullptr);

        m_urlEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_URL_EDIT),
            m_hInst,
            nullptr);

        m_serverEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_SERVER_EDIT), m_hInst, nullptr);

        m_refreshBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_REFRESH_BTN),
            m_hInst,
            nullptr);

        m_boundaryBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"Download UK boundary",
            WS_CHILD | WS_VISIBLE | BS_COMMANDLINK,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_DOWNLOAD_BOUNDARY_BTN),
            m_hInst,
            nullptr);

        m_searchEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEARCH_EDIT),
            m_hInst,
            nullptr);

        m_severityCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_SEVERITY_COMBO),
            m_hInst,
            nullptr);

        m_listView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_LISTVIEW),
            m_hInst,
            nullptr);

        m_detailsEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_DETAILS_EDIT),
            m_hInst,
            nullptr);

        m_chatHistory = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_CHAT_HISTORY), m_hInst, nullptr);

        m_chatEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_CHAT_EDIT), m_hInst, nullptr);

        m_chatSendBtn = CreateWindowExW(
            0, L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_CHAT_SEND_BTN), m_hInst, nullptr);

        m_noteEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_NOTE_EDIT), m_hInst, nullptr);

        m_noteBtn = CreateWindowExW(
            0, L"BUTTON", L"Leave note", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(IDC_NOTE_BTN), m_hInst, nullptr);

        m_statusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            L"Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hwnd,
            reinterpret_cast<HMENU>(IDC_STATUS_BAR),
            m_hInst,
            nullptr);

        if (!m_headerLabel || !m_urlLabel || !m_searchLabel || !m_severityLabel ||
            !m_urlEdit || !m_serverLabel || !m_serverEdit || !m_refreshBtn || !m_boundaryBtn || !m_searchEdit || !m_severityCombo || !m_listView || !m_detailsEdit || !m_chatHistory || !m_chatEdit || !m_chatSendBtn || !m_noteLabel || !m_noteEdit || !m_noteBtn || !m_statusBar)
        {
            MessageBoxW(m_hwnd, L"Failed to create one or more child controls.", L"Traffic England Alerts Map", MB_ICONERROR);
            return;
        }

        for (HWND h : { m_urlLabel, m_serverLabel, m_searchLabel, m_severityLabel, m_noteLabel, m_urlEdit, m_serverEdit, m_refreshBtn, m_boundaryBtn, m_searchEdit, m_severityCombo, m_listView, m_detailsEdit, m_chatHistory, m_chatEdit, m_chatSendBtn, m_noteEdit, m_noteBtn, m_statusBar }) {
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
            ApplyExplorerTheme(h);
        }
        SendMessageW(m_headerLabel, WM_SETFONT, reinterpret_cast<WPARAM>(m_headerFont), TRUE);

        SendMessageW(m_searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Filter by road, region, or description"));
        SendMessageW(m_urlEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"https://www.trafficengland.com/traffic-alerts"));
        SendMessageW(m_serverEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"http://localhost:8080"));
        SendMessageW(m_chatEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Message local responders..."));
        SendMessageW(m_noteEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Note text for the selected map location"));
        SendMessageW(m_boundaryBtn, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(L"Optional: cache a high-detail UK outline for the native map."));

        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"All"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Severe"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Moderate"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Minor"));
        SendMessageW(m_severityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Unknown"));
        SendMessageW(m_severityCombo, CB_SETCURSEL, 0, 0);

        SendMessageW(m_listView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        std::wstring s0 = L"Severity";
        col.pszText = const_cast<LPWSTR>(s0.c_str());
        col.cx = 90;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&col));

        std::wstring s1 = L"Alert";
        col.pszText = const_cast<LPWSTR>(s1.c_str());
        col.cx = 260;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 1, reinterpret_cast<LPARAM>(&col));

        std::wstring s2 = L"Updated";
        col.pszText = const_cast<LPWSTR>(s2.c_str());
        col.cx = 160;
        SendMessageW(m_listView, LVM_INSERTCOLUMNW, 2, reinterpret_cast<LPARAM>(&col));

        m_map.Create(m_hwnd, 0, 0, 100, 100);
        if (!m_map.Hwnd()) {
            MessageBoxW(m_hwnd, L"Could not create the native map view.", L"Traffic England Alerts Map", MB_ICONERROR);
            return;
        }

        std::wstring boundaryError;
        if (!m_map.LoadUkBoundaryFromFile(GetBoundaryCachePath(), &boundaryError)) {
            OutputDebugStringW((L"Boundary cache load: " + boundaryError + L"\n").c_str());
        }

        m_map.SetSelectCallback([this](const std::wstring& id) {
            SelectAlertById(id, true);
            });
        m_map.SetNoteLocationCallback([this](double lat, double lon) {
            m_pendingNoteLat = lat;
            m_pendingNoteLon = lon;
            m_hasPendingNoteLocation = true;
            SetStatusText(L"Note location selected: " + std::to_wstring(lat) + L", " + std::to_wstring(lon));
            SetFocus(m_noteEdit);
            });

        SetWindowTextW(m_urlEdit, L"https://www.trafficengland.com/traffic-alerts");
        SetWindowTextW(m_serverEdit, L"http://localhost:8080");

        Layout();
        SetStatusText(L"Ready.");
        SetTimer(m_hwnd, 1, 5 * 60 * 1000, nullptr);
        SetTimer(m_hwnd, 2, 8 * 1000, nullptr);

        RefreshFeedAsync();
        PollServerAsync();
    }

    void Layout()
    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        LONG width = std::max<LONG>(1L, rc.right - rc.left);
        LONG height = std::max<LONG>(1L, rc.bottom - rc.top);

        const int pad = 16;
        const int labelH = 18;
        const int controlH = 28;
        const int topBarH = 148;
        const int statusH = 24;

        MoveWindow(m_headerLabel, pad, 12, std::max<LONG>(200L, width - pad * 2), 28, TRUE);

        const int endpointY = 52;
        const LONG refreshW = 132;
        LONG topEditW = std::max<LONG>(220L, (width - refreshW - pad * 4) / 2);
        MoveWindow(m_urlLabel, pad, endpointY, 160, labelH, TRUE);
        MoveWindow(m_urlEdit, pad, endpointY + labelH + 2, topEditW, controlH, TRUE);
        MoveWindow(m_serverLabel, pad * 2 + topEditW, endpointY, 180, labelH, TRUE);
        MoveWindow(m_serverEdit, pad * 2 + topEditW, endpointY + labelH + 2, std::max<LONG>(220L, width - refreshW - topEditW - pad * 4), controlH, TRUE);
        MoveWindow(m_refreshBtn, width - refreshW - pad, endpointY + labelH + 2, refreshW, controlH, TRUE);

        int bodyTop = topBarH;
        int leftW = 440;
        int detailsH = 185;
        int chatH = 154;

        int leftX = pad;
        int leftY = bodyTop + pad;
        int leftInnerW = leftW - pad * 2;

        MoveWindow(m_searchLabel, leftX, leftY, leftInnerW, labelH, TRUE);
        MoveWindow(m_searchEdit, leftX, leftY + labelH + 2, leftInnerW, controlH, TRUE);

        const int severityY = leftY + labelH + controlH + 12;
        MoveWindow(m_severityLabel, leftX, severityY, leftInnerW, labelH, TRUE);
        MoveWindow(m_severityCombo, leftX, severityY + labelH + 2, leftInnerW, 180, TRUE);

        const int boundaryY = severityY + labelH + controlH + 16;
        MoveWindow(m_boundaryBtn, leftX, boundaryY, leftInnerW, 56, TRUE);

        int listTop = boundaryY + 68;
        int bodyHeight = height - bodyTop - statusH - pad * 2;
        int listHeight = std::max(110, bodyHeight - (listTop - leftY) - detailsH - chatH - 58);

        MoveWindow(m_listView, leftX, listTop, leftInnerW, listHeight, TRUE);
        int detailsTop = listTop + listHeight + 10;
        MoveWindow(m_detailsEdit, leftX, detailsTop, leftInnerW, detailsH, TRUE);
        int chatTop = detailsTop + detailsH + 10;
        MoveWindow(m_chatHistory, leftX, chatTop, leftInnerW, chatH - controlH - 8, TRUE);
        MoveWindow(m_chatEdit, leftX, chatTop + chatH - controlH, leftInnerW - 72, controlH, TRUE);
        MoveWindow(m_chatSendBtn, leftX + leftInnerW - 66, chatTop + chatH - controlH, 66, controlH, TRUE);

        int mapX = leftW + pad;
        int mapY = bodyTop + pad;
        LONG mapW = std::max<LONG>(100L, width - mapX - pad);
        LONG mapH = std::max<LONG>(100L, height - mapY - statusH - pad - 66);

        MoveWindow(m_map.Hwnd(), mapX, mapY, mapW, mapH, TRUE);
        int noteY = mapY + mapH + 8;
        MoveWindow(m_noteLabel, mapX, noteY, mapW, labelH, TRUE);
        MoveWindow(m_noteEdit, mapX, noteY + labelH + 2, std::max<LONG>(180L, mapW - 132), controlH, TRUE);
        MoveWindow(m_noteBtn, mapX + mapW - 122, noteY + labelH + 2, 122, controlH, TRUE);

        SendMessageW(m_statusBar, WM_SIZE, 0, 0);

        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 0, 94);
        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 1, std::max(120, leftInnerW - 264));
        SendMessageW(m_listView, LVM_SETCOLUMNWIDTH, 2, 160);
    }

    void OnCommand(int id, int code)
    {
        switch (id) {
        case IDC_REFRESH_BTN:
            if (code == BN_CLICKED)
                RefreshFeedAsync();
            break;

        case IDC_SEARCH_EDIT:
            if (code == EN_CHANGE) {
                size_t visible = ApplyFilters(false);
                SetStatusText(L"Showing " + std::to_wstring(visible) + L" alert(s).");
            }
            break;

        case IDC_SEVERITY_COMBO:
            if (code == CBN_SELCHANGE) {
                size_t visible = ApplyFilters(false);
                SetStatusText(L"Showing " + std::to_wstring(visible) + L" alert(s).");
            }
            break;
        case IDC_DOWNLOAD_BOUNDARY_BTN:
            if (code == BN_CLICKED)
                DownloadBoundaryFromGitHubAsync();
            break;

        case IDC_CHAT_SEND_BTN:
            if (code == BN_CLICKED)
                SendChatAsync();
            break;

        case IDC_NOTE_BTN:
            if (code == BN_CLICKED)
                SendNoteAsync();
            break;
        }
    }

    void OnNotify(NMHDR* nmh)
    {
        if (!nmh)
            return;

        if (nmh->hwndFrom == m_listView && nmh->code == LVN_ITEMCHANGED) {
            if (m_programmaticSelection)
                return;

            NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(nmh);
            if ((lv->uChanged & LVIF_STATE) && (lv->uNewState & LVIS_SELECTED)) {
                int selected = static_cast<int>(SendMessageW(m_listView, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (selected >= 0 && selected < static_cast<int>(m_filteredAlerts.size())) {
                    const std::wstring& id = m_filteredAlerts[static_cast<size_t>(selected)].id;
                    SelectAlertById(id, true);
                }
            }
        }
    }

    void SetStatusText(const std::wstring& text)
    {
        std::wstring title = kBaseTitle;
        if (!text.empty()) {
            title += L" - ";
            title += text;
        }
        SetWindowTextW(m_hwnd, title.c_str());
        if (m_statusBar)
            SendMessageW(m_statusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }

    void RefreshFeedAsync()
    {
        if (g_fetchInProgress.exchange(true)) {
            SetStatusText(L"Already fetching alerts...");
            return;
        }

        std::wstring url = NormalizeUrl(GetWindowTextString(m_urlEdit));
        if (url.empty()) {
            g_fetchInProgress.store(false);
            SetStatusText(L"Please enter a feed URL.");
            return;
        }

        SetStatusText(L"Fetching alerts...");

        HWND hwnd = m_hwnd;

        std::thread([hwnd, url]() {
            auto* result = new FeedResult{};
            std::string body;
            std::wstring error;

            if (IsTrafficEnglandAlertsPageUrl(url)) {
                std::vector<TrafficAlert> alerts;
                if (FetchTrafficEnglandAlerts(alerts, error)) {
                    result->ok = true;
                    result->alerts = std::move(alerts);
                }
                else {
                    result->ok = false;
                    result->error = error + L" Showing sample data.";
                    result->alerts = SampleAlerts();
                }
            }
            else if (HttpGetText(url, body, error)) {
                std::wstring parseError;
                std::vector<TrafficAlert> alerts = ParseTrafficAlerts(body, parseError);

                if (!alerts.empty()) {
                    result->ok = true;
                    result->alerts = std::move(alerts);
                }
                else {
                    result->ok = false;
                    result->error = parseError.empty()
                        ? L"Feed could not be parsed. Showing sample data."
                        : parseError + L" Showing sample data.";
                    result->alerts = SampleAlerts();
                }
            }
            else {
                result->ok = false;
                result->error = error + L" Showing sample data.";
                result->alerts = SampleAlerts();
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_fetchInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_FEED_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_fetchInProgress.store(false);
            }
            }).detach();
    }

    void OnFeedReady(FeedResult* result)
    {
        g_fetchInProgress.store(false);

        if (!result)
            return;

        m_allAlerts = result->alerts;
        std::sort(m_allAlerts.begin(), m_allAlerts.end(),
            [](const TrafficAlert& a, const TrafficAlert& b)
            {
                std::wstring ar = ToLower(Trim(a.road.empty() ? a.region : a.road));
                std::wstring br = ToLower(Trim(b.road.empty() ? b.region : b.road));

                if (ar != br)
                    return ar < br;

                return ToLower(Trim(a.title)) < ToLower(Trim(b.title));
            });
        delete result;

        size_t visible = ApplyFilters(true);

        if (m_allAlerts.empty()) {
            SetStatusText(L"No alerts available.");
        }
        else {
            SetStatusText(L"Loaded " + std::to_wstring(visible) + L" alert(s).");
        }
    }

    bool TextFilterMatches(const TrafficAlert& a) const
    {
        std::wstring q = ToLower(Trim(GetWindowTextString(m_searchEdit)));
        if (q.empty())
            return true;

        std::wstring hay =
            ToLower(a.id + L" " + a.title + L" " + a.description + L" " +
                a.road + L" " + a.region + L" " + a.severity);

        return hay.find(q) != std::wstring::npos;
    }

    bool SeverityFilterMatches(const TrafficAlert& a) const
    {
        int idx = static_cast<int>(SendMessageW(m_severityCombo, CB_GETCURSEL, 0, 0));
        if (idx <= 0)
            return true;

        std::wstring filter;
        switch (idx) {
        case 1: filter = L"severe"; break;
        case 2: filter = L"moderate"; break;
        case 3: filter = L"minor"; break;
        case 4: filter = L"unknown"; break;
        default: return true;
        }

        return SeverityBucket(a.severity) == filter;
    }

    size_t ApplyFilters(bool fitMap)
    {
        std::wstring previousSelected = m_selectedId;

        m_filteredAlerts.clear();
        for (size_t i = 0; i < m_allAlerts.size(); ++i) {
            if (TextFilterMatches(m_allAlerts[i]) && SeverityFilterMatches(m_allAlerts[i]))
                m_filteredAlerts.push_back(m_allAlerts[i]);
        }

        m_programmaticSelection = true;
        SendMessageW(m_listView, LVM_DELETEALLITEMS, 0, 0);

        for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
            const TrafficAlert& a = m_filteredAlerts[i];

            std::wstring sev = BuildSeverityDisplay(a.severity);
            std::wstring summary = BuildAlertSummary(a);
            std::wstring updated = a.updatedText.empty() ? L"" : a.updatedText;

            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(sev.c_str());

            int row = static_cast<int>(
                SendMessageW(m_listView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item))
                );

            if (row >= 0) {
                LVITEMW sub{};
                sub.iSubItem = 1;
                sub.pszText = const_cast<LPWSTR>(summary.c_str());
                SendMessageW(m_listView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));

                sub.iSubItem = 2;
                sub.pszText = const_cast<LPWSTR>(updated.c_str());
                SendMessageW(m_listView, LVM_SETITEMTEXTW, row, reinterpret_cast<LPARAM>(&sub));
            }
        }

        m_programmaticSelection = false;

        m_map.SetAlerts(m_filteredAlerts);

        if (m_filteredAlerts.empty()) {
            m_selectedId.clear();
            m_map.SetSelectedId(L"");
            SetWindowTextSafe(m_detailsEdit, L"No alerts match the current filters.");
        }
        else {
            bool foundPrevious = false;
            for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
                if (m_filteredAlerts[i].id == previousSelected) {
                    foundPrevious = true;
                    break;
                }
            }

            if (foundPrevious)
                m_selectedId = previousSelected;
            else
                m_selectedId = m_filteredAlerts.front().id;

            SelectAlertById(m_selectedId, false);
        }

        if (fitMap)
            m_map.FitToAlerts();

        return m_filteredAlerts.size();
    }

    int FindAlertIndexById(const std::wstring& id) const
    {
        for (size_t i = 0; i < m_filteredAlerts.size(); ++i) {
            if (m_filteredAlerts[i].id == id)
                return static_cast<int>(i);
        }

        return -1;
    }

    void SelectAlertById(const std::wstring& id, bool centerMap)
    {
        int idx = FindAlertIndexById(id);
        if (idx < 0)
            return;

        m_selectedId = id;

        const TrafficAlert& a = m_filteredAlerts[static_cast<size_t>(idx)];
        SetWindowTextSafe(m_detailsEdit, BuildAlertDetails(a));
        m_map.SetSelectedId(id);

        if (centerMap)
            m_map.CenterOnAlert(id);

        m_programmaticSelection = true;

        LVITEMW state{};
        state.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        state.state = LVIS_SELECTED | LVIS_FOCUSED;
        state.iItem = idx;
        state.iSubItem = 0;

        SendMessageW(m_listView, LVM_SETITEMSTATE, idx, reinterpret_cast<LPARAM>(&state));
        SendMessageW(m_listView, LVM_ENSUREVISIBLE, idx, TRUE);

        m_programmaticSelection = false;
    }


    std::wstring ServerBaseUrl() const
    {
        return NormalizeUrl(GetWindowTextString(m_serverEdit));
    }

    void RenderChatHistory()
    {
        std::wstring text;
        for (const auto& msg : m_chatMessages) {
            if (!msg.timestamp.empty()) {
                text += L"[" + msg.timestamp + L"] ";
            }
            if (!msg.author.empty())
                text += msg.author + L": ";
            text += msg.text + L"\r\n";
        }
        SetWindowTextSafe(m_chatHistory, text);
    }

    void PollServerAsync()
    {
        if (m_serverRequestInProgress.exchange(true))
            return;

        std::wstring server = ServerBaseUrl();
        if (server.empty()) {
            m_serverRequestInProgress.store(false);
            return;
        }

        HWND hwnd = m_hwnd;
        std::thread([hwnd, server]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::Poll;

            std::string chatBody;
            std::wstring chatError;
            if (HttpGetText(AppendPath(server, L"/api/chat"), chatBody, chatError)) {
                try {
                    result->chat = ParseChatMessages(json::parse(chatBody));
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    result->error = L"Chat parse failed: " + Utf8ToWide(e.what());
                }
            }
            else {
                result->error = L"Chat poll failed: " + chatError;
            }

            std::string noteBody;
            std::wstring noteError;
            if (HttpGetText(AppendPath(server, L"/api/notes"), noteBody, noteError)) {
                try {
                    result->notes = ParseMapNotes(json::parse(noteBody));
                    result->ok = true;
                }
                catch (const std::exception& e) {
                    if (!result->error.empty())
                        result->error += L" ";
                    result->error += L"Notes parse failed: " + Utf8ToWide(e.what());
                }
            }
            else if (result->error.empty()) {
                result->error = L"Notes poll failed: " + noteError;
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                return;
            }
            PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            }).detach();
    }

    void SendChatAsync()
    {
        std::wstring text = Trim(GetWindowTextString(m_chatEdit));
        if (text.empty())
            return;

        SetWindowTextSafe(m_chatEdit, L"");
        ChatMessage local{ L"Me", text, L"pending" };
        m_chatMessages.push_back(local);
        RenderChatHistory();

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        std::thread([hwnd, server, text]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendChat;
            std::string response;
            std::wstring error;
            std::string body = "{\"author\":" + JsonEscape(L"ERCTools") + ",\"text\":" + JsonEscape(text) + "}";
            result->ok = HttpPostJsonText(AppendPath(server, L"/api/chat"), body, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            }).detach();
    }

    void SendNoteAsync()
    {
        std::wstring text = Trim(GetWindowTextString(m_noteEdit));
        if (text.empty()) {
            SetStatusText(L"Type note text first.");
            return;
        }
        if (!m_hasPendingNoteLocation) {
            SetStatusText(L"Double-click the map to choose where the note belongs.");
            return;
        }

        MapNote note;
        note.author = L"Me";
        note.text = text;
        note.timestamp = L"pending";
        note.latitude = m_pendingNoteLat;
        note.longitude = m_pendingNoteLon;
        m_notes.push_back(note);
        m_map.SetNotes(m_notes);
        SetWindowTextSafe(m_noteEdit, L"");

        std::wstring server = ServerBaseUrl();
        HWND hwnd = m_hwnd;
        double lat = m_pendingNoteLat;
        double lon = m_pendingNoteLon;
        std::thread([hwnd, server, text, lat, lon]() {
            auto* result = new ServerResult{};
            result->action = ServerAction::SendNote;
            std::string response;
            std::wstring error;
            std::string body = "{\"author\":" + JsonEscape(L"ERCTools") +
                ",\"text\":" + JsonEscape(text) +
                ",\"latitude\":" + std::to_string(lat) +
                ",\"longitude\":" + std::to_string(lon) + "}";
            result->ok = HttpPostJsonText(AppendPath(server, L"/api/notes"), body, response, error);
            result->error = error;
            if (!g_appQuitting.load() && IsWindow(hwnd))
                PostMessageW(hwnd, WM_APP_SERVER_READY, 0, reinterpret_cast<LPARAM>(result));
            else
                delete result;
            }).detach();
    }

    void OnServerReady(ServerResult* result)
    {
        m_serverRequestInProgress.store(false);
        if (!result)
            return;

        if (result->action == ServerAction::Poll && result->ok) {
            if (!result->chat.empty()) {
                m_chatMessages = std::move(result->chat);
                RenderChatHistory();
            }
            if (!result->notes.empty()) {
                m_notes = std::move(result->notes);
                m_map.SetNotes(m_notes);
            }
        }
        else if (result->action == ServerAction::SendChat) {
            SetStatusText(result->ok ? L"Chat message sent." : L"Chat send failed; kept locally.");
            PollServerAsync();
        }
        else if (result->action == ServerAction::SendNote) {
            SetStatusText(result->ok ? L"Map note shared." : L"Note share failed; kept locally.");
            PollServerAsync();
        }

        delete result;
    }

    void DownloadBoundaryFromGitHubAsync()
    {
        if (g_boundaryDownloadInProgress.exchange(true)) {
            SetStatusText(L"Boundary download already in progress...");
            return;
        }

        SetStatusText(L"Downloading UK boundary from geoBoundaries...");

        HWND hwnd = m_hwnd;

        std::thread([hwnd]() {
            auto* result = new BoundaryDownloadResult{};
            std::vector<BYTE> bytes;
            std::wstring error;

            if (!HttpGetBinary(kUkBoundarySourceUrl, bytes, error)) {
                result->ok = false;
                result->error = L"Boundary download failed: " + error;
            }
            else {
                std::filesystem::path path = GetBoundaryCachePath();
                if (!SaveBinaryToFile(path, bytes)) {
                    result->ok = false;
                    result->error = L"Boundary downloaded but could not be saved locally.";
                }
                else {
                    result->ok = true;
                    result->filePath = path;
                }
            }

            if (g_appQuitting.load() || !IsWindow(hwnd)) {
                delete result;
                g_boundaryDownloadInProgress.store(false);
                return;
            }

            if (!PostMessageW(hwnd, WM_APP_BOUNDARY_READY, 0, reinterpret_cast<LPARAM>(result))) {
                delete result;
                g_boundaryDownloadInProgress.store(false);
            }
            }).detach();
    }

    void OnBoundaryReady(BoundaryDownloadResult* result)
    {
        g_boundaryDownloadInProgress.store(false);

        if (!result)
            return;

        if (!result->ok) {
            SetStatusText(result->error);
            delete result;
            return;
        }

        std::wstring loadError;
        if (m_map.LoadUkBoundaryFromFile(result->filePath, &loadError)) {
            SetStatusText(L"UK boundary downloaded and loaded.");

        }
        else {
            SetStatusText(L"Boundary downloaded, but could not load it.");
            OutputDebugStringW((L"Boundary load failed: " + loadError + L"\n").c_str());
        }

        delete result;
    }

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInst = nullptr;
    HFONT m_font = nullptr;
    HFONT m_headerFont = nullptr;

    HWND m_headerLabel = nullptr;
    HWND m_urlLabel = nullptr;
    HWND m_searchLabel = nullptr;
    HWND m_severityLabel = nullptr;
    HWND m_statusBar = nullptr;
    HWND m_serverLabel = nullptr;
    HWND m_noteLabel = nullptr;
    HWND m_urlEdit = nullptr;
    HWND m_serverEdit = nullptr;
    HWND m_refreshBtn = nullptr;
    HWND m_searchEdit = nullptr;
    HWND m_severityCombo = nullptr;
    HWND m_listView = nullptr;
    HWND m_detailsEdit = nullptr;
    HWND m_boundaryBtn = nullptr;
    HWND m_chatHistory = nullptr;
    HWND m_chatEdit = nullptr;
    HWND m_chatSendBtn = nullptr;
    HWND m_noteEdit = nullptr;
    HWND m_noteBtn = nullptr;

    MapView m_map;

    std::vector<TrafficAlert> m_allAlerts;
    std::vector<TrafficAlert> m_filteredAlerts;
    std::vector<ChatMessage> m_chatMessages;
    std::vector<MapNote> m_notes;
    std::wstring m_selectedId;
    bool m_programmaticSelection = false;
    std::atomic_bool m_serverRequestInProgress{ false };
    bool m_hasPendingNoteLocation = false;
    double m_pendingNoteLat = 0.0;
    double m_pendingNoteLon = 0.0;
};


int RunMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    MainWindow win;
    if (!win.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"Traffic England Alerts Map", MB_ICONERROR);
        return 0;
    }

    return win.Run(nCmdShow);
}
