#include "hooking.hpp"
#include "unity/crafting_tools.hpp"

#include <algorithm>

namespace big
{
	bool hooks::is_known_material(MonoObject* player, MonoString* name)
	{
		if (player && (crafting_tools::active(player) || g_settings.self.unlock_materials))
		{
			return true;
		}
		return detour_base::get_original<is_known_material>()(player, name);
	}
	bool hooks::zone_get_global_key(MonoObject* zone, int key)
	{
		if ((key == 21 && (crafting_tools::active() || g_settings.self.unlock_building_pieces))
		    || (key == 22 && g_settings.self.ignore_building_workbench)
		    || (key == 23 && (crafting_tools::active() || g_settings.self.unlock_runtime_recipes)))
		{
			return true;
		}
		return detour_base::get_original<zone_get_global_key>()(zone, key);
	}
	int hooks::crafting_station_get_level(MonoObject* station, bool check_extensions)
	{
		if (g_settings.self.station_level_override)
		{
			return std::clamp(g_settings.self.crafting_station_level, 1, 100);
		}
		return detour_base::get_original<crafting_station_get_level>()(station, check_extensions);
	}
}
