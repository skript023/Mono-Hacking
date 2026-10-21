#include "animal_tools.hpp"
#include "astra/host/canvas.hpp"
#include <astra/host/notification.hpp>
#include "pointers.hpp"
#include "unity/character.hpp"
#include "unity/localization.hpp"
#include "unity/player.hpp"
#include "unity/self.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <imgui.h>
#include <mutex>
#include <thread>
#include "task_mgr.hpp"

namespace big
{
	namespace
	{
		bool is_hostile_to_player(MonoObject* creature, MonoObject* player)
		{
			static auto get_base_ai   = mono::get_method("Character", "GetBaseAI", 0, "assembly_valheim");
			static auto get_faction   = mono::get_method("Character", "GetFaction", 0, "assembly_valheim");
			static auto is_enemy      = mono::get_method("BaseAI", "IsEnemy", 1, "assembly_valheim");
			static auto is_aggravated = mono::get_method("BaseAI", "IsAggravated", 0, "assembly_valheim");
			auto base_ai              = get_base_ai ? mono::invoke_method(get_base_ai, creature, nullptr) : nullptr;
			if (!base_ai)
			{
				return false;
			}
			auto faction = get_faction ? mono::invoke_method(get_faction, creature, nullptr) : nullptr;
			if (faction && *reinterpret_cast<int*>(mono::object_unbox(faction)) == 1)
			{
				auto aggravated = is_aggravated ? mono::invoke_method(is_aggravated, base_ai, nullptr) : nullptr;
				return aggravated && *reinterpret_cast<bool*>(mono::object_unbox(aggravated));
			}
			if (!is_enemy)
			{
				return false;
			}
			void* args[] = {player};
			auto result  = mono::invoke_method(is_enemy, base_ai, args);
			return result && *reinterpret_cast<bool*>(mono::object_unbox(result));
		}
	}

	void animal_tools::set_options_impl(const options& opt)
	{
		g_options = opt;
	}

	animal_tools::options animal_tools::get_options_impl()
	{
		return g_options;
	}

	animal_tools::creature_info animal_tools::get_snapshot_impl()
	{
		std::lock_guard lock(g_mutex);
		return g_snapshot;
	}

	void animal_tools::update_impl()
	{
		auto local_player_obj = unity::get_local_player();
		if (!local_player_obj)
		{
			std::lock_guard lock(g_mutex);
			g_snapshot.valid = false;
			return;
		}

		player local_p(local_player_obj);
		character target = local_p.get_hover_creature();
		auto now         = std::chrono::steady_clock::now();

		if (!target || !target.get_object())
		{
			std::lock_guard lock(g_mutex);
			if (g_snapshot.valid && (now - g_last_seen) > HYSTERESIS_MS)
			{
				g_snapshot.valid = false;
			}
			return;
		}

		if (target == local_p || target.is_player() || target.is_dead())
		{
			std::lock_guard lock(g_mutex);
			g_snapshot.valid = false;
			return;
		}

		creature_info info{};
		info.valid     = true;
		info.character = target.get_object();
		info.name      = target.get_hover_name();
		if (info.name.empty() || info.name == "unknown")
		{
			info.name = "Creature";
		}

		info.level = target.get_level();
		if (info.level <= 0)
		{
			info.level = 1;
		}
		info.stars = std::max(0, info.level - 1);

		info.health     = target.get_health();
		info.max_health = target.get_max_health();
		info.is_tamed   = target.is_tamed();

		info.position  = target.get_position();
		Vector3 my_pos = local_p.get_position();
		info.distance  = my_pos.distance(info.position);

		// Check Tameable component
		auto tame = target.get_tameable();
		if (tame)
		{
			info.has_tameable   = true;
			info.is_tamed       = info.is_tamed || tame.is_tamed();
			info.tameness       = tame.get_tameness();
			info.remaining_time = tame.get_remaining_time();
			info.is_hungry      = tame.is_hungry();
		}

		// Check Procreation component
		auto proc = target.get_procreation();
		if (proc)
		{
			info.has_procreation      = true;
			info.is_pregnant          = proc.is_pregnant();
			info.love_points          = proc.get_love_points();
			info.required_love_points = proc.get_required_love_points();
		}

		// Check MonsterAI component
		auto ai = target.get_monster_ai();
		if (ai)
		{
			info.is_alerted = ai.is_alerted();
		}

		std::lock_guard lock(g_mutex);
		g_snapshot  = std::move(info);
		g_last_seen = now;
	}

	bool animal_tools::tame_creature_impl(character creature)
	{
		if (!creature || !creature.get_object())
		{
			return false;
		}

		// 1. Claim ownership on ZNetView if present
		auto nview           = creature.get_nview();
		static auto claim_m  = mono::get_method("ZNetView", "ClaimOwnership", 0, "assembly_valheim");
		static auto is_owner = mono::get_method("ZNetView", "IsOwner", 0, "assembly_valheim");
		if (!nview || !claim_m || !is_owner)
		{
			notification::warning("Animal Tamer", "Creature ownership methods are unavailable.");
			return false;
		}
		mono::invoke_method(claim_m, nview, nullptr);
		auto owner_result = mono::invoke_method(is_owner, nview, nullptr);
		if (!owner_result || !*reinterpret_cast<bool*>(mono::object_unbox(owner_result)))
		{
			notification::warning("Animal Tamer", "Could not claim this creature to tame it.");
			return false;
		}

		// 2. Tameable component handling
		auto tame = creature.get_tameable();
		if (tame)
		{
			tame.tame();
		}

		// 3. AI pacification
		auto ai = creature.get_monster_ai();
		if (ai)
		{
			ai.make_tame();
			ai.set_alerted(false);
		}

		// 4. Character SetTamed(true)
		creature.set_tamed(true);

		// 5. Heal to full if option set
		if (g_options.heal_on_tame)
		{
			heal_creature(creature);
		}

		std::string name = creature.get_hover_name();
		notification::success("Animal Tamer", std::format("Successfully tamed {}!", name));

		return true;
	}

	bool animal_tools::tame_creature_impl(MonoObject* creature)
	{
		return tame_creature(character(creature));
	}

	bool animal_tools::tame_aimed_creature_impl()
	{
		auto snap = get_snapshot();
		if (!snap.valid || !snap.character)
		{
			notification::warning("Animal Tamer", "No animal or creature aimed at!");
			return false;
		}

		return tame_creature(snap.character);
	}

	bool animal_tools::heal_creature_impl(character creature)
	{
		if (!creature || !creature.get_object())
		{
			return false;
		}

		float max_hp = creature.get_max_health();
		if (max_hp <= 0.f)
		{
			max_hp = 100.f;
		}

		float missing_health = max_hp - creature.get_health();
		if (missing_health <= 0.f)
		{
			return true;
		}

		static auto heal_method = mono::get_method("Character", "Heal", 2, "assembly_valheim");
		if (!heal_method)
		{
			return false;
		}

		bool show_text     = true;
		void* heal_args[2] = {&missing_health, &show_text};
		mono::invoke_method(heal_method, creature.get_object(), heal_args);
		return true;
	}

	bool animal_tools::heal_creature_impl(MonoObject* creature)
	{
		return heal_creature(character(creature));
	}

	bool animal_tools::heal_aimed_creature_impl()
	{
		auto snap = get_snapshot();
		if (!snap.valid || !snap.character)
		{
			notification::warning("Animal Tamer", "No animal aimed at to heal!");
			return false;
		}

		if (heal_creature(snap.character))
		{
			notification::success("Animal Tamer", std::format("Restored {} to full health!", snap.name));
			return true;
		}

		return false;
	}

	bool animal_tools::accelerate_aimed_breeding_impl(float multiplier)
	{
		auto snap = get_snapshot();
		if (!snap.valid || !snap.character)
		{
			notification::warning("Breeding", "Aim at an animal with a Procreation component first.");
			return false;
		}

		character target(snap.character);
		auto procreation = target.get_procreation();
		if (!procreation)
		{
			notification::warning("Breeding", "Could not change timing fields on this creature.");
			return false;
		}

		auto nview                  = target.get_nview();
		static auto claim_ownership = mono::get_method("ZNetView", "ClaimOwnership", 0, "assembly_valheim");
		static auto is_owner        = mono::get_method("ZNetView", "IsOwner", 0, "assembly_valheim");
		if (!nview || !claim_ownership || !is_owner)
		{
			notification::warning("Breeding", "Creature ownership methods are unavailable.");
			return false;
		}
		mono::invoke_method(claim_ownership, nview, nullptr);
		auto owner_result = mono::invoke_method(is_owner, nview, nullptr);
		if (!owner_result || !*reinterpret_cast<bool*>(mono::object_unbox(owner_result)))
		{
			notification::warning("Breeding", "Could not claim this creature to update its breeding timer.");
			return false;
		}

		if (!procreation.accelerate_timing(multiplier))
		{
			notification::warning("Breeding", "Could not change timing fields on this creature.");
			return false;
		}

		notification::success("Breeding", std::format("{} breeding and pregnancy timing set to {:.1f}x.", snap.name, multiplier));
		return true;
	}

	int animal_tools::tame_all_in_radius_impl(float radius)
	{
		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			notification::warning("Animal Tamer", "Player not loaded. Join a world first!");
			return 0;
		}

		Vector3 my_pos  = unity::get_position(local_player);
		auto characters = character::get_all_characters();
		int count       = 0;

		for (auto char_wrap : characters)
		{
			auto obj = char_wrap.get_object();
			if (!obj || (uintptr_t)obj < 0x10000 || obj == local_player)
			{
				continue;
			}

			if (char_wrap.is_player() || char_wrap.is_dead())
			{
				continue;
			}

			Vector3 pos = char_wrap.get_position();
			float dist  = my_pos.distance(pos);
			if (dist <= radius)
			{
				if (tame_creature(char_wrap))
				{
					count++;
				}
			}
		}

		if (count > 0)
		{
			notification::success("Animal Tamer", std::format("Tamed {} creatures within {:.0f}m!", count, radius));
		}
		else
		{
			notification::warning("Animal Tamer", "No untamed creatures found in radius.");
		}

		return count;
	}

	int animal_tools::heal_tamed_in_radius_impl(float radius)
	{
		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			return 0;
		}
		const auto origin = unity::get_position(local_player);
		int healed        = 0;
		for (auto creature : character::get_all_characters())
		{
			if (!creature || creature.is_player() || !creature.is_tamed() || creature.is_dead()
			    || creature.get_position().distance(origin) > radius)
			{
				continue;
			}
			if (heal_creature(creature))
			{
				++healed;
			}
		}
		notification::info("Creature Tools", std::format("Healed {} tamed creatures in range.", healed));
		return healed;
	}

	int animal_tools::kill_hostiles_in_radius_impl(float radius)
	{
		struct damage_types
		{
			float damage, blunt, slash, pierce, chop, pickaxe, fire, frost, lightning, poison, spirit;
		};
		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			return 0;
		}
		static auto hit_data_class = mono::get_class("HitData", "assembly_valheim");
		static auto damage_method  = mono::get_method("Character", "Damage", 1, "assembly_valheim");
		static auto damage_field   = mono::get_field("HitData", "m_damage", "assembly_valheim");
		if (!hit_data_class || !damage_method || !damage_field)
		{
			return 0;
		}
		const auto origin = unity::get_position(local_player);
		int affected      = 0;
		for (auto target : character::get_all_characters())
		{
			if (!target || target.is_player() || target.is_tamed() || target.is_dead() || target.get_position().distance(origin) > radius
			    || !target.get_monster_ai() || !is_hostile_to_player(target.get_object(), local_player))
			{
				continue;
			}
			auto hit = mono::object_new(hit_data_class);
			if (!hit)
			{
				continue;
			}
			damage_types damage{1000000.f, 1000000.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
			mono::set_field_value(hit, damage_field, &damage);
			void* args[1] = {hit};
			mono::invoke_method(damage_method, target.get_object(), args);
			++affected;
		}
		notification::info("Creature Tools", std::format("Sent lethal damage to {} hostile creatures.", affected));
		return affected;
	}

	int animal_tools::set_ai_frozen_in_radius_impl(float radius, bool frozen)
	{
		if (!frozen)
		{
			const auto changed = static_cast<int>(m_frozen_ai.size());
			restore_frozen_ai_impl();
			notification::info("Creature Tools", std::format("Restored AI on {} creatures to their previous state.", changed));
			return changed;
		}

		auto local_player = unity::get_local_player();
		static auto get_enabled = mono::get_method_overload("Behaviour", "get_enabled", 0, "System.Boolean", nullptr, "UnityEngine.CoreModule", "UnityEngine");
		static auto set_enabled = mono::get_method_overload("Behaviour", "set_enabled", 1, "System.Void", "System.Boolean", "UnityEngine.CoreModule", "UnityEngine");
		static auto claim_ownership = mono::get_method("ZNetView", "ClaimOwnership", 0, "assembly_valheim");
		static auto is_owner        = mono::get_method("ZNetView", "IsOwner", 0, "assembly_valheim");
		if (!local_player || !get_enabled || !set_enabled || !claim_ownership || !is_owner)
		{
			return 0;
		}
		const auto origin = unity::get_position(local_player);
		int changed       = 0;
		for (auto creature : character::get_all_characters())
		{
			if (!creature || creature.is_player() || creature.is_tamed() || creature.is_dead()
			    || creature.get_position().distance(origin) > radius)
			{
				continue;
			}
			auto ai = creature.get_monster_ai();
			if (!ai || std::ranges::any_of(m_frozen_ai, [&](const frozen_ai_entry& entry) {
				    return mono::retained_object(entry.handle) == ai.get_object();
			    }))
			{
				continue;
			}

			auto nview = creature.get_nview();
			if (!nview)
			{
				continue;
			}
			auto owner_result = mono::invoke_method(is_owner, nview, nullptr);
			if (!owner_result || !*static_cast<bool*>(mono::object_unbox(owner_result)))
			{
				mono::invoke_method(claim_ownership, nview, nullptr);
				owner_result = mono::invoke_method(is_owner, nview, nullptr);
				if (!owner_result || !*static_cast<bool*>(mono::object_unbox(owner_result)))
				{
					continue;
				}
			}

			MonoObject* exception = nullptr;
			auto enabled_result   = mono::invoke_method(get_enabled, ai.get_object(), nullptr, &exception);
			if (exception || !enabled_result)
			{
				continue;
			}
			const bool was_enabled = *static_cast<bool*>(mono::object_unbox(enabled_result));
			const auto handle      = mono::retain(ai.get_object());
			if (!handle)
			{
				continue;
			}
			void* args[1] = {&frozen};
			mono::invoke_method(set_enabled, ai.get_object(), args, &exception);
			if (exception)
			{
				mono::release(handle);
				continue;
			}
			m_frozen_ai.push_back({handle, was_enabled});
			++changed;
		}
		m_ai_frozen.store(!m_frozen_ai.empty(), std::memory_order_release);
		notification::info("Creature Tools", std::format("{} AI on {} hostile creatures.", frozen ? "Paused" : "Resumed", changed));
		return changed;
	}

	void animal_tools::restore_frozen_ai_impl()
	{
		static auto set_enabled = mono::get_method_overload("Behaviour", "set_enabled", 1, "System.Void", "System.Boolean", "UnityEngine.CoreModule", "UnityEngine");
		for (const auto& entry : m_frozen_ai)
		{
			auto ai = mono::retained_object(entry.handle);
			if (ai && set_enabled)
			{
				void* args[1]         = {const_cast<bool*>(&entry.was_enabled)};
				MonoObject* exception = nullptr;
				mono::invoke_method(set_enabled, ai, args, &exception);
			}
			mono::release(entry.handle);
		}
		m_frozen_ai.clear();
		m_ai_frozen.store(false, std::memory_order_release);
	}

	void animal_tools::shutdown_impl()
	{
		m_cleanup_pending.store(true, std::memory_order_release);
		async_run([this] {
			restore_frozen_ai_impl();
			m_cleanup_pending.store(false, std::memory_order_release);
		});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (m_cleanup_pending.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void animal_tools::hotkey_tick_impl()
	{
		static bool was_down = false;
		if (!g_options.enable_hotkey)
		{
			was_down = false;
			return;
		}

		bool down = (GetAsyncKeyState(g_options.hotkey) & 0x8000) != 0;
		if (down && !was_down && g_pointers && g_pointers->m_hwnd && GetForegroundWindow() == g_pointers->m_hwnd)
		{
			const bool imgui_text_input = ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantTextInput;
			if (!imgui_text_input)
			{
				tame_aimed_creature();
			}
		}
		was_down = down;
	}

	void animal_tools::draw_overlay_impl()
	{
		if (!g_options.show_inspector)
		{
			return;
		}

		auto s = get_snapshot();
		if (!s.valid || !s.character)
		{
			return;
		}

		auto* viewport         = ImGui::GetMainViewport();
		const float card_width = 300.f;

		// Default position centered horizontally, slightly below crosshair
		ImGui::SetNextWindowPos({(viewport->WorkSize.x - card_width) * 0.5f, viewport->WorkSize.y * 0.58f}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({card_width, 0.f}, ImGuiCond_Always);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
		if (!canvas::is_opened())
		{
			flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav;
		}

		if (ImGui::Begin("Animal Inspector##Overlay", nullptr, flags))
		{
			ImGui::TextUnformatted(s.name.c_str());
			ImGui::SameLine();
			if (s.stars == 0)
			{
				ImGui::TextDisabled("Lvl %d", s.level);
			}
			else
			{
				ImGui::TextDisabled("%d %s", s.stars, s.stars == 1 ? "star" : "stars");
			}

			const float fraction = s.max_health > 0.f ? std::clamp(s.health / s.max_health, 0.f, 1.f) : 0.f;
			ImGui::ProgressBar(fraction, ImVec2(-1, 8.f), "");

			// Status is spelled out in words; the old green, yellow, and orange text was unreadable on light themes.
			std::string status = "Wild";
			if (s.is_tamed)
			{
				status = "Tamed";
			}
			else if (s.has_tameable && s.tameness > 0)
			{
				const int remaining = static_cast<int>(s.remaining_time);
				status = std::format("Taming {}% (~{}m {}s)", s.tameness, remaining / 60, remaining % 60);
			}
			if (ImGui::BeginTable("##animal_status", 2))
			{
				ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
				const auto row = [](const char* label, const std::string& value) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextDisabled("%s", label);
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(value.c_str());
				};
				row("Health", std::format("{:.0f} / {:.0f} ({:.0f}%)", s.health, s.max_health, fraction * 100.f));
				row("Distance", std::format("{:.1f} m", s.distance));
				row("Status", status);
				row("Mood", s.is_alerted ? "Alerted" : "Calm");
				row("Hunger", s.is_hungry ? "Hungry" : "Well-fed");
				if (s.has_procreation && s.is_pregnant)
				{
					row("Breeding", "Pregnant");
				}
				else if (s.has_procreation && s.love_points > 0)
				{
					row("Breeding", std::format("Ready (love {} / {})", s.love_points, s.required_love_points));
				}
				ImGui::EndTable();
			}

			if (g_options.enable_hotkey)
			{
				ImGui::Separator();
				auto accent = canvas::current_theme().accent;
				accent.w    = 1.f;
				ImGui::TextColored(accent, "Press [%c] to tame instantly", static_cast<char>(g_options.hotkey));
			}

			if (canvas::is_opened())
			{
				if (ImGui::Button("Instant Tame", ImVec2(120.f, 0.f)))
				{
					tame_aimed_creature();
				}
				ImGui::SameLine();
				if (ImGui::Button("Heal Full", ImVec2(100.f, 0.f)))
				{
					heal_aimed_creature();
				}
			}
		}
		ImGui::End();
	}
}
