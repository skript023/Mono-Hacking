#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::humanoid_drain_durability(MonoObject* humanoid, MonoObject* item, float dt)
	{
		if (g_settings.self.infinite_durability && humanoid && humanoid == unity::get_local_player())
		{
			if (item)
			{
				mono::set_field_value<"ItemDrop/ItemData", "m_durability">(item, 1000.f);
			}
			return;
		}

		detour_base::get_original<humanoid_drain_durability>()(humanoid, item, dt);
	}
}
