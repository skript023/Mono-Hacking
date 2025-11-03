#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_teleportable(void* _this)
	{
		if (g_settings.self.is_teleportable)
		{
			LOG(INFO) << "Pointer of player is " << _this;
			return true;
		}

		return detour_base::get_original<is_teleportable>()(_this);
	}
}