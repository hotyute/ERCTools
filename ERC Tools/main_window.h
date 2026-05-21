// =================================================================================
// FILE: main_window.h
// =================================================================================


#pragma once
#include "common.h"
#include "auth_session.h"

constexpr int kMainWindowLogoutExitCode = 100;

int RunMainWindow(HINSTANCE hInstance, int nCmdShow, const ClientSession& session);
