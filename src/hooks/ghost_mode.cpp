#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::player_in_ghost_mode(MonoObject* player)
	{
		if (g_running && player && player == unity::get_local_player() && g_settings.self.ghost_mode)
		{
			return true;
		}

		return detour_base::get_original<player_in_ghost_mode>()(player);
	}
}
