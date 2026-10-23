#include "hooking.hpp"
#include <script_mgr.hpp>
#include "utility/unity.hpp"

namespace big
{
	bool hooks::is_debug_flying(MonoObject* _this)
	{
		if (g_running && g_settings.self.flying && _this && _this == unity::get_local_player())
		{
			return true;
		}

		return detour_base::get_original<is_debug_flying>()(_this);
	}
}
