#include "../view.hpp"
#include "unity/cheat_status.hpp"

namespace big
{
	namespace
	{
		float cheat_radius = 30.f;
		int cheat_panel_frame = -1;
		bool fresh(const cheat_status::snapshot& state)
		{
			return state.ready && state.radius == cheat_radius && std::chrono::steady_clock::now() - state.sampled < std::chrono::seconds(3);
		}
		const char* character_status(const cheat_status::snapshot& state)
		{
			if (std::chrono::steady_clock::now() - state.sampled >= std::chrono::seconds(3)) return "Checking...";
			if (!state.character_ready) return "Not available";
			return state.character_marked ? "Marked" : "Clean";
		}
		void status_row(regular_submenu* sub, const std::string& title, const char* description)
		{
			sub->add_option<reguler_option>(title.c_str(), description, nullptr);
		}
	}
	void view::cheat_status_submenu()
	{
		canvas::add_submenu<regular_submenu>("Cheat Status", "CheatStatus"_hash, [](regular_submenu* sub) {
			cheat_panel_frame = ImGui::GetFrameCount();
			cheat_status::request(cheat_status::action::inspect, cheat_radius);
			auto state = cheat_status::get_snapshot();
			sub->add_option<number_option<float>>("Inspection Radius (m)", "Loaded objects only; this is not a whole-world scan.", &cheat_radius, 5.f, 100.f, 5.f, 0);
			status_row(sub, std::format("Character: {}", character_status(state)), "Shows whether your character is marked for using cheats. The mod may have already removed this mark. This is information only.");
			if (fresh(state))
			{
				status_row(sub, std::format("World: {} ({} keys)", state.world_keys.empty() ? "clean" : "MARKED", state.world_keys.size()), "Shows whether your world settings cause the game to mark this world as cheated.");
				status_row(sub, std::format("Cheated items: {} inventory / {} chests", state.inventory_items, state.chest_items), "Each stack counts as one item. Chests that are open or controlled by another player are skipped.");
				status_row(sub, std::format("Nearby marked objects: {} ({} flags)", state.marked_objects, state.marked_flags), "Checks nearby creatures, buildings and production stations, including items being processed.");
			}
			else status_row(sub, "Status: waiting / incomplete", state.error.empty() ? "Waiting for the latest check. Join a world to see its status." : state.error.c_str());
			sub->add_option<reguler_option>("Clean Character & Inventory", "Removes cheat marks from your character and carried items, resets the cheat-use count, then asks the game to save.", [] {
				cheat_status::request(cheat_status::action::character, cheat_radius);
			});
			sub->add_option<reguler_option>("Clean World Keys (Host)", "Removes world settings that count as cheats, including their effects, then asks the game to save. Other settings stay the same.", [] {
				cheat_status::request(cheat_status::action::world, cheat_radius);
			});
			sub->add_option<reguler_option>("Clean Nearby Area (Host)", "Removes cheat marks from nearby objects and chest items, then asks the game to save. Objects controlled by others and open chests are skipped.", [] {
				cheat_status::request(cheat_status::action::nearby, cheat_radius);
			});
		});
	}
	void view::cheat_status_panel()
	{
		if (!canvas::is_opened() || cheat_panel_frame != ImGui::GetFrameCount()) return;
		auto state = cheat_status::get_snapshot();
		if (canvas::begin_panel("Cheat Status Inspector", nullptr, {.size = {440.f, 480.f}, .min_size = {260.f, 180.f}, .collapsible = false}))
		{
			ImGui::PushTextWrapPos(0.f);
			ImGui::TextUnformatted("Current status / updated every second");
			ImGui::TextUnformatted("Shows the current game session. Recent changes may still need saving. The mod may have already cleaned your character and carried items.");
			ImGui::Separator();
			ImGui::Text("Character: %s", character_status(state));
			if (!fresh(state)) ImGui::TextUnformatted(state.error.empty() ? "Waiting for a fresh scan. Join a world to inspect." : state.error.c_str());
			else
			{
				ImGui::Text("Playing as: %s | Radius: %.0fm", state.host ? "Host" : "Client (world/area cleanup unavailable)", state.radius);
				ImGui::Text("Mod protection: %s", state.mod_bypass ? "Active" : "Inactive");
				ImGui::TextUnformatted("Protection skips cheat checks while the mod is active. It does not mean your save is clean.");
				ImGui::Text("Chest inventories inspected: %d | Uninspected: %d", state.chests, state.uninspected_chests);
				ImGui::Text("Marked objects controlled by others: %d", state.skipped_objects);
				ImGui::Text("Marked: %d creatures / %d pieces / %d stations", state.marked_creatures, state.marked_pieces, state.marked_stations);
				ImGui::TextUnformatted("Station counts overlap building pieces; total objects are counted once.");
				ImGui::TextUnformatted("World settings causing the cheat mark:");
				if (state.world_keys.empty()) ImGui::TextUnformatted("None");
				for (const auto& key : state.world_keys) ImGui::BulletText("%s", key.c_str());
			}
			ImGui::Separator();
			if (!state.result.empty()) ImGui::TextUnformatted(state.result.c_str());
			ImGui::TextUnformatted("After cleaning, wait for the game to save successfully before exiting or removing the mod. Areas outside this check and skipped chests may still contain cheat marks.");
			ImGui::TextUnformatted("Cleaning world settings also removes their gameplay effects. Other conditions, including other mods, may still affect achievements.");
			ImGui::PopTextWrapPos();
		}
		canvas::end_panel();
	}
}
