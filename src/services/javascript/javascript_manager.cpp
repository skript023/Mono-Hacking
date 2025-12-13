#include "javascript_manager.hpp"

#include "utility/joaat.hpp"
#include "bindings/javascript_binding.hpp"
#include "bindings/command_binding.hpp"
#include "bindings/mono_binding.hpp"
#include "bindings/gui_binding.hpp"
#include "bindings/store_binding.hpp"
#include "bindings/trampoline_binding.hpp"

namespace big
{
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

		m_scripts_folder = file_manager::get_project_folder("./scripts");
		m_scripts_config_folder = m_scripts_folder.get_folder("./scripts_config");
		m_disabled_scripts_folder = m_scripts_folder.get_folder("./disabled");
		m_wake_time_changed_scripts_check = std::chrono::high_resolution_clock::now() + m_delay_between_changed_scripts_check;

		try
		{
			m_context.eval(R"(
				import * as log from 'Logger';
				log.info("This is a test log from javascript");
				console.log('hello from js', 123, true);
			)", "<eval>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL);
		}
		catch (qjs::exception&)
		{
			auto exc = m_context.getException();
			LOG(WARNING) << "[JS Error] " << (std::string)exc;
		}
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
			const auto new_module_path = move_file_relative_to_folder(m_disabled_scripts_folder.get_path(), m_scripts_folder.get_path(), script_path);
			if (new_module_path)
			{
				load_module_impl(*new_module_path);
			}
		}
	}

	void javascript_manager::load_all_modules_impl()
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_scripts_folder.get_path(), std::filesystem::directory_options::skip_permission_denied))
			if (entry.is_regular_file() && entry.path().extension() == ".js")
				load_module_impl(entry.path());
	}

	void javascript_manager::unload_all_modules_impl()
	{
		{
			std::lock_guard guard(m_module_lock);

			for (auto& module : m_modules)
				module.reset();
			m_modules.clear();
		}
		{
			std::lock_guard guard(m_disabled_module_lock);

			for (auto& module : m_disabled_modules)
				module.reset();
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
			return {};

		const auto module_name = module_path.filename().string();
		const auto id = joaat(module_name);

		std::lock_guard guard(m_module_lock);
		for (const auto& module : m_modules)
		{
			if (module->module_id() == id)
			{
				LOG(WARNING) << "Module with the name " << module_name << " already loaded.";
				return {};
			}
		}

		const auto rel = relative(module_path, m_disabled_scripts_folder.get_path());
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
		try
		{
			m_context.eval(script, "<eval>", JS_EVAL_TYPE_MODULE | JS_EVAL_TYPE_GLOBAL);
		}
		catch (qjs::exception&)
		{
			auto exc = m_context.getException();
			LOG(WARNING) << "[JS Error] " << (std::string)exc;
		}
	}
}