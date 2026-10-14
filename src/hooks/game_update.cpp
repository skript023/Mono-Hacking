#include "hooking.hpp"
#include "task_mgr.hpp"
#include "unity/camera_tools.hpp"
#include "unity/animal_tools.hpp"
#include "unity/crafting_tools.hpp"
#include "unity/hud.hpp"
#include "unity/world_time.hpp"
#include "utility/unity.hpp"


namespace big
{
	void hooks::game_update(MonoObject* game)
	{
		if (!g_running && (unity::hud_manager::cleanup_pending() || crafting_tools::cleanup_pending() || camera_tools::cleanup_pending() || world_time::cleanup_pending() || animal_tools::cleanup_pending()))
		{
			task_mgr::tick();
		}

		if (g_running)
		{
			task_mgr::tick();
			static bool had_local_player = false;
			const bool has_local_player  = unity::get_local_player() != nullptr;
			if (had_local_player && !has_local_player)
			{
				camera_tools::reset();
				animal_tools::restore_frozen_ai();
				world_time::reset_time_scale();
			}
			had_local_player = has_local_player;

			static auto s_last_sanitize = std::chrono::steady_clock::now();
			auto now                    = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::seconds>(now - s_last_sanitize).count() >= 2)
			{
				s_last_sanitize = now;
				unity::sanitize_local_player_inventory();
			}
		}

		return detour_base::get_original<game_update>()(game);
	}
}
