#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::create_tomb_stone(void* _this)
	{
		if (g_settings.self.no_drop_on_dead)
		{
			LOG(INFO) << "Pointer of player is " << _this;
			return;
		}

		return detour_base::get_original<create_tomb_stone>()(_this);
	}
}