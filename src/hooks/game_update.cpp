#include "hooking.hpp"
#include "task_mgr.hpp"
#include "script_mgr.hpp"

#include "unity/hud.hpp"
#include "utility/unity.hpp"
#include "unity/crafting_tools.hpp"

namespace big
{
	void hooks::game_update(MonoObject* game)
	{
		TRY_CLAUSE
		{
			if (!g_running && (unity::hud_manager::cleanup_pending() || crafting_tools::cleanup_pending()))
			{
				g_script_mgr.tick();
				task_mgr::tick();
			}

			if (g_running)
			{
				g_script_mgr.tick();
				task_mgr::tick();

				static auto s_last_sanitize = std::chrono::steady_clock::now();
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::seconds>(now - s_last_sanitize).count() >= 2)
				{
					s_last_sanitize = now;
					unity::sanitize_local_player_inventory();
				}
			}

			return detour_base::get_original<game_update>()(game);
		}
		EXCEPT_CLAUSE
	}
}
