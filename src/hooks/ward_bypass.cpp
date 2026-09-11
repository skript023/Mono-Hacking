#include "hooking.hpp"

namespace big
{
	bool hooks::private_area_check_access(Vector3 point, float radius, bool flash, bool wardCheck)
	{
		if (g_settings.self.ward_bypass)
		{
			return true;
		}

		return detour_base::get_original<private_area_check_access>()(point, radius, flash, wardCheck);
	}
}

