#include "animal_tools.hpp"
#include "unity/character.hpp"
#include "unity/player.hpp"
#include "unity/localization.hpp"
#include "utility/unity.hpp"
#include "pointers.hpp"
#include "notification/notification_service.hpp"
#include "astra/host/canvas.hpp"

#include <imgui.h>
#include <mutex>
#include <chrono>
#include <format>
#include <algorithm>

namespace big::animal_tools
{
	namespace
	{
		options g_options;
		std::mutex g_mutex;
		creature_info g_snapshot;
		std::chrono::steady_clock::time_point g_last_seen{};
		constexpr auto HYSTERESIS_MS = std::chrono::milliseconds(500);

		MonoClassField* find_field(MonoClass* klass, const char* name)
		{
			for (auto k = klass; k != nullptr; k = mono::class_get_parent(k))
			{
				if (auto f = mono::get_field(k, name))
					return f;
			}
			return nullptr;
		}

		MonoMethod* find_method(MonoClass* klass, const char* name, int param_count = 0)
		{
			for (auto k = klass; k != nullptr; k = mono::class_get_parent(k))
			{
				if (auto m = mono::class_get_method_from_name(k, name, param_count))
					return m;
			}
			return nullptr;
		}

		template<typename T>
		T get_field_val(MonoObject* obj, const char* name)
		{
			T val{};
			if (obj)
			{
				if (auto f = find_field(mono::object_get_class(obj), name))
					mono::get_field_value(obj, f, &val);
			}
			return val;
		}

		MonoObject* call_method(MonoObject* obj, const char* name)
		{
			if (!obj)
				return nullptr;
			auto m = find_method(mono::object_get_class(obj), name, 0);
			return m ? mono::invoke_method(m, obj, nullptr) : nullptr;
		}

		template<typename T>
		T call_value(MonoObject* obj, const char* name)
		{
			auto res = call_method(obj, name);
			return res ? *static_cast<T*>(mono::object_unbox(res)) : T{};
		}

		MonoObject* get_component(MonoObject* obj, const char* class_name)
		{
			if (!obj)
				return nullptr;
			auto klass = mono::get_class(class_name, "assembly_valheim");
			if (!klass)
				return nullptr;
			auto type = mono::reflection_type(klass);
			if (!type)
				return nullptr;

			static auto comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (comp_method)
			{
				void* args[] = {type};
				return mono::invoke_method(comp_method, obj, args);
			}
			return nullptr;
		}

		std::string get_creature_name(MonoObject* creature)
		{
			if (!creature)
				return "Unknown";

			auto m = find_method(mono::object_get_class(creature), "GetHoverName", 0);
			if (m)
			{
				auto res = mono::invoke_method(m, creature, nullptr);
				if (res)
				{
					auto str = mono::from_mono_string(reinterpret_cast<MonoString*>(res));
					if (!str.empty() && str != "unknown")
						return str;
				}
			}

			auto name_field = find_field(mono::object_get_class(creature), "m_name");
			if (name_field)
			{
				MonoString* raw_str = nullptr;
				mono::get_field_value(creature, name_field, &raw_str);
				if (raw_str)
				{
					auto s = mono::from_mono_string(raw_str);
					if (!s.empty())
						return localization::get_instance().localize(s);
				}
			}

			return "Creature";
		}

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

		MonoObject* get_hovering_creature()
		{
			auto player = unity::get_local_player();
			if (!player)
				return nullptr;

			static auto method = mono::get_method("Player", "GetHoverCreature", 0, "assembly_valheim");
			if (method)
			{
				auto res = mono::invoke_method(method, player, nullptr);
				if (res)
					return res;
			}

			static auto field = mono::get_field("Player", "m_hoveringCreature");
			if (field)
			{
				MonoObject* res = nullptr;
				mono::get_field_value(player, field, &res);
				if (res)
					return res;
			}

			return nullptr;
		}
	}

	void set_options(const options& opt)
	{
		g_options = opt;
	}

	options get_options()
	{
		return g_options;
	}

	creature_info get_snapshot()
	{
		std::lock_guard lock(g_mutex);
		return g_snapshot;
	}

	void update()
	{
		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			std::lock_guard lock(g_mutex);
			g_snapshot.valid = false;
			return;
		}

		auto creature = get_hovering_creature();
		auto now = std::chrono::steady_clock::now();

		if (!creature)
		{
			std::lock_guard lock(g_mutex);
			if (g_snapshot.valid && (now - g_last_seen) > HYSTERESIS_MS)
			{
				g_snapshot.valid = false;
			}
			return;
		}

		// Don't inspect the local player or dead entities
		if (creature == local_player || call_value<bool>(creature, "IsPlayer") || call_value<bool>(creature, "IsDead"))
		{
			std::lock_guard lock(g_mutex);
			g_snapshot.valid = false;
			return;
		}

		creature_info info{};
		info.valid = true;
		info.character = creature;
		info.name = get_creature_name(creature);

		int lvl = call_value<int>(creature, "GetLevel");
		info.level = lvl > 0 ? lvl : 1;
		info.stars = std::max(0, info.level - 1);

		info.health = call_value<float>(creature, "GetHealth");
		info.max_health = call_value<float>(creature, "GetMaxHealth");
		info.is_tamed = call_value<bool>(creature, "IsTamed");

		info.position = unity::get_position(creature);
		Vector3 player_pos = unity::get_position(local_player);
		info.distance = player_pos.distance(info.position);

		// Check Tameable
		auto tameable = get_component(creature, "Tameable");
		if (tameable)
		{
			info.has_tameable = true;
			info.is_tamed = info.is_tamed || call_value<bool>(tameable, "IsTamed");
			info.tameness = call_value<int>(tameable, "GetTameness");
			info.remaining_time = call_value<float>(tameable, "GetRemainingTime");
			info.is_hungry = call_value<bool>(tameable, "IsHungry");
		}

		// Check Procreation
		auto procreation = get_component(creature, "Procreation");
		if (procreation)
		{
			info.has_procreation = true;
			info.is_pregnant = call_value<bool>(procreation, "IsPregnant");
			info.love_points = call_value<int>(procreation, "GetLovePoints");
			int req = get_field_val<int>(procreation, "m_requiredLovePoints");
			info.required_love_points = req > 0 ? req : 4;
		}

		// Check AI
		auto ai = call_method(creature, "GetBaseAI");
		if (!ai)
			ai = get_component(creature, "MonsterAI");
		if (!ai)
			ai = get_component(creature, "BaseAI");
		if (ai)
		{
			info.is_alerted = call_value<bool>(ai, "IsAlerted");
		}

		std::lock_guard lock(g_mutex);
		g_snapshot = std::move(info);
		g_last_seen = now;
	}

	bool tame_creature(MonoObject* creature)
	{
		if (!creature || (uintptr_t)creature < 0x10000)
			return false;

		// 1. Claim ownership on ZNetView
		auto nview = get_field_val<MonoObject*>(creature, "m_nview");
		if (!nview)
			nview = get_component(creature, "ZNetView");
		if (nview)
		{
			call_method(nview, "ClaimOwnership");
		}

		// 2. Tameable component handling
		auto tameable = get_component(creature, "Tameable");
		if (tameable)
		{
			auto dec_m = find_method(mono::object_get_class(tameable), "DecreaseRemainingTime", 1);
			if (dec_m)
			{
				float huge_val = 999999.f;
				void* args[] = {&huge_val};
				mono::invoke_method(dec_m, tameable, args);
			}

			auto tame_m = find_method(mono::object_get_class(tameable), "Tame", 0);
			if (tame_m)
			{
				mono::invoke_method(tame_m, tameable, nullptr);
			}
		}

		// 3. AI pacification
		auto ai = call_method(creature, "GetBaseAI");
		if (!ai)
			ai = get_component(creature, "MonsterAI");
		if (!ai)
			ai = get_component(creature, "BaseAI");
		if (ai)
		{
			auto make_tame = find_method(mono::object_get_class(ai), "MakeTame", 0);
			if (make_tame)
				mono::invoke_method(make_tame, ai, nullptr);

			auto set_alert = find_method(mono::object_get_class(ai), "SetAlerted", 1);
			if (set_alert)
			{
				bool alert = false;
				void* args[] = {&alert};
				mono::invoke_method(set_alert, ai, args);
			}

			auto reset_patrol = find_method(mono::object_get_class(ai), "ResetPatrolPoint", 0);
			if (reset_patrol)
				mono::invoke_method(reset_patrol, ai, nullptr);
		}

		// 4. Character SetTamed(true)
		auto set_tamed = find_method(mono::object_get_class(creature), "SetTamed", 1);
		if (set_tamed)
		{
			bool tamed = true;
			void* args[] = {&tamed};
			mono::invoke_method(set_tamed, creature, args);
		}

		// 5. Heal to full if option set
		if (g_options.heal_on_tame)
		{
			heal_creature(creature);
		}

		std::string name = get_creature_name(creature);
		notification::success("Animal Tamer", std::format("Successfully tamed {}!", name));
		show_center_message(std::format("{} tamed!", name));

		return true;
	}

	bool tame_aimed_creature()
	{
		auto snap = get_snapshot();
		if (!snap.valid || !snap.character)
		{
			notification::warning("Animal Tamer", "No animal or creature aimed at!");
			return false;
		}

		return tame_creature(snap.character);
	}

	bool heal_creature(MonoObject* creature)
	{
		if (!creature)
			return false;

		float max_hp = call_value<float>(creature, "GetMaxHealth");
		if (max_hp <= 0.f)
			max_hp = 100.f;

		auto set_hp = find_method(mono::object_get_class(creature), "SetHealth", 1);
		if (set_hp)
		{
			void* args[] = {&max_hp};
			mono::invoke_method(set_hp, creature, args);
			return true;
		}

		return false;
	}

	bool heal_aimed_creature()
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

	int tame_all_in_radius(float radius)
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
				if (tame_creature(obj))
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

	void hotkey_tick()
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

	void draw_overlay()
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
