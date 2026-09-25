#include "hooking.hpp"
#include "utility/unity.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
	struct damage_types
	{
		float m_damage;
		float m_blunt;
		float m_slash;
		float m_pierce;
		float m_chop;
		float m_pickaxe;
		float m_fire;
		float m_frost;
		float m_lightning;
		float m_poison;
		float m_spirit;
	};

	void hooks::attack_modify_damage(MonoObject* attack, MonoObject* hit_data, float damage_factor)
	{
		detour_base::get_original<attack_modify_damage>()(attack, hit_data, damage_factor);

		if (!attack || !hit_data)
			return;

		TRY_CLAUSE
		{
			static auto char_field = mono::get_field("Attack", "m_character", "assembly_valheim");
			static uint32_t char_offset = char_field ? mono::get_field_offset(char_field) : 0;

			MonoObject* attacker = char_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)attack + char_offset) : nullptr;
			auto local_player = unity::get_local_player();

			if (attacker && attacker == local_player)
			{
				static auto damage_field = mono::get_field("HitData", "m_damage", "assembly_valheim");
				static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;

				static auto tool_tier_field = mono::get_field("HitData", "m_toolTier", "assembly_valheim");
				static uint32_t tool_tier_offset = tool_tier_field ? mono::get_field_offset(tool_tier_field) : 0;

				if (damage_offset)
				{
					auto* dmg = reinterpret_cast<damage_types*>((uintptr_t)hit_data + damage_offset);

					if (g_settings.self.one_hit_resource)
					{
						if (tool_tier_offset)
						{
							*reinterpret_cast<int16_t*>((uintptr_t)hit_data + tool_tier_offset) = 100;
						}
						dmg->m_chop = 99999.f;
						dmg->m_pickaxe = 99999.f;
					}

					if (g_settings.self.damage_multiplier > 1.f)
					{
						dmg->m_damage *= g_settings.self.damage_multiplier;
						dmg->m_blunt *= g_settings.self.damage_multiplier;
						dmg->m_slash *= g_settings.self.damage_multiplier;
						dmg->m_pierce *= g_settings.self.damage_multiplier;
						dmg->m_fire *= g_settings.self.damage_multiplier;
						dmg->m_frost *= g_settings.self.damage_multiplier;
						dmg->m_lightning *= g_settings.self.damage_multiplier;
						dmg->m_poison *= g_settings.self.damage_multiplier;
						dmg->m_spirit *= g_settings.self.damage_multiplier;
					}
				}

				if (g_settings.self.infinite_durability)
				{
					static auto weapon_field = mono::get_field("Attack", "m_weapon", "assembly_valheim");
					static uint32_t weapon_offset = weapon_field ? mono::get_field_offset(weapon_field) : 0;

					if (weapon_offset)
					{
						MonoObject* weapon = *reinterpret_cast<MonoObject**>((uintptr_t)attack + weapon_offset);
						if (weapon)
						{
							mono::set_field_value<"ItemDrop/ItemData", "m_durability">(weapon, 1000.f);
						}
					}
				}
			}
		}
		EXCEPT_CLAUSE
	}
}
