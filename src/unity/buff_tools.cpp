#include "buff_tools.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

#include <chrono>
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
				method = mono::get_method_overload("SEMan", "AddStatusEffect", 5, "StatusEffect", "System.Int32", "assembly_valheim");
			if (!method)
				method = mono::get_method_overload("SEMan", "AddStatusEffect", 4, "StatusEffect", "System.Int32", "assembly_valheim");
			if (!method)
			{
				notification::warning("Buff Manager", "Compatible AddStatusEffect method not found.");
				return nullptr;
			}
			bool reset_time = true;
			int16_t variant = -1;
			void* args[] = {&hash, &reset_time, &level, &skill, &variant};
			auto effect = mono::invoke_method(method, seman, args);
			if (!effect)
				notification::warning("Buff Manager", "Status effect was not applied locally. Check player ownership and effect availability.");
			return effect;
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

	void buff_tools::apply_rested_impl(int comfort)
	{
		auto seman = get_seman();
		if (!seman)
		{
			notification::warning("Buff Manager", "Local player or status effect manager is not ready.");
			return;
		}

		int hash = get_stable_hash("Rested");
		int item_level = std::clamp(comfort, 1, 50);
		if (!add_effect(seman, hash, item_level, 0.f))
			return;

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
			return;

		// Staff of Protection bubble
		int hash = get_stable_hash("StaffShield");
		int level = static_cast<int>(hp);
		if (!add_effect(seman, hash, level, 100.f))
			return;

		notification::success("Eitr Shield", std::format("Staff of Protection shield applied with {} HP!", hp));
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

	void buff_tools::clear_all_debuffs_impl()
	{
		auto seman = get_seman();
		if (!seman)
		{
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

		notification::success("Debuff Purge", "All harmful status effects removed (Wet, Poison, Freeze, Burn, etc.)!");
	}

	void buff_tools::update_impl()
	{
		if (g_settings.self.auto_cleanse_debuffs)
		{
			auto seman = get_seman();
			if (seman)
			{
				auto remove_se_method = get_remove_effect_method();
				if (remove_se_method)
				{
					bool quiet = true;
					for (const auto& debuff : s_debuff_names)
					{
						int hash = get_stable_hash(debuff);
						void* args[2] = {&hash, &quiet};
						mono::invoke_method(remove_se_method, seman, args);
					}
				}
			}
		}

		if (g_settings.self.keep_rested)
		{
			if (!get_seman())
				return;
			static auto last_applied = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			if (now - last_applied > std::chrono::seconds(10))
			{
				last_applied = now;
				apply_rested(25);
			}
		}
	}
}
