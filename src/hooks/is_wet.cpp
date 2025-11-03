#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_under_roof(void* _this)
	{
		if (g_settings.self.is_wet)
		{
			LOG(INFO) << "Pointer of EnvMan is " << _this;

			return false;
		}

		return detour_base::get_original<is_under_roof>()(_this);
	}
}