#include "hooking.hpp"
#include "unity/buff_tools.hpp"

namespace big
{
	MonoObject* hooks::seman_add_status_effect(MonoObject* seman, MonoObject* effect, bool reset_time, int item_level, float skill_level, int16_t hit_variant)
	{
		if (g_running && buff_tools::blocks_effect(seman, effect))
		{
			return nullptr;
		}

		return detour_base::get_original<seman_add_status_effect>()(seman, effect, reset_time, item_level, skill_level, hit_variant);
	}
}
