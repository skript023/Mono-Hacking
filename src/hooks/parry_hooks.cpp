#include "hooking.hpp"
#include "utility/unity.hpp"
#include "unity/character.hpp"
#include "logger/exception_handler.hpp"
#include "menu_settings.hpp"

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
			float m_nonPlayer;
		};
	}

	void hooks::character_rpc_damage(MonoObject* character, int64_t sender, MonoObject* hit)
	{
		auto local_player = unity::get_local_player();
		if (character && character == local_player)
		{
			if (g_settings.self.auto_parry && hit)
			{
				TRY_CLAUSE
				{
					static auto blockable_field = mono::get_field("HitData", "m_blockable", "assembly_valheim");
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
			else if (g_settings.self.god_mode)
			{
				return;
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

		const bool should_parry = g_settings.self.auto_parry || g_settings.self.always_parry;
		if (!should_parry)
		{
			return detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);
		}

		TRY_CLAUSE
		{
			// 1. Force m_blockTimer to 0.0f to guarantee perfect parry window
			static auto block_timer_field = mono::get_field("Humanoid", "m_blockTimer", "assembly_valheim");
			static uint32_t block_timer_offset = block_timer_field ? mono::get_field_offset(block_timer_field) : 0;
			if (block_timer_offset)
			{
				*reinterpret_cast<float*>((uintptr_t)humanoid + block_timer_offset) = 0.0f;
			}

			// 2. Adjust hit direction so 360-degree parry succeeds
			if (hit)
			{
				static auto dir_field = mono::get_field("HitData", "m_dir", "assembly_valheim");
				static uint32_t dir_offset = dir_field ? mono::get_field_offset(dir_field) : 0;
				if (dir_offset)
				{
					Vector3* hit_dir = reinterpret_cast<Vector3*>((uintptr_t)hit + dir_offset);
					character local_char(humanoid);
					Vector3 forward = local_char.get_forward();
					float dot = hit_dir->dot(forward);
					if (dot >= 0.0f)
					{
						*hit_dir = Vector3{-forward.x, -forward.y, -forward.z};
					}
				}
			}

			// 3. Ensure player has stamina so block does not fail due to exhaustion
			static auto stamina_field = mono::get_field("Player", "m_stamina", "assembly_valheim");
			static uint32_t stamina_offset = stamina_field ? mono::get_field_offset(stamina_field) : 0;
			if (stamina_offset)
			{
				float* stamina = reinterpret_cast<float*>((uintptr_t)humanoid + stamina_offset);
				if (*stamina <= 0.0f)
				{
					*stamina = 10.0f;
				}
			}

			// 4. Zero out perfect block stamina drain temporarily
			static auto perfect_stam_field = mono::get_field("Humanoid", "m_perfectBlockStaminaDrain", "assembly_valheim");
			static uint32_t perfect_stam_offset = perfect_stam_field ? mono::get_field_offset(perfect_stam_field) : 0;
			float original_stam_drain = 0.0f;
			float* p_stam_drain = nullptr;
			if (perfect_stam_offset)
			{
				p_stam_drain = reinterpret_cast<float*>((uintptr_t)humanoid + perfect_stam_offset);
				original_stam_drain = *p_stam_drain;
				*p_stam_drain = 0.0f;
			}

			// 5. Blocker enhancements (bonus & power boost)
			static auto get_blocker_method = mono::get_method("Humanoid", "GetCurrentBlocker", 0, "assembly_valheim");
			MonoObject* blocker = get_blocker_method ? mono::invoke(get_blocker_method, humanoid) : nullptr;

			float original_timed_bonus = 0.0f;
			float original_block_power = 0.0f;
			float* p_timed_bonus = nullptr;
			float* p_block_power = nullptr;
			bool* p_use_durability = nullptr;
			bool original_use_durability = true;

			if (blocker)
			{
				static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
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
						p_timed_bonus = reinterpret_cast<float*>((uintptr_t)shared_data + timed_bonus_offset);
						original_timed_bonus = *p_timed_bonus;
						if (*p_timed_bonus <= 1.0f)
						{
							*p_timed_bonus = 2.5f;
						}
					}

					if (block_power_offset && hit)
					{
						p_block_power = reinterpret_cast<float*>((uintptr_t)shared_data + block_power_offset);
						original_block_power = *p_block_power;

						static auto damage_field = mono::get_field("HitData", "m_damage", "assembly_valheim");
						static uint32_t damage_offset = damage_field ? mono::get_field_offset(damage_field) : 0;
						if (damage_offset)
						{
							auto* dmg = reinterpret_cast<damage_types*>((uintptr_t)hit + damage_offset);
							float total_incoming = dmg->m_damage + dmg->m_blunt + dmg->m_slash + dmg->m_pierce +
												   dmg->m_fire + dmg->m_frost + dmg->m_lightning + dmg->m_poison + dmg->m_spirit + dmg->m_nonPlayer;
							float bonus = p_timed_bonus ? *p_timed_bonus : 2.0f;
							float current_parry_power = (*p_block_power) * bonus;
							if (current_parry_power < total_incoming + 50.0f)
							{
								float needed = (total_incoming + 50.0f) / bonus;
								*p_block_power = std::max(*p_block_power, needed);
							}
						}
					}

					if (g_settings.self.infinite_durability && use_durability_offset)
					{
						p_use_durability = reinterpret_cast<bool*>((uintptr_t)shared_data + use_durability_offset);
						original_use_durability = *p_use_durability;
						*p_use_durability = false;
					}
				}
			}

			bool result = detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);

			if (p_timed_bonus)
				*p_timed_bonus = original_timed_bonus;
			if (p_block_power)
				*p_block_power = original_block_power;
			if (p_stam_drain)
				*p_stam_drain = original_stam_drain;
			if (p_use_durability)
				*p_use_durability = original_use_durability;

			return result;
		}
		EXCEPT_CLAUSE

		return detour_base::get_original<humanoid_block_attack>()(humanoid, hit, attacker);
	}
}

