#pragma once
#include "utility/joaat.hpp"
#include "quickjspp.hpp"
#include "file_manager.hpp"

namespace big
{
	class javascript_module
	{
	public:
		javascript_module(
		    const std::filesystem::path& module_path,
		    folder& script_folder,
		    bool disabled) :
		    m_path(module_path),
		    m_root(script_folder),
		    m_module_name(module_path.filename().string()),
		    m_disabled(disabled)
		{
			m_id = joaat(m_module_name);
		}

		void load_and_call_script(qjs::Context& ctx)
		{
			try
			{
				ctx.evalFile(m_path.string().c_str(), JS_EVAL_TYPE_MODULE);
			}
			catch (qjs::exception&)
			{
				LOG(WARNING) << "[JS Error] " << (std::string)ctx.getException();
			}
		}

		bool is_disabled() const
		{
			return m_disabled;
		}
		uint32_t module_id() const
		{
			return m_id;
		}
		const std::filesystem::path& module_path() const
		{
			return m_path;
		}

	private:
		uint32_t m_id{};
		std::filesystem::path m_path{};
		folder m_root{};
		std::string m_module_name;
		bool m_disabled = false;
	};

}