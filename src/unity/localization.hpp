#pragma once
#include "mono/mono.hpp"

namespace big
{
	class localization
	{
                MonoObject* m_localization;
                public:
                localization(MonoObject* obj);
                ~localization() noexcept;

                std::string localize(std::string const& height);
                std::string localize(MonoString* text);
                std::string get_selected_language();
                static localization get_instance();
	};
}