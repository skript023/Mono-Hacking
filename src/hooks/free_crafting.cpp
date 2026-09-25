#include "hooking.hpp"

namespace big
{
	bool hooks::player_no_cost_cheat(MonoObject* player)
	{
		if (g_settings.self.free_crafting)
		{
			return true;
		}

		return detour_base::get_original<player_no_cost_cheat>()(player);
	}
}
