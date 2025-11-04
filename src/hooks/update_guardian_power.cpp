#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::update_guardian_power(MonoObject* player, float dt)
	{
		if (g_settings.self.forsaken_power_always_ready)
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_guardianPowerCooldown = mono::get_field(klass, "m_guardianPowerCooldown");

			float cooldown = 0.f;
			mono::set_field_value(player, m_guardianPowerCooldown, &cooldown);
		}

		return detour_base::get_original<update_guardian_power>()(player, dt);
	}
}