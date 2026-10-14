#include "status_effect_browser.hpp"

#include "buff_tools.hpp"
#include "localization.hpp"
#include "notification/notification_service.hpp"
#include "task_mgr.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <cctype>
#include <imgui.h>

namespace big
{
	namespace
	{
		bool s_open = false;
		int s_tab   = 0;
		char s_search[96]{};

		std::string effect_display_name(MonoObject* effect)
		{
			auto raw = mono::get_field_value<"StatusEffect", "m_name", MonoString*>(effect);
			if (!raw)
			{
				return unity::get_name(effect);
			}
			return localization::get_instance().localize(mono::from_mono_string(raw));
		}

		std::vector<MonoObject*> get_registered_effects()
		{
			auto db            = unity::get_object_db();
			auto db_class      = db ? mono::object_get_class(db) : nullptr;
			auto effects_field = db_class ? mono::get_field(db_class, "m_StatusEffects") : nullptr;
			MonoObject* list   = nullptr;
			if (db && effects_field)
			{
				mono::get_field_value(db, effects_field, &list);
			}
			return unity::list_to_vector(list);
		}

		std::vector<MonoObject*> get_active_effects()
		{
			auto player             = unity::get_local_player();
			static auto get_seman   = mono::get_method("Character", "GetSEMan", 0, "assembly_valheim");
			auto seman              = player && get_seman ? mono::invoke_method(get_seman, player, nullptr) : nullptr;
			static auto get_effects = mono::get_method("SEMan", "GetStatusEffects", 0, "assembly_valheim");
			auto list               = seman && get_effects ? mono::invoke_method(get_effects, seman, nullptr) : nullptr;
			return unity::list_to_vector(list);
		}

		bool matches_search(MonoObject* effect, const std::string& query)
		{
			if (query.empty())
			{
				return true;
			}
			std::string text = unity::get_name(effect) + " " + effect_display_name(effect);
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return text.find(query) != std::string::npos;
		}
	}

	void status_effect_browser::toggle_window()
	{
		s_open = !s_open;
	}

	void status_effect_browser::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		ImGui::SetNextWindowSize({600.f, 520.f}, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Status Effect Browser", &s_open))
		{
			ImGui::End();
			return;
		}
		ImGui::TextDisabled("Runtime ObjectDB  /  search, apply, or remove effects");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##EffectSearch", "Search effect name", s_search, sizeof(s_search));
		if (ImGui::BeginTabBar("EffectTabs"))
		{
			if (ImGui::BeginTabItem("Active"))
			{
				s_tab = 0;
				ImGui::BeginChild("ActiveEffects", {0.f, -38.f}, true);
				auto effects      = get_active_effects();
				std::string query = s_search;
				std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
				for (auto* effect : effects)
				{
					if (!effect || !matches_search(effect, query))
					{
						continue;
					}
					auto prefab = unity::get_name(effect);
					ImGui::PushID(effect);
					ImGui::TextUnformatted(effect_display_name(effect).c_str());
					ImGui::SameLine();
					ImGui::TextDisabled("%s", prefab.c_str());
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove"))
					{
						async_run([prefab] {
							buff_tools::remove_status_effect(prefab);
						});
					}
					ImGui::PopID();
				}
				ImGui::EndChild();
				if (ImGui::Button("Remove All Active", {-1.f, 30.f}))
				{
					for (auto* effect : effects)
					{
						if (effect)
						{
							auto prefab = unity::get_name(effect);
							async_run([prefab] {
								buff_tools::remove_status_effect(prefab);
							});
						}
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Registered"))
			{
				s_tab = 1;
				ImGui::BeginChild("RegisteredEffects", {0.f, -38.f}, true);
				auto effects      = get_registered_effects();
				std::string query = s_search;
				std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
				for (auto* effect : effects)
				{
					if (!effect || !matches_search(effect, query))
					{
						continue;
					}
					auto prefab = unity::get_name(effect);
					ImGui::PushID(effect);
					ImGui::TextUnformatted(effect_display_name(effect).c_str());
					ImGui::SameLine();
					ImGui::TextDisabled("%s", prefab.c_str());
					ImGui::SameLine();
					if (ImGui::SmallButton("Apply"))
					{
						async_run([prefab] {
							if (!buff_tools::apply_registered_effect(prefab))
							{
								notification::warning("Status Effects", std::format("Could not apply {}.", prefab));
							}
						});
					}
					ImGui::PopID();
				}
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}
}
