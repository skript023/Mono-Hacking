#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::create_tomb_stone(MonoObject* player)
	{
		if (g_settings.self.no_drop_on_dead)
		{
			return;
		}

		return detour_base::get_original<create_tomb_stone>()(player);
	}
}