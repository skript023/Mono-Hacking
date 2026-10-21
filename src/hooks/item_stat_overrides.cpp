#include "hooking.hpp"
#include "unity/item_data.hpp"

namespace big
{
	float hooks::item_get_armor(MonoObject* item, int quality, float world_level)
	{
		const auto original  = detour_base::get_original<item_get_armor>()(item, quality, world_level);
		float override_value = 0.f;
		return item_data(item).get_stat_override("ew_armor", override_value) ? override_value : original;
	}

	float hooks::item_get_max_durability(MonoObject* item, int quality)
	{
		const auto original  = detour_base::get_original<item_get_max_durability>()(item, quality);
		float override_value = 0.f;
		return item_data(item).get_stat_override("ew_max_durability", override_value) ? override_value : original;
	}

	float hooks::item_get_base_block_power(MonoObject* item, int quality)
	{
		const auto original  = detour_base::get_original<item_get_base_block_power>()(item, quality);
		float override_value = 0.f;
		return item_data(item).get_stat_override("ew_block", override_value) ? override_value : original;
	}

	float hooks::item_get_deflection_force(MonoObject* item, int quality)
	{
		const auto original  = detour_base::get_original<item_get_deflection_force>()(item, quality);
		float override_value = 0.f;
		return item_data(item).get_stat_override("ew_parry", override_value) ? override_value : original;
	}
}
