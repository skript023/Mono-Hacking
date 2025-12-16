#pragma once
#include "mono/mono.hpp"
#include "file_manager.hpp"

namespace big
{
	class plugins
	{
		plugins()
		{
			auto game_dir = std::filesystem::current_path().string();

			SetEnvironmentVariableA("BEPINEX_ROOT_PATH", (game_dir + "\\BepInEx").c_str());
			SetEnvironmentVariableA("BEPINEX_CORE_PATH", (game_dir + "\\BepInEx\\core").c_str());
			SetEnvironmentVariableA("BEPINEX_PLUGIN_PATH", (game_dir + "\\BepInEx\\plugins").c_str());
			SetEnvironmentVariableA("BEPINEX_CONFIG_PATH", (game_dir + "\\BepInEx\\config").c_str());
			SetEnvironmentVariableA("BEPINEX_LOG_PATH", (game_dir + "\\BepInEx\\logs").c_str());
		}
		static plugins& get()
		{
			static plugins instance;
			return instance;
		}

		void init_impl()
		{
			get();
			MonoDomain* domain = mono::get_root_domain();

			mono::thread_attach(domain);

			std::string preloader = m_bephinex_folder.get_file("./plugins/AzuExtendedPlayerInventory.dll").get_path().string();

			MonoAssembly* asm_preloader = mono::domain_assembly_open(domain, preloader.c_str());

			if (!asm_preloader)
			{
				LOG(WARNING) << "Failed load AzuExtendedPlayerInventory.dll";

				return;
			}

			MonoImage* img = mono::assembly_get_image(asm_preloader);

			if (!img)
			{
				LOG(WARNING) << "Failed get image from AzuExtendedPlayerInventory.dll";

				return;
			}

			MonoClass* klass = mono::get_class_from_name(img, "AzuExtendedPlayerInventory", "AzuExtendedPlayerInventoryPlugin");

			if (!klass)
			{
				LOG(WARNING) << "Preloader class not found", "loader";

				return;
			}

			MonoMethod* run = mono::class_get_method_from_name(klass, "Awake", 0);

			if (!run)
			{
				LOG(WARNING) << "Preloader.Run not found";

				return;
			}

			LOG(INFO) << "BepInEx Folder: " << m_bephinex_folder.get_path().string();
			LOG(INFO) << "BepInEx Preload Executed: " << run;
			LOG(INFO) << "Loading BepInEx Preloader...";

			mono::invoke_method(run);
		}
	public:
		static void init()
		{
			get().init_impl();
		}

	private:
		folder m_bephinex_folder = std::filesystem::current_path() / std::filesystem::path("./BepInEx/");
	};
}