#include "localization.hpp"
#include "script.hpp"

namespace big
{
	localization::localization(MonoObject* obj): m_localization(obj)
	{}

	localization::~localization() noexcept
	{
		m_localization = nullptr;
	}

	std::string localization::localize(std::string const& text)
	{
		if (text.empty())
			return {};

		if (!m_localization)
		{
			auto inst = get_instance();
			m_localization = inst.m_localization;
			if (!m_localization)
				return text;
		}

		// Use get_method_overload to specifically match String Localize(String text)
		// and avoid the ambiguous overload Void Localize(Transform root) which crashes UnityPlayer.dll
		static auto method = mono::get_method_overload("Localization", "Localize", 1, "String", "String", "assembly_guiutils");
		if (!method)
		{
			LOG(WARNING) << "Failed to find method Localization.Localize(string)";
			return text;
		}

		TRY_CLAUSE
		{
			auto ms = mono::to_mono_string(text);
			if (!ms)
				return text;

			// Mono reference arguments are passed directly; only value types use their address.
			void* args[] = { ms };
			auto ret = mono::invoke_method(method, m_localization, args);
			if (!ret)
				return text;

			return mono::from_mono_string(reinterpret_cast<MonoString*>(ret));
		}
		EXCEPT_CLAUSE

		return text;
	}

	std::string localization::localize(MonoString* text)
	{
		if (!text)
			return {};

		std::string str = mono::from_mono_string(text);
		return localize(str);
	}

	std::string localization::get_selected_language()
	{
		if (!m_localization)
		{
			auto inst = get_instance();
			m_localization = inst.m_localization;
			if (!m_localization)
				return {};
		}

		static auto method = mono::get_method("Localization", "GetSelectedLanguage", 0, "assembly_guiutils");
		if (!method)
		{
			LOG(WARNING) << "Failed to find method Localization.GetSelectedLanguage()";
			return {};
		}

		TRY_CLAUSE
		{
			auto ret = mono::invoke(method, m_localization);
			if (!ret)
				return {};

			return mono::from_mono_string(reinterpret_cast<MonoString*>(ret));
		}
		EXCEPT_CLAUSE

		return {};
	}

	localization localization::get_instance()
	{
		// 1. Try reading static field m_instance first (pure memory read, zero managed code execution)
		MonoClass* klass = mono::get_class("Localization", "assembly_guiutils");
		if (klass)
		{
			MonoClassField* field = mono::get_field(klass, "m_instance");
			if (field)
			{
				void* static_data = mono::get_static_field_data(klass);
				if (static_data)
				{
					uint32_t offset = mono::get_field_offset(field);
					void* ptr_addr = (void*)((uintptr_t)static_data + offset);
					if (ptr_addr && *(MonoObject**)ptr_addr)
						return localization(*(MonoObject**)ptr_addr);
				}
			}
		}

		// 2. Fallback to property getter get_instance
		static auto method = mono::get_method("Localization", "get_instance", 0, "assembly_guiutils");
		if (method)
		{
			MonoObject* obj = mono::invoke_method(method);
			if (obj)
				return localization(obj);
		}

		return localization(nullptr);
	}
}
