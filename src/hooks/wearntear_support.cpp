#include "hooking.hpp"

namespace big
{
	float hooks::wearntear_get_support(MonoObject* this_ptr)
	{
		if (g_settings.self.infinite_stability)
		{
			return 1500.f;
		}

		return detour_base::get_original<wearntear_get_support>()(this_ptr);
	}

	bool hooks::wearntear_have_support(MonoObject* this_ptr)
	{
		if (g_settings.self.infinite_stability)
		{
			return true;
		}

		return detour_base::get_original<wearntear_have_support>()(this_ptr);
	}
}

