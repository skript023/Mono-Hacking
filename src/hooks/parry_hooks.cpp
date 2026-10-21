#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "menu_settings.hpp"
#include "unity/character.hpp"
#include "utility/unity.hpp"

namespace big
{
	namespace
	{
		thread_local bool s_in_auto_parry = false;

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

		bool is_hostile_target(MonoObject* target, MonoObject* attacker)
		{
			static auto get_base_ai   = mono::get_method("Character", "GetBaseAI", 0, "assembly_valheim");
			static auto get_faction   = mono::get_method("Character", "GetFaction", 0, "assembly_valheim");
			static auto is_enemy      = mono::get_method("BaseAI", "IsEnemy", 1, "assembly_valheim");
			static auto is_aggravated = mono::get_method("BaseAI", "IsAggravated", 0, "assembly_valheim");
			MonoObject* base_ai       = get_base_ai ? mono::invoke_method(get_base_ai, target, nullptr) : nullptr;
			if (!base_ai)
			{
				return false;
			}
			if (get_faction)
			{
				auto faction = mono::invoke_method(get_faction, target, nullptr);
				if (faction && *reinterpret_cast<int*>(mono::object_unbox(faction)) == 1)
				{
					auto aggravated = is_aggravated ? mono::invoke_method(is_aggravated, base_ai, nullptr) : nullptr;
					return aggravated && *reinterpret_cast<bool*>(mono::object_unbox(aggravated));
				}
			}
			if (!is_enemy)
			{
				return false;
			}

			void* enemy_args[] = {attacker};
			auto enemy         = mono::invoke_method(is_enemy, base_ai, enemy_args);
			return enemy && *reinterpret_cast<bool*>(mono::object_unbox(enemy));
		}
	}

	void hooks::character_damage(MonoObject* character, MonoObject* hit)
	{
		if (g_running && g_settings.self.hostile_one_hit && character && hit)
		{
			static auto get_attacker = mono::get_method("HitData", "GetAttacker", 0, "assembly_valheim");
			MonoObject* attacker     = get_attacker ? mono::invoke_method(get_attacker, hit, nullptr) : nullptr;
			if (attacker && attacker == unity::get_local_player() && is_hostile_target(character, attacker))
			{
				static auto damage_field      = mono::get_field("HitData", "m_damage", "assembly_valheim");
				static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;
				if (damage_offset)
				{
					auto* damage            = reinterpret_cast<damage_types*>((uintptr_t)hit + damage_offset);
					constexpr float one_hit = 1000000.f;
					damage->m_damage        = std::max(damage->m_damage, one_hit);
					damage->m_blunt         = std::max(damage->m_blunt, one_hit);
					damage->m_slash         = std::max(damage->m_slash, one_hit);
					damage->m_pierce        = std::max(damage->m_pierce, one_hit);
					damage->m_chop          = std::max(damage->m_chop, one_hit);
					damage->m_pickaxe       = std::max(damage->m_pickaxe, one_hit);
					damage->m_fire          = std::max(damage->m_fire, one_hit);
					damage->m_frost         = std::max(damage->m_frost, one_hit);
					damage->m_lightning     = std::max(damage->m_lightning, one_hit);
					damage->m_poison        = std::max(damage->m_poison, one_hit);
					damage->m_spirit        = std::max(damage->m_spirit, one_hit);
				}
			}
		}

		detour_base::get_original<character_damage>()(character, hit);
	}

	void hooks::character_rpc_damage(MonoObject* character, int64_t sender, MonoObject* hit)
	{
		auto local_player = unity::get_local_player();
		if (character && character == local_player)
		{
			if (g_settings.self.god_mode || g_settings.self.incoming_invulnerability)
			{
				return;
			}

			if (hit && g_settings.self.no_fall_damage)
			{
				static auto hit_type_field      = mono::get_field("HitData", "m_hitType", "assembly_valheim");
				static uint32_t hit_type_offset = hit_type_field ? mono::get_field_offset(hit_type_field) : 0;
				if (hit_type_offset && *reinterpret_cast<uint8_t*>((uintptr_t)hit + hit_type_offset) == 3)
				{
					static auto damage_field      = mono::get_field("HitData", "m_damage", "assembly_valheim");
					static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;
					if (damage_offset)
					{
						auto* damage = reinterpret_cast<damage_types*>((uintptr_t)hit + damage_offset);
						*damage      = {};
					}
				}
			}

			if (hit && g_settings.self.incoming_damage_multiplier != 1.f)
			{
				static auto damage_field      = mono::get_field("HitData", "m_damage", "assembly_valheim");
				static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;
				if (damage_offset)
				{
					auto* damage           = reinterpret_cast<damage_types*>((uintptr_t)hit + damage_offset);
					const float multiplier = std::clamp(g_settings.self.incoming_damage_multiplier, 0.f, 100.f);
					damage->m_damage *= multiplier;
					damage->m_blunt *= multiplier;
					damage->m_slash *= multiplier;
					damage->m_pierce *= multiplier;
					damage->m_chop *= multiplier;
					damage->m_pickaxe *= multiplier;
					damage->m_fire *= multiplier;
					damage->m_frost *= multiplier;
					damage->m_lightning *= multiplier;
					damage->m_poison *= multiplier;
					damage->m_spirit *= multiplier;
				}
			}

			if (hit && g_settings.self.stagger_multiplier != 1.f)
			{
				static auto stagger_field      = mono::get_field("HitData", "m_staggerMultiplier", "assembly_valheim");
				static uint32_t stagger_offset = stagger_field ? mono::get_field_offset(stagger_field) : 0;
				if (stagger_offset)
				{
					*reinterpret_cast<float*>((uintptr_t)hit + stagger_offset) *= std::clamp(g_settings.self.stagger_multiplier, 0.f, 10.f);
				}
			}

			if (g_settings.self.auto_parry && hit)
			{
				TRY_CLAUSE
				{
					static auto blockable_field      = mono::get_field("HitData", "m_blockable", "assembly_valheim");
					static uint32_t blockable_offset = blockable_field ? mono::get_field_offset(blockable_field) : 0;
					if (blockable_offset)
					{
						*reinterpret_cast<bool*>((uintptr_t)hit + blockable_offset) = true;
					}

					s_in_auto_parry = true;
					detour_base::get_original<character_rpc_damage>()(character, sender, hit);
					s_in_auto_parry = false;
					return;
				}
				EXCEPT_CLAUSE

				s_in_auto_parry = false;
			}
		}

		detour_base::get_original<character_rpc_damage>()(character, sender, hit);
	}

	bool hooks::humanoid_is_blocking(MonoObject* humanoid)
	{
		if (humanoid && humanoid == unity::get_local_player())
		{
			if (s_in_auto_parry && g_settings.self.auto_parry)
			{
				return true;
			}
		}

		return detour_base::get_original<humanoid_is_blocking>()(humanoid);
	}

	bool hooks::humanoid_block_attack(MonoObject* humanoid, MonoObject* hit, MonoObject* attacker)
	{
		auto local_player = unity::get_local_player();
		if (!humanoid || humanoid != local_player)
		{
			return detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);
		}

		const bool should_parry       = g_settings.self.auto_parry || g_settings.self.always_parry;
		const float force_multiplier  = std::clamp(g_settings.self.parry_force_multiplier, 0.f, 100.f);
		const float window_multiplier = std::clamp(g_settings.self.parry_window_multiplier, 0.f, 100.f);
		const bool modify_parry       = should_parry || force_multiplier != 1.f || window_multiplier != 1.f;
		if (!modify_parry)
		{
			return detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);
		}

		float original_stam_drain          = 0.f;
		float* p_stam_drain                = nullptr;
		float original_block_timer         = 0.f;
		float* p_block_timer               = nullptr;
		static auto block_timer_field      = mono::get_field("Humanoid", "m_blockTimer", "assembly_valheim");
		static uint32_t block_timer_offset = block_timer_field ? mono::get_field_offset(block_timer_field) : 0;
		if (block_timer_offset)
		{
			p_block_timer        = reinterpret_cast<float*>((uintptr_t)humanoid + block_timer_offset);
			original_block_timer = *p_block_timer;
			if (should_parry)
			{
				*p_block_timer = 0.f;
			}
			else if (window_multiplier <= 0.f)
			{
				*p_block_timer = 9999.f;
			}
			else if (window_multiplier != 1.f)
			{
				*p_block_timer /= window_multiplier;
			}
		}

		if (should_parry)
		{
			// Adjust hit direction so automatic parry can catch attacks from any side.
			if (hit)
			{
				static auto dir_field      = mono::get_field("HitData", "m_dir", "assembly_valheim");
				static uint32_t dir_offset = dir_field ? mono::get_field_offset(dir_field) : 0;
				if (dir_offset)
				{
					Vector3* hit_dir = reinterpret_cast<Vector3*>((uintptr_t)hit + dir_offset);
					character local_char(humanoid);
					Vector3 forward = local_char.get_forward();
					float dot       = hit_dir->dot(forward);
					if (dot >= 0.0f)
					{
						*hit_dir = Vector3{-forward.x, -forward.y, -forward.z};
					}
				}
			}

			// Ensure automatic parry does not fail due to exhaustion.
			static auto stamina_field      = mono::get_field("Player", "m_stamina", "assembly_valheim");
			static uint32_t stamina_offset = stamina_field ? mono::get_field_offset(stamina_field) : 0;
			if (stamina_offset)
			{
				float* stamina = reinterpret_cast<float*>((uintptr_t)humanoid + stamina_offset);
				if (*stamina <= 0.0f)
				{
					*stamina = 10.0f;
				}
			}

			// Remove the extra stamina drain used by perfect blocks.
			static auto perfect_stam_field = mono::get_field("Humanoid", "m_perfectBlockStaminaDrain", "assembly_valheim");
			static uint32_t perfect_stam_offset = perfect_stam_field ? mono::get_field_offset(perfect_stam_field) : 0;
			if (perfect_stam_offset)
			{
				p_stam_drain        = reinterpret_cast<float*>((uintptr_t)humanoid + perfect_stam_offset);
				original_stam_drain = *p_stam_drain;
				*p_stam_drain       = 0.0f;
			}
		}

		// 5. Blocker enhancements (bonus & power boost)
		static auto get_blocker_method = mono::get_method("Humanoid", "GetCurrentBlocker", 0, "assembly_valheim");
		MonoObject* blocker            = get_blocker_method ? mono::invoke(get_blocker_method, humanoid) : nullptr;

		float original_timed_bonus   = 0.0f;
		float original_block_power   = 0.0f;
		float* p_timed_bonus         = nullptr;
		float* p_block_power         = nullptr;
		bool* p_use_durability       = nullptr;
		bool original_use_durability = true;

		if (blocker)
		{
			static auto shared_field      = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
			static uint32_t shared_offset = shared_field ? mono::get_field_offset(shared_field) : 0;
			MonoObject* shared_data = shared_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)blocker + shared_offset) : nullptr;

			if (shared_data)
			{
				static auto timed_bonus_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_timedBlockBonus", "assembly_valheim");
				static uint32_t timed_bonus_offset = timed_bonus_field ? mono::get_field_offset(timed_bonus_field) : 0;

				static auto block_power_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_blockPower", "assembly_valheim");
				static uint32_t block_power_offset = block_power_field ? mono::get_field_offset(block_power_field) : 0;

				static auto use_durability_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_useDurability", "assembly_valheim");
				static uint32_t use_durability_offset = use_durability_field ? mono::get_field_offset(use_durability_field) : 0;

				if (timed_bonus_offset)
				{
					p_timed_bonus        = reinterpret_cast<float*>((uintptr_t)shared_data + timed_bonus_offset);
					original_timed_bonus = *p_timed_bonus;
					if (should_parry && *p_timed_bonus <= 1.0f)
					{
						*p_timed_bonus = 2.5f;
					}
				}

				if (block_power_offset)
				{
					p_block_power        = reinterpret_cast<float*>((uintptr_t)shared_data + block_power_offset);
					original_block_power = *p_block_power;

					if (should_parry && hit)
					{
						static auto damage_field      = mono::get_field("HitData", "m_damage", "assembly_valheim");
						static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;
						if (damage_offset)
						{
							auto* dmg = reinterpret_cast<damage_types*>((uintptr_t)hit + damage_offset);
							float total_incoming = dmg->m_damage + dmg->m_blunt + dmg->m_slash + dmg->m_pierce + dmg->m_chop + dmg->m_pickaxe + dmg->m_fire + dmg->m_frost + dmg->m_lightning + dmg->m_poison + dmg->m_spirit;
							float bonus               = std::max(p_timed_bonus ? *p_timed_bonus : 2.0f, 0.01f);
							float current_parry_power = (*p_block_power) * bonus;
							if (current_parry_power < total_incoming + 50.0f)
							{
								float needed   = (total_incoming + 50.0f) / bonus;
								*p_block_power = std::max(*p_block_power, needed);
							}
						}
					}
					*p_block_power *= force_multiplier;
				}

				if (g_settings.self.infinite_durability && use_durability_offset)
				{
					p_use_durability        = reinterpret_cast<bool*>((uintptr_t)shared_data + use_durability_offset);
					original_use_durability = *p_use_durability;
					*p_use_durability       = false;
				}
			}
		}

		bool result = detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);

		if (p_block_timer)
		{
			*p_block_timer = original_block_timer;
		}
		if (p_timed_bonus)
		{
			*p_timed_bonus = original_timed_bonus;
		}
		if (p_block_power)
		{
			*p_block_power = original_block_power;
		}
		if (p_stam_drain)
		{
			*p_stam_drain = original_stam_drain;
		}
		if (p_use_durability)
		{
			*p_use_durability = original_use_durability;
		}

		return result;
	}
}
