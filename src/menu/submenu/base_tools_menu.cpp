#include "../view.hpp"
#include "unity/base_tools.hpp"
#include "fiber_pool.hpp"

namespace big
{
	namespace
	{
		base_tools::options config;
		int panel_frame = -1;
		int panel_section = 0;
		void show_panel(int section)
		{
			panel_frame = ImGui::GetFrameCount();
			panel_section = section;
			base_tools::request_scan();
		}
		void queue(void (*action)())
		{
			g_fiber_pool->queue_job(action);
		}
		void refresh(regular_submenu* sub)
		{
			sub->add_option<reguler_option>("Refresh Nearby Objects", "Scan loaded objects within the selected radius.", [] {
				base_tools::request_scan();
			});
		}
	}

	void view::base_tools_submenu()
	{
		canvas::add_submenu<regular_submenu>("Base Tools", "BaseTools"_hash, [](regular_submenu* sub) {
			sub->add_option<number_option<float>>("Action Radius (m)", "Loaded objects only; comfort uses the game's 10m radius.", &config.radius, 5.f, 100.f, 5.f, 0);
			sub->add_option<sub_option>("Quick Stack", "Deposit matching items in nearby accessible chests.", "BaseStack"_hash);
			sub->add_option<sub_option>("Production Monitor", "Smelter queues, fuel, fermentation and honey.", "BaseProduction"_hash);
			sub->add_option<sub_option>("Repair Radius", "Repair nearby buildings and locate damage.", "BaseRepair"_hash);
			sub->add_option<sub_option>("Farming Assistant", "Plant health, growth time and instant growth.", "BaseFarming"_hash);
			sub->add_option<sub_option>("Production Speed", "Local ownership required. Set 1x to restore normal speed.", "BaseSpeed"_hash);
			sub->add_option<sub_option>("Weather & Daylight", "Local environment overrides; does not advance world time.", "BaseEnvironment"_hash);
			sub->add_option<sub_option>("Comfort Inspector", "Nearby furniture and current comfort level.", "BaseComfort"_hash);
		});
		canvas::add_submenu<regular_submenu>("Quick Stack", "BaseStack"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Keep Food & Potions", "Exclude consumables from quick stack.", &config.keep_food);
			sub->add_option<bool_option<bool>>("Keep Ammo", "Keep arrows, bolts and other ammunition.", &config.keep_ammo);
			sub->add_option<bool_option<bool>>("Keep Hotbar", "Exclude all items in the first inventory row.", &config.keep_hotbar);
			sub->add_option<bool_option<bool>>("Enable F7 Hotkey", "Press F7 while the game is focused to quick stack.", &config.hotkey);
			sub->add_option<reguler_option>("Stack into Nearby Chests", "Only matching item types; equipped items remain with you.", [] {
				base_tools::set_options(config);
				queue(base_tools::quick_stack);
			});
		});
		canvas::add_submenu<regular_submenu>("Production Monitor", "BaseProduction"_hash, [](regular_submenu* sub) {
			show_panel(0);
			refresh(sub);
		});
		canvas::add_submenu<regular_submenu>("Repair Radius", "BaseRepair"_hash, [](regular_submenu* sub) {
			show_panel(1);
			sub->add_option<bool_option<bool>>("Mark Damaged Buildings", "Draw health labels on nearby damaged buildings.", &config.damaged_markers);
			sub->add_option<reguler_option>("Repair Nearby Buildings", "Send repair requests for accessible damaged pieces.", [] {
				queue(base_tools::repair_nearby);
			});
			refresh(sub);
		});
		canvas::add_submenu<regular_submenu>("Farming Assistant", "BaseFarming"_hash, [](regular_submenu* sub) {
			show_panel(2);
			sub->add_option<number_option<float>>("Growth Speed", "Shortens total growth time; existing plants can mature now.", &config.plant_speed, 1.f, 20.f, 1.f, 0);
			sub->add_option<reguler_option>("Grow Healthy Plants Now", "Only healthy, accessible plants owned by this client.", [] {
				queue(base_tools::grow_nearby);
			});
			refresh(sub);
		});
		canvas::add_submenu<regular_submenu>("Production Speed", "BaseSpeed"_hash, [](regular_submenu* sub) {
			sub->add_option<number_option<float>>("Smelter Speed", "Speeds processing and fuel consumption proportionally.", &config.smelter_speed, 1.f, 20.f, 1.f, 0);
			sub->add_option<number_option<float>>("Fermenter Speed", "Scales elapsed fermentation time, including existing batches.", &config.fermenter_speed, 1.f, 20.f, 1.f, 0);
			sub->add_option<number_option<float>>("Honey Production Speed", "Speeds honey production on locally owned hives.", &config.honey_speed, 1.f, 20.f, 1.f, 0);
			sub->add_option<reguler_option>("Reset All Speeds to 1x", "Restore normal production and plant growth speeds.", [] {
				config.smelter_speed = config.fermenter_speed = config.honey_speed = config.plant_speed = 1.f;
			});
		});
		canvas::add_submenu<regular_submenu>("Weather & Daylight", "BaseEnvironment"_hash, [](regular_submenu* sub) {
			base_tools::request_scan();
			sub->add_option<bool_option<bool>>("Lock Daylight", "Override local day/night lighting without skipping world time.", &config.lock_daylight);
			sub->add_option<number_option<float>>("Day Fraction", "0 = midnight, 0.25 = morning, 0.5 = noon, 0.75 = evening.", &config.daylight, 0.f, 1.f, .05f, 2);
			sub->add_option<sub_option>("Select Weather", "Choose from environments defined by the current game.", "BaseWeather"_hash);
			sub->add_option<reguler_option>("Restore Automatic Environment", "Clear weather and daylight overrides.", [] {
				config.weather.clear();
				config.lock_daylight = false;
			});
		});
		canvas::add_submenu<regular_submenu>("Select Weather", "BaseWeather"_hash, [](regular_submenu* sub) {
			base_tools::request_scan();
			sub->add_option<reguler_option>("Automatic Weather", "Use the game's normal environment selection.", [] {
				config.weather.clear();
			});
			for (const auto& weather : base_tools::get_snapshot().weather)
				sub->add_option<reguler_option>((weather + (config.weather == weather ? " (Selected)" : "")).c_str(), "Apply this environment locally.", [weather] {
					config.weather = weather;
				});
			refresh(sub);
		});
		canvas::add_submenu<regular_submenu>("Comfort Inspector", "BaseComfort"_hash, [](regular_submenu* sub) {
			show_panel(3);
			refresh(sub);
		});
	}

	void view::base_tools_panel()
	{
		base_tools::set_options(config);
		if (config.damaged_markers)
			for (const auto& marker : base_tools::get_markers())
			{
				const ImVec2 point{marker.screen.x, marker.screen.y};
				auto* draw = ImGui::GetBackgroundDrawList();
				const auto size = ImGui::CalcTextSize(marker.text.c_str());
				draw->AddRectFilled({point.x - 3.f, point.y - 2.f}, {point.x + size.x + 3.f, point.y + size.y + 2.f}, IM_COL32(20, 20, 20, 230), 2.f);
				draw->AddText(point, IM_COL32(255, 220, 150, 255), marker.text.c_str());
			}
		if (!canvas::is_opened() || panel_frame != ImGui::GetFrameCount())
			return;
		auto state = base_tools::get_snapshot();
		auto* viewport = ImGui::GetMainViewport();
		const float width = std::min(440.f, viewport->WorkSize.x);
		ImGui::SetNextWindowPos({viewport->WorkPos.x + viewport->WorkSize.x - width - 16.f, viewport->WorkPos.y + 32.f}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({width, std::min(520.f, viewport->WorkSize.y)}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints({std::min(260.f, width), 180.f}, viewport->WorkSize);
		if (ImGui::Begin("Base Inspector", nullptr, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::PushTextWrapPos(0.f);
			const char* titles[] = {"Production", "Damaged Buildings", "Plants", "Comfort"};
			ImGui::TextUnformatted(titles[panel_section]);
			ImGui::Separator();
			if (!state.ready)
				ImGui::TextUnformatted("Waiting for nearby objects. Join a world to scan your base.");
			else
			{
				if (panel_section == 3)
				{
					ImGui::Text("Current comfort: %d | Sheltered: %s", state.comfort_level, state.sheltered ? "Yes" : "No");
					ImGui::TextUnformatted("Within the game's 10m comfort radius. Furniture from the same group may not stack; shelter is required.");
				}
				const auto& rows = panel_section == 0 ? state.production : panel_section == 1 ? state.buildings :
				    panel_section == 2                                                        ? state.plants :
				                                                                                state.comfort;
				ImGui::Text("%zu objects | Updates every second", rows.size());
				if (rows.empty())
					ImGui::TextUnformatted("No matching loaded objects in range.");
				for (const auto& row : rows)
				{
					ImGui::Separator();
					ImGui::TextUnformatted(row.name.c_str());
					ImGui::TextUnformatted(row.detail.c_str());
					ImGui::Text("Position: %.1f, %.1f, %.1f", row.position.x, row.position.y, row.position.z);
				}
			}
			ImGui::PopTextWrapPos();
		}
		ImGui::End();
	}
}
