#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::update(MonoObject* player)
	{
		TRY_CLAUSE
		{
			auto local_player = unity::get_local_player();

			if (g_running && local_player)
			{
				g_script_mgr.tick();

				static float s_last_pickup_range = 2.f;
				if (s_last_pickup_range != g_settings.self.pickup_range)
				{
					s_last_pickup_range = g_settings.self.pickup_range;
					mono::set_field_value<"Player", "m_autoPickupRange">(local_player, g_settings.self.pickup_range);
				}

				static bool s_last_no_cost = false;
				if (s_last_no_cost != g_settings.self.free_crafting)
				{
					s_last_no_cost = g_settings.self.free_crafting;
					mono::set_field_value<"Player", "m_noPlacementCost">(local_player, g_settings.self.free_crafting);
				}
			}

			return detour_base::get_original<update>()(player);
		}
		EXCEPT_CLAUSE
	}
}
