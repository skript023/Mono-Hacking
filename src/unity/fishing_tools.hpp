#pragma once
#include "mono/mono.hpp"

namespace big
{
	class fishing_tools
	{
	public:
		struct fishing_status
		{
			bool rod_active{ false };
			bool fish_hooked{ false };
			float line_length{ 0.f };
		};

		static void update();
		static fishing_status get_status();
		static void instant_catch();
	};
}

