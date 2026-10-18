#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "task_mgr.hpp"
#include "thread_pool.hpp"
#include "benchmark.hpp"
#include "file_manager.hpp"
#include "render/render_vulkan.hpp"

#include "mono/mono.hpp"
#include "logger/logger.hpp"
#include "plugins/plugins.hpp"
#include "settings/settings.hpp"
#include "server/server_module.hpp"
#include "worker/main_worker.hpp"
#include "worker/entity_worker.hpp"
#include "unity/hud.hpp"
#include "unity/animal_tools.hpp"
#include "unity/camera_tools.hpp"
#include "unity/crafting_tools.hpp"
#include "unity/world_time.hpp"
#include "javascript/javascript_manager.hpp"

#include <astra/host/notification.hpp>

DWORD APIENTRY main_thread(LPVOID)
{
	using namespace big;

	do
	{
		if (FindWindow(WINDOW_CLASS, WINDOW_NAME))
		{
			break;
		}
		std::this_thread::sleep_for(10ms);
	} while (g_running);

	benchmark initialization_benchmark("Initialization");

	std::filesystem::path base_dir = std::filesystem::current_path();
	base_dir /= MOD_FOLDER_NAME;

	file_manager::init(base_dir);

	logger::initialize(WINDOW_NAME, file_manager::get_project_file("./logs/log.txt"), true);

	try
	{
		LOG(static_cast<eLogLevel>(5)) << R"kek(
 __  __                   _    _            _    _             
|  \/  |                 | |  | |          | |  (_)            
| \  / | ___  _ __   ___ | |__| | __ _  ___| | ___ _ __   __ _ 
| |\/| |/ _ \| '_ \ / _ \|  __  |/ _` |/ __| |/ / | '_ \ / _` |
| |  | | (_) | | | | (_) | |  | | (_| | (__|   <| | | | | (_| |
|_|  |_|\___/|_| |_|\___/|_|  |_|\__,_|\___|_|\_\_|_| |_|\__, |
                                                          __/ |
                                                         |___/ 
)kek";
		settings::initialize(file_manager::get_project_file("./settings.json"));
		g_settings.load();
		LOG(INFO) << "Settings initialized.";

		mono::init();
		LOG(INFO) << "Mono initialized.";

		auto pointers_instance = std::make_unique<pointers>();
		LOG(INFO) << "Pointers initialized.";

		auto renderer_instance = std::make_unique<renderer>();
		LOG(INFO) << "Renderer initialized.";

		auto thread_pool_instance = std::make_unique<thread_pool>();
		LOG(INFO) << "Thread Pool initialized.";

		auto hooking_instance = std::make_unique<hooking>();
		LOG(INFO) << "Hooking initialized.";

		g_pointers->update();

		javascript_manager::init();
		LOG(INFO) << "Service registered.";

#ifdef PRODUCTION
		auto server_instance = std::make_unique<server_module>();
		LOG(INFO) << "Server initialized.";
#endif

		task_mgr::add(&main_worker::run);
		task_mgr::add(&main_worker::slow_run);
		task_mgr::add(&entity_worker::run);
		LOG(INFO) << "Tasks registered.";

		g_hooking->enable();
		g_renderer->attach();
		LOG(INFO) << "Hooking enabled.";

		initialization_benchmark.get_runtime();
		initialization_benchmark.reset();

		while (g_running)
		{
#ifdef PRODUCTION
			if (!server_instance->authorized())
			{
				LOG(WARNING) << "[License] DLL session revoked or expired; shutting down safely.";
				g_running = false;
				break;
			}
#endif
			g_settings.attempt_save();
			settings::tick();
			std::this_thread::sleep_for(1s);
		}

		crafting_tools::shutdown();
		animal_tools::shutdown();
		world_time::shutdown();
		camera_tools::shutdown();
		unity::hud_manager::shutdown();
		render_vulkan::detach();
		g_hooking->disable();
		LOG(INFO) << "Hooking disabled.";

		std::this_thread::sleep_for(1000ms);

		task_mgr::clear();
		LOG(INFO) << "Tasks unregistered.";

#ifdef PRODUCTION
		server_instance.reset();
		LOG(INFO) << "Server unregistered.";
#endif

		LOG(INFO) << "Service unregistered.";

		hooking_instance.reset();
		LOG(INFO) << "Hooking uninitialized.";


		g_thread_pool->destroy();
		LOG(INFO) << "Destroyed thread pool.";

		thread_pool_instance.reset();
		LOG(INFO) << "Thread Pool uninitialized.";

		renderer_instance.reset();
		LOG(INFO) << "Renderer uninitialized.";

		pointers_instance.reset();
		LOG(INFO) << "Pointers uninitialized.";

		g_settings.attempt_save();
		LOG(INFO) << "Settings saved and uninitialized.";
	}
	catch (std::exception const& ex)
	{
		LOG(WARNING) << ex.what();
		MessageBoxA(nullptr, ex.what(), nullptr, MB_OK | MB_ICONEXCLAMATION);
	}

	if (!render_vulkan::stop_capture())
	{
		LOG(WARNING) << "DLL retained because Vulkan hooks are still active.";
		return 0;
	}

	if (!detour_base::hooks().empty())
	{
		LOG(WARNING) << "DLL retained because another program still calls into its hooks. Restart the game before injecting again.";
		return 0;
	}

	// This DLL owns a static OpenSSL instance; all networking threads are gone.
	OPENSSL_cleanup();
	LOG(INFO) << "OpenSSL cleaned up. Releasing DLL.";
	LOG(INFO) << "Farewell!";
	logger::destroy();

	CloseHandle(g_main_thread);
	FreeLibraryAndExitThread(g_hmodule, 0);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, PVOID)
{
	using namespace big;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hmod);

		g_hmodule     = hmod;
		g_main_thread = CreateThread(nullptr, 0, &main_thread, nullptr, 0, &g_main_thread_id);
		break;
	case DLL_PROCESS_DETACH: g_running = false; break;
	}

	return true;
}
