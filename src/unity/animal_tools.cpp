#include "animal_tools.hpp"
#include "astra/host/canvas.hpp"
#include "notification/notification_service.hpp"
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

namespace big
{
	namespace
	{
		void show_center_message(const std::string& msg)
		{
			auto player = unity::get_local_player();
			if (!player)
				return;
			static auto method = mono::get_method("Player", "Message", 4, "assembly_valheim");
			if (!method)
				return;
			int type = 2; // MessageHud.MessageType.Center
			auto ms = mono::to_mono_string(msg);
			int amount = 0;
			void* icon = nullptr;
			void* args[4] = {&type, ms, &amount, icon};
			mono::invoke_method(method, player, args);
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
		auto now = std::chrono::steady_clock::now();

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
		info.valid = true;
		info.character = target.get_object();
		info.name = target.get_hover_name();
		if (info.name.empty() || info.name == "unknown")
			info.name = "Creature";

		info.level = target.get_level();
		if (info.level <= 0)
			info.level = 1;
		info.stars = std::max(0, info.level - 1);

		info.health = target.get_health();
		info.max_health = target.get_max_health();
		info.is_tamed = target.is_tamed();

		info.position = target.get_position();
		Vector3 my_pos = local_p.get_position();
		info.distance = my_pos.distance(info.position);

		// Check Tameable component
		auto tame = target.get_tameable();
		if (tame)
		{
			info.has_tameable = true;
			info.is_tamed = info.is_tamed || tame.is_tamed();
			info.tameness = tame.get_tameness();
			info.remaining_time = tame.get_remaining_time();
			info.is_hungry = tame.is_hungry();
		}

		// Check Procreation component
		auto proc = target.get_procreation();
		if (proc)
		{
			info.has_procreation = true;
			info.is_pregnant = proc.is_pregnant();
			info.love_points = proc.get_love_points();
			info.required_love_points = proc.get_required_love_points();
		}

		// Check MonsterAI component
		auto ai = target.get_monster_ai();
		if (ai)
		{
			info.is_alerted = ai.is_alerted();
		}

		std::lock_guard lock(g_mutex);
		g_snapshot = std::move(info);
		g_last_seen = now;
	}

	bool animal_tools::tame_creature_impl(character creature)
	{
		if (!creature || !creature.get_object())
			return false;

		// 1. Claim ownership on ZNetView if present
		auto nview = creature.get_nview();
		if (nview)
		{
			static auto claim_m = mono::get_method("ZNetView", "ClaimOwnership", 0, "assembly_valheim");
			if (claim_m)
				mono::invoke_method(claim_m, nview, nullptr);
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
		show_center_message(std::format("{} tamed!", name));

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
			return false;

		float max_hp = creature.get_max_health();
		if (max_hp <= 0.f)
			max_hp = 100.f;

		creature.set_health(max_hp);
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

	int animal_tools::tame_all_in_radius_impl(float radius)
	{
		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			notification::warning("Animal Tamer", "Player not loaded. Join a world first!");
			return 0;
		}

		Vector3 my_pos = unity::get_position(local_player);
		auto characters = character::get_all_characters();
		int count = 0;

		for (auto char_wrap : characters)
		{
			auto obj = char_wrap.get_object();
			if (!obj || (uintptr_t)obj < 0x10000 || obj == local_player)
				continue;

			if (char_wrap.is_player() || char_wrap.is_dead())
				continue;

			Vector3 pos = char_wrap.get_position();
			float dist = my_pos.distance(pos);
			if (dist <= radius)
			{
				if (tame_creature(char_wrap))
					count++;
			}
		}

		if (count > 0)
		{
			notification::success("Animal Tamer", std::format("Tamed {} creatures within {:.0f}m!", count, radius));
			show_center_message(std::format("Tamed {} nearby creatures!", count));
		}
		else
		{
			notification::warning("Animal Tamer", "No untamed creatures found in radius.");
		}

		return count;
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
		if (down && !was_down && GetForegroundWindow() == g_pointers->m_hwnd)
		{
			if (!ImGui::GetIO().WantTextInput)
			{
				tame_aimed_creature();
			}
		}
		was_down = down;
	}

	void animal_tools::draw_overlay_impl()
	{
		if (!g_options.show_inspector)
			return;

		auto s = get_snapshot();
		if (!s.valid || !s.character)
			return;

		auto* viewport = ImGui::GetMainViewport();
		const float card_width = 300.f;

		// Default position centered horizontally, slightly below crosshair
		ImGui::SetNextWindowPos(
		    {(viewport->WorkSize.x - card_width) * 0.5f, viewport->WorkSize.y * 0.58f},
		    ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({card_width, 0.f}, ImGuiCond_Always);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
		if (!canvas::is_opened())
		{
			flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav;
		}

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.08f, 0.13f, 0.92f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.45f, 0.80f, 0.65f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 10.f));

		if (ImGui::Begin("Animal Inspector##Overlay", nullptr, flags))
		{
			ImGui::PushTextWrapPos(0.f);

			// Header: Name and Stars
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 1.0f, 1.0f), "%s", s.name.c_str());
			ImGui::SameLine();
			if (s.stars == 0)
			{
				ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.70f, 1.0f), "[Lvl %d]", s.level);
			}
			else if (s.stars == 1)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.20f, 1.0f), "[★ 1 Star]");
			}
			else if (s.stars == 2)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.15f, 1.0f), "[★★ 2 Stars]");
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.30f, 0.30f, 1.0f), "[★★★ %d Stars]", s.stars);
			}

			ImGui::Separator();

			// Health Bar
			float fraction = s.max_health > 0.f ? std::clamp(s.health / s.max_health, 0.f, 1.f) : 0.f;
			ImVec4 bar_color = fraction > 0.5f ? ImVec4(0.2f, 0.85f, 0.3f, 1.f) : fraction > 0.25f ? ImVec4(0.95f, 0.85f, 0.2f, 1.f) :
			                                                                                         ImVec4(0.95f, 0.25f, 0.2f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, bar_color);
			ImGui::ProgressBar(fraction, ImVec2(-1, 8.f), "");
			ImGui::PopStyleColor();

			ImGui::Text("HP: %.0f / %.0f (%.0f%%)  |  Dist: %.1fm", s.health, s.max_health, fraction * 100.f, s.distance);

			// Tame status
			if (s.is_tamed)
			{
				ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.4f, 1.0f), "● Tamed (Friendly)");
			}
			else if (s.has_tameable && s.tameness > 0)
			{
				int rem_min = static_cast<int>(s.remaining_time) / 60;
				int rem_sec = static_cast<int>(s.remaining_time) % 60;
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "● Taming: %d%% (~%dm %ds)", s.tameness, rem_min, rem_sec);
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "● Wild (Untamed)");
			}

			// Mood & Hunger
			ImGui::TextUnformatted("State: ");
			ImGui::SameLine();
			if (s.is_alerted)
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Scared / Alerted!");
			else
				ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Calm");

			ImGui::SameLine();
			ImGui::TextUnformatted(" • ");
			ImGui::SameLine();
			if (s.is_hungry)
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "Hungry");
			else
				ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Well-fed");

			// Breeding status
			if (s.has_procreation)
			{
				if (s.is_pregnant)
					ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.85f, 1.0f), "♥ Pregnant!");
				else if (s.love_points > 0)
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.85f, 1.0f), "Ready to breed (Love: %d/%d)", s.love_points, s.required_love_points);
			}

			// Hotkey Hint & Menu Buttons
			ImGui::Separator();
			if (g_options.enable_hotkey)
			{
				ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.95f, 1.0f), "Press [%c] to Instant Tame", static_cast<char>(g_options.hotkey));
			}

			if (canvas::is_opened())
			{
				if (ImGui::Button("Instant Tame", ImVec2(120.f, 24.f)))
				{
					tame_aimed_creature();
				}
				ImGui::SameLine();
				if (ImGui::Button("Heal Full", ImVec2(100.f, 24.f)))
				{
					heal_aimed_creature();
				}
			}

			ImGui::PopTextWrapPos();
		}
		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
	}
}
