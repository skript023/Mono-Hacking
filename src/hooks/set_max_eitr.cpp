#include "hooking.hpp"
#include "commands/commands.hpp"
#include "features/features.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::player_set_max_eitr(MonoObject* player, float max_eitr, bool flash)
	{
		static auto eitr_override = commands::get_bool_command("eitr"_hash);
		if (g_running && eitr_override && eitr_override->get_state() && player && player == unity::get_local_player())
		{
			features::_food_max_eitr = max_eitr;
			max_eitr                 = features::_eitr_amount.get_state();
		}

		detour_base::get_original<player_set_max_eitr>()(player, max_eitr, flash);
	}
}
