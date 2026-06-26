// =================================================================================
// FILE: main_window.h
// =================================================================================


#pragma once
#include "core/common.h"
#include "net/auth_session.h"

constexpr int kMainWindowLogoutExitCode = 100;

int RunMainWindow(HINSTANCE hInstance, int nCmdShow, const ClientSession& session);
