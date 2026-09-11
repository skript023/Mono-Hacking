#include "hooking.hpp"

namespace big
{
	bool hooks::player_in_ghost_mode(MonoObject* player)
	{
		if (g_settings.self.ghost_mode)
		{
			return true;
		}

		return detour_base::get_original<player_in_ghost_mode>()(player);
	}
}

