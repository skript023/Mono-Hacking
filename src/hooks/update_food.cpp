#include "hooking.hpp"

#include "menu_settings.hpp"
#include "utility/unity.hpp"

#include <algorithm>

namespace big
{
	void hooks::update_food(MonoObject* player, float dt, bool force_update)
	{
		auto original = detour_base::get_original<update_food>();
		if (!g_running || force_update || player != unity::get_local_player() || g_settings.self.food_duration_multiplier == 1.f)
		{
			return original(player, dt, force_update);
		}

		static auto food_timer_field      = mono::get_field("Player", "m_foodUpdateTimer", "assembly_valheim");
		static uint32_t food_timer_offset = food_timer_field ? mono::get_field_offset(food_timer_field) : 0;
		if (!food_timer_offset)
		{
			return original(player, dt, force_update);
		}

		float* timer               = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + food_timer_offset);
		const float elapsed        = std::max(dt, 0.f);
		const float previous_timer = *timer;

		// Let vanilla advance its separate food-regeneration clock without decaying food.
		*timer = -1000000.f;
		original(player, dt, false);

		float food_time = previous_timer + elapsed * std::clamp(g_settings.self.food_duration_multiplier, 0.f, 10.f);
		while (food_time >= 1.f)
		{
			food_time -= 1.f;
			float no_elapsed = 0.f;
			bool force       = true;
			original(player, no_elapsed, force);
		}
		*timer = food_time;
	}
}
