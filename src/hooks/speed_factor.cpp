#include "hooking.hpp"
#include "utility/unity.hpp"

#include <algorithm>

namespace big
{
	float hooks::player_get_run_speed_factor(MonoObject* player)
	{
		float factor = detour_base::get_original<player_get_run_speed_factor>()(player);

		float movement_multiplier = g_settings.self.movement_speed_multiplier;
		if (movement_multiplier == 1.f && g_settings.self.speed_multiplier > 1.f)
		{
			movement_multiplier = g_settings.self.speed_multiplier;
		}
		if (player && player == unity::get_local_player())
		{
			return factor * std::clamp(movement_multiplier, 0.1f, 10.f) * std::clamp(g_settings.self.run_speed_multiplier, 0.1f, 10.f);
		}

		return factor;
	}

	float hooks::player_get_jog_speed_factor(MonoObject* player)
	{
		float factor = detour_base::get_original<player_get_jog_speed_factor>()(player);

		float movement_multiplier = g_settings.self.movement_speed_multiplier;
		if (movement_multiplier == 1.f && g_settings.self.speed_multiplier > 1.f)
		{
			movement_multiplier = g_settings.self.speed_multiplier;
		}
		if (player && player == unity::get_local_player())
		{
			return factor * std::clamp(movement_multiplier, 0.1f, 10.f) * std::clamp(g_settings.self.jog_speed_multiplier, 0.1f, 10.f);
		}

		return factor;
	}
}
