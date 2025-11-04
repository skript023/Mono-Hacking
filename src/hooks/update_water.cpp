#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::update_water(MonoObject* _this, float dt)
	{
		if (g_settings.self.is_wet)
		{
            static MonoClass* player_class = mono::get_class("Character", "assembly_valheim");

            // Field Targets
            static MonoClassField* water_level_field = mono::get_field(player_class, "m_waterLevel");
            static MonoClassField* tar_level_field = mono::get_field(player_class, "m_tarLevel");

            // Hanya jika field ditemukan (static field pointer akan tetap ada setelah call pertama)
            if (water_level_field != nullptr && tar_level_field != nullptr)
            {
                float zero_level = 0.0f;

                // Atur m_waterLevel = 0.0f
                mono::set_field_value(_this, water_level_field, &zero_level);

                // Atur m_tarLevel = 0.0f
                // Ini mencegah Wet dan Tarred, karena kondisi if (m_waterLevel > m_tarLevel) akan selalu false
                mono::set_field_value(_this, tar_level_field, &zero_level);

                // Logika selanjutnya di original_UpdateWater akan mencoba mengatur ulang field,
                // tetapi karena field sudah di-set ke 0, AddStatusEffect tidak akan terpicu
            }
		}

		return detour_base::get_original<update_water>()(_this, dt);
	}
}