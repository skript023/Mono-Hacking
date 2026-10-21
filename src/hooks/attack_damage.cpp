#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "utility/unity.hpp"
#include "unity/item_data.hpp"


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
		{
			return;
		}

		static auto char_field      = mono::get_field("Attack", "m_character", "assembly_valheim");
		static uint32_t char_offset = char_field ? mono::get_field_offset(char_field) : 0;

		MonoObject* attacker = char_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)attack + char_offset) : nullptr;
		auto local_player    = unity::get_local_player();

		if (attacker && attacker == local_player)
		{
			static auto weapon_field      = mono::get_field("Attack", "m_weapon", "assembly_valheim");
			static uint32_t weapon_offset = weapon_field ? mono::get_field_offset(weapon_field) : 0;
			MonoObject* weapon = weapon_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)attack + weapon_offset) : nullptr;
			item_data weapon_data(weapon);
			if (g_settings.self.push_force_multiplier != 1.f)
			{
				static auto push_field      = mono::get_field("HitData", "m_pushForce", "assembly_valheim");
				static uint32_t push_offset = push_field ? mono::get_field_offset(push_field) : 0;
				if (push_offset)
				{
					*reinterpret_cast<float*>((uintptr_t)hit_data + push_offset) *= std::clamp(g_settings.self.push_force_multiplier, 0.f, 100.f);
				}
			}

			static auto damage_field      = mono::get_field("HitData", "m_damage", "assembly_valheim");
			static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;

			static auto tool_tier_field      = mono::get_field("HitData", "m_toolTier", "assembly_valheim");
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
					dmg->m_chop    = 99999.f;
					dmg->m_pickaxe = 99999.f;
				}

				const float damage_multiplier = std::clamp(g_settings.self.damage_multiplier, 0.f, 50.f);
				if (damage_multiplier != 1.f)
				{
					dmg->m_damage *= damage_multiplier;
					dmg->m_blunt *= damage_multiplier;
					dmg->m_slash *= damage_multiplier;
					dmg->m_pierce *= damage_multiplier;
					dmg->m_chop *= damage_multiplier;
					dmg->m_pickaxe *= damage_multiplier;
					dmg->m_fire *= damage_multiplier;
					dmg->m_frost *= damage_multiplier;
					dmg->m_lightning *= damage_multiplier;
					dmg->m_poison *= damage_multiplier;
					dmg->m_spirit *= damage_multiplier;
				}

				float item_damage_multiplier = 1.f;
				if (weapon_data.get_stat_override("ew_damage_multiplier", item_damage_multiplier))
				{
					dmg->m_damage *= item_damage_multiplier;
					dmg->m_blunt *= item_damage_multiplier;
					dmg->m_slash *= item_damage_multiplier;
					dmg->m_pierce *= item_damage_multiplier;
					dmg->m_chop *= item_damage_multiplier;
					dmg->m_pickaxe *= item_damage_multiplier;
					dmg->m_fire *= item_damage_multiplier;
					dmg->m_frost *= item_damage_multiplier;
					dmg->m_lightning *= item_damage_multiplier;
					dmg->m_poison *= item_damage_multiplier;
					dmg->m_spirit *= item_damage_multiplier;
				}
			}

			static auto push_field      = mono::get_field("HitData", "m_pushForce", "assembly_valheim");
			static uint32_t push_offset = push_field ? mono::get_field_offset(push_field) : 0;
			float push_override         = 0.f;
			if (push_offset && weapon_data.get_stat_override("ew_knockback", push_override))
			{
				*reinterpret_cast<float*>((uintptr_t)hit_data + push_offset) = push_override;
			}

			static auto backstab_field      = mono::get_field("HitData", "m_backstabBonus", "assembly_valheim");
			static uint32_t backstab_offset = backstab_field ? mono::get_field_offset(backstab_field) : 0;
			float backstab_override         = 0.f;
			if (backstab_offset && weapon_data.get_stat_override("ew_backstab", backstab_override))
			{
				*reinterpret_cast<float*>((uintptr_t)hit_data + backstab_offset) = backstab_override;
			}
			if (backstab_offset && g_settings.self.backstab_multiplier != 1.f)
			{
				*reinterpret_cast<float*>((uintptr_t)hit_data + backstab_offset) *= std::clamp(g_settings.self.backstab_multiplier, 0.f, 100.f);
			}

			if (g_settings.self.infinite_durability)
			{
				if (weapon)
				{
					mono::set_field_value<"ItemDrop/ItemData", "m_durability">(weapon, 1000.f);
				}
			}
		}
	}
}
