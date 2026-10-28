#include "global_key_editor.hpp"

#include <astra/host/notification.hpp>
#include "task_mgr.hpp"
#include "zone_system.hpp"

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
		char s_search[96]{};
		char s_new_key[128]{};

		std::mutex s_keys_mutex;
		std::vector<std::string> s_keys;
		bool s_world_ready            = false;
		std::atomic_bool s_refreshing = false;
		std::chrono::steady_clock::time_point s_refreshed{};

		void refresh_keys()
		{
			if (s_refreshing.exchange(true))
			{
				return;
			}
			async_run([] {
				auto system = zone_system::get_instance();
				auto keys   = system ? system.get_global_keys() : std::vector<std::string>{};
				{
					std::lock_guard lock(s_keys_mutex);
					s_world_ready = static_cast<bool>(system);
					s_keys        = std::move(keys);
				}
				s_refreshing = false;
			});
		}
	}

	void global_key_editor::toggle_window()
	{
		s_open      = !s_open;
		s_refreshed = {};
	}

	void global_key_editor::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		if (!canvas::begin_panel("Global Key Editor", &s_open, {.size = {540.f, 480.f}}))
		{
			canvas::end_panel();
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		if (now - s_refreshed >= std::chrono::seconds(1))
		{
			s_refreshed = now;
			refresh_keys();
		}
		bool world_ready = false;
		std::vector<std::string> keys;
		{
			std::lock_guard lock(s_keys_mutex);
			world_ready = s_world_ready;
			keys        = s_keys;
		}
		if (!world_ready)
		{
			ImGui::TextDisabled("Join a world to inspect its global keys.");
			canvas::end_panel();
			return;
		}
		ImGui::TextDisabled("World flags  /  Server synchronized");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##GlobalKeySearch", "Filter active keys", s_search, sizeof(s_search));
		ImGui::BeginChild("ActiveGlobalKeys", {0.f, -140.f}, true);
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
		if (ImGui::Button("Set Global Key", {-1.f, 0.f}) && s_new_key[0])
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
		canvas::end_panel();
	}
}
