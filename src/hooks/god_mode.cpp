#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::player_in_god_mode(MonoObject* player)
	{
		if (g_settings.self.god_mode)
		{
			return true;
		}

		return detour_base::get_original<player_in_god_mode>()(player);
	}
}

