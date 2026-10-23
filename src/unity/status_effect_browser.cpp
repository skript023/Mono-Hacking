#include "status_effect_browser.hpp"

#include "buff_tools.hpp"
#include "localization.hpp"
#include <astra/host/notification.hpp>
#include "task_mgr.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <astra/host/canvas.hpp>
#include <atomic>
#include <cctype>
#include <chrono>
#include <imgui.h>
#include <mutex>

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

		struct effect_row
		{
			std::string name;
			std::string prefab;
			std::string search;
		};

		std::string lowercase(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return text;
		}

		std::vector<effect_row> to_rows(const std::vector<MonoObject*>& effects)
		{
			std::vector<effect_row> rows;
			for (auto* effect : effects)
			{
				if (!effect)
				{
					continue;
				}
				effect_row row{effect_display_name(effect), unity::get_name(effect)};
				row.search = lowercase(row.prefab + " " + row.name);
				rows.push_back(std::move(row));
			}
			return rows;
		}

		std::mutex s_rows_mutex;
		std::vector<effect_row> s_active_rows;
		std::vector<effect_row> s_registered_rows;
		std::atomic_bool s_refreshing = false;
		std::chrono::steady_clock::time_point s_refreshed{};
		bool s_registered_loaded = false;

		bool refresh_rows(bool registered)
		{
			if (s_refreshing.exchange(true))
			{
				return false;
			}
			async_run([registered] {
				auto active = to_rows(get_active_effects());
				std::vector<effect_row> all;
				if (registered)
				{
					all = to_rows(get_registered_effects());
				}
				{
					std::lock_guard lock(s_rows_mutex);
					s_active_rows = std::move(active);
					if (registered)
					{
						s_registered_rows = std::move(all);
					}
				}
				s_refreshing = false;
			});
			return true;
		}
	}

	void status_effect_browser::toggle_window()
	{
		s_open              = !s_open;
		s_registered_loaded = false;
		s_refreshed         = {};
	}

	void status_effect_browser::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		if (!canvas::begin_panel("Status Effect Browser", &s_open, {.size = {600.f, 520.f}}))
		{
			canvas::end_panel();
			return;
		}
		const auto now = std::chrono::steady_clock::now();
		if (now - s_refreshed >= std::chrono::milliseconds(500) && refresh_rows(!s_registered_loaded))
		{
			s_refreshed         = now;
			s_registered_loaded = true;
		}
		std::vector<effect_row> active_rows;
		std::vector<effect_row> registered_rows;
		{
			std::lock_guard lock(s_rows_mutex);
			active_rows     = s_active_rows;
			registered_rows = s_registered_rows;
		}

		ImGui::TextDisabled("Runtime ObjectDB  /  search, apply, or remove effects");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##EffectSearch", "Search effect name", s_search, sizeof(s_search));
		const std::string query = lowercase(s_search);
		if (ImGui::BeginTabBar("EffectTabs"))
		{
			if (ImGui::BeginTabItem("Active"))
			{
				s_tab = 0;
				ImGui::BeginChild("ActiveEffects", {0.f, -38.f}, true);
				for (const auto& row : active_rows)
				{
					if (!query.empty() && row.search.find(query) == std::string::npos)
					{
						continue;
					}
					ImGui::PushID(row.prefab.c_str());
					ImGui::TextUnformatted(row.name.c_str());
					ImGui::SameLine();
					ImGui::TextDisabled("%s", row.prefab.c_str());
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove"))
					{
						async_run([prefab = row.prefab] {
							buff_tools::remove_status_effect(prefab);
						});
					}
					ImGui::PopID();
				}
				ImGui::EndChild();
				if (ImGui::Button("Remove All Active", {-1.f, 0.f}))
				{
					for (const auto& row : active_rows)
					{
						async_run([prefab = row.prefab] {
							buff_tools::remove_status_effect(prefab);
						});
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Registered"))
			{
				s_tab = 1;
				ImGui::BeginChild("RegisteredEffects", {0.f, -38.f}, true);
				for (const auto& row : registered_rows)
				{
					if (!query.empty() && row.search.find(query) == std::string::npos)
					{
						continue;
					}
					ImGui::PushID(row.prefab.c_str());
					ImGui::TextUnformatted(row.name.c_str());
					ImGui::SameLine();
					ImGui::TextDisabled("%s", row.prefab.c_str());
					ImGui::SameLine();
					if (ImGui::SmallButton("Apply"))
					{
						async_run([prefab = row.prefab] {
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
		canvas::end_panel();
	}
}
