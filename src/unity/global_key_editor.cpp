#include "global_key_editor.hpp"

#include "notification/notification_service.hpp"
#include "task_mgr.hpp"
#include "zone_system.hpp"

#include <algorithm>
#include <cctype>
#include <imgui.h>

namespace big
{
	namespace
	{
		bool s_open = false;
		char s_search[96]{};
		char s_new_key[128]{};
	}

	void global_key_editor::toggle_window()
	{
		s_open = !s_open;
	}

	void global_key_editor::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		ImGui::SetNextWindowSize({540.f, 480.f}, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Global Key Editor", &s_open))
		{
			ImGui::End();
			return;
		}

		auto system = zone_system::get_instance();
		if (!system)
		{
			ImGui::TextDisabled("Join a world to inspect its global keys.");
			ImGui::End();
			return;
		}
		ImGui::TextDisabled("World flags  /  Server synchronized");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##GlobalKeySearch", "Filter active keys", s_search, sizeof(s_search));
		ImGui::BeginChild("ActiveGlobalKeys", {0.f, -140.f}, true);
		auto keys         = system.get_global_keys();
		std::string query = s_search;
		std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		for (std::size_t i = 0; i < keys.size(); ++i)
		{
			std::string lower = keys[i];
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (!query.empty() && lower.find(query) == std::string::npos)
			{
				continue;
			}
			ImGui::PushID(static_cast<int>(i));
			ImGui::TextUnformatted(keys[i].c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
			{
				const auto key = keys[i];
				async_run([key] {
					if (zone_system::get_instance().remove_global_key(key))
					{
						notification::success("Global Key Editor", "Removal requested from the world host.");
					}
					else
					{
						notification::warning("Global Key Editor", "Could not send the removal request to the world host.");
					}
				});
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		ImGui::Text("Add key or key value, for example: defeated_eikthyr or worldlevel 2");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##NewGlobalKey", "global_key [value]", s_new_key, sizeof(s_new_key));
		if (ImGui::Button("Set Global Key", {-1.f, 30.f}) && s_new_key[0])
		{
			std::string key = s_new_key;
			async_run([key] {
				if (zone_system::get_instance().set_global_key(key))
				{
					notification::success("Global Key Editor", "Key request sent to the world host.");
				}
				else
				{
					notification::warning("Global Key Editor", "Could not send the key request to the world host.");
				}
			});
			s_new_key[0] = '\0';
		}
		ImGui::End();
	}
}
