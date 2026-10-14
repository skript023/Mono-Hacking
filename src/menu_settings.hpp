#pragma once
#include "file_manager.hpp"
#include "logger/logger.hpp"
#include "imgui.h"

#include "class/vector.hpp"
#include "utility/utility.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace big
{
	class menu_settings
	{
		nlohmann::json default_options;
		nlohmann::json options;

	public:
		void attempt_save();

		bool load();
		void* m_camera_obj = nullptr;

	private:
		std::filesystem::path get_settings_file_path() const;

		bool deep_compare(nlohmann::json& current_settings, const nlohmann::json& default_settings, bool compare_value = false);
		bool save();

		bool write_default_config();

	public:
		struct self
		{
			bool flying{false};
			bool noclip{false};
			bool preserve_ammunition{false};
			bool is_teleportable{false};
			bool open_all_recipe_and_free_craft{false};
			bool no_drop_on_dead{false};
			bool forsaken_power_always_ready{false};
			bool enable_max_hp{false};
			bool enable_max_stam{false};
			bool no_animal_alert{false};
			float max_hp{25.f};
			float max_stam{50.f};

			bool is_wet{false};
			bool always_wind{false};
			bool no_weight{false};
			bool no_encumbrance{false};
			bool maintain_health{false};
			bool maintain_stamina{false};
			bool maintain_eitr{false};
			bool incoming_invulnerability{false};
			bool no_fall_damage{false};
			bool prevent_freezing{false};
			bool prevent_cold{false};
			float health_regen_rate{0.f};
			float food_duration_multiplier{1.f};
			float swim_speed_multiplier{1.f};
			float jump_force_multiplier{1.f};
			bool allow_pickup_fish{false};
			bool map_click_teleport{true};
			bool inventory_top_first{true};
			bool infinite_stability{false};
			bool ward_bypass{false};
			bool instant_bow_draw{false};

			bool god_mode{false};
			bool ghost_mode{false};
			bool infinite_durability{false};
			bool free_crafting{false};
			float crafting_speed_multiplier{1.f};
			bool free_material_costs{false};
			bool ignore_crafting_station{false};
			bool ignore_building_workbench{false};
			bool unlock_runtime_recipes{false};
			bool unlock_materials{false};
			bool unlock_building_pieces{false};
			bool station_level_override{false};
			int crafting_station_level{1};
			bool one_hit_resource{false};
			bool hostile_one_hit{false};
			float damage_multiplier{1.f};
			float incoming_damage_multiplier{1.f};
			float melee_stamina_multiplier{1.f};
			float ranged_stamina_multiplier{1.f};
			float attack_eitr_multiplier{1.f};
			float backstab_multiplier{1.f};
			float parry_force_multiplier{1.f};
			float parry_window_multiplier{1.f};
			float attack_animation_speed{1.f};
			float push_force_multiplier{1.f};
			float stagger_multiplier{1.f};
			float movement_speed_multiplier{1.f};
			float speed_multiplier{1.f};
			float run_speed_multiplier{1.f};
			float jog_speed_multiplier{1.f};
			float pickup_range{2.f};
			int max_food_slots{3};

			bool equipment_slots_enabled{true};
			bool equipment_slots_window{false};
			bool equipment_slots_native_ui{true};
			float equipment_slots_native_x{-620.f};
			float equipment_slots_native_y{0.f};
			bool equipment_slots_hotkeys{true};
			bool multi_utility_enabled{true};

			bool auto_parry{false};
			bool always_parry{false};
			float camera_fov{65.f};
			float camera_zoom_sensitivity{10.f};
			float camera_smoothness{0.1f};
			float freecam_speed{20.f};
			float freecam_smoothness{0.f};
			bool camera_shake_enabled{true};
			float build_distance{5.f};
			float build_rotation_step{22.5f};
			float build_place_delay{0.4f};
			float build_remove_delay{0.25f};
			bool allow_rotated_piece_overlap{false};
			std::vector<std::string> item_spawner_favorites;
			std::vector<std::string> item_spawner_recent;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(self, is_teleportable, open_all_recipe_and_free_craft, no_drop_on_dead, map_click_teleport, inventory_top_first, infinite_stability, ward_bypass, instant_bow_draw, god_mode, ghost_mode, infinite_durability, free_crafting, one_hit_resource, damage_multiplier, speed_multiplier, pickup_range, max_food_slots, equipment_slots_enabled, equipment_slots_window, equipment_slots_native_ui, equipment_slots_native_x, equipment_slots_native_y, equipment_slots_hotkeys, multi_utility_enabled, auto_parry, always_parry)
		} self;

		struct window
		{
			ImU32 color        = 3357612055;
			float gui_scale    = 1.f;
			float transparency = 1.f;
			Color custom_background{8, 12, 20, 255};
			Color custom_panel{15, 24, 38, 255};
			Color custom_text{225, 235, 250, 255};

			ImFont* font_title     = nullptr;
			ImFont* font_sub_title = nullptr;
			ImFont* font_small     = nullptr;
			ImFont* font_icon      = nullptr;

			int layout         = 0; // 0: List, 1: Window
			int theme          = 4; // Emerald, Violet, Ocean
			int banner         = 0; // 0: Quantum Green, 1: Astra Standard
			bool switched_view = true;
			bool mouse_active  = false;
			bool input         = false;
			bool censor        = true;
			bool overlay       = false;
			bool js_eval       = false;

			uint32_t open_key{VK_INSERT};
			uint32_t back_key{VK_NUMPAD0};
			uint32_t enter_key{VK_NUMPAD5};
			uint32_t up_key{VK_NUMPAD8};
			uint32_t down_key{VK_NUMPAD2};
			uint32_t left_key{VK_NUMPAD4};
			uint32_t right_key{VK_NUMPAD6};
			uint32_t left_tab_key{VK_NUMPAD7};
			uint32_t right_tab_key{VK_NUMPAD9};

			Vector2 m_pos                 = {25.f, 25.f};
			float m_width                 = 450.f;
			std::size_t m_option_per_page = 11;
			bool m_sounds                 = true;
			// Input
			std::int32_t m_open_delay       = 200;
			std::int32_t m_back_delay       = 300;
			std::int32_t m_enter_delay      = 300;
			std::int32_t m_vectical_delay   = 120;
			std::int32_t m_horizontal_delay = 120;
			std::int32_t m_tabbar_switch    = 200;

			// Submenu bar
			Color m_submenu_bar_background_color{24, 24, 24, 255};
			Color m_submenu_bar_text_color{153, 153, 155, 255};

			//Options
			Color m_toggle_on_color{255, 255, 255, 200};
			Color m_toggle_off_color{0, 0, 0, 150};
			Color m_submenu_rect_color{255, 255, 255, 180};
			Color m_option_selected_text_color{0, 0, 0, 255};
			Color m_option_unselected_text_color{255, 255, 255, 255};
			Color m_option_selected_background_color{255, 255, 255, 200};
			Color m_option_unselected_background_color{0, 0, 0, 150};

			//Footer
			Color m_footer_background_color{0, 0, 0, 150};

			//Tabbar
			Color m_tab_unselected_text_color{255, 255, 255, 255};
			Color m_tab_selected_text_color{0, 0, 0, 255};
			Color m_tab_unselected_color{0, 0, 0, 150};
			Color m_tab_selected_color{255, 255, 255, 200};

			//Description
			Color description_background_color{0, 0, 0, 150};
			Color description_text_color{255, 255, 255, 255};

			//Sliderbar
			Color m_slider_track_color = {255, 255, 255, 255};
			Color m_slider_knob_color  = {0, 0, 0, 255};


			NLOHMANN_DEFINE_TYPE_INTRUSIVE(window, layout, theme, banner, color, gui_scale, transparency, custom_background, custom_panel, custom_text, mouse_active, censor, overlay, open_key, back_key, enter_key, up_key, down_key, left_key, right_key, left_tab_key, right_tab_key, m_pos, m_width, m_option_per_page, m_sounds, m_open_delay, m_enter_delay, m_back_delay, m_horizontal_delay, m_vectical_delay, m_tabbar_switch, m_submenu_bar_background_color, m_submenu_bar_text_color, m_toggle_on_color, m_toggle_off_color, m_submenu_rect_color, m_option_selected_background_color, m_option_unselected_background_color, m_option_selected_text_color, m_option_unselected_text_color, m_footer_background_color, m_tab_selected_color, m_tab_unselected_color, m_tab_selected_text_color, m_tab_unselected_text_color, description_background_color, description_text_color)
		} window;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(menu_settings, window, self)

		void serialize_extra_options(nlohmann::json& json) const
		{
			json["self"]["forsaken_power_always_ready"] = self.forsaken_power_always_ready;
			json["self"]["enable_max_hp"]               = self.enable_max_hp;
			json["self"]["enable_max_stam"]             = self.enable_max_stam;
			json["self"]["no_animal_alert"]             = self.no_animal_alert;
			json["self"]["max_hp"]                      = self.max_hp;
			json["self"]["max_stam"]                    = self.max_stam;
			json["self"]["is_wet"]                      = self.is_wet;
			json["self"]["always_wind"]                 = self.always_wind;
			json["self"]["no_weight"]                   = self.no_weight;
			json["self"]["no_encumbrance"]              = self.no_encumbrance;
			json["self"]["flying"]                      = self.flying;
			json["self"]["speed_multiplier"]            = self.speed_multiplier;
			json["self"]["movement_speed_multiplier"]   = self.movement_speed_multiplier;
			json["self"]["food_duration_multiplier"]    = self.food_duration_multiplier;
			json["self"]["backstab_multiplier"]         = self.backstab_multiplier;
			json["self"]["parry_force_multiplier"]      = self.parry_force_multiplier;
			json["self"]["parry_window_multiplier"]     = self.parry_window_multiplier;
			json["self"]["allow_pickup_fish"]           = self.allow_pickup_fish;
			json["self"]["noclip"]                      = self.noclip;
			json["self"]["preserve_ammunition"]         = self.preserve_ammunition;
			json["self"]["ignore_building_workbench"]   = self.ignore_building_workbench;
			json["self"]["attack_animation_speed"]      = self.attack_animation_speed;
			json["self"]["maintain_health"]             = self.maintain_health;
			json["self"]["maintain_stamina"]            = self.maintain_stamina;
			json["self"]["maintain_eitr"]               = self.maintain_eitr;
			json["self"]["incoming_invulnerability"]    = self.incoming_invulnerability;
			json["self"]["no_fall_damage"]              = self.no_fall_damage;
			json["self"]["prevent_freezing"]            = self.prevent_freezing;
			json["self"]["prevent_cold"]                = self.prevent_cold;
			json["self"]["health_regen_rate"]           = self.health_regen_rate;
			json["self"]["swim_speed_multiplier"]       = self.swim_speed_multiplier;
			json["self"]["jump_force_multiplier"]       = self.jump_force_multiplier;
			json["self"]["crafting_speed_multiplier"]   = self.crafting_speed_multiplier;
			json["self"]["free_material_costs"]         = self.free_material_costs;
			json["self"]["ignore_crafting_station"]     = self.ignore_crafting_station;
			json["self"]["unlock_runtime_recipes"]      = self.unlock_runtime_recipes;
			json["self"]["unlock_materials"]            = self.unlock_materials;
			json["self"]["unlock_building_pieces"]      = self.unlock_building_pieces;
			json["self"]["station_level_override"]      = self.station_level_override;
			json["self"]["crafting_station_level"]      = self.crafting_station_level;
			json["self"]["hostile_one_hit"]             = self.hostile_one_hit;
			json["self"]["incoming_damage_multiplier"]  = self.incoming_damage_multiplier;
			json["self"]["melee_stamina_multiplier"]    = self.melee_stamina_multiplier;
			json["self"]["ranged_stamina_multiplier"]   = self.ranged_stamina_multiplier;
			json["self"]["attack_eitr_multiplier"]      = self.attack_eitr_multiplier;
			json["self"]["push_force_multiplier"]       = self.push_force_multiplier;
			json["self"]["stagger_multiplier"]          = self.stagger_multiplier;
			json["self"]["run_speed_multiplier"]        = self.run_speed_multiplier;
			json["self"]["jog_speed_multiplier"]        = self.jog_speed_multiplier;
			json["self"]["max_food_slots"]              = self.max_food_slots;
			json["self"]["equipment_slots_enabled"]     = self.equipment_slots_enabled;
			json["self"]["equipment_slots_window"]      = self.equipment_slots_window;
			json["self"]["equipment_slots_native_ui"]   = self.equipment_slots_native_ui;
			json["self"]["equipment_slots_native_x"]    = self.equipment_slots_native_x;
			json["self"]["equipment_slots_native_y"]    = self.equipment_slots_native_y;
			json["self"]["equipment_slots_hotkeys"]     = self.equipment_slots_hotkeys;
			json["self"]["multi_utility_enabled"]       = self.multi_utility_enabled;
			json["self"]["camera_fov"]                  = self.camera_fov;
			json["self"]["camera_zoom_sensitivity"]     = self.camera_zoom_sensitivity;
			json["self"]["camera_smoothness"]           = self.camera_smoothness;
			json["self"]["freecam_speed"]               = self.freecam_speed;
			json["self"]["freecam_smoothness"]          = self.freecam_smoothness;
			json["self"]["camera_shake_enabled"]        = self.camera_shake_enabled;
			json["self"]["build_distance"]              = self.build_distance;
			json["self"]["build_rotation_step"]         = self.build_rotation_step;
			json["self"]["build_place_delay"]           = self.build_place_delay;
			json["self"]["build_remove_delay"]          = self.build_remove_delay;
			json["self"]["allow_rotated_piece_overlap"] = self.allow_rotated_piece_overlap;
			json["self"]["item_spawner_favorites"]      = self.item_spawner_favorites;
			json["self"]["item_spawner_recent"]         = self.item_spawner_recent;
		}

		void deserialize_extra_options(const nlohmann::json& json)
		{
			if (auto it = json.find("self"); it != json.end() && it->is_object())
			{
				self.forsaken_power_always_ready = it->value("forsaken_power_always_ready", self.forsaken_power_always_ready);
				self.enable_max_hp             = it->value("enable_max_hp", self.enable_max_hp);
				self.enable_max_stam           = it->value("enable_max_stam", self.enable_max_stam);
				self.no_animal_alert           = it->value("no_animal_alert", self.no_animal_alert);
				self.max_hp                    = it->value("max_hp", self.max_hp);
				self.max_stam                  = it->value("max_stam", self.max_stam);
				self.is_wet                    = it->value("is_wet", self.is_wet);
				self.always_wind               = it->value("always_wind", self.always_wind);
				self.no_weight                 = it->value("no_weight", self.no_weight);
				self.no_encumbrance            = it->value("no_encumbrance", self.no_encumbrance);
				self.flying                    = it->value("flying", self.flying);
				self.speed_multiplier          = it->value("speed_multiplier", self.speed_multiplier);
				self.movement_speed_multiplier = it->value("movement_speed_multiplier", self.movement_speed_multiplier);
				self.food_duration_multiplier  = it->value("food_duration_multiplier", self.food_duration_multiplier);
				self.backstab_multiplier       = it->value("backstab_multiplier", self.backstab_multiplier);
				self.parry_force_multiplier    = it->value("parry_force_multiplier", self.parry_force_multiplier);
				self.parry_window_multiplier   = it->value("parry_window_multiplier", self.parry_window_multiplier);
				self.allow_pickup_fish         = it->value("allow_pickup_fish", self.allow_pickup_fish);
				self.noclip                    = it->value("noclip", self.noclip);
				self.preserve_ammunition       = it->value("preserve_ammunition", self.preserve_ammunition);
				self.ignore_building_workbench = it->value("ignore_building_workbench", self.ignore_building_workbench);
				self.attack_animation_speed    = it->value("attack_animation_speed", self.attack_animation_speed);
				self.maintain_health           = it->value("maintain_health", self.maintain_health);
				self.maintain_stamina          = it->value("maintain_stamina", self.maintain_stamina);
				self.maintain_eitr             = it->value("maintain_eitr", self.maintain_eitr);
				self.incoming_invulnerability  = it->value("incoming_invulnerability", self.incoming_invulnerability);
				self.no_fall_damage            = it->value("no_fall_damage", self.no_fall_damage);
				self.prevent_freezing          = it->value("prevent_freezing", self.prevent_freezing);
				self.prevent_cold              = it->value("prevent_cold", self.prevent_cold);
				self.health_regen_rate         = it->value("health_regen_rate", self.health_regen_rate);
				self.swim_speed_multiplier     = it->value("swim_speed_multiplier", self.swim_speed_multiplier);
				self.jump_force_multiplier     = it->value("jump_force_multiplier", self.jump_force_multiplier);
				self.crafting_speed_multiplier = it->value("crafting_speed_multiplier", self.crafting_speed_multiplier);
				self.free_material_costs       = it->value("free_material_costs", self.free_material_costs);
				self.ignore_crafting_station   = it->value("ignore_crafting_station", self.ignore_crafting_station);
				self.unlock_runtime_recipes    = it->value("unlock_runtime_recipes", self.unlock_runtime_recipes);
				self.unlock_materials          = it->value("unlock_materials", self.unlock_materials);
				self.unlock_building_pieces    = it->value("unlock_building_pieces", self.unlock_building_pieces);
				self.station_level_override    = it->value("station_level_override", self.station_level_override);
				self.crafting_station_level    = it->value("crafting_station_level", self.crafting_station_level);
				self.hostile_one_hit           = it->value("hostile_one_hit", self.hostile_one_hit);
				self.incoming_damage_multiplier = it->value("incoming_damage_multiplier", self.incoming_damage_multiplier);
				self.melee_stamina_multiplier  = it->value("melee_stamina_multiplier", self.melee_stamina_multiplier);
				self.ranged_stamina_multiplier = it->value("ranged_stamina_multiplier", self.ranged_stamina_multiplier);
				self.attack_eitr_multiplier    = it->value("attack_eitr_multiplier", self.attack_eitr_multiplier);
				self.push_force_multiplier     = it->value("push_force_multiplier", self.push_force_multiplier);
				self.stagger_multiplier        = it->value("stagger_multiplier", self.stagger_multiplier);
				self.run_speed_multiplier      = it->value("run_speed_multiplier", self.run_speed_multiplier);
				self.jog_speed_multiplier      = it->value("jog_speed_multiplier", self.jog_speed_multiplier);
				self.max_food_slots            = it->value("max_food_slots", self.max_food_slots);
				self.equipment_slots_enabled   = it->value("equipment_slots_enabled", self.equipment_slots_enabled);
				self.equipment_slots_window    = it->value("equipment_slots_window", self.equipment_slots_window);
				self.equipment_slots_native_ui = it->value("equipment_slots_native_ui", self.equipment_slots_native_ui);
				self.equipment_slots_native_x  = it->value("equipment_slots_native_x", self.equipment_slots_native_x);
				self.equipment_slots_native_y  = it->value("equipment_slots_native_y", self.equipment_slots_native_y);
				self.equipment_slots_hotkeys   = it->value("equipment_slots_hotkeys", self.equipment_slots_hotkeys);
				self.multi_utility_enabled     = it->value("multi_utility_enabled", self.multi_utility_enabled);
				self.camera_fov                = it->value("camera_fov", self.camera_fov);
				self.camera_zoom_sensitivity   = it->value("camera_zoom_sensitivity", self.camera_zoom_sensitivity);
				self.camera_smoothness         = it->value("camera_smoothness", self.camera_smoothness);
				self.freecam_speed             = it->value("freecam_speed", self.freecam_speed);
				self.freecam_smoothness        = it->value("freecam_smoothness", self.freecam_smoothness);
				self.camera_shake_enabled      = it->value("camera_shake_enabled", self.camera_shake_enabled);
				self.build_distance            = it->value("build_distance", self.build_distance);
				self.build_rotation_step       = it->value("build_rotation_step", self.build_rotation_step);
				self.build_place_delay         = it->value("build_place_delay", self.build_place_delay);
				self.build_remove_delay        = it->value("build_remove_delay", self.build_remove_delay);
				self.allow_rotated_piece_overlap = it->value("allow_rotated_piece_overlap", self.allow_rotated_piece_overlap);
				self.item_spawner_favorites = it->value("item_spawner_favorites", self.item_spawner_favorites);
				self.item_spawner_recent    = it->value("item_spawner_recent", self.item_spawner_recent);
			}
		}
	};

	inline menu_settings g_settings{};
}
