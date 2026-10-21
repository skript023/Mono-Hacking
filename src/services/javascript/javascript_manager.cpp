#include "javascript_manager.hpp"

#include "bindings/command_binding.hpp"
#include "bindings/gui_binding.hpp"
#include "bindings/javascript_binding.hpp"
#include "bindings/mono_binding.hpp"
#include "bindings/sigscanner_binding.hpp"
#include "bindings/store_binding.hpp"
#include "bindings/trampoline_binding.hpp"
#include "utility/joaat.hpp"

namespace big
{
	javascript_manager::~javascript_manager()
	{
		unload_all_modules_impl();
		js::trampoline::shutdown();
		js::sig::shutdown();
		js_gui::shutdown();
		js::command::shutdown();
	}
	std::optional<std::filesystem::path> move_file_relative_to_folder(const std::filesystem::path& original, const std::filesystem::path& target, const std::filesystem::path& file)
	{
		// keeps folder hierarchy intact
		const auto new_module_path = target / relative(file, original);
		file_manager::ensure_file_can_be_created(new_module_path);

		try
		{
			rename(file, new_module_path);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			LOG(FATAL) << "Failed to move Lua file: " << e.what();

			return std::nullopt;
		}
		return {new_module_path};
	}

	void javascript_manager::init_impl()
	{
		javacript_binding::bind(m_context);
		js::mono::bind(m_context);
		js::command::bind(m_context);
		js_gui::bind(m_context);
		js::storage::bind(m_context);
		js::trampoline::bind(m_context);
		js::sig::bind(m_context);

		m_scripts_folder          = file_manager::get_project_folder("./scripts");
		m_scripts_config_folder   = m_scripts_folder.get_folder("./scripts_config");
		m_disabled_scripts_folder = m_scripts_folder.get_folder("./disabled");
		m_wake_time_changed_scripts_check = std::chrono::high_resolution_clock::now() + m_delay_between_changed_scripts_check;
	}

	void javascript_manager::log_to_console(js_log_level level, const std::string& message)
	{
		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local{};
		localtime_s(&local, &now);
		char timestamp[16];
		std::snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
		std::lock_guard lock(m_console_mutex);
		m_console_entries.push_back({timestamp, level, message});
		if (m_console_entries.size() > 500)
		{
			m_console_entries.erase(m_console_entries.begin(), m_console_entries.begin() + 50);
		}
	}

	std::vector<js_console_entry> javascript_manager::get_console_entries_impl() const
	{
		std::lock_guard lock(m_console_mutex);
		return m_console_entries;
	}

	void javascript_manager::clear_console_impl()
	{
		std::lock_guard lock(m_console_mutex);
		m_console_entries.clear();
	}

	void javascript_manager::enable_all_modules_impl()
	{
		std::vector<std::filesystem::path> script_paths;

		{
			std::lock_guard guard(m_disabled_module_lock);
			for (auto& module : m_disabled_modules)
			{
				script_paths.push_back(module->module_path());

				module.reset();
			}
			m_disabled_modules.clear();
		}

		for (const auto& script_path : script_paths)
		{
			const auto new_module_path =
			    move_file_relative_to_folder(m_disabled_scripts_folder.get_path(), m_scripts_folder.get_path(), script_path);
			if (new_module_path)
			{
				load_module_impl(*new_module_path);
			}
		}
	}

	void javascript_manager::load_all_modules_impl()
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_scripts_folder.get_path(), std::filesystem::directory_options::skip_permission_denied))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".js")
			{
				load_module_impl(entry.path());
			}
		}
	}

	void javascript_manager::unload_all_modules_impl()
	{
		{
			std::lock_guard guard(m_module_lock);

			for (auto& module : m_modules)
			{
				module.reset();
			}
			m_modules.clear();
		}
		{
			std::lock_guard guard(m_disabled_module_lock);

			for (auto& module : m_disabled_modules)
			{
				module.reset();
			}
			m_disabled_modules.clear();
		}
	}

	void javascript_manager::unload_module_impl(uint32_t module_id)
	{
		std::lock_guard guard(m_module_lock);
		std::erase_if(m_modules, [module_id](auto& module) {
			return module_id == module->module_id();
		});

		std::lock_guard guard2(m_disabled_module_lock);
		std::erase_if(m_disabled_modules, [module_id](auto& module) {
			return module_id == module->module_id();
		});
	}

	std::weak_ptr<javascript_module> javascript_manager::load_module_impl(const std::filesystem::path& module_path)
	{
		if (!std::filesystem::exists(module_path))
		{
			LOG(WARNING) << reinterpret_cast<const char*>(module_path.u8string().c_str()) << " does not exist in the filesystem. Not loading it.";
			return {};
		}

		// Some scripts are library scripts, they do nothing on their own and are intended to be used with require, they take up space in the script list for no reason.
		if (std::filesystem::relative(module_path.parent_path(), m_scripts_folder.get_path()).wstring().contains(L"includes"))
		{
			return {};
		}

		const auto module_name = module_path.filename().string();
		const auto id          = joaat(module_name);

		std::lock_guard guard(m_module_lock);
		for (const auto& module : m_modules)
		{
			if (module->module_id() == id)
			{
				LOG(WARNING) << "Module with the name " << module_name << " already loaded.";
				return {};
			}
		}

		const auto rel                = relative(module_path, m_disabled_scripts_folder.get_path());
		const auto is_disabled_module = !rel.empty() && rel.native()[0] != '.';
		const auto module = std::make_shared<javascript_module>(module_path, m_scripts_folder, is_disabled_module);
		if (!module->is_disabled())
		{
			module->load_and_call_script(m_context);
			m_modules.push_back(module);

			return module;
		}

		std::lock_guard disabled_guard(m_disabled_module_lock);
		m_disabled_modules.push_back(module);
		return module;
	}
	void javascript_manager::eval_script_impl(std::string const& script)
	{
		log_to_console(js_log_level::eval_in, "> " + script);
		try
		{
			auto result      = m_context.eval(script, "<eval>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL);
			const char* text = JS_ToCString(m_context.ctx, result.v);
			if (text)
			{
				if (*text && std::string_view(text) != "undefined")
				{
					log_to_console(js_log_level::eval_out, text);
				}
				JS_FreeCString(m_context.ctx, text);
			}
		}
		catch (qjs::exception&)
		{
			auto exc          = m_context.getException();
			std::string error = exc.as<std::string>();
			if ((bool)exc["stack"])
			{
				error += "\n" + exc["stack"].as<std::string>();
			}
			LOG(WARNING) << "[JS Error] " << error;
			log_to_console(js_log_level::error, error);
		}
	}
	void javascript_manager::eval_file_impl(const std::filesystem::path& path)
	{
		log_to_console(js_log_level::eval_in, "> " + path.filename().string());
		try
		{
			m_context.evalFile(path.string().c_str(), JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL);
			log_to_console(js_log_level::info, "Loaded " + path.filename().string());
		}
		catch (qjs::exception&)
		{
			auto exc          = m_context.getException();
			std::string error = exc.as<std::string>();
			if ((bool)exc["stack"])
			{
				error += "\n" + exc["stack"].as<std::string>();
			}
			LOG(WARNING) << "[JS Error] " << error;
			log_to_console(js_log_level::error, error);
		}
	}
}
