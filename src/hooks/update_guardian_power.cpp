#include "hooking.hpp"
#include <script_mgr.hpp>
#include "utility/unity.hpp"

namespace big
{
	void hooks::update_guardian_power(MonoObject* player, float dt)
	{
		if (g_running && g_settings.self.forsaken_power_always_ready && player && player == unity::get_local_player())
		{
			static auto m_guardianPowerCooldown = mono::get_field("Player", "m_guardianPowerCooldown", "assembly_valheim");

			float cooldown = 0.f;
			mono::set_field_value(player, m_guardianPowerCooldown, &cooldown);
		}

		return detour_base::get_original<update_guardian_power>()(player, dt);
	}
}
