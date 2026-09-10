#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_teleportable(void* _this, bool allow_all_items)
	{
		if (g_settings.self.is_teleportable)
		{
			return true;
		}

		return detour_base::get_original<is_teleportable>()(_this, allow_all_items);
	}
}
