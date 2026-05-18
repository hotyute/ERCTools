// =================================================================================
// FILE: app.cpp
// =================================================================================


#include "common.h"
#include "app_state.h"
#include "login_dialog.h"
#include "main_window.h"
#include "util.h"

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


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    OpenConsole();
    ConsoleLog(L"Starting Traffic England Alerts Map...");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"COM initialization failed.", L"Traffic England Alerts Map", MB_ICONERROR);
        return 0;
    }

    if (!InitGraphicsFactories()) {
        MessageBoxW(nullptr, L"Graphics initialization failed.", L"Traffic England Alerts Map", MB_ICONERROR);
        CoUninitialize();
        return 0;
    }

    ClientSession session;
    if (!ShowLoginDialog(hInstance, session)) {
        CoUninitialize();
        return 0;
    }

    const int rc = RunMainWindow(hInstance, nCmdShow, session);

    g_dwriteFactory.Reset();
    g_wicFactory.Reset();
    g_d2dFactory.Reset();
    CoUninitialize();
    return rc;
}
