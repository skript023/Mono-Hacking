#include "hooking.hpp"

namespace big
{
	float hooks::get_attack_draw_percentage(MonoObject* humanoid)
	{
		if (g_settings.self.instant_bow_draw)
		{
			return 1.0f;
		}

		return detour_base::get_original<get_attack_draw_percentage>()(humanoid);
	}

	bool hooks::is_weapon_loaded(MonoObject* player)
	{
		if (g_settings.self.instant_bow_draw)
		{
			return true;
		}

		return detour_base::get_original<is_weapon_loaded>()(player);
	}
}
