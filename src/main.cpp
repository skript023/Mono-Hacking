#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "render/render_vulkan.hpp"
#include "benchmark.hpp"
#include "script_mgr.hpp"
#include "fiber_pool.hpp"
#include "file_manager.hpp"

#include "mono/mono.hpp"
#include "logger/logger.hpp"
#include "plugins/plugins.hpp"
#include "settings/settings.hpp"
#include "server/server_module.hpp"
#include "worker/main_worker.hpp"
#include "worker/entity_worker.hpp"
#include "javascript/javascript_manager.hpp"

#include "services/notification/notification_service.hpp"

DWORD APIENTRY main_thread(LPVOID)
{
	using namespace big;

	auto minhook_instance = std::make_unique<minhook_keepalive>();
	// Capture device creation before waiting for Unity's window and Mono startup.
	do
	{
		render_vulkan::start_capture();
		if (FindWindow(WINDOW_CLASS, WINDOW_NAME))
			break;
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

		auto fiber_pool_instance = std::make_unique<fiber_pool>(10);
		LOG(INFO) << "Fiber pool initialized.";

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

		g_script_mgr.add_script(std::make_unique<script>(&main_worker::run));
		g_script_mgr.add_script(std::make_unique<script>(&main_worker::slow_run));
		g_script_mgr.add_script(std::make_unique<script>(&entity_worker::run));
		LOG(INFO) << "Scripts registered.";

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

		render_vulkan::detach();
		g_hooking->disable();
		LOG(INFO) << "Hooking disabled.";

		std::this_thread::sleep_for(1000ms);

		g_script_mgr.remove_all_scripts();
		LOG(INFO) << "Scripts unregistered.";

#ifdef PRODUCTION
		server_instance.reset();
		LOG(INFO) << "Server unregistered.";
#endif

		LOG(INFO) << "Service unregistered.";

		hooking_instance.reset();
		LOG(INFO) << "Hooking uninitialized.";

		fiber_pool_instance.reset();
		LOG(INFO) << "Fiber pool uninitialized.";

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

	render_vulkan::stop_capture();
	minhook_instance.reset();

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

		g_hmodule = hmod;
		g_main_thread = CreateThread(nullptr, 0, &main_thread, nullptr, 0, &g_main_thread_id);
		break;
	case DLL_PROCESS_DETACH:
		g_running = false;
		break;
	}

	return true;
}
