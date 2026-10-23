#include "../view.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"
#include "unity/online_players.hpp"
#include "task_mgr.hpp"
#include <astra/host/notification.hpp>
#include "unity/exploration.hpp"
#include "unity/animal_tools.hpp"
#include "unity/camera_tools.hpp"
#include "unity/hud.hpp"
#include "unity/item_spawner.hpp"
#include "unity/ship_tools.hpp"
#include "unity/buff_tools.hpp"
#include "unity/fishing_tools.hpp"
#include "unity/world_tools.hpp"
#include "unity/skills.hpp"
#include "unity/equipment_slots.hpp"
#include "unity/inventory_editor.hpp"
#include "unity/creature_spawner.hpp"
#include "unity/prefab_spawner.hpp"
#include "unity/global_key_editor.hpp"
#include "unity/status_effect_browser.hpp"
#include "unity/world_time.hpp"
#include "unity/minimap.hpp"
#include "unity/bookmark_manager.hpp"
#include "unity/crafting_tools.hpp"
#include "unity/zone_system.hpp"
#include "menu/esp/esp.h"
#include "services/input/input_service.hpp"
#include "render_queue.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>

namespace big
{
	namespace
	{
		std::string selected_player_id;
		std::optional<online_players::entry> panel_player;
		std::string panel_status;
		int panel_frame        = -1;
		float world_time_scale = 1.f;

		// Shared by Exploration & Map, Boss Altars and Traders & POIs so Pin All World Altars
		// applies to every reveal action in all three.
		exploration::options& exploration_options()
		{
			static exploration::options options = exploration::get_options();
			return options;
		}

		struct generated_location_browser_state
		{
			bool open{};
			bool loading{};
			std::string search;
			std::vector<generated_location> locations;
		};
		generated_location_browser_state location_browser;
		std::mutex location_browser_mutex;

		template<typename F>
		void queue_job(F&& func)
		{
			async_run(std::forward<F>(func));
		}

		void refresh_players()
		{
			queue_job([] {
				online_players::update();
			});
		}

		void refresh_generated_locations()
		{
			{
				std::lock_guard lock(location_browser_mutex);
				if (location_browser.loading)
				{
					return;
				}
				location_browser.loading = true;
			}
			queue_job([] {
				std::vector<generated_location> locations;
				auto player_obj = unity::get_local_player();
				auto zones      = zone_system::get_instance();
				if (player_obj && zones)
				{
					locations = zones.get_generated_locations(unity::get_position(player_obj));
				}
				std::lock_guard lock(location_browser_mutex);
				location_browser.locations = std::move(locations);
				location_browser.loading   = false;
			});
		}

		std::string lowercase(std::string value)
		{
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			});
			return value;
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

		void set_world_time_preset(float fraction, const char* label)
		{
			queue_job([fraction, label = std::string(label)] {
				if (world_time::set_day_fraction(fraction))
				{
					notification::success("World Time", std::format("Set world time to {}.", label));
				}
				else
				{
					notification::warning("World Time", "Set world time requires a loaded world hosted by this session.");
				}
			});
		}
	}

	void view::online_player_panel()
	{
		if (!canvas::is_opened() || panel_frame != ImGui::GetFrameCount())
		{
			return;
		}
		astra::info_window info;
		if (!panel_player)
		{
			info.message = panel_status;
		}
		else
		{
			const auto& p = *panel_player;
			info.heading  = p.name;
			auto& rows    = info.rows;
			rows.push_back({"", p.local ? "You" : "Online player"});
			rows.push_back({"Character ID", p.id});
			rows.push_back({"Platform", p.platform});
			if (p.platform == "Steam")
			{
				rows.push_back({"Steam ID", p.account_id});
			}
			else
			{
				rows.push_back({"Steam ID", "not available for this platform"});
				if (!p.account_id.empty())
				{
					rows.push_back({"Platform account ID", p.account_id});
				}
			}
			rows.push_back({"Character", p.loaded ? "loaded nearby" : "outside loaded area / spawning"});
			rows.push_back({"Sharing map position", p.public_position ? "Yes" : "No"});
			rows.push_back({"Health", p.health && p.max_health ? std::format("{:.0f} / {:.0f}", *p.health, *p.max_health) : "not available outside loaded area"});
			if (p.position)
			{
				rows.push_back({"Position", coordinates(*p.position)});
				rows.push_back({"Position source", p.loaded ? "loaded character" : "shared map position"});
			}
			else
			{
				rows.push_back({"Position", "not shared and character not loaded"});
			}
			if (p.distance)
			{
				rows.push_back({"Distance", std::format("{:.1f} m", *p.distance)});
			}
			info.note = "Updates every second. Shared map positions may lag behind movement.";
		}
		canvas::draw_info_window("Player Info", info);
	}

	void view::toggle_generated_location_browser()
	{
		location_browser.open = !location_browser.open;
		if (location_browser.open)
		{
			refresh_generated_locations();
		}
	}

	void view::generated_location_browser()
	{
		if (!location_browser.open)
		{
			return;
		}

		if (!canvas::begin_panel("Generated Locations", &location_browser.open, {.size = {520.f, 480.f}, .collapsible = false}))
		{
			canvas::end_panel();
			return;
		}

		ImGui::TextWrapped("Browse location types that have already been generated in this world. Select one to teleport to its nearest instance.");
		// Leaves exactly the button's room, so the button never runs past the panel's edge.
		const auto& style = ImGui::GetStyle();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Scanning...").x - style.FramePadding.x * 2 - style.ItemSpacing.x);
		ImGui::InputTextWithHint("##GeneratedLocationSearch", "Search name or prefab...", &location_browser.search);
		ImGui::SameLine();
		bool loading{};
		{
			std::lock_guard lock(location_browser_mutex);
			loading = location_browser.loading;
		}
		if (loading)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button(loading ? "Scanning..." : "Refresh"))
		{
			refresh_generated_locations();
		}
		if (loading)
		{
			ImGui::EndDisabled();
		}

		std::vector<generated_location> locations;
		{
			std::lock_guard lock(location_browser_mutex);
			locations = location_browser.locations;
		}
		const std::string search = lowercase(location_browser.search);
		const auto matches       = [&search](const generated_location& location) {
			if (search.empty())
			{
				return true;
			}
			return lowercase(location.name).find(search) != std::string::npos || lowercase(location.prefab_name).find(search) != std::string::npos;
		};
		const auto match_count = std::ranges::count_if(locations, matches);
		ImGui::Text("%zu generated location types", match_count);
		ImGui::Separator();
		if (locations.empty())
		{
			ImGui::TextWrapped("No results loaded. Press Refresh while connected to a world.");
		}
		else
		{
			const bool child_visible = ImGui::BeginChild("##GeneratedLocationResults", {0.f, 0.f}, ImGuiChildFlags_Borders);
			if (child_visible)
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(match_count));
				while (clipper.Step())
				{
					int visible_index = 0;
					for (const auto& location : locations)
					{
						if (!matches(location))
						{
							continue;
						}
						if (visible_index >= clipper.DisplayStart && visible_index < clipper.DisplayEnd)
						{
							const auto label = location.name.empty() ? location.prefab_name : location.name;
							ImGui::PushID(location.prefab_name.c_str());
							ImGui::TextUnformatted(label.c_str());
							if (label != location.prefab_name)
							{
								ImGui::SameLine();
								ImGui::TextDisabled("%s", location.prefab_name.c_str());
							}
							ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 156.f);
							ImGui::TextDisabled("%.0f m", location.distance);
							ImGui::SameLine();
							if (ImGui::SmallButton("Go"))
							{
								const auto target = location;
								queue_job([target] {
									if (unity::get_local_player())
									{
										unity::teleport_to_world_point(target.position);
										notification::success("Location Search",
										    std::format("Teleported to '{}' ({}).",
										        target.name.empty() ? target.prefab_name : target.name,
										        target.prefab_name));
									}
									else
									{
										notification::warning("Location Search", "Join a world before teleporting.");
									}
								});
							}
							ImGui::PopID();
						}
						++visible_index;
					}
				}
			}
			ImGui::EndChild();
		}
		canvas::end_panel();
	}

	void view::draw_overlay()
	{
		if (!g_settings.window.overlay)
		{
			return;
		}

		auto local_player_obj = unity::get_local_player();
		if (!local_player_obj)
		{
			return;
		}

		player local(local_player_obj);
		const auto position     = local.get_position();
		const float health      = local.get_health();
		const float max_health  = local.get_max_health();
		const float stamina     = mono::get_field_value<"Player", "m_stamina", float>(local_player_obj);
		const float eitr        = mono::get_field_value<"Player", "m_eitr", float>(local_player_obj);
		const float max_stamina = local.get_max_stamina();
		const float max_eitr    = local.get_max_eitr();

		const astra::overlay_row rows[] = {
		    {"Health", std::format("{:.0f} / {:.0f}", health, max_health)},
		    {"Stamina", std::format("{:.0f} / {:.0f}", stamina, max_stamina)},
		    {"Eitr", std::format("{:.0f} / {:.0f}", eitr, max_eitr)},
		    {"Position", std::format("{:.1f}, {:.1f}, {:.1f}", position.x, position.y, position.z)},
		};
		canvas::draw_overlay("Player status", rows);
	}

	void view::player_submenu()
	{
		canvas::add_tab<regular_submenu>("Home", SubmenuHome, [](regular_submenu* sub) {
			sub->add_option<sub_option>("Self", nullptr, "SubmenuSelf"_hash);
			sub->add_option<sub_option>("Skills", "Raise individual skills, set levels, or max progression.", "SubmenuSkills"_hash);
			sub->add_option<sub_option>("Item Spawner", "Spawn weapons, armor, foods, materials and items.", "SubmenuItemSpawner"_hash);
			sub->add_option<sub_option>("Buffs & Effects", "Runtime guardian powers, rested bonuses, and a searchable status effect browser.", "SubmenuBuffs"_hash);
			sub->add_option<sub_option>("Ship & Sailing", "Ashlands immunity, wave protection, speed boost, anchor.", "SubmenuShip"_hash);
			sub->add_option<sub_option>("Fishing Assistant", "Instant bite, unbreakable line, zero stamina, auto-catch.", "SubmenuFishing"_hash);
			sub->add_option<sub_option>("Inventory", nullptr, "SubmenuInventory"_hash);
			sub->add_option<sub_option>("Building & Crafting", "Free crafting and building, recipe and piece unlocks, stability, ward bypass.", "SubmenuBuilding"_hash);
			sub->add_option<sub_option>("World", nullptr, "SubmenuWorld"_hash);
			sub->add_option<sub_option>("Base Tools", "Storage, production, repair, farming and environment.", "BaseTools"_hash);
			sub->add_option<sub_option>("Cheat Status", "Check for cheat marks on your character, world and nearby items, and remove them.", "CheatStatus"_hash);
			sub->add_option<sub_option>("Online Players", nullptr, SubmenuPlayerList);
			sub->add_option<sub_option>("ESP", nullptr, "SubmenuESP"_hash);
			sub->add_option<sub_option>("Aimbot", nullptr, "SubmenuAimbot"_hash);
		});

		canvas::add_submenu<regular_submenu>("Self", "SubmenuSelf"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Show Information Overlay",
			    "Show player health, stamina, eitr, and world coordinates on screen.",
			    &g_settings.window.overlay);
			sub->add_option<bool_option<bool>>("allowed_command"_hash);
			sub->add_option<sub_option>("Attack", "Damage, backstab, one-hit kills, attack speed, push force, melee and eitr costs.", "SubmenuSelfAttack"_hash);
			sub->add_option<sub_option>("Bows & Crossbows", "Instant draw and reload, ammunition, ranged stamina cost.", "SubmenuSelfRanged"_hash);
			sub->add_option<sub_option>("Parry", "Automatic and guaranteed parries, parry force and timing window.", "SubmenuSelfParry"_hash);
			sub->add_option<sub_option>("Defense", "God mode, ghost mode, incoming damage and stagger, fall damage, body armor.", "SubmenuSelfDefense"_hash);
			sub->add_option<sub_option>("Health, Stamina & Eitr", "Keep them full, regeneration, and base maximums.", "SubmenuSelfVitals"_hash);
			sub->add_option<sub_option>("Food", "Food slots, food duration, and food timer.", "SubmenuSelfFood"_hash);
			sub->add_option<sub_option>("Weather Effects", "Freezing, cold, and wet.", "SubmenuSelfWeather"_hash);
			sub->add_option<sub_option>("Movement & Flight", "Flying, noclip, and speed and jump multipliers.", "SubmenuMovement"_hash);
			sub->add_option<sub_option>("Camera", "Freecam, field of view, zoom, smoothing, camera shake, and HUD.", "SubmenuSelfCamera"_hash);
			sub->add_option<sub_option>("Gathering & Pickup", "One-hit trees and ore, pickup range, and collecting nearby drops.", "SubmenuSelfGathering"_hash);
		});

		canvas::add_submenu<regular_submenu>("Attack", "SubmenuSelfAttack"_hash, [](regular_submenu* sub) {
			sub->add_option<number_option<float>>("Damage Multiplier",
			    "Multiplier for player attacks (set high for 1-hit kill).",
			    &g_settings.self.damage_multiplier,
			    0.f,
			    50.f,
			    1.f,
			    1);
			sub->add_option<number_option<float>>("Backstab Multiplier",
			    "Scale backstab damage for all player attacks after any item-specific override.",
			    &g_settings.self.backstab_multiplier,
			    0.f,
			    20.f,
			    0.25f,
			    2);
			sub->add_option<bool_option<bool>>("Hostile One-Hit Damage",
			    "Set outgoing damage high enough to defeat hostile targets in one hit.",
			    &g_settings.self.hostile_one_hit);
			sub->add_option<number_option<float>>("Attack Animation Speed",
			    "Scale the local player's animation speed while a melee or ranged attack animation is active.",
			    &g_settings.self.attack_animation_speed,
			    0.5f,
			    5.f,
			    0.1f,
			    1);
			sub->add_option<number_option<float>>("Push Force Multiplier",
			    "Scale push force applied by player attacks.",
			    &g_settings.self.push_force_multiplier,
			    0.f,
			    20.f,
			    0.25f,
			    1);
			sub->add_option<number_option<float>>("Melee Stamina Cost",
			    "Scale stamina cost for local melee attacks.",
			    &g_settings.self.melee_stamina_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
			sub->add_option<number_option<float>>("Eitr Attack Cost",
			    "Scale eitr cost for player attacks.",
			    &g_settings.self.attack_eitr_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
		});

		canvas::add_submenu<regular_submenu>("Bows & Crossbows", "SubmenuSelfRanged"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Instant Bow & Crossbow",
			    "Instant full bow draw and instant crossbow reload.",
			    &g_settings.self.instant_bow_draw);
			sub->add_option<bool_option<bool>>("Preserve Ammunition",
			    "Restore the ammunition item after a successful ranged attack.",
			    &g_settings.self.preserve_ammunition);
			sub->add_option<number_option<float>>("Ranged Stamina Cost",
			    "Scale stamina cost for local bows and reload based ranged attacks.",
			    &g_settings.self.ranged_stamina_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
		});

		canvas::add_submenu<regular_submenu>("Parry", "SubmenuSelfParry"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Auto Parry",
			    "Automatically parry incoming melee and ranged attacks (360-degree, infinite parry power, staggers attackers).",
			    &g_settings.self.auto_parry);
			sub->add_option<bool_option<bool>>("Always Parry",
			    "Guarantees a perfect parry (staggers enemy) whenever you manually block, ignoring the parry timing window.",
			    &g_settings.self.always_parry);
			sub->add_option<number_option<float>>("Parry Force Multiplier",
			    "Scale block power during manual and automatic parries.",
			    &g_settings.self.parry_force_multiplier,
			    0.f,
			    20.f,
			    0.25f,
			    2);
			sub->add_option<number_option<float>>("Parry Window Multiplier",
			    "Scale the timed parry bonus while blocking.",
			    &g_settings.self.parry_window_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
		});

		canvas::add_submenu<regular_submenu>("Defense", "SubmenuSelfDefense"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("God Mode",
			    "Invulnerable to all incoming damage, fall, hazards, and debuffs.",
			    &g_settings.self.god_mode);
			sub->add_option<bool_option<bool>>("Incoming Hit Invulnerability",
			    "Ignore damage RPCs while leaving other world systems active.",
			    &g_settings.self.incoming_invulnerability);
			sub->add_option<bool_option<bool>>("Ghost Mode",
			    "Enemies and bosses cannot detect, target, or hear you.",
			    &g_settings.self.ghost_mode);
			sub->add_option<number_option<float>>("Incoming Damage Multiplier",
			    "Scale damage received by the local player; 0 disables hit damage.",
			    &g_settings.self.incoming_damage_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
			sub->add_option<number_option<float>>("Incoming Stagger Multiplier",
			    "Scale stagger taken; 0 prevents stagger from incoming hits.",
			    &g_settings.self.stagger_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
			sub->add_option<bool_option<bool>>("No Fall Damage",
			    "Ignore damage from falls while keeping other incoming damage rules active.",
			    &g_settings.self.no_fall_damage);
			sub->add_option<bool_slider_float_option>("max_body_armor"_hash, "num_body_armor"_hash, 10.f);
		});

		canvas::add_submenu<regular_submenu>("Health, Stamina & Eitr", "SubmenuSelfVitals"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Maintain Health",
			    "Restore health to maximum after each player update.",
			    &g_settings.self.maintain_health);
			sub->add_option<number_option<float>>("Health Regeneration / sec",
			    "Add health at this rate up to the current maximum.",
			    &g_settings.self.health_regen_rate,
			    0.f,
			    10000.f,
			    10.f,
			    0);
			sub->add_option<bool_slider_float_option>("max_health"_hash, "max_hp"_hash, 10.f);
			sub->add_option<bool_option<bool>>("Maintain Stamina",
			    "Keep stamina at its current maximum.",
			    &g_settings.self.maintain_stamina);
			sub->add_option<bool_slider_float_option>("max_stamina"_hash, "max_stam"_hash, 10.f);
			sub->add_option<bool_slider_float_option>("stamina_regen"_hash, "stamina_regen_amount"_hash, 5.f);
			sub->add_option<bool_option<bool>>("Maintain Eitr", "Keep eitr at its current maximum.", &g_settings.self.maintain_eitr);
			sub->add_option<bool_slider_float_option>("eitr"_hash, "eitr_amount"_hash, 5.f);
			sub->add_option<bool_slider_float_option>("eitr_regen"_hash, "eitr_regen_amount"_hash, 5.f);
		});

		canvas::add_submenu<regular_submenu>("Food", "SubmenuSelfFood"_hash, [](regular_submenu* sub) {
			sub->add_option<number_option<int>>("Max Food Slots",
			    "Allow eating more than 3 foods simultaneously (HP, Stamina, Eitr buffs stack!).",
			    &g_settings.self.max_food_slots,
			    3,
			    10,
			    1,
			    0);
			sub->add_option<number_option<float>>("Food Duration Multiplier",
			    "Scale how long food lasts; 2.0 doubles the duration and 0 keeps food from decreasing.",
			    &g_settings.self.food_duration_multiplier,
			    0.f,
			    10.f,
			    0.1f,
			    1);
			sub->add_option<bool_slider_float_option>("max_food_health"_hash, "food_hp"_hash, 1000.f);
		});

		canvas::add_submenu<regular_submenu>("Weather Effects", "SubmenuSelfWeather"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Prevent Freezing",
			    "Remove the Freezing environment effect from the local player.",
			    &g_settings.self.prevent_freezing);
			sub->add_option<bool_option<bool>>("Prevent Cold",
			    "Remove the Cold environment effect from the local player.",
			    &g_settings.self.prevent_cold);
			sub->add_option<bool_option<bool>>("Never Wet or Tarred",
			    "Keep the local player from getting Wet from rain or water, and Tarred from tar pits.",
			    &g_settings.self.is_wet);
		});

		canvas::add_submenu<regular_submenu>("Movement & Flight", "SubmenuMovement"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Flying", "Toggle creative free flight mode.", &g_settings.self.flying);
			sub->add_option<bool_option<bool>>("Noclip", "Pass through world collision while enabled.", &g_settings.self.noclip);
			sub->add_option<number_option<float>>("Movement Speed Multiplier",
			    "Scale normal movement and sprint speed before their separate modifiers.",
			    &g_settings.self.movement_speed_multiplier,
			    0.1f,
			    10.f,
			    0.1f,
			    1);
			sub->add_option<number_option<float>>("Sprint Speed Multiplier",
			    "Scale sprint speed independently (no armor penalty).",
			    &g_settings.self.run_speed_multiplier,
			    0.1f,
			    10.f,
			    0.5f,
			    1);
			sub->add_option<number_option<float>>("Jog Speed Multiplier",
			    "Scale normal jogging speed independently (no armor penalty).",
			    &g_settings.self.jog_speed_multiplier,
			    0.1f,
			    10.f,
			    0.5f,
			    1);
			sub->add_option<number_option<float>>("Swim Speed Multiplier",
			    "Scale the player's swim speed.",
			    &g_settings.self.swim_speed_multiplier,
			    0.1f,
			    20.f,
			    0.1f,
			    1);
			sub->add_option<number_option<float>>("Jump Strength Multiplier",
			    "Scale jump impulse while keeping the game's movement logic.",
			    &g_settings.self.jump_force_multiplier,
			    0.1f,
			    20.f,
			    0.1f,
			    1);
		});

		canvas::add_submenu<regular_submenu>("Camera", "SubmenuSelfCamera"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Toggle Freecam", "Toggle Valheim's camera free flight mode.", [] {
				queue_job([] {
					camera_tools::toggle_freecam();
					camera_tools::apply_settings();
				});
			});
			sub->add_option<number_option<float>>("Freecam Speed",
			    "Movement speed used by Valheim's free camera.",
			    &g_settings.self.freecam_speed,
			    1.f,
			    1000.f,
			    5.f,
			    0);
			sub->add_option<number_option<float>>("Freecam Smoothing",
			    "Movement smoothing used by Valheim's free camera.",
			    &g_settings.self.freecam_smoothness,
			    0.f,
			    1.f,
			    0.05f,
			    2);
			sub->add_option<number_option<float>>("Camera Field of View",
			    "Main camera field of view in degrees.",
			    &g_settings.self.camera_fov,
			    30.f,
			    120.f,
			    1.f,
			    0);
			sub->add_option<number_option<float>>("Camera Zoom Sensitivity",
			    "Adjust how quickly the camera zooms with the mouse wheel.",
			    &g_settings.self.camera_zoom_sensitivity,
			    0.1f,
			    30.f,
			    0.5f,
			    1);
			sub->add_option<number_option<float>>("Camera Smoothing",
			    "Adjust the smoothing applied to the player camera.",
			    &g_settings.self.camera_smoothness,
			    0.f,
			    1.f,
			    0.05f,
			    2);
			sub->add_option<bool_option<bool>>("Camera Shake",
			    "Enable or disable camera shake effects.",
			    &g_settings.self.camera_shake_enabled);
			sub->add_option<reguler_option>("Apply Camera Settings", "Apply the selected camera and freecam settings now.", [] {
				queue_job([] {
					camera_tools::apply_settings();
				});
			});
			sub->add_option<reguler_option>("Toggle HUD Visibility", "Use Valheim's own HUD hide/show state.", [] {
				unity::hud_manager::toggle_visibility();
			});
		});

		canvas::add_submenu<regular_submenu>("Gathering & Pickup", "SubmenuSelfGathering"_hash, [](regular_submenu* sub) {
			static float drop_action_radius = 30.f;
			sub->add_option<bool_option<bool>>("One-Hit Tree & Mine Breaker",
			    "Instantly fell trees and shatter rocks / ore veins in one hit.",
			    &g_settings.self.one_hit_resource);
			sub->add_option<number_option<float>>("Vacuum Pickup Range",
			    "Item magnet pickup radius in meters (default: 2m).",
			    &g_settings.self.pickup_range,
			    2.f,
			    50.f,
			    2.f,
			    1);
			sub->add_option<number_option<float>>("Nearby Drop Radius", "Maximum distance for the nearby drop collection action.", &drop_action_radius, 1.f, 100.f, 5.f, 0);
			sub->add_option<reguler_option>("Collect Nearby Drops", "Pick up available dropped items within the selected radius.", [] {
				queue_job([radius = drop_action_radius] {
					const int count = world_tools::pickup_nearby_drops(radius);
					if (count > 0)
					{
						notification::success("Nearby Drops", std::format("Collected {} item drop{}.", count, count == 1 ? "" : "s"));
					}
					else
					{
						notification::info("Nearby Drops", "No available item drops were collected.");
					}
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Building & Crafting", "SubmenuBuilding"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Free Crafting & Building",
			    "Craft anywhere with 0 cost, deconstruct/repair pieces with no station requirement.",
			    &g_settings.self.free_crafting);
			sub->add_option<bool_option<bool>>("Free Material Costs",
			    "Skip resource consumption for recipes and build pieces.",
			    &g_settings.self.free_material_costs);
			sub->add_option<sub_option>("Crafting", "Crafting speed, station requirements and level, recipe unlocks, recipe browser.", "SubmenuCrafting"_hash);
			sub->add_option<sub_option>("Building", "Workbench requirement, piece unlocks, stability, wards, placement, piece browser.", "SubmenuBuildingPieces"_hash);
		});

		canvas::add_submenu<regular_submenu>("Crafting", "SubmenuCrafting"_hash, [](regular_submenu* sub) {
			sub->add_option<number_option<float>>("Crafting Speed Multiplier",
			    "Scales the time required for single and multi-item crafts.",
			    &g_settings.self.crafting_speed_multiplier,
			    0.1f,
			    20.f,
			    0.5f,
			    1);
			sub->add_option<bool_option<bool>>("Ignore Crafting Station Requirement",
			    "Craft and repair recipes without the required station.",
			    &g_settings.self.ignore_crafting_station);
			sub->add_option<bool_option<bool>>("Override Crafting Station Level",
			    "Use a custom level for station checks while keeping the station itself required.",
			    &g_settings.self.station_level_override);
			sub->add_option<number_option<int>>("Crafting Station Level",
			    "Level reported by CraftingStation.GetLevel when the override is enabled.",
			    &g_settings.self.crafting_station_level,
			    1,
			    100,
			    1,
			    0);
			sub->add_option<bool_option<bool>>("Unlock Runtime Recipes",
			    "Treat all enabled ObjectDB recipes as unlocked in crafting lists.",
			    &g_settings.self.unlock_runtime_recipes);
			sub->add_option<bool_option<bool>>("Unlock Materials",
			    "Treat all runtime materials as discovered.",
			    &g_settings.self.unlock_materials);
			sub->add_option<bool_option<bool>>("open_all_recepies"_hash);
			sub->add_option<reguler_option>("Open Runtime Recipe Browser", "Search ObjectDB recipes, view ingredients, and send an output to Item Spawner.", [] {
				crafting_tools::toggle_recipe_browser();
			});
		});

		canvas::add_submenu<regular_submenu>("Building", "SubmenuBuildingPieces"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Ignore Building Workbench Requirement",
			    "Place build pieces without a nearby workbench or crafting station.",
			    &g_settings.self.ignore_building_workbench);
			sub->add_option<bool_option<bool>>("Unlock Building Pieces",
			    "Allow every piece in the current build tables, including pieces without known recipes.",
			    &g_settings.self.unlock_building_pieces);
			sub->add_option<bool_option<bool>>("Infinite Building Stability",
			    "Pieces never collapse from lack of support.",
			    &g_settings.self.infinite_stability);
			sub->add_option<bool_option<bool>>("Ward / Guard Stone Bypass",
			    "Bypass all player wards and guard stones.",
			    &g_settings.self.ward_bypass);
			sub->add_option<number_option<float>>("Build Placement Distance",
			    "Maximum distance for placing build pieces.",
			    &g_settings.self.build_distance,
			    5.f,
			    50.f,
			    1.f,
			    0);
			sub->add_option<number_option<float>>("Fine Rotation Step",
			    "Degrees applied per rotation step while placing pieces.",
			    &g_settings.self.build_rotation_step,
			    1.f,
			    22.5f,
			    0.5f,
			    1);
			sub->add_option<number_option<float>>("Piece Placement Delay",
			    "Delay between consecutive piece placement actions.",
			    &g_settings.self.build_place_delay,
			    0.01f,
			    2.f,
			    0.05f,
			    2);
			sub->add_option<number_option<float>>("Piece Removal Delay",
			    "Delay between consecutive hammer removal actions.",
			    &g_settings.self.build_remove_delay,
			    0.01f,
			    2.f,
			    0.05f,
			    2);
			sub->add_option<bool_option<bool>>("Allow Rotated Same-Piece Overlap",
			    "Allow identical pieces to overlap when their rotations differ, following Valheim's piece overlap rule.",
			    &g_settings.self.allow_rotated_piece_overlap);
			sub->add_option<reguler_option>("Open Dynamic Build Piece Browser", "Browse runtime Piece prefabs and find the build tool whose PieceTable contains the selected piece.", [] {
				prefab_spawner::open_piece_browser();
			});
		});

		canvas::add_submenu<regular_submenu>("Skills", "SubmenuSkills"_hash, [](regular_submenu* sub) {
			static const std::vector<const char*> skill_display_names{"Swords", "Knives", "Clubs", "Polearms", "Spears", "Blocking", "Axes", "Bows", "Crossbows", "Unarmed", "Elemental Magic", "Blood Magic", "Run", "Jump", "Sneak", "Swim", "Dodge", "Ride", "Fishing", "Wood Cutting", "Pickaxes", "Cooking", "Farming", "Crafting", "All Skills"};

			static const std::vector<const char*> skill_csharp_names{"Swords", "Knives", "Clubs", "Polearms", "Spears", "Blocking", "Axes", "Bows", "Crossbows", "Unarmed", "ElementalMagic", "BloodMagic", "Run", "Jump", "Sneak", "Swim", "Dodge", "Ride", "Fishing", "WoodCutting", "Pickaxes", "Cooking", "Farming", "Crafting", "All"};

			static int selected_skill_index = 0;
			static float raise_amount       = 10.f;
			static float exact_skill_level  = 100.f;

			sub->add_option<choose_option<const char*, int>>("Select Skill", "Choose a specific skill to raise or adjust.", &skill_display_names, &selected_skill_index);

			sub->add_option<number_option<float>>("Exact Skill Level", "Level to set directly (0-100).", &exact_skill_level, 0.f, 100.f, 1.f, 0);
			sub->add_option<reguler_option>("Set Selected Skill Level", "Set the selected skill to the exact level above.", [] {
				queue_job([] {
					static constexpr SkillType skill_types[] = {SkillType::Swords, SkillType::Knives, SkillType::Clubs, SkillType::Polearms, SkillType::Spears, SkillType::Blocking, SkillType::Axes, SkillType::Bows, SkillType::Crossbows, SkillType::Unarmed, SkillType::ElementalMagic, SkillType::BloodMagic, SkillType::Run, SkillType::Jump, SkillType::Sneak, SkillType::Swim, SkillType::Dodge, SkillType::Ride, SkillType::Fishing, SkillType::WoodCutting, SkillType::Pickaxes, SkillType::Cooking, SkillType::Farming, SkillType::Crafting, SkillType::All};
					auto sk = skills::get_local_skills();
					if (!sk || selected_skill_index < 0 || selected_skill_index >= static_cast<int>(std::size(skill_types)))
					{
						notification::warning("Skills", "Player or selected skill is unavailable.");
						return;
					}
					sk.set_skill_level_direct(skill_types[selected_skill_index], exact_skill_level);
					notification::success("Skills", std::format("Set {} to {:.0f}.", skill_display_names[selected_skill_index], exact_skill_level));
				});
			});

			sub->add_option<number_option<float>>("Raise Amount", "Amount to increase skill level by (press Enter to apply).", &raise_amount, 1.f, 100.f, 5.f, 0, false, "+", " Lvl", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					if (selected_skill_index < 0 || selected_skill_index >= static_cast<int>(skill_csharp_names.size()))
					{
						return;
					}

					const char* cs_name   = skill_csharp_names[selected_skill_index];
					const char* disp_name = skill_display_names[selected_skill_index];
					sk.cheat_raise_skill(cs_name, raise_amount, true);
					notification::success("Skills", std::format("Raised {} by +{:.0f}!", disp_name, raise_amount));
				});
			});

			sub->add_option<reguler_option>("Raise Selected Skill", "Increase the selected skill by the chosen amount.", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					if (selected_skill_index < 0 || selected_skill_index >= static_cast<int>(skill_csharp_names.size()))
					{
						return;
					}

					const char* cs_name   = skill_csharp_names[selected_skill_index];
					const char* disp_name = skill_display_names[selected_skill_index];
					sk.cheat_raise_skill(cs_name, raise_amount, true);
					notification::success("Skills", std::format("Raised {} by +{:.0f}!", disp_name, raise_amount));
				});
			});

			sub->add_option<reguler_option>("Lower Selected Skill", "Decrease the selected skill by the chosen amount.", [] {
				queue_job([] {
					static constexpr SkillType skill_types[] = {SkillType::Swords, SkillType::Knives, SkillType::Clubs, SkillType::Polearms, SkillType::Spears, SkillType::Blocking, SkillType::Axes, SkillType::Bows, SkillType::Crossbows, SkillType::Unarmed, SkillType::ElementalMagic, SkillType::BloodMagic, SkillType::Run, SkillType::Jump, SkillType::Sneak, SkillType::Swim, SkillType::Dodge, SkillType::Ride, SkillType::Fishing, SkillType::WoodCutting, SkillType::Pickaxes, SkillType::Cooking, SkillType::Farming, SkillType::Crafting, SkillType::All};
					auto sk = skills::get_local_skills();
					if (!sk || selected_skill_index < 0 || selected_skill_index >= static_cast<int>(std::size(skill_types)))
					{
						notification::warning("Skills", "Player or selected skill is unavailable.");
						return;
					}
					sk.raise_skill_direct(skill_types[selected_skill_index], -raise_amount);
					notification::success("Skills", std::format("Lowered {} by {:.0f}.", skill_display_names[selected_skill_index], raise_amount));
				});
			});

			sub->add_option<reguler_option>("Max Selected Skill (Level 100)", "Set the selected skill to maximum level (100).", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					if (selected_skill_index < 0 || selected_skill_index >= static_cast<int>(skill_csharp_names.size()))
					{
						return;
					}

					const char* cs_name   = skill_csharp_names[selected_skill_index];
					const char* disp_name = skill_display_names[selected_skill_index];
					sk.cheat_raise_skill(cs_name, 100.f, true);
					notification::success("Skills", std::format("Set {} to max level (100)!", disp_name));
				});
			});

			sub->add_option<reguler_option>("Reset Selected Skill (Level 0)", "Reset the selected skill to level 0.", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					if (selected_skill_index < 0 || selected_skill_index >= static_cast<int>(skill_csharp_names.size()))
					{
						return;
					}

					const char* cs_name   = skill_csharp_names[selected_skill_index];
					const char* disp_name = skill_display_names[selected_skill_index];
					sk.cheat_reset_skill(cs_name);
					notification::info("Skills", std::format("Reset {} to level 0.", disp_name));
				});
			});

			sub->add_option<reguler_option>("Max All Skills (Level 100)", "Set every skill to level 100 immediately.", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					sk.cheat_raise_skill("All", 100.f, true);
					notification::success("Skills", "All skills maxed to level 100!");
				});
			});

			sub->add_option<reguler_option>("Set All Skills to Exact Level", "Set every skill to the selected exact level above (0-100).", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					sk.set_skill_level_direct(SkillType::All, exact_skill_level);
					notification::success("Skills", std::format("Set all skills to {:.0f}.", exact_skill_level));
				});
			});

			sub->add_option<reguler_option>("Reset All Skills (Level 0)", "Reset all skill levels back to 0.", [] {
				queue_job([] {
					auto sk = skills::get_local_skills();
					if (!sk)
					{
						notification::warning("Skills", "Player not in world or skills not available.");
						return;
					}
					sk.cheat_reset_skill("All");
					notification::info("Skills", "All skills reset to level 0.");
				});
			});

			sub->add_option<bool_slider_float_option>("enable_raise_skill"_hash, "raise_skill"_hash, 1.f);
			sub->add_option<bool_option<bool>>("prevent_skill_loss"_hash);
			sub->add_option<bool_option<bool>>("freeze_skills"_hash);

			sub->add_option<sub_option>("Combat Skills", "Swords, bows, axes, polearms, magic, blocking, unarmed...", "SubmenuCombatSkills"_hash);
			sub->add_option<sub_option>("Physical & Survival Skills", "Run, jump, sneak, swim, dodge, ride, fishing...", "SubmenuPhysicalSkills"_hash);
			sub->add_option<sub_option>("Gathering & Crafting Skills", "Wood cutting, pickaxes, cooking, farming, crafting...", "SubmenuCraftingSkills"_hash);
		});

		canvas::add_submenu<regular_submenu>("Combat Skills", "SubmenuCombatSkills"_hash, [](regular_submenu* sub) {
			struct skill_item
			{
				const char* name;
				const char* cs_name;
			};
			static constexpr skill_item list[] = {
			    {"Swords", "Swords"},
			    {"Knives", "Knives"},
			    {"Clubs", "Clubs"},
			    {"Polearms", "Polearms"},
			    {"Spears", "Spears"},
			    {"Blocking", "Blocking"},
			    {"Axes", "Axes"},
			    {"Bows", "Bows"},
			    {"Crossbows", "Crossbows"},
			    {"Unarmed", "Unarmed"},
			    {"Elemental Magic", "ElementalMagic"},
			    {"Blood Magic", "BloodMagic"},
			};
			static float amounts[std::size(list)] = {10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f};

			for (std::size_t i = 0; i < std::size(list); ++i)
			{
				const auto& item = list[i];
				sub->add_option<number_option<float>>(item.name,
				    "Raise amount (press Enter to apply to this skill).",
				    &amounts[i],
				    1.f,
				    100.f,
				    5.f,
				    0,
				    false,
				    "+",
				    " Lvl",
				    [cs = item.cs_name, disp = item.name, idx = i] {
					    queue_job([cs, disp, idx] {
						    auto sk = skills::get_local_skills();
						    if (!sk)
						    {
							    notification::warning("Skills", "Player not in world or skills not available.");
							    return;
						    }
						    sk.cheat_raise_skill(cs, amounts[idx], true);
						    notification::success("Skills", std::format("Raised {} by +{:.0f}!", disp, amounts[idx]));
					    });
				    });
			}
		});

		canvas::add_submenu<regular_submenu>("Physical & Survival Skills", "SubmenuPhysicalSkills"_hash, [](regular_submenu* sub) {
			struct skill_item
			{
				const char* name;
				const char* cs_name;
			};
			static constexpr skill_item list[] = {
			    {"Run", "Run"},
			    {"Jump", "Jump"},
			    {"Sneak", "Sneak"},
			    {"Swim", "Swim"},
			    {"Dodge", "Dodge"},
			    {"Ride", "Ride"},
			    {"Fishing", "Fishing"},
			};
			static float amounts[std::size(list)] = {10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f};

			for (std::size_t i = 0; i < std::size(list); ++i)
			{
				const auto& item = list[i];
				sub->add_option<number_option<float>>(item.name,
				    "Raise amount (press Enter to apply to this skill).",
				    &amounts[i],
				    1.f,
				    100.f,
				    5.f,
				    0,
				    false,
				    "+",
				    " Lvl",
				    [cs = item.cs_name, disp = item.name, idx = i] {
					    queue_job([cs, disp, idx] {
						    auto sk = skills::get_local_skills();
						    if (!sk)
						    {
							    notification::warning("Skills", "Player not in world or skills not available.");
							    return;
						    }
						    sk.cheat_raise_skill(cs, amounts[idx], true);
						    notification::success("Skills", std::format("Raised {} by +{:.0f}!", disp, amounts[idx]));
					    });
				    });
			}
		});

		canvas::add_submenu<regular_submenu>("Gathering & Crafting Skills", "SubmenuCraftingSkills"_hash, [](regular_submenu* sub) {
			struct skill_item
			{
				const char* name;
				const char* cs_name;
			};
			static constexpr skill_item list[] = {
			    {"Wood Cutting", "WoodCutting"},
			    {"Pickaxes", "Pickaxes"},
			    {"Cooking", "Cooking"},
			    {"Farming", "Farming"},
			    {"Crafting", "Crafting"},
			};
			static float amounts[std::size(list)] = {10.f, 10.f, 10.f, 10.f, 10.f};

			for (std::size_t i = 0; i < std::size(list); ++i)
			{
				const auto& item = list[i];
				sub->add_option<number_option<float>>(item.name,
				    "Raise amount (press Enter to apply to this skill).",
				    &amounts[i],
				    1.f,
				    100.f,
				    5.f,
				    0,
				    false,
				    "+",
				    " Lvl",
				    [cs = item.cs_name, disp = item.name, idx = i] {
					    queue_job([cs, disp, idx] {
						    auto sk = skills::get_local_skills();
						    if (!sk)
						    {
							    notification::warning("Skills", "Player not in world or skills not available.");
							    return;
						    }
						    sk.cheat_raise_skill(cs, amounts[idx], true);
						    notification::success("Skills", std::format("Raised {} by +{:.0f}!", disp, amounts[idx]));
					    });
				    });
			}
		});

		canvas::add_submenu<regular_submenu>("Inventory", "SubmenuInventory"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Open Inventory Editor", "Search, sort, edit, duplicate, equip, drop, delete, and consolidate inventory items.", [] {
				inventory_editor::toggle_window();
			});
			sub->add_option<reguler_option>("Repair All Items (Anywhere)", "Instantly restore all equipped weapons, armor, and inventory items to 100% durability.", [] {
				queue_job([] {
					world_tools::repair_all_inventory();
				});
			});
			sub->add_option<bool_option<bool>>("Infinite Durability",
			    "Weapons, tools, shields, and armor never degrade.",
			    &g_settings.self.infinite_durability);
			sub->add_option<sub_option>("Carry Weight & Size", "Item weight, encumbrance, max carry weight, inventory size and fill order.", "SubmenuInventoryCapacity"_hash);
			sub->add_option<sub_option>("Item Overrides", "Dropped stack size and the selected item's durability, quality, stack and variant.", "SubmenuInventoryOverrides"_hash);
			sub->add_option<sub_option>("Equipment & Quick Slots", "BetrValheim gear, food, ammo, trinket, and quick slots.", "SubmenuEquipmentSlots"_hash);
		});

		canvas::add_submenu<regular_submenu>("Carry Weight & Size", "SubmenuInventoryCapacity"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("No Weight", nullptr, &g_settings.self.no_weight);
			sub->add_option<bool_option<bool>>("No Encumbrance",
			    "Ignore carry weight when checking whether the local player is encumbered. Item weights stay unchanged.",
			    &g_settings.self.no_encumbrance);
			sub->add_option<bool_slider_float_option>("max_carry_weight"_hash, "carry_amount"_hash, 100.f);
			sub->add_option<bool_slider_int_option>("inventory_size"_hash, "inventory_height"_hash, 1);
			sub->add_option<bool_option<bool>>("Fill Inventory Top First",
			    "Items added to inventory will fill slots from the top down instead of bottom.",
			    &g_settings.self.inventory_top_first);
		});

		canvas::add_submenu<regular_submenu>("Item Overrides", "SubmenuInventoryOverrides"_hash, [](regular_submenu* sub) {
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

		canvas::add_submenu<regular_submenu>("Equipment & Quick Slots", "SubmenuEquipmentSlots"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Enabled",
			    "Enable BetrValheim extra inventory and equipment slots. Items in these slots never drop on death, and they are only visible while the mod is loaded.",
			    &g_settings.self.equipment_slots_enabled);
			sub->add_option<bool_option<bool>>("Native Unity Inventory Grid",
			    "Render equipment slots natively in Valheim's inventory screen.",
			    &g_settings.self.equipment_slots_native_ui);
			sub->add_option<number_option<float>>("Native Grid X Offset",
			    "Horizontal position offset for native inventory grid.",
			    &g_settings.self.equipment_slots_native_x,
			    -1500.f,
			    1500.f,
			    10.f,
			    0);
			sub->add_option<number_option<float>>("Native Grid Y Offset",
			    "Vertical position offset for native inventory grid.",
			    &g_settings.self.equipment_slots_native_y,
			    -1000.f,
			    1000.f,
			    10.f,
			    0);
			sub->add_option<bool_option<bool>>("Open ImGui Overlay Window",
			    "Show interactive equipment slots only while the inventory is open.",
			    &g_settings.self.equipment_slots_window);
			sub->add_option<bool_option<bool>>("Slot Hotkeys",
			    "Alt+1..6 (Quick), Shift+1..3 (Food), Alt+R/T/Z (Ammo).",
			    &g_settings.self.equipment_slots_hotkeys);
			sub->add_option<bool_option<bool>>("Quadruple Utility Stacking",
			    "Allow equipping up to 4 utility belts/items at once.",
			    &g_settings.self.multi_utility_enabled);
			sub->add_option<reguler_option>("Auto-Fill from Inventory", "Automatically move compatible armor, utilities, food, and ammo from inventory into slots.", [] {
				queue_job([] {
					equipment_manager::get().auto_fill_from_inventory();
				});
			});
			sub->add_option<reguler_option>("Deposit All to Inventory", "Move all items from equipment and quick slots back to main inventory.", [] {
				queue_job([] {
					equipment_manager::get().deposit_all_to_inventory();
				});
			});
			sub->add_option<reguler_option>("Save Slots to Character", "Force write slots data to player customData for persistence.", [] {
				queue_job([] {
					if (equipment_manager::get().save_to_player())
					{
						notification::success("Equipment Slots", "Equipment slots saved to character customData!");
					}
					else
					{
						notification::warning("Equipment Slots", "Could not save equipment slots. Join a world and try again.");
					}
				});
			});
		});


		canvas::add_submenu<regular_submenu>("World", "SubmenuWorld"_hash, [](regular_submenu* sub) {
			sub->add_option<sub_option>("Time", "Simulation speed, pause, and time of day.", "SubmenuWorldTime"_hash);
			sub->add_option<sub_option>("World Preset & Modifiers", "Apply a native world preset, or change combat, death penalty, resource, raid and portal settings independently.", "SubmenuWorldModifiers"_hash);
			sub->add_option<sub_option>("Exploration & Map", "Reveal map fog, locate all bosses and traders.", "SubmenuExploration"_hash);
			sub->add_option<sub_option>("Creatures & Taming", "Aimed animal inspection, instant tame hotkey, and area taming.", "SubmenuCreatures"_hash);
			sub->add_option<sub_option>("Base Raids & Events", "Trigger or stop custom raids, disable base attacks.", "SubmenuRaids"_hash);
			sub->add_option<sub_option>("Death & Tombstone", "Keep items and skills on death, and return to your tombstone.", "SubmenuTombstone"_hash);
			sub->add_option<sub_option>("Remote Merchant", "Open Haldor, Hildir, or Bog Witch store from anywhere.", "SubmenuTrader"_hash);
			sub->add_option<sub_option>("World Upgrade & Maintenance", "Save the world, edit global keys, upgrade old worlds, generate missing update POIs, and optimize terrain.", "SubmenuUpgradeWorld"_hash);
		});

		canvas::add_submenu<regular_submenu>("Time", "SubmenuWorldTime"_hash, [](regular_submenu* sub) {
			static float day_fraction = 0.5f;
			sub->add_option<number_option<float>>("World Time Scale", "Local Unity simulation speed: 0 pauses, 1 is normal.", &world_time_scale, 0.f, 5.f, 0.1f, 1);
			sub->add_option<reguler_option>("Apply Time Scale", "Apply the selected simulation speed.", [] {
				const float scale = std::clamp(world_time_scale, 0.f, 5.f);
				queue_job([scale] {
					world_time::set_time_scale(scale);
				});
			});
			sub->add_option<reguler_option>(world_time::is_time_paused() ? "Resume Time" : "Pause Time", "Pause or resume local world simulation.", [] {
				const bool paused = !world_time::is_time_paused();
				const float scale = paused ? 0.f : (world_time_scale > 0.f ? world_time_scale : 1.f);
				queue_job([scale] {
					world_time::set_time_scale(scale);
				});
			});
			sub->add_option<reguler_option>("Reset Time Scale", "Restore local simulation speed to normal.", [] {
				world_time_scale = 1.f;
				queue_job([] {
					world_time::reset_time_scale();
				});
			});
			sub->add_option<number_option<float>>("Day Fraction", "Select a time of day from 0.00 to 1.00.", &day_fraction, 0.f, 0.99f, 0.01f, 2);
			sub->add_option<reguler_option>("Set Day Fraction", "Set the current world time to the selected day fraction.", [] {
				queue_job([fraction = day_fraction] {
					if (!world_time::set_day_fraction(fraction))
					{
						notification::warning("World Time", "Set world time requires a loaded world hosted by this session.");
					}
					else
					{
						notification::success("World Time", "Applied the selected day fraction.");
					}
				});
			});
			sub->add_option<reguler_option>("Morning", "Set time to early morning.", [] {
				set_world_time_preset(0.2f, "morning");
			});
			sub->add_option<reguler_option>("Noon", "Set time to midday.", [] {
				set_world_time_preset(0.5f, "noon");
			});
			sub->add_option<reguler_option>("Evening", "Set time to evening.", [] {
				set_world_time_preset(0.75f, "evening");
			});
			sub->add_option<reguler_option>("Next Morning", "Advance world time to the next morning.", [] {
				queue_job([] {
					if (world_time::next_morning())
					{
						notification::success("World Time", "Advanced world time to the next morning.");
					}
					else
					{
						notification::warning("World Time", "Changing world time requires a loaded world hosted by this session.");
					}
				});
			});
		});

		canvas::add_submenu<regular_submenu>("World Preset & Modifiers", "SubmenuWorldModifiers"_hash, [](regular_submenu* sub) {
			static const std::vector<const char*> world_presets{"Default", "Custom", "Normal", "Casual", "Easy", "Hard", "Hardcore", "Immersive", "Hammer"};
			static int selected_world_preset = 0;
			sub->add_option<choose_option<const char*, int>>("Native World Preset", "Choose a preset registered by Valheim's WorldPresets enum.", &world_presets, &selected_world_preset);
			sub->add_option<reguler_option>("Apply Native World Preset", "Replace the current world's native world modifier preset.", [] {
				const std::string preset = world_presets.at(
				    static_cast<std::size_t>(std::clamp(selected_world_preset, 0, static_cast<int>(world_presets.size()) - 1)));
				queue_job([preset] {
					if (world_tools::apply_world_preset(preset))
					{
						notification::success("World Preset", std::format("Applied native '{}' preset.", preset));
					}
					else
					{
						notification::warning("World Preset", "Valheim could not apply this preset in the current session.");
					}
				});
			});

			static const std::vector<const char*> combat_values{"VeryEasy", "Easy", "Hard", "VeryHard"};
			static const std::vector<const char*> death_values{"Casual", "VeryEasy", "Easy", "Hard", "Hardcore"};
			static const std::vector<const char*> resource_values{"MuchLess", "Less", "More", "MuchMore", "Most"};
			static const std::vector<const char*> raid_values{"None", "MuchLess", "Less", "More", "MuchMore"};
			static const std::vector<const char*> portal_values{"Casual", "Hard", "VeryHard"};
			static int combat_option   = 0;
			static int death_option    = 0;
			static int resource_option = 0;
			static int raid_option     = 0;
			static int portal_option   = 0;

			auto add_modifier = [sub](const char* label, const std::string& modifier, const std::vector<const char*>& values, int& selected) {
				sub->add_option<choose_option<const char*, int>>(label, "Select a native World Modifier value.", &values, &selected);
				const auto* values_ptr = &values;
				int* selected_ptr      = &selected;
				sub->add_option<reguler_option>(std::format("Apply {} Modifier", label).c_str(), "Apply this modifier to the active world using Valheim's world modifier screen.", [modifier, values_ptr, selected_ptr] {
					if (*selected_ptr < 0 || *selected_ptr >= static_cast<int>(values_ptr->size()))
					{
						return;
					}
					const std::string option = (*values_ptr)[static_cast<std::size_t>(*selected_ptr)];
					queue_job([modifier, option] {
						if (world_tools::apply_world_modifier(modifier, option))
						{
							notification::success("World Modifiers", std::format("Applied {}: {}.", modifier, option));
						}
						else
						{
							notification::warning("World Modifiers", "Could not apply the selected modifier. Load a world hosted by this session.");
						}
					});
				});
			};

			add_modifier("Combat", "Combat", combat_values, combat_option);
			add_modifier("Death Penalty", "DeathPenalty", death_values, death_option);
			add_modifier("Resources", "Resources", resource_values, resource_option);
			add_modifier("Raids", "Raids", raid_values, raid_option);
			add_modifier("Portals", "Portals", portal_values, portal_option);
		});

		canvas::add_submenu<regular_submenu>("Exploration & Map", "SubmenuExploration"_hash, [](regular_submenu* sub) {
			static exploration::options& exp_cfg = exploration_options();
			sub->add_option<reguler_option>("Named World Bookmarks", "Save, teleport to, and remove locations stored per world.", [] {
				bookmark_manager::toggle_window();
			});
			sub->add_option<reguler_option>("Search Generated Locations", "Browse and search runtime location types with generated instances.", [] {
				view::toggle_generated_location_browser();
			});
			sub->add_option<reguler_option>("Copy Current Coordinates", "Copy your current X, Y, Z coordinates.", [] {
				queue_job([] {
					auto player_obj = unity::get_local_player();
					if (!player_obj)
					{
						notification::warning("Coordinates", "Join a world first.");
						return;
					}
					player local_player(player_obj);
					render_queue::post([text = coordinates(local_player.get_position())] {
						ImGui::SetClipboardText(text.c_str());
						notification::success("Coordinates", "Copied current coordinates.");
					});
				});
			});
			sub->add_option<reguler_option>("Add Map Pin Here", "Create a saved map icon pin at your current location.", [] {
				if (!unity::get_local_player())
				{
					notification::warning("Map Pin", "Join a world before adding a pin.");
					return;
				}
				g_input_service.show("Map Pin Name", [](const std::string& name) {
					if (name.empty())
					{
						return;
					}
					queue_job([name] {
						auto player_obj = unity::get_local_player();
						if (!player_obj)
						{
							notification::warning("Map Pin", "Join a world before adding a pin.");
							return;
						}
						const Vector3 position = unity::get_position(player_obj);
						if (minimap::get_instance().add_pin(position, 4, name))
						{
							notification::success("Map Pin", std::format("Added map pin '{}'.", name));
						}
						else
						{
							notification::warning("Map Pin", "Could not add a pin; the minimap API is unavailable.");
						}
					});
				});
			});
			sub->add_option<reguler_option>("Explore Entire Map", "Reveal all fog of war on the minimap.", [] {
				queue_job([] {
					exploration::explore_all_map();
				});
			});
			sub->add_option<reguler_option>("Reset Map Fog", "Reset map exploration fog.", [] {
				queue_job([] {
					exploration::reset_map();
				});
			});
			sub->add_option<bool_option<bool>>("Pin All World Altars",
			    "If enabled, discovers all altars in the world instead of only the closest one.",
			    &exp_cfg.discover_all);
			sub->add_option<reguler_option>("Reveal Everything", "Pin all bosses, traders, and special locations.", [] {
				queue_job([discover_all = exp_cfg.discover_all] {
					exploration::discover_everything(discover_all);
				});
			});
			sub->add_option<sub_option>("Boss Altars", "Individual progression boss altar pins.", "SubmenuBosses"_hash);
			sub->add_option<sub_option>("Traders & POIs", "Merchants and special quest locations.", "SubmenuTraders"_hash);
		});

		canvas::add_submenu<regular_submenu>("Boss Altars", "SubmenuBosses"_hash, [](regular_submenu* sub) {
			static exploration::options& exp_cfg = exploration_options();
			sub->add_option<reguler_option>("Reveal All Bosses", "Request boss altar pins, including Ashlands and Deep North. Enable Pin All World Altars for every location.", [] {
				queue_job([discover_all = exp_cfg.discover_all] {
					exploration::discover_all_bosses(discover_all);
				});
			});
			for (const auto& boss : exploration::get_boss_entries())
			{
				std::string title = std::format("Pin {} ({})", boss.display_name, boss.biome);
				sub->add_option<reguler_option>(title.c_str(), "Send discovery request for this boss.", [boss] {
					queue_job([boss, discover_all = exp_cfg.discover_all] {
						if (exploration::discover_location(boss, discover_all))
						{
							notification::success("Boss Tracker", std::format("Pin requested for {}!", boss.display_name));
						}
					});
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Traders & POIs", "SubmenuTraders"_hash, [](regular_submenu* sub) {
			static exploration::options& exp_cfg = exploration_options();
			sub->add_option<reguler_option>("Reveal All Traders", "Pin all merchants and quest locations.", [] {
				queue_job([discover_all = exp_cfg.discover_all] {
					exploration::discover_all_traders(discover_all);
				});
			});
			for (const auto& trader : exploration::get_trader_entries())
			{
				std::string title = std::format("Pin {} ({})", trader.display_name, trader.biome);
				sub->add_option<reguler_option>(title.c_str(), "Send discovery request for this trader/POI.", [trader] {
					queue_job([trader, discover_all = exp_cfg.discover_all] {
						exploration::discover_location(trader, discover_all);
						notification::success("Trader Tracker", std::format("Pin requested for {}!", trader.display_name));
					});
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Creatures & Taming", "SubmenuCreatures"_hash, [](regular_submenu* sub) {
			static animal_tools::options anim_cfg = animal_tools::get_options();
			animal_tools::set_options(anim_cfg);
			sub->add_option<reguler_option>("Open Creature Spawner", "Browse runtime creature prefabs and configure levels, count, distance, and taming.", [] {
				creature_spawner::toggle_window();
			});
			sub->add_option<reguler_option>("Open Prefab Spawner", "Search runtime prefabs, filter by category, configure position and rotation, then undo recent spawns.", [] {
				prefab_spawner::toggle_window();
			});

			sub->add_option<bool_option<bool>>("No Alert Animal",
			    "Animals never become alarmed or flee.",
			    &g_settings.self.no_animal_alert);
			sub->add_option<bool_option<bool>>("Show Animal Inspector HUD",
			    "Display creature stats, health, stars, and taming info when aiming.",
			    &anim_cfg.show_inspector);
			sub->add_option<bool_option<bool>>("Enable Tame Hotkey [T]",
			    "Press T to instantly tame whatever animal you are looking at.",
			    &anim_cfg.enable_hotkey);
			sub->add_option<bool_option<bool>>("Heal on Tame", "Restore animal to full health upon taming.", &anim_cfg.heal_on_tame);
			sub->add_option<number_option<float>>("Breeding Speed",
			    "Timing multiplier for the aimed animal's breeding checks and pregnancy duration.",
			    &anim_cfg.breeding_speed,
			    1.f,
			    100.f,
			    1.f,
			    1);
			sub->add_option<reguler_option>("Apply Breeding Speed to Aimed Animal", "Apply the selected timing multiplier to the aimed animal with a Procreation component.", [] {
				queue_job([multiplier = anim_cfg.breeding_speed] {
					animal_tools::accelerate_aimed_breeding(multiplier);
				});
			});
			sub->add_option<reguler_option>("Tame Aimed Creature", "Instantly tame the animal currently in your crosshairs.", [] {
				queue_job([] {
					animal_tools::tame_aimed_creature();
				});
			});
			sub->add_option<reguler_option>("Heal Aimed Creature", "Restore targeted creature to 100% health.", [] {
				queue_job([] {
					animal_tools::heal_aimed_creature();
				});
			});
			sub->add_option<number_option<float>>("Area Tame Radius (m)",
			    "Radius in meters for mass taming nearby creatures.",
			    &anim_cfg.area_tame_radius,
			    10.f,
			    100.f,
			    5.f,
			    0);
			sub->add_option<reguler_option>("Tame All in Radius", "Tame all wild animals within the chosen radius.", [] {
				queue_job([radius = anim_cfg.area_tame_radius] {
					animal_tools::tame_all_in_radius(radius);
				});
			});
			sub->add_option<reguler_option>("Heal Tamed Creatures in Radius", "Restore nearby tamed creatures to full health.", [] {
				queue_job([radius = anim_cfg.area_tame_radius] {
					animal_tools::heal_tamed_in_radius(radius);
				});
			});
			sub->add_option<reguler_option>("Kill Hostiles in Radius", "Deal lethal damage to nearby hostile creatures with MonsterAI.", [] {
				queue_job([radius = anim_cfg.area_tame_radius] {
					animal_tools::kill_hostiles_in_radius(radius);
				});
			});
			sub->add_option<reguler_option>(animal_tools::is_ai_frozen() ? "Resume Nearby Creature AI" : "Freeze Nearby Creature AI", "Pause or resume MonsterAI on untamed creatures within the selected radius.", [] {
				const bool frozen = !animal_tools::is_ai_frozen();
				queue_job([radius = anim_cfg.area_tame_radius, frozen] {
					animal_tools::set_ai_frozen_in_radius(radius, frozen);
				});
			});
			sub->add_option<reguler_option>("Tame All Deer (World)", "Tame all deer loaded in the world.", [] {
				queue_job([] {
					commands::get_command<command>("tamed_all_deer"_hash)->call();
				});
			});
			sub->add_option<reguler_option>("Tame All Boar (World)", "Tame all boar loaded in the world.", [] {
				queue_job([] {
					commands::get_command<command>("tamed_all_boar"_hash)->call();
				});
			});
			sub->add_option<reguler_option>("Tame All Wolves (World)", "Tame all wolves loaded in the world.", [] {
				queue_job([] {
					commands::get_command<command>("tamed_all_wolf"_hash)->call();
				});
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
				{
					panel_player = p;
				}
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
			const auto found  = std::find_if(roster.players.begin(), roster.players.end(), [](const auto& p) {
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
			{
				sub->add_option<reguler_option>("Teleport to Player", "Move near the latest available player position.", [id = p.id] {
					queue_job([id] {
						if (online_players::teleport_to(id))
						{
							notification::info("Online Players", "Teleport requested.");
						}
						else
						{
							notification::warning("Online Players", "Player or position is no longer available.");
						}
					});
				});
			}
			sub->add_option<reguler_option>("Copy Player Name", "Copy this player's name to the clipboard.", [name = p.name] {
				copy_player_text(name);
			});
			sub->add_option<reguler_option>("Copy Character ID", "Copy this character's network ID.", [id = p.id] {
				copy_player_text(id);
			});
			if (!p.account_id.empty())
			{
				sub->add_option<reguler_option>(p.platform == "Steam" ? "Copy Steam ID" : "Copy Platform Account ID", "Copy the account ID supplied by the session roster.", [id = p.account_id] {
					copy_player_text(id);
				});
			}
			if (p.position)
			{
				sub->add_option<reguler_option>("Copy Coordinates",
				    "Copy the displayed X, Y, Z coordinates.",
				    [position = *p.position] {
					    copy_player_text(coordinates(position));
				    });
			}
			sub->add_option<reguler_option>("Refresh Player List", "Update player details from the session roster.", refresh_players);
		});

		canvas::add_submenu<regular_submenu>("ESP", "SubmenuESP"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("esp_activate"_hash);
			sub->add_option<bool_option<bool>>("draw_line"_hash);
			sub->add_option<bool_option<bool>>("draw_name"_hash);
			sub->add_option<bool_option<bool>>("draw_health"_hash);
			sub->add_option<bool_option<bool>>("draw_box"_hash);
			sub->add_option<bool_option<bool>>("esp_dropped_items"_hash);
			sub->add_option<bool_option<bool>>("esp_item_stack"_hash);
			sub->add_option<bool_option<bool>>("esp_distance"_hash);
			sub->add_option<bool_option<bool>>("esp_target_marker"_hash);
			sub->add_option<reguler_option>("ESP Color Settings", "Adjust and save creature, player, and dropped item label colors.", [] {
				esp::toggle_color_settings();
			});
		});

		canvas::add_submenu<regular_submenu>("Aimbot", "SubmenuAimbot"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("aimbot"_hash);
			sub->add_option<bool_option<bool>>("tp"_hash);
			sub->add_option<bool_option<bool>>("draw_fov"_hash);
			sub->add_option<number_option<float>>("aimbot_fov"_hash);
		});

		canvas::add_submenu<regular_submenu>("Item Spawner", "SubmenuItemSpawner"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Toggle Full Spawner Window", "Open interactive ImGui window with search & 500+ items.", [] {
				item_spawner::toggle_standalone_window();
			});
			sub->add_option<reguler_option>("Rescan ObjectDB from Game", "Fetch all items, mods, and prefabs from memory.", [] {
				queue_job([] {
					item_spawner::refresh();
				});
			});
			sub->add_option<sub_option>("Quick Weapons", "Instantly spawn legendary end-game weapons.", "SubmenuQuickWeapons"_hash);
			sub->add_option<sub_option>("Quick Armor", "Instantly spawn best-in-slot armor pieces.", "SubmenuQuickArmor"_hash);
			sub->add_option<sub_option>("Quick Materials", "Spawn 50x stacks of core building metals & materials.", "SubmenuQuickMats"_hash);
			sub->add_option<sub_option>("Quick Foods", "Spawn end-game food & potion feast.", "SubmenuQuickFood"_hash);
		});

		canvas::add_submenu<regular_submenu>("Quick Weapons", "SubmenuQuickWeapons"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Mistwalker (Sword)", "Spawn Level 4 Mistwalker into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("SwordMistwalker", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Frostner (Silver Mace)", "Spawn Level 4 Frostner into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MaceSilver", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Demolisher (Sledge)", "Spawn Level 4 Demolisher into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("SledgeDemolisher", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Himmin Afl (Atgeir)", "Spawn Level 4 Lightning Atgeir into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("AtgeirHimminAfl", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Staff of Embers (Fire)", "Spawn Level 4 Fireball staff into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("StaffFireball", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Staff of Protection (Shield)", "Spawn Level 4 Shield staff into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("StaffShield", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Ashlands Bow", "Spawn Level 4 Ashlands Bow into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("BowAshlands", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Arbalest (Crossbow)", "Spawn Level 4 Arbalest into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("CrossbowArbalest", 1, 4);
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Quick Armor", "SubmenuQuickArmor"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Feather Cape", "Spawn Feather Cape (No fall damage + gliding).", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("CapeFeather", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Full Flametal Armor Set", "Spawn complete Flametal Ashlands set.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("HelmetAshlandsMedium", 1, 4);
					item_spawner::spawn_to_inventory("ArmorAshlandsMediumChest", 1, 4);
					item_spawner::spawn_to_inventory("ArmorAshlandsMediumLegs", 1, 4);
					item_spawner::spawn_to_inventory("CapeAsh", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Full Carapace Armor Set", "Spawn complete Carapace Mistlands set.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("HelmetCarapace", 1, 4);
					item_spawner::spawn_to_inventory("ArmorCarapaceChest", 1, 4);
					item_spawner::spawn_to_inventory("ArmorCarapaceLegs", 1, 4);
					item_spawner::spawn_to_inventory("ShieldCarapace", 1, 4);
				});
			});
			sub->add_option<reguler_option>("Full Eitr-Weave Mage Set", "Spawn complete Mage robe and hood.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("HelmetMage", 1, 4);
					item_spawner::spawn_to_inventory("ArmorMageChest", 1, 4);
					item_spawner::spawn_to_inventory("ArmorMageLegs", 1, 4);
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Quick Materials", "SubmenuQuickMats"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Spawn 30x Iron", "Spawn 30x Iron bars into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("Iron", 30, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 30x Silver", "Spawn 30x Silver bars into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("Silver", 30, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 30x Black Metal", "Spawn 30x Black Metal bars into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("BlackMetal", 30, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 30x Flametal", "Spawn 30x Flametal bars into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("FlametalNew", 30, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 50x Refined Eitr", "Spawn 50x Refined Eitr into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("Eitr", 50, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 20x Black Core", "Spawn 20x Black Cores for crafting.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("BlackCore", 20, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn 50x Black Marble", "Spawn 50x Black Marble into inventory.", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("BlackMarble", 50, 1);
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Quick Foods", "SubmenuQuickFood"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Spawn Meat Platter (10x)", "Top Health Food (+80 HP)", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MeatPlatter", 10, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn Misthare Supreme (10x)", "Top Balanced Food (+85 HP, +28 Stamina)", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MisthareSupreme", 10, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn Salad (10x)", "Top Stamina Food (+80 Stamina)", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("Salad", 10, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn Stuffed Mushroom (10x)", "Top Eitr Food (+75 Eitr)", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MagicallyStuffedMushroom", 10, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn Major Healing Mead (10x)", "Instant burst heal mead", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MeadHealthMajor", 10, 1);
				});
			});
			sub->add_option<reguler_option>("Spawn Lingering Stamina Mead (10x)", "+25% stamina regen for 5m", [] {
				queue_job([] {
					item_spawner::spawn_to_inventory("MeadStaminaLingering", 10, 1);
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Buffs & Effects", "SubmenuBuffs"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Open Status Effect Browser", "Search active effects, remove them, or apply effects registered in ObjectDB.", [] {
				status_effect_browser::toggle_window();
			});
			sub->add_option<reguler_option>("Apply Eitr Shield (1,000 HP)", "Grant indestructible Staff of Protection bubble dome.", [] {
				queue_job([] {
					buff_tools::apply_eitr_shield(1000.f);
				});
			});
			sub->add_option<sub_option>("Rested", "Keep the Rested buff, or apply it at a chosen comfort level.", "SubmenuBuffsRested"_hash);
			sub->add_option<sub_option>("Debuffs", "Cleanse harmful status effects automatically or now.", "SubmenuBuffsDebuffs"_hash);
			sub->add_option<sub_option>("Guardian Powers", "Discover, activate, apply, and reset guardian powers.", "SubmenuGuardianPowers"_hash);
			sub->add_option<sub_option>("Individual Boss Powers", "Select specific boss powers to toggle.", "SubmenuBossPowers"_hash);
		});

		canvas::add_submenu<regular_submenu>("Rested", "SubmenuBuffsRested"_hash, [](regular_submenu* sub) {
			static int rested_comfort = 25;
			sub->add_option<bool_option<bool>>("keep_rested"_hash);
			sub->add_option<number_option<int>>("Rested Comfort Level", "Comfort value used when applying the Rested effect.", &rested_comfort, 1, 25, 1, 0);
			sub->add_option<reguler_option>("Apply Rested Buff Now", "Apply the Rested effect using the selected comfort level.", [] {
				queue_job([comfort = rested_comfort] {
					buff_tools::apply_rested(comfort);
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Debuffs", "SubmenuBuffsDebuffs"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("auto_cleanse_debuffs"_hash);
			sub->add_option<reguler_option>("Clear All Harmful Debuffs", "Immediately cleanse all negative status effects.", [] {
				queue_job([] {
					buff_tools::clear_all_debuffs();
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Guardian Powers", "SubmenuGuardianPowers"_hash, [](regular_submenu* sub) {
			static int selected_power = 0;
			static std::vector<const char*> power_names;
			sub->add_option<bool_option<bool>>("Forsaken Power Always Ready", nullptr, &g_settings.self.forsaken_power_always_ready);
			sub->add_option<reguler_option>("Apply All Registered Guardian Effects", "Directly apply every guardian effect registered by the current game runtime.", [] {
				queue_job([] {
					buff_tools::activate_all_guardian_powers();
				});
			});
			const auto& registered_powers = buff_tools::get_boss_powers();
			power_names.clear();
			for (const auto& power : registered_powers)
			{
				power_names.push_back(power.display_name.c_str());
			}
			if (!power_names.empty())
			{
				selected_power = std::clamp(selected_power, 0, static_cast<int>(power_names.size()) - 1);
				sub->add_option<reguler_option>("Discover Guardian Powers", "Unlock every guardian power registered by the current game runtime.", [] {
					queue_job([] {
						if (!buff_tools::discover_guardian_powers())
						{
							notification::warning("Guardian Powers", "No registered powers or local player are available.");
						}
					});
				});
				sub->add_option<choose_option<const char*, int>>("Selected Guardian Power", "Choose a runtime registered power.", &power_names, &selected_power);
				const int selected_power_index = selected_power;
				sub->add_option<reguler_option>("Activate Selected Power", "Select and start the chosen power through Valheim's native animation and activation path.", [selected_power_index, powers = registered_powers] {
					if (selected_power_index < 0 || selected_power_index >= static_cast<int>(powers.size()))
					{
						return;
					}
					const auto name = powers[static_cast<std::size_t>(selected_power_index)].internal_name;
					queue_job([name] {
						if (!buff_tools::activate_selected_guardian_power(name))
						{
							notification::warning("Guardian Powers", "Power could not start. Check movement state and cooldown.");
						}
					});
				});
			}
			sub->add_option<reguler_option>("Reset Guardian Power Cooldown", "Clear the current guardian power cooldown immediately.", [] {
				queue_job([] {
					if (!buff_tools::reset_guardian_power_cooldown())
					{
						notification::warning("Guardian Power", "Player or cooldown field is unavailable.");
					}
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Individual Boss Powers", "SubmenuBossPowers"_hash, [](regular_submenu* sub) {
			for (const auto& bp : buff_tools::get_boss_powers())
			{
				std::string title = std::format("Apply {} Effect", bp.display_name);
				sub->add_option<reguler_option>(title.c_str(), bp.effect_description.c_str(), [name = bp.internal_name] {
					queue_job([name] {
						buff_tools::activate_guardian_power(name);
					});
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Ship & Sailing", "SubmenuShip"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("ship_ashlands_immune"_hash);
			sub->add_option<bool_option<bool>>("ship_no_wave_damage"_hash);
			sub->add_option<bool_slider_float_option>("ship_speed"_hash, "ship_speed_multiplier"_hash, 0.5f, 1);
			sub->add_option<bool_option<bool>>("Always Have Wind Facing Boat",
			    "Wind always blows from behind your boat.",
			    &g_settings.self.always_wind);
			sub->add_option<reguler_option>("Emergency Anchor", "Instantly brake and halt ship momentum to prevent drifting.", [] {
				queue_job([] {
					ship_tools::emergency_anchor();
				});
			});
			sub->add_option<reguler_option>("Forward Thruster Boost", "Apply a burst of forward momentum to your ship.", [] {
				queue_job([] {
					ship_tools::boost_forward(1500.f);
				});
			});
			sub->add_option<reguler_option>("Repair Controlled Ship", "Restore ship hull and pieces to 100% health.", [] {
				queue_job([] {
					ship_tools::repair_ship();
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Fishing Assistant", "SubmenuFishing"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("fishing_unbreakable_line"_hash);
			sub->add_option<bool_option<bool>>("fishing_no_stamina"_hash);
			sub->add_option<bool_option<bool>>("fishing_auto_catch"_hash);
			sub->add_option<bool_option<bool>>("Allow Picking Up Fish in Water", nullptr, &g_settings.self.allow_pickup_fish);
			sub->add_option<reguler_option>("Instant Catch Current Fish", "Reel in active bobber directly into inventory.", [] {
				queue_job([] {
					fishing_tools::instant_catch();
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Base Raids & Events", "SubmenuRaids"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("disable_raids"_hash);
			sub->add_option<reguler_option>("Stop Active Raid", "Instantly end and dismiss any ongoing raid event.", [] {
				queue_job([] {
					world_tools::stop_current_raid();
				});
			});
			for (const auto& r : world_tools::get_available_raids())
			{
				std::string title = std::format("Start '{}'", r.display_name);
				std::string desc  = std::format("Enemies: {}", r.enemies);
				sub->add_option<reguler_option>(title.c_str(), desc.c_str(), [id = r.internal_name] {
					queue_job([id] {
						world_tools::trigger_raid(id);
					});
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Death & Tombstone", "SubmenuTombstone"_hash, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Don't Drop Items when Dead", nullptr, &g_settings.self.no_drop_on_dead);
			sub->add_option<bool_option<bool>>("keep_skills_on_death"_hash);
			sub->add_option<reguler_option>("Teleport to Last Tombstone", "Instantly warp to the location where you last died.", [] {
				queue_job([] {
					world_tools::teleport_to_tombstone();
				});
			});
		});

		canvas::add_submenu<regular_submenu>("World Upgrade & Maintenance", "SubmenuUpgradeWorld"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Save World Now", "Run Valheim's native world and player profile save on the host.", [] {
				queue_job([] {
					if (world_time::save_world())
					{
						notification::success("World Save", "Native world and player profile save requested.");
					}
					else
					{
						notification::warning("World Save", "This action requires a loaded world hosted by this session.");
					}
				});
			});
			sub->add_option<reguler_option>("Open Global Key Editor", "Search active world keys and request keys to be set or removed.", [] {
				global_key_editor::toggle_window();
			});
			sub->add_option<reguler_option>("Generate Missing Locations (genloc)", "Distribute all newly added update locations (Bog Witch, Hildir, Ashlands, Mistlands dungeons, Frost Caves, Tar Pits) into unexplored zones.", [] {
				queue_job([] {
					world_tools::generate_missing_locations();
				});
			});
			sub->add_option<reguler_option>("Upgrade WorldGen Version (v2)", "Upgrade old world generation version to v2 (Modern rules & biome distribution).", [] {
				queue_job([] {
					world_tools::upgrade_worldgen_version();
				});
			});
			sub->add_option<reguler_option>("Optimize Old Terrain (optterrain)", "Convert pre-Hearth&Home terrain modifications in loaded area to the modern system (fixes FPS drops).", [] {
				queue_job([] {
					world_tools::upgrade_terrain();
				});
			});
			sub->add_option<reguler_option>("Spawn Bog Witch Camp Nearby", "Force spawn the Bog Witch merchant camp in front of you (saves permanently to world).", [] {
				queue_job([] {
					world_tools::force_spawn_location("BogWitch_Camp");
				});
			});
			sub->add_option<reguler_option>("Spawn Hildir Camp Nearby", "Force spawn Hildir's merchant camp in front of you (saves permanently to world).", [] {
				queue_job([] {
					world_tools::force_spawn_location("Hildir_camp");
				});
			});
		});

		canvas::add_submenu<regular_submenu>("Remote Merchant", "SubmenuTrader"_hash, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Open Haldor Store (Black Forest)", "Trade for Megingjord, Fishing Rod, Bait, Ymir Flesh, etc.", [] {
				queue_job([] {
					world_tools::open_trader_gui("Haldor");
				});
			});
			sub->add_option<reguler_option>("Open Hildir Store (Meadows)", "Trade for clothes, hats, sparklers, fireworks, etc.", [] {
				queue_job([] {
					world_tools::open_trader_gui("Hildir");
				});
			});
			sub->add_option<reguler_option>("Open Bog Witch Store (Swamp)", "Trade for potions, alchemy ingredients, feast materials, scythe, etc.", [] {
				queue_job([] {
					world_tools::open_trader_gui("BogWitch");
				});
			});
		});
	}
}
