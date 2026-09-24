#include "../view.hpp"
#include "script.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"
#include "unity/online_players.hpp"
#include "fiber_pool.hpp"
#include "notification/notification_service.hpp"
#include "unity/exploration.hpp"
#include "unity/animal_tools.hpp"

namespace big
{
	namespace
	{
		std::string selected_player_id;
		std::optional<online_players::entry> panel_player;
		std::string panel_status;
		int panel_frame = -1;

		void refresh_players()
		{
			g_fiber_pool->queue_job([] {
				online_players::update();
			});
		}

		std::string coordinates(const Vector3& p)
		{
			return std::format("{:.1f}, {:.1f}, {:.1f}", p.x, p.y, p.z);
		}

		void copy_player_text(const std::string& text)
		{
			ImGui::SetClipboardText(text.c_str());
			notification::success("Online Players", "Copied to clipboard.");
		}
	}

	void view::online_player_panel()
	{
		if (!canvas::is_opened() || panel_frame != ImGui::GetFrameCount())
			return;
		auto* viewport = ImGui::GetMainViewport();
		const float width = std::min(360.f, viewport->WorkSize.x);
		ImGui::SetNextWindowPos({viewport->WorkPos.x + viewport->WorkSize.x - width - 16.f,
		                            viewport->WorkPos.y + 32.f},
		    ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({width, std::min(390.f, viewport->WorkSize.y)}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints({std::min(240.f, width), 180.f}, viewport->WorkSize);
		if (ImGui::Begin("Player Info", nullptr, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::PushTextWrapPos(0.f);
			if (!panel_player)
				ImGui::TextUnformatted(panel_status.c_str());
			else
			{
				const auto& p = *panel_player;
				ImGui::TextUnformatted(p.name.c_str());
				ImGui::Separator();
				ImGui::TextUnformatted(p.local ? "You" : "Online player");
				ImGui::Text("Character ID: %s", p.id.empty() ? "Not available" : p.id.c_str());
				ImGui::Text("Platform: %s", p.platform.empty() ? "Not available" : p.platform.c_str());
				if (p.platform == "Steam")
					ImGui::Text("Steam ID: %s", p.account_id.empty() ? "Not available" : p.account_id.c_str());
				else
				{
					ImGui::TextUnformatted("Steam ID: not available for this platform");
					if (!p.account_id.empty())
						ImGui::Text("Platform account ID: %s", p.account_id.c_str());
				}
				ImGui::TextUnformatted(p.loaded ? "Character: loaded nearby" : "Character: outside loaded area / spawning");
				ImGui::Text("Sharing map position: %s", p.public_position ? "Yes" : "No");
				if (p.health && p.max_health)
					ImGui::Text("Health: %.0f / %.0f", *p.health, *p.max_health);
				else
					ImGui::TextUnformatted("Health: not available outside loaded area");
				if (p.position)
				{
					ImGui::Text("Position: %s", coordinates(*p.position).c_str());
					ImGui::TextUnformatted(p.loaded ? "Position source: loaded character" : "Position source: shared map position");
				}
				else
					ImGui::TextUnformatted("Position: not shared and character not loaded");
				if (p.distance)
					ImGui::Text("Distance: %.1f m", *p.distance);
				ImGui::Separator();
				ImGui::TextUnformatted("Updates every second. Shared map positions may lag behind movement.");
			}
			ImGui::PopTextWrapPos();
		}
		ImGui::End();
	}

	void view::player_submenu()
	{
		canvas::add_tab<regular_submenu>("Home", SubmenuHome, [](regular_submenu* sub) {
			sub->add_option<sub_option>("Self", nullptr, "SubmenuSelf"_hash);
			sub->add_option<sub_option>("Inventory", nullptr, "SubmenuInventory"_hash);
			sub->add_option<sub_option>("World", nullptr, "SubmenuWorld"_hash);
			sub->add_option<sub_option>("Base Tools", "Storage, production, repair, farming and environment.", "BaseTools"_hash);
			sub->add_option<sub_option>("Online Players", nullptr, SubmenuPlayerList);
			sub->add_option<sub_option>("ESP", nullptr, "SubmenuESP"_hash);
			sub->add_option<sub_option>("Aimbot", nullptr, "SubmenuAimbot"_hash);
		});

		canvas::add_submenu<regular_submenu>("Self", "SubmenuSelf"_hash, [](regular_submenu* sub) {
			sub->add_option<sub_option>("Combat & Survival", "God mode, damage multiplier, instant bow, stamina, health.", "SubmenuCombat"_hash);
			sub->add_option<sub_option>("Movement & Flight", "Fly mode, speed multiplier, wind direction, teleportation.", "SubmenuMovement"_hash);
			sub->add_option<sub_option>("Building & Crafting", "Free crafting & building, infinite durability, stability, ward bypass.", "SubmenuBuilding"_hash);
			sub->add_option<sub_option>("Farming & World QoL", "Loot vacuum magnet, 1-hit tree/mine breaker, animal alerts.", "SubmenuFarmingQoL"_hash);
		});

		canvas::add_submenu<regular_submenu>("Combat & Survival", "SubmenuCombat"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("God Mode", "Invulnerable to all incoming damage, fall, hazards, and debuffs.", &g_settings.self.god_mode);
			sub->add_option<bool_option<bool>>("Ghost Mode", "Enemies and bosses cannot detect, target, or hear you.", &g_settings.self.ghost_mode);
			sub->add_option<number_option<float>>("Damage Multiplier", "Multiplier for player attacks (set high for 1-hit kill).", &g_settings.self.damage_multiplier, 1.f, 50.f, 1.f, 1);
			sub->add_option<bool_option<bool>>("Instant Bow & Crossbow", "Instant full bow draw and instant crossbow reload.", &g_settings.self.instant_bow_draw);
			sub->add_option<number_option<int>>("Max Food Slots", "Allow eating more than 3 foods simultaneously (HP, Stamina, Eitr buffs stack!).", &g_settings.self.max_food_slots, 3, 10, 1, 0);
			sub->add_option<bool_option<bool>>("Don't Drop Items when Dead", nullptr, &g_settings.self.no_drop_on_dead);
			sub->add_option<bool_option<bool>>("Never Wet", nullptr, &g_settings.self.is_wet);
			sub->add_option<bool_option<bool>>("Forsaken Power Always Ready", nullptr, &g_settings.self.forsaken_power_always_ready);
			sub->add_option<bool_option<bool>>("infinite_stamina"_hash);
			sub->add_option<bool_option<bool>>("allowed_command"_hash);
			sub->add_option<bool_slider_float_option>("max_health"_hash, "max_hp"_hash, 10.f);
			sub->add_option<bool_slider_float_option>("max_stamina"_hash, "max_stam"_hash, 10.f);
			sub->add_option<bool_slider_float_option>("stamina_regen"_hash, "stamina_regen_amount"_hash, 5.f);
			sub->add_option<bool_slider_float_option>("eitr"_hash, "eitr_amount"_hash, 5.f);
			sub->add_option<bool_slider_float_option>("eitr_regen"_hash, "eitr_regen_amount"_hash, 5.f);
			sub->add_option<bool_slider_float_option>("max_food_health"_hash, "food_hp"_hash, 1000.f);
			sub->add_option<bool_slider_float_option>("max_body_armor"_hash, "num_body_armor"_hash, 10.f);
			sub->add_option<bool_slider_float_option>("enable_raise_skill"_hash, "raise_skill"_hash, 1.f);
		});

		canvas::add_submenu<regular_submenu>("Movement & Flight", "SubmenuMovement"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Flying", "Toggle creative free flight mode.", &g_settings.self.flying);
			sub->add_option<number_option<float>>("Speed Multiplier", "Sprint & jog speed multiplier (no armor penalty).", &g_settings.self.speed_multiplier, 1.f, 10.f, 0.5f, 1);
			sub->add_option<bool_option<bool>>("Always Have Wind Facing Boat", "Wind always blows from behind your boat.", &g_settings.self.always_wind);
			sub->add_option<bool_option<bool>>("Allow Teleporting with Any Items", "Teleport through portals with ores and metals.", &g_settings.self.is_teleportable);
		});

		canvas::add_submenu<regular_submenu>("Building & Crafting", "SubmenuBuilding"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Free Crafting & Building", "Craft anywhere with 0 cost, no station/roof/fire, free placement.", &g_settings.self.free_crafting);
			sub->add_option<bool_option<bool>>("Infinite Durability", "Weapons, tools, shields, and armor never degrade.", &g_settings.self.infinite_durability);
			sub->add_option<bool_option<bool>>("Infinite Building Stability", "Pieces never collapse from lack of support.", &g_settings.self.infinite_stability);
			sub->add_option<bool_option<bool>>("Ward / Guard Stone Bypass", "Bypass all player wards and guard stones.", &g_settings.self.ward_bypass);
			sub->add_option<bool_option<bool>>("open_all_recepies"_hash);
		});

		canvas::add_submenu<regular_submenu>("Farming & World QoL", "SubmenuFarmingQoL"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("One-Hit Tree & Mine Breaker", "Instantly fell trees and shatter rocks / ore veins in one hit.", &g_settings.self.one_hit_resource);
			sub->add_option<number_option<float>>("Vacuum Pickup Range", "Item magnet pickup radius in meters (default: 2m).", &g_settings.self.pickup_range, 2.f, 50.f, 2.f, 1);
			sub->add_option<bool_option<bool>>("No Alert Animal", "Animals never become alarmed or flee.", &g_settings.self.no_animal_alert);
			sub->add_option<bool_option<bool>>("Allow Picking Up Fish in Water", nullptr, &g_settings.self.allow_pickup_fish);
			sub->add_option<bool_option<bool>>("No Weight", nullptr, &g_settings.self.no_weight);
			sub->add_option<bool_slider_float_option>("max_carry_weight"_hash, "carry_amount"_hash, 100.f);
			sub->add_option<bool_option<bool>>("Fill Inventory Top First", "Items added to inventory will fill slots from the top down instead of bottom.", &g_settings.self.inventory_top_first);
			sub->add_option<bool_slider_int_option>("inventory_size"_hash, "inventory_height"_hash, 1);
		});

		canvas::add_submenu<regular_submenu>("Inventory", "SubmenuInventory"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Fill Inventory Top First", "Items added to inventory will fill slots from the top down instead of bottom.", &g_settings.self.inventory_top_first);
			sub->add_option<bool_slider_int_option>("override_drop"_hash, "drop_amount"_hash);
			sub->add_option<bool_option<bool>>("override_selected"_hash);
			if (features::_override_selected.get_state())
			{
				sub->add_option<number_option<float>>("durability"_hash);
				sub->add_option<number_option<int>>("quality"_hash);
				sub->add_option<number_option<int>>("stack"_hash);
				sub->add_option<number_option<int>>("variant"_hash);
			}
		});


		canvas::add_submenu<regular_submenu>("World", "SubmenuWorld"_hash, [](regular_submenu* sub) {
			sub->add_option<sub_option>("Exploration & Map", "Reveal map fog, locate all bosses and traders.", "SubmenuExploration"_hash);
			sub->add_option<sub_option>("Creatures & Taming", "Aimed animal inspection, instant tame hotkey, and area taming.", "SubmenuCreatures"_hash);
		});

		canvas::add_submenu<regular_submenu>("Exploration & Map", "SubmenuExploration"_hash, [](regular_submenu* sub) {
			static exploration::options exp_cfg = exploration::get_options();
			sub->add_option<bool_option<bool>>("Pin All World Altars", "If enabled, discovers all altars in the world instead of only the closest one.", &exp_cfg.discover_all);
			sub->add_option<sub_option>("Boss Altars", "Individual progression boss altar pins.", "SubmenuBosses"_hash);
			sub->add_option<sub_option>("Traders & POIs", "Merchants and special quest locations.", "SubmenuTraders"_hash);
			sub->add_option<reguler_option>("Reveal All 7 Bosses", "Pin altars for all 7 bosses across the world.", [] {
				exploration::discover_all_bosses(exp_cfg.discover_all);
			});
			sub->add_option<reguler_option>("Reveal All Traders & Quests", "Pin Haldor, Hildir, Bog Witch, and dungeons.", [] {
				exploration::discover_all_traders(exp_cfg.discover_all);
			});
			sub->add_option<reguler_option>("Reveal Everything", "Pin all bosses, traders, and special locations.", [] {
				exploration::discover_everything(exp_cfg.discover_all);
			});
			sub->add_option<reguler_option>("Explore Entire Map", "Reveal all fog of war on the minimap.", [] {
				exploration::explore_all_map();
			});
			sub->add_option<reguler_option>("Reset Map Fog", "Reset map exploration fog.", [] {
				exploration::reset_map();
			});
		});

		canvas::add_submenu<regular_submenu>("Boss Altars", "SubmenuBosses"_hash, [](regular_submenu* sub) {
			static exploration::options exp_cfg = exploration::get_options();
			sub->add_option<bool_option<bool>>("Pin All World Altars", "Discover all instances across the world map.", &exp_cfg.discover_all);
			sub->add_option<reguler_option>("Reveal All 7 Bosses", "Pin all boss altars simultaneously.", [] {
				exploration::discover_all_bosses(exp_cfg.discover_all);
			});
			for (const auto& boss : exploration::get_boss_entries())
			{
				std::string title = std::format("Pin {} ({})", boss.display_name, boss.biome);
				sub->add_option<reguler_option>(title.c_str(), "Send discovery request for this boss.", [boss] {
					exploration::discover_location(boss.location_name, boss.pin_name, boss.pin_type, exp_cfg.discover_all);
					notification::success("Boss Tracker", std::format("Pin requested for {}!", boss.display_name));
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Traders & POIs", "SubmenuTraders"_hash, [](regular_submenu* sub) {
			static exploration::options exp_cfg = exploration::get_options();
			sub->add_option<reguler_option>("Reveal All Traders", "Pin all merchants and quest locations.", [] {
				exploration::discover_all_traders(exp_cfg.discover_all);
			});
			for (const auto& trader : exploration::get_trader_entries())
			{
				std::string title = std::format("Pin {} ({})", trader.display_name, trader.biome);
				sub->add_option<reguler_option>(title.c_str(), "Send discovery request for this trader/POI.", [trader] {
					exploration::discover_location(trader.location_name, trader.pin_name, trader.pin_type, exp_cfg.discover_all);
					notification::success("Trader Tracker", std::format("Pin requested for {}!", trader.display_name));
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Creatures & Taming", "SubmenuCreatures"_hash, [](regular_submenu* sub) {
			static animal_tools::options anim_cfg = animal_tools::get_options();
			animal_tools::set_options(anim_cfg);

			sub->add_option<bool_option<bool>>("Show Animal Inspector HUD", "Display creature stats, health, stars, and taming info when aiming.", &anim_cfg.show_inspector);
			sub->add_option<bool_option<bool>>("Enable Tame Hotkey [T]", "Press T to instantly tame whatever animal you are looking at.", &anim_cfg.enable_hotkey);
			sub->add_option<bool_option<bool>>("Heal on Tame", "Restore animal to full health upon taming.", &anim_cfg.heal_on_tame);
			sub->add_option<reguler_option>("Tame Aimed Creature", "Instantly tame the animal currently in your crosshairs.", [] {
				animal_tools::tame_aimed_creature();
			});
			sub->add_option<reguler_option>("Heal Aimed Creature", "Restore targeted creature to 100% health.", [] {
				animal_tools::heal_aimed_creature();
			});
			sub->add_option<number_option<float>>("Area Tame Radius (m)", "Radius in meters for mass taming nearby creatures.", &anim_cfg.area_tame_radius, 10.f, 100.f, 5.f, 0);
			sub->add_option<reguler_option>("Tame All in Radius", "Tame all wild animals within the chosen radius.", [] {
				animal_tools::tame_all_in_radius(anim_cfg.area_tame_radius);
			});
			sub->add_option<reguler_option>("Tame All Deer (World)", "Tame all deer loaded in the world.", [] {
				commands::get_command<command>("tamed_all_deer"_hash)->call();
			});
			sub->add_option<reguler_option>("Tame All Boar (World)", "Tame all boar loaded in the world.", [] {
				commands::get_command<command>("tamed_all_boar"_hash)->call();
			});
			sub->add_option<reguler_option>("Tame All Wolves (World)", "Tame all wolves loaded in the world.", [] {
				commands::get_command<command>("tamed_all_wolf"_hash)->call();
			});
		});

		canvas::add_submenu<regular_submenu>("Online Players", SubmenuPlayerList, [](regular_submenu* sub) {
			const auto roster = online_players::get_snapshot();
			sub->set_name(std::format("Online Players ({})", roster.players.size()).c_str());
			panel_frame = ImGui::GetFrameCount();
			panel_player.reset();
			panel_status = roster.ready ? "Select a player to see their details." : "Waiting for the session player list. Join a world to load players.";
			const auto highlighted = sub->get_selected_option();
			for (std::size_t i = 0; i < roster.players.size(); ++i)
			{
				const auto& p = roster.players[i];
				if (i == highlighted)
					panel_player = p;
				const auto label = p.name + (p.local ? " (You)" : "");
				sub->add_option<sub_option>(label.c_str(), "Player details and location actions.", SubmenuSelectedPlayer, [id = p.id] {
					selected_player_id = id;
				});
			}
			sub->add_option<reguler_option>("Refresh Player List", "Read the current session roster again.", refresh_players);
		});

		canvas::add_submenu<regular_submenu>("Online Players", SubmenuSelectedPlayer, [](regular_submenu* sub) {
			panel_frame = ImGui::GetFrameCount();
			panel_player.reset();
			const auto roster = online_players::get_snapshot();
			const auto found = std::find_if(roster.players.begin(), roster.players.end(), [](const auto& p) {
				return !selected_player_id.empty() && selected_player_id != "0:0" && p.id == selected_player_id;
			});
			if (found == roster.players.end())
			{
				sub->set_name("Player Unavailable");
				panel_status = "Player left, is respawning, or session data is unavailable. Go back and select a player again.";
				sub->add_option<reguler_option>("Refresh Player List", "Check whether session data is available.", refresh_players);
				return;
			}
			const auto p = *found;
			panel_player = p;
			sub->set_name(p.name.c_str());
			if (!p.local && p.position)
				sub->add_option<reguler_option>("Teleport to Player", "Move near the latest available player position.", [id = p.id] {
					g_fiber_pool->queue_job([id] {
						if (online_players::teleport_to(id))
							notification::info("Online Players", "Teleport requested.");
						else
							notification::warning("Online Players", "Player or position is no longer available.");
					});
				});
			sub->add_option<reguler_option>("Copy Player Name", "Copy this player's name to the clipboard.", [name = p.name] {
				copy_player_text(name);
			});
			sub->add_option<reguler_option>("Copy Character ID", "Copy this character's network ID.", [id = p.id] {
				copy_player_text(id);
			});
			if (!p.account_id.empty())
				sub->add_option<reguler_option>(p.platform == "Steam" ? "Copy Steam ID" : "Copy Platform Account ID",
				    "Copy the account ID supplied by the session roster.",
				    [id = p.account_id] {
					    copy_player_text(id);
				    });
			if (p.position)
				sub->add_option<reguler_option>("Copy Coordinates", "Copy the displayed X, Y, Z coordinates.", [position = *p.position] {
					copy_player_text(coordinates(position));
				});
			sub->add_option<reguler_option>("Refresh Player List", "Update player details from the session roster.", refresh_players);
		});

		canvas::add_submenu<regular_submenu>("ESP", "SubmenuESP"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("esp_activate"_hash);
			sub->add_option<bool_option<bool>>("draw_anim"_hash);
			sub->add_option<bool_option<bool>>("draw_line"_hash);
			sub->add_option<bool_option<bool>>("draw_name"_hash);
			sub->add_option<bool_option<bool>>("draw_health"_hash);
			sub->add_option<bool_option<bool>>("draw_box"_hash);
			sub->add_option<bool_option<bool>>("draw_fov"_hash);
		});

		canvas::add_submenu<regular_submenu>("Aimbot", "SubmenuAimbot"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("aimbot"_hash);
			sub->add_option<bool_option<bool>>("tp"_hash);
			sub->add_option<bool_option<bool>>("draw_fov"_hash);
			sub->add_option<number_option<float>>("aimbot_fov"_hash);
			sub->add_option<number_option<float>>("aimbot_smooth"_hash);
			sub->add_option<number_option<int>>("aimbot_trigger"_hash);
			sub->add_option<bool_option<bool>>("triggerbot"_hash);
			sub->add_option<number_option<float>>("trigger_fov"_hash);
		});
	}
}
