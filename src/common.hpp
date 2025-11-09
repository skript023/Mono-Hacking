#ifndef COMMON_INC
#define COMMON_INC

// clang-format off

#pragma warning(disable:4369 4129)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <Xinput.h>
#include <d3d9.h>
#include <D3D11.h>
#include <D3D12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <Psapi.h>

#include <cinttypes>
#include <cstddef>
#include <cstdint>

#include <chrono>
#include <ctime>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <atomic>
#include <mutex>
#include <thread>

#include <memory>
#include <new>

#include <sstream>
#include <string>
#include <string_view>

#include <algorithm>
#include <functional>
#include <utility>

#include <stack>
#include <vector>

#include <typeinfo>
#include <type_traits>

#include <exception>
#include <stdexcept>

#include <any>
#include <optional>
#include <variant>

#include <regex>
#include <tlhelp32.h>

#include <nlohmann/json.hpp>

#include "menu_settings.hpp"
#include "logger.hpp"

#pragma comment(lib, "Xinput.lib")

#define GAME "Valheim.exe"
#define GAME_NAME "Valheim"
#define FOLDER_NAME "Valheim"
#define LOG_NAME "Valheim.log"
#define LOG_EVENT_NAME "ValheimEvents.log"

#define WINDOW_CLASS "UnityWndClass"
#define WINDOW_NAME "Valheim"

namespace big
{
	using namespace std::chrono_literals;
	
	template <typename T>
	using comptr = Microsoft::WRL::ComPtr<T>;

	inline HMODULE g_hmodule{};
	inline HANDLE g_main_thread{};
	inline DWORD g_main_thread_id{};
	inline std::atomic_bool g_running{ true };
}

#endif