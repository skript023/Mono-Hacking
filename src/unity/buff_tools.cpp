#include "buff_tools.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

#include <chrono>
#include <cstring>
#include <format>

namespace big
{
	namespace
	{
		const std::vector<buff_tools::boss_power> s_boss_powers = {
		    {"GP_Eikthyr", "Eikthyr's Surge", "Eikthyr (Meadows)", "-60% Run and Jump stamina usage"},
		    {"GP_TheElder", "Elder's Wrath", "The Elder (Black Forest)", "+60% Wood cutting speed and axe damage"},
		    {"GP_Bonemass", "Bonemass' Resilience", "Bonemass (Swamp)", "Massive physical resistance (Blunt, Slash, Pierce)"},
		    {"GP_Moder", "Moder's Flight", "Moder (Mountain)", "Tailwind always blows from behind your ship"},
		    {"GP_Yagluth", "Yagluth's Aegis", "Yagluth (Plains)", "High resistance against Fire, Frost, and Lightning"},
		    {"GP_Queen", "Queen's Transcendence", "The Queen (Mistlands)", "+100% Eitr regeneration and +60% Mining speed"},
		    {"GP_Ashlands", "Fader's Dominion", "Fader (Ashlands)", "+Movement speed, carry weight, and ferocious attack buff"},
		    {"GP_DeepNorth", "Deep North Mastery", "Deep North Boss", "Cold immunity, hyper-stamina and frost empowerment"}};

		const std::vector<std::string> s_debuff_names = {
		    "Wet",
		    "Poison",
		    "Freezing",
		    "Cold",
		    "Burning",
		    "Frost",
		    "Smoked",
		    "Tared",
		    "Encumbered",
		    "SoftDeath"};

		int get_stable_hash(const std::string& name)
		{
			static thread_local MonoMethod* method = nullptr;
			if (!method)
				method = mono::get_method("StringExtensionMethods", "GetStableHashCode", 1, "assembly_utils");
			if (method)
			{
				auto ms = mono::to_mono_string(name);
				void* args[1] = {ms};
				auto ret = mono::invoke_method(method, nullptr, args);
				if (ret)
				{
					auto unboxed = mono::object_unbox(ret);
					if (unboxed)
						return *reinterpret_cast<int*>(unboxed);
				}
			}

			// Fallback djb2 stable hash
			uint32_t h1 = 5381, h2 = h1;
			for (size_t i = 0; i < name.length() && name[i] != 0; i += 2)
			{
				h1 = ((h1 << 5) + h1) ^ static_cast<int>(name[i]);
				if (i == name.length() - 1 || name[i + 1] == 0)
					break;
				h2 = ((h2 << 5) + h2) ^ static_cast<int>(name[i + 1]);
			}
			return static_cast<int>(h1 + h2 * 1566083941u);
		}

		MonoObject* get_seman()
		{
			auto player = unity::get_local_player();
			if (!player)
				return nullptr;

			static thread_local MonoMethod* method = nullptr;
			if (!method)
				method = mono::get_method("Character", "GetSEMan", 0, "assembly_valheim");
			if (!method)
				return nullptr;

			return mono::invoke_method(method, player, nullptr);
		}

		MonoObject* add_effect(MonoObject* seman, int hash, int level, float skill)
		{
			static thread_local MonoMethod* method = nullptr;
			if (!method)
				method = mono::get_method_overload("SEMan", "AddStatusEffect", 4, "StatusEffect", "Int32", "assembly_valheim");
			if (!method)
				method = mono::get_method_overload("SEMan", "AddStatusEffect", 4, "StatusEffect", "System.Int32", "assembly_valheim");
			if (!method)
				method = mono::get_method("SEMan", "AddStatusEffect", 4, "assembly_valheim");
			if (!method)
			{
				notification::warning("Buff Manager", "Compatible AddStatusEffect method not found.");
				return nullptr;
			}
			bool reset_time = true;
			void* args[] = {&hash, &reset_time, &level, &skill};
			auto effect = mono::invoke_method(method, seman, args);
			if (!effect)
			{
				// If status effect was already active, Valheim's AddStatusEffect resets time and returns null.
				// Query GetStatusEffect to verify if the effect is currently active.
				static thread_local MonoMethod* get_se_method = nullptr;
				if (!get_se_method)
					get_se_method = mono::get_method_overload("SEMan", "GetStatusEffect", 1, "StatusEffect", "Int32", "assembly_valheim");
				if (!get_se_method)
					get_se_method = mono::get_method("SEMan", "GetStatusEffect", 1, "assembly_valheim");
				if (get_se_method)
				{
					void* get_args[] = {&hash};
					effect = mono::invoke_method(get_se_method, seman, get_args);
				}
			}
			return effect;
		}

		MonoObject* find_shield_in_object_db(int& out_hash)
		{
			auto obj_db = unity::get_object_db();
			if (!obj_db)
				return nullptr;

			auto db_klass = mono::object_get_class(obj_db);
			if (!db_klass)
				return nullptr;

			auto se_list_field = mono::get_field(db_klass, "m_StatusEffects");
			if (!se_list_field)
				return nullptr;

			MonoObject* se_list = nullptr;
			mono::get_field_value(obj_db, se_list_field, &se_list);
			if (!se_list)
				return nullptr;

			auto effects = unity::list_to_vector(se_list);
			for (auto* se : effects)
			{
				if (!se)
					continue;
				auto se_klass = mono::object_get_class(se);
				if (!se_klass)
					continue;
				const char* name = mono::class_get_name(se_klass);
				if (name && std::strcmp(name, "SE_Shield") == 0)
				{
					auto hash_method = mono::class_get_method_from_name(se_klass, "NameHash", 0);
					if (hash_method)
					{
						auto ret = mono::invoke_method(hash_method, se, nullptr);
						if (ret)
						{
							auto unboxed = mono::object_unbox(ret);
							if (unboxed)
								out_hash = *reinterpret_cast<int*>(unboxed);
						}
					}
					return se;
				}
			}
			return nullptr;
		}

		MonoObject* get_active_shield(MonoObject* seman, int known_hash = 0)
		{
			if (known_hash != 0)
			{
				static thread_local MonoMethod* get_se_method = nullptr;
				if (!get_se_method)
					get_se_method = mono::get_method_overload("SEMan", "GetStatusEffect", 1, "StatusEffect", "Int32", "assembly_valheim");
				if (!get_se_method)
					get_se_method = mono::get_method("SEMan", "GetStatusEffect", 1, "assembly_valheim");
				if (get_se_method)
				{
					void* args[] = {&known_hash};
					auto res = mono::invoke_method(get_se_method, seman, args);
					if (res)
						return res;
				}
			}

			static thread_local MonoMethod* get_all_method = nullptr;
			if (!get_all_method)
				get_all_method = mono::get_method("SEMan", "GetStatusEffects", 0, "assembly_valheim");
			if (get_all_method)
			{
				auto list_obj = mono::invoke_method(get_all_method, seman, nullptr);
				if (list_obj)
				{
					auto active_effects = unity::list_to_vector(list_obj);
					for (auto* se : active_effects)
					{
						if (!se)
							continue;
						auto klass = mono::object_get_class(se);
						if (!klass)
							continue;
						const char* name = mono::class_get_name(klass);
						if (name && std::strcmp(name, "SE_Shield") == 0)
							return se;
					}
				}
			}
			return nullptr;
		}

		MonoMethod* get_remove_effect_method()
		{
			static thread_local MonoMethod* method = nullptr;
			if (!method)
				method = mono::get_method_overload("SEMan", "RemoveStatusEffect", 2, "System.Boolean", "System.Int32", "assembly_valheim");
			return method;
		}
	}

	const std::vector<buff_tools::boss_power>& buff_tools::get_boss_powers_impl()
	{
		return s_boss_powers;
	}

	void buff_tools::apply_rested_impl(int comfort, bool notify)
	{
		auto seman = get_seman();
		if (!seman)
		{
			if (notify)
				notification::warning("Buff Manager", "Local player or status effect manager is not ready.");
			return;
		}

		int hash = get_stable_hash("Rested");
		int item_level = std::clamp(comfort, 1, 50);
		if (!add_effect(seman, hash, item_level, 0.f))
			return;

		if (notify)
			notification::success("Buff Manager", std::format("Rested buff applied with Comfort Level {}!", item_level));
	}

	void buff_tools::activate_guardian_power_impl(const std::string& power_name)
	{
		auto seman = get_seman();
		if (!seman)
		{
			notification::warning("Buff Manager", "Local player or status effect manager is not ready.");
			return;
		}

		int hash = get_stable_hash(power_name);
		if (!add_effect(seman, hash, 0, 0.f))
			return;

		notification::success("Boss Buff", std::format("Activated {}!", power_name));
	}

	void buff_tools::activate_all_guardian_powers_impl()
	{
		auto seman = get_seman();
		if (!seman)
		{
			notification::warning("Buff Manager", "Local player or status effect manager is not ready.");
			return;
		}

		for (const auto& bp : s_boss_powers)
		{
			int hash = get_stable_hash(bp.internal_name);
			if (!add_effect(seman, hash, 0, 0.f))
				return;
		}

		notification::success("Boss Buffs", "All 7 Guardian Powers successfully activated together!");
	}

	void buff_tools::apply_eitr_shield_impl(float hp)
	{
		auto seman = get_seman();
		if (!seman)
		{
			notification::warning("Eitr Shield", "Player or StatusEffect manager is not ready.");
			return;
		}

		int shield_hash = 0;
		MonoObject* shield_prefab = find_shield_in_object_db(shield_hash);

		if (shield_hash == 0)
			shield_hash = get_stable_hash("Staff_shield");

		MonoObject* active_shield = get_active_shield(seman, shield_hash);

		if (!active_shield)
		{
			if (shield_prefab)
			{
				static thread_local MonoMethod* add_se_method = nullptr;
				if (!add_se_method)
					add_se_method = mono::get_method_overload("SEMan", "AddStatusEffect", 4, "StatusEffect", "StatusEffect", "assembly_valheim");
				if (add_se_method)
				{
					bool reset_time = true;
					int item_level = 1;
					float skill_level = 100.f;
					void* args[] = {shield_prefab, &reset_time, &item_level, &skill_level};
					active_shield = mono::invoke_method(add_se_method, seman, args);
				}
			}

			if (!active_shield)
			{
				const int candidate_hashes[] = {
				    shield_hash,
				    get_stable_hash("Staff_shield"),
				    get_stable_hash("SE_Shield"),
				    get_stable_hash("StaffShield")};

				for (int h : candidate_hashes)
				{
					if (h == 0)
						continue;
					active_shield = add_effect(seman, h, 1, 100.f);
					if (active_shield)
					{
						shield_hash = h;
						break;
					}
					active_shield = get_active_shield(seman, h);
					if (active_shield)
					{
						shield_hash = h;
						break;
					}
				}
			}
		}

		if (!active_shield)
			active_shield = get_active_shield(seman, shield_hash);

		if (!active_shield)
		{
			notification::warning("Eitr Shield", "Could not apply Staff of Protection shield. Join a world first!");
			return;
		}

		// Configure shield stats on the active SE_Shield instance
		auto se_klass = mono::object_get_class(active_shield);
		if (se_klass)
		{
			auto absorb_field = mono::get_field(se_klass, "m_totalAbsorbDamage");
			if (absorb_field)
				mono::set_field_value(active_shield, absorb_field, &hp);

			auto damage_field = mono::get_field(se_klass, "m_damage");
			if (damage_field)
			{
				float zero = 0.f;
				mono::set_field_value(active_shield, damage_field, &zero);
			}
		}

		auto status_effect_klass = mono::get_class("StatusEffect", "assembly_valheim");
		if (status_effect_klass)
		{
			auto ttl_field = mono::get_field(status_effect_klass, "m_ttl");
			if (ttl_field)
			{
				float ttl = 3600.f;
				mono::set_field_value(active_shield, ttl_field, &ttl);
			}

			auto time_field = mono::get_field(status_effect_klass, "m_time");
			if (time_field)
			{
				float zero = 0.f;
				mono::set_field_value(active_shield, time_field, &zero);
			}
		}

		notification::success("Eitr Shield", std::format("Staff of Protection shield applied with {:.0f} HP (1 hour)!", hp));
	}

	void buff_tools::remove_status_effect_impl(const std::string& name)
	{
		auto seman = get_seman();
		if (!seman)
			return;

		auto remove_se_method = get_remove_effect_method();
		if (!remove_se_method)
			return;

		int hash = get_stable_hash(name);
		bool quiet = true;
		void* args[2] = {&hash, &quiet};
		mono::invoke_method(remove_se_method, seman, args);
	}

	void buff_tools::clear_all_debuffs_impl(bool notify)
	{
		auto seman = get_seman();
		if (!seman)
		{
			if (notify)
				notification::warning("Buff Manager", "Local player or status effect manager is not ready.");
			return;
		}

		auto remove_se_method = get_remove_effect_method();
		if (!remove_se_method)
			return;

		bool quiet = true;
		for (const auto& debuff : s_debuff_names)
		{
			int hash = get_stable_hash(debuff);
			void* args[2] = {&hash, &quiet};
			mono::invoke_method(remove_se_method, seman, args);
		}

		if (notify)
			notification::success("Debuff Purge", "All harmful status effects removed (Wet, Poison, Freeze, Burn, etc.)!");
	}
}

