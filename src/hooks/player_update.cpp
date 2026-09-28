#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::update(MonoObject* player)
	{
		TRY_CLAUSE
		{
			auto local_player = unity::get_local_player();

			if (g_running && local_player && player == local_player)
			{
				static float s_last_pickup_range = 2.f;
				if (s_last_pickup_range != g_settings.self.pickup_range)
				{
					s_last_pickup_range = g_settings.self.pickup_range;
					mono::set_field_value<"Player", "m_autoPickupRange">(local_player, g_settings.self.pickup_range);
				}
			}

			return detour_base::get_original<update>()(player);
		}
		EXCEPT_CLAUSE
	}
}
