#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	float hooks::get_weight(void* _this, int stackOverride)
	{
		if (g_settings.self.no_weight)
		{
			LOG(INFO) << "Pointer of player is " << _this;
			return 0.f;
		}

		return detour_base::get_original<get_weight>()(_this, stackOverride);
	}
}