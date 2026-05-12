// =================================================================================
// FILE: common.h
// =================================================================================

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <winhttp.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <uxtheme.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <iostream>

using Microsoft::WRL::ComPtr;
using json = nlohmann::json;
