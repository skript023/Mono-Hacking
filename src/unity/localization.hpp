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
	};
}