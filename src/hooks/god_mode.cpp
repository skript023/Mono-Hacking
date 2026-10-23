#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::player_in_god_mode(MonoObject* player)
	{
		if (g_running && g_settings.self.god_mode && player && player == unity::get_local_player())
		{
			return true;
		}

		return detour_base::get_original<player_in_god_mode>()(player);
	}
}
