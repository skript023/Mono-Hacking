#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_out_of_water(MonoObject* player)
	{
		if (g_settings.self.allow_pickup_fish)
		{
			return true;
		}

		return detour_base::get_original<is_out_of_water>()(player);
	}
}