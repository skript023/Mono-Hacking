#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	float hooks::player_get_run_speed_factor(MonoObject* player)
	{
		float factor = detour_base::get_original<player_get_run_speed_factor>()(player);

		if (g_settings.self.speed_multiplier > 1.f && player && player == unity::get_local_player())
		{
			return factor * g_settings.self.speed_multiplier;
		}

		return factor;
	}

	float hooks::player_get_jog_speed_factor(MonoObject* player)
	{
		float factor = detour_base::get_original<player_get_jog_speed_factor>()(player);

		if (g_settings.self.speed_multiplier > 1.f && player && player == unity::get_local_player())
		{
			return factor * g_settings.self.speed_multiplier;
		}

		return factor;
	}
}
