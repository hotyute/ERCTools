// =================================================================================
// FILE: app.cpp
// =================================================================================


#include "core/common.h"
#include "app/app_state.h"
#include "ui/windows/login_dialog.h"
#include "ui/windows/main_window.h"
#include "core/util.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

bool InitGraphicsFactories()
{
    if (!g_d2dFactory) {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&g_d2dFactory))))
            return false;
    }

    if (!g_wicFactory) {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_wicFactory))))
            return false;
    }

    if (!g_dwriteFactory) {
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(g_dwriteFactory.GetAddressOf()))))
            return false;
    }

    return true;
}

static BOOL WINAPI ConsoleShutdownHandler(DWORD signal)
{
    switch (signal) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        break;
    default:
        return FALSE;
    }

    g_appQuitting.store(true);

    HWND hwnd = FindWindowW(kMainClassName, nullptr);
    if (!hwnd)
        return FALSE;

    DWORD_PTR result = 0;
    SendMessageTimeoutW(
        hwnd,
        WM_CLOSE,
        0,
        0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK,
        4000,
        &result);
    return TRUE;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    OpenConsole();
    SetConsoleCtrlHandler(ConsoleShutdownHandler, TRUE);
    ConsoleLog(L"Starting ERC Tools...");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"COM initialization failed.", L"ERC Tools", MB_ICONERROR);
        return 0;
    }

    if (!InitGraphicsFactories()) {
        MessageBoxW(nullptr, L"Graphics initialization failed.", L"ERC Tools", MB_ICONERROR);
        CoUninitialize();
        return 0;
    }

    int rc = 0;
    for (;;) {
        ClientSession session;
        if (!ShowLoginDialog(hInstance, session))
            break;

        g_appQuitting.store(false);
        rc = RunMainWindow(hInstance, nCmdShow, session);
        if (rc != kMainWindowLogoutExitCode)
            break;

        rc = 0;
    }

    g_dwriteFactory.Reset();
    g_wicFactory.Reset();
    g_d2dFactory.Reset();
    SetConsoleCtrlHandler(ConsoleShutdownHandler, FALSE);
    CoUninitialize();
    return rc;
}
