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

		static MonoObject* get_local_ship();
		static void update();
		static ship_status get_status();
		static void emergency_anchor();
		static void boost_forward(float force = 1000.f);
		static void repair_ship();
	};
}
