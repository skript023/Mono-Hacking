#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_debug_flying(MonoObject* _this)
	{
		if (g_settings.self.flying)
		{

			return true;
		}

		return detour_base::get_original<is_debug_flying>()(_this);
	}
}