#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_wind_controll_active(MonoObject* player)
	{
		if (g_settings.self.always_wind)
		{
			return true;
		}

		return detour_base::get_original<is_wind_controll_active>()(player);
	}
}