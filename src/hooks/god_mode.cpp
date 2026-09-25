#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::character_rpc_damage(MonoObject* character, int64_t sender, MonoObject* hit)
	{
		if (g_settings.self.god_mode && character && character == unity::get_local_player())
		{
			return;
		}

		detour_base::get_original<character_rpc_damage>()(character, sender, hit);
	}

	bool hooks::player_in_god_mode(MonoObject* player)
	{
		if (g_settings.self.god_mode)
		{
			return true;
		}

		return detour_base::get_original<player_in_god_mode>()(player);
	}
}
