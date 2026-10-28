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
	static void clear_used_cheats(MonoObject* game)
	{
		static auto get_profile   = mono::get_method("Game", "GetPlayerProfile", 0, "assembly_valheim");
		static auto profile_class = mono::get_class("PlayerProfile", "assembly_valheim");
		static auto used_cheats   = profile_class ? mono::get_field(profile_class, "m_usedCheats") : nullptr;
		static auto player_stats  = profile_class ? mono::get_field(profile_class, "m_playerStats") : nullptr;
		static auto set_stat      = mono::get_method("PlayerProfile/PlayerStats", "set_Item", 2, "assembly_valheim");
		if (!game || !get_profile || !used_cheats)
		{
			return;
		}

		MonoObject* exception = nullptr;
		auto profile          = mono::invoke_method(get_profile, game, nullptr, &exception);
		if (exception || !profile)
		{
			return;
		}
		bool cheated = false;
		mono::get_field_value(profile, used_cheats, &cheated);
		if (!cheated)
		{
			return;
		}
		cheated = false;
		mono::set_field_value(profile, used_cheats, &cheated);

		MonoArray* stats = nullptr;
		if (player_stats && set_stat)
		{
			mono::get_field_value(profile, player_stats, &stats);
		}
		if (!stats)
		{
			return;
		}
		int cheats_stat = 4;
		float zero      = 0.f;
		for (int i = 0; i < mono::array_length(stats); i++)
		{
			auto entry = *static_cast<MonoObject**>(mono::array_with_size(stats, sizeof(MonoObject*), i));
			if (entry)
			{
				void* args[] = {&cheats_stat, &zero};
				mono::invoke_method(set_stat, entry, args, &exception);
			}
		}
	}

	void hooks::game_update(MonoObject* game)
	{
		if (!g_running && (g_shutdown_pending || unity::hud_manager::cleanup_pending() || crafting_tools::cleanup_pending() || camera_tools::cleanup_pending() || world_time::cleanup_pending() || animal_tools::cleanup_pending()))
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
				clear_used_cheats(game);
			}
		}

		return detour_base::get_original<game_update>()(game);
	}
}
