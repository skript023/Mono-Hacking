#pragma once
#include "quickjspp.hpp"
#include "file_manager.hpp"
#include "javascript_module.hpp"

namespace big
{
	class javascript_manager final
	{
		qjs::Runtime m_runtime;
		qjs::Context m_context;

		javascript_manager() :
		    m_context(m_runtime)
		{
		} // Context depends on Runtime

		// disable copy
		javascript_manager(const javascript_manager&) = delete;
		javascript_manager& operator=(const javascript_manager&) = delete;

		static javascript_manager& get()
		{
			static javascript_manager instance;

			return instance;
		}

		void init_impl();
		void enable_all_modules_impl();
		void load_all_modules_impl();
		void unload_all_modules_impl();
		void unload_module_impl(uint32_t module_id);
		std::weak_ptr<javascript_module> load_module_impl(const std::filesystem::path& module_path);
		void eval_script_impl(std::string const& script);
		void eval_file_impl(const std::filesystem::path& path);
	public:
		static void init()
		{
			get().init_impl();
		}

		static void enable_all_modules()
		{
			get().enable_all_modules_impl();
		}

		static void load_all_modules()
		{
			get().load_all_modules_impl();
		}

		static void unload_all_modules()
		{
			get().unload_all_modules();
		}

		static void unload_module(uint32_t module_id)
		{
			get().unload_module_impl(module_id);
		}

		static std::weak_ptr<javascript_module> load_module(const std::filesystem::path& module_path)
		{
			return get().load_module_impl(module_path);
		}

		static void eval_script(std::string const& script)
		{
			get().eval_script_impl(script);
		}

		static void eval_file(const std::filesystem::path& path)
		{
			get().eval_file_impl(path);
		}
	private:
		std::mutex m_module_lock;
		std::vector<std::shared_ptr<javascript_module>> m_modules;
		std::mutex m_disabled_module_lock;
		std::vector<std::shared_ptr<javascript_module>> m_disabled_modules;

		static constexpr std::chrono::seconds m_delay_between_changed_scripts_check = 3s;
		std::chrono::high_resolution_clock::time_point m_wake_time_changed_scripts_check;

		folder m_disabled_scripts_folder;
		folder m_scripts_folder;
		folder m_scripts_config_folder;
	};
}