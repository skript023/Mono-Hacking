#pragma once
#include "mono/mono.hpp"

namespace big
{
	class localization
	{
		static localization& instance()
		{
			static localization value(nullptr);
			return value;
		}
		localization get_instance_impl();
		MonoObject* m_localization;

	public:
		localization(MonoObject* obj);
		~localization() noexcept;

		std::string localize(std::string const& height);
		std::string localize(MonoString* text);
		std::string get_selected_language();
		static localization get_instance()
		{
			return instance().get_instance_impl();
		}
	};
}
