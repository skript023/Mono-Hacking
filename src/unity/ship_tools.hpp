#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"
#include <string>

namespace big
{
	class ship_tools
	{
	public:
		struct ship_status
		{
			bool in_ship{false};
			float speed{0.f};
			float rudder{0.f};
			float health{0.f};
			float max_health{0.f};
			bool ashlands_ready{false};
			Vector3 position{};
		};

		static MonoObject* get_local_ship()
		{
			return instance().get_local_ship_impl();
		}
		static void update()
		{
			return instance().update_impl();
		}
		static ship_status get_status()
		{
			return instance().get_status_impl();
		}
		static void emergency_anchor()
		{
			return instance().emergency_anchor_impl();
		}
		static void boost_forward(float force = 1000.f)
		{
			return instance().boost_forward_impl(force);
		}
		static void repair_ship()
		{
			return instance().repair_ship_impl();
		}

	private:
		ship_tools() = default;
		ship_tools(const ship_tools&) = delete;
		ship_tools& operator=(const ship_tools&) = delete;
		static ship_tools& instance()
		{
			static ship_tools value;
			return value;
		}

		MonoObject* get_local_ship_impl();
		void update_impl();
		ship_status get_status_impl();
		void emergency_anchor_impl();
		void boost_forward_impl(float force);
		void repair_ship_impl();
	};
}
