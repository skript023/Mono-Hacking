#pragma once
#include "mono/mono.hpp"

namespace big
{
	class fishing_tools
	{
	public:
		struct fishing_status
		{
			bool rod_active{false};
			bool fish_hooked{false};
			float line_length{0.f};
		};

		static void update()
		{
			return instance().update_impl();
		}
		static fishing_status get_status()
		{
			return instance().get_status_impl();
		}
		static void instant_catch()
		{
			return instance().instant_catch_impl();
		}

	private:
		fishing_tools() = default;
		fishing_tools(const fishing_tools&) = delete;
		fishing_tools& operator=(const fishing_tools&) = delete;
		static fishing_tools& instance()
		{
			static fishing_tools value;
			return value;
		}

		void update_impl();
		fishing_status get_status_impl();
		void instant_catch_impl();
	};
}
