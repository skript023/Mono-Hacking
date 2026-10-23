#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::player_in_ghost_mode(MonoObject* player)
	{
		if (g_running && g_settings.self.ghost_mode && player && player == unity::get_local_player())
		{
			return true;
		}

		return detour_base::get_original<player_in_ghost_mode>()(player);
	}
}
