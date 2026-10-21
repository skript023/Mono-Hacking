#include "hooking.hpp"
#include <script_mgr.hpp>
#include "unity/item_data.hpp"

namespace big
{
	float hooks::get_weight(void* _this, int stackOverride)
	{
		if (g_settings.self.no_weight)
		{
			return 0.f;
		}

		float override_weight = 0.f;
		if (item_data(static_cast<MonoObject*>(_this)).get_stat_override("ew_weight", override_weight))
		{
			auto item = static_cast<MonoObject*>(_this);
			const int stack = stackOverride >= 0 ? stackOverride : mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item);
			return override_weight * static_cast<float>(std::max(0, stack));
		}

		return detour_base::get_original<get_weight>()(_this, stackOverride);
	}
}
