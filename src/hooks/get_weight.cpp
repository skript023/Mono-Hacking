#include "hooking.hpp"
#include <script_mgr.hpp>
#include "unity/item_data.hpp"
#include "utility/unity.hpp"

namespace big
{
	float hooks::get_weight(void* _this, int stackOverride)
	{
		float override_weight = 0.f;
		if (item_data(static_cast<MonoObject*>(_this)).get_stat_override("ew_weight", override_weight))
		{
			auto item = static_cast<MonoObject*>(_this);
			const int stack = stackOverride >= 0 ? stackOverride : mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item);
			return override_weight * static_cast<float>(std::max(0, stack));
		}

		return detour_base::get_original<get_weight>()(_this, stackOverride);
	}

	void hooks::inventory_update_total_weight(MonoObject* inventory)
	{
		detour_base::get_original<inventory_update_total_weight>()(inventory);
		if (!g_running || !g_settings.self.no_weight || !inventory)
		{
			return;
		}

		auto player = unity::get_local_player();
		if (player && inventory == mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player))
		{
			mono::set_field_value<"Inventory", "m_totalWeight">(inventory, 0.f);
		}
	}
}
