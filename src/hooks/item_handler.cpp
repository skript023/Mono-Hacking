#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "unity/localization.hpp"


namespace big
{
	bool hooks::can_get_achievements(bool cheated)
	{
		if (g_running)
		{
			return true;
		}
		return detour_base::get_original<can_get_achievements>()(cheated);
	}
	bool hooks::achievements_is_cheated()
	{
		if (g_running)
		{
			return false;
		}
		return detour_base::get_original<achievements_is_cheated>()();
	}
	bool hooks::bypass_cheat_checks()
	{
		if (g_running)
		{
			return true;
		}
		return detour_base::get_original<bypass_cheat_checks>()();
	}

	bool hooks::inventory_any_cheated_item(MonoObject* inventory)
	{
		return false;
	}

	void hooks::character_drop_items(MonoObject* drops, Vector3 center_pos, float drop_area, bool cheated)
	{
		detour_base::get_original<character_drop_items>()(drops, center_pos, drop_area, cheated && !g_running);
	}

	bool hooks::inventory_cheated_damaging_item_equipped(MonoObject* inventory)
	{
		if (g_running)
		{
			return false;
		}
		return detour_base::get_original<inventory_cheated_damaging_item_equipped>()(inventory);
	}

	void hooks::cooking_station_spawn_item(MonoObject* station, MonoString* name, int slot, Vector3 user_point, bool cheated)
	{
		detour_base::get_original<cooking_station_spawn_item>()(station, name, slot, user_point, cheated && !g_running);
	}

	void hooks::game_register_kill(MonoObject* game, int64_t target, MonoString* name, int boss_order, int kill_modifiers, int level, bool cheats_used)
	{
		detour_base::get_original<game_register_kill>()(game, target, name, boss_order, kill_modifiers, level, cheats_used && !g_running);
	}

	MonoString* hooks::item_get_tooltip(MonoObject* item, int quality, bool crafting, float world_level, int stack_override, bool appending)
	{
		static bool logged_entry = false;
		if (!logged_entry)
		{
			logged_entry = true;
			LOG(INFO) << "Item tooltip filter invoked.";
		}
		auto result = detour_base::get_original<item_get_tooltip>()(item, quality, crafting, world_level, stack_override, appending);
		if (!result)
		{
			return result;
		}

		auto text = mono::from_mono_string(result);
		if (text.find("\n<color=#808080><i>") == std::string::npos)
		{
			return result;
		}

		static std::string s_cached_line;
		static bool s_label_initialized = false;
		if (!s_label_initialized)
		{
			auto localization = big::localization::get_instance();
			const auto label  = localization.localize(std::string("$achievements_cheated_item_inventory"));
			if (!label.empty())
			{
				s_cached_line       = "\n<color=#808080><i>" + label + "</i></color>";
				s_label_initialized = true;
			}
		}

		if (s_cached_line.empty())
		{
			return result;
		}

		auto pos = text.find(s_cached_line);
		if (pos == std::string::npos)
		{
			static bool logged_mismatch = false;
			if (!logged_mismatch)
			{
				logged_mismatch = true;
				LOG(WARNING) << "Item tooltip filter: localized cheat label did not match.";
			}
			return result;
		}

		do
		{
			text.erase(pos, s_cached_line.size());
			pos = text.find(s_cached_line, pos);
		} while (pos != std::string::npos);

		auto filtered              = mono::to_mono_string(text);
		static bool logged_removal = false;
		if (filtered && !logged_removal)
		{
			logged_removal = true;
			LOG(INFO) << "Item tooltip filter removed the dev-command label.";
		}
		return filtered ? filtered : result;
	}
}
