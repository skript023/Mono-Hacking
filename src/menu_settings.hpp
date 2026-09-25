#pragma once
#include "logger/logger.hpp"
#include "imgui.h"

#include "class/vector.hpp"
#include "utility/utility.hpp"

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
		const char* settings_location = "\\Scarlet Nexus Trainer\\menu_settings.json";

		bool deep_compare(nlohmann::json& current_settings, const nlohmann::json& default_settings, bool compare_value = false);
		bool save();

		bool write_default_config();
	public:
		struct self
		{
			bool flying{ false };
			bool is_teleportable{ false };
			bool open_all_recipe_and_free_craft{ false };
			bool no_drop_on_dead{ false };
			bool forsaken_power_always_ready{ false };
			bool enable_max_hp{ false };
			bool enable_max_stam{ false };
			bool no_animal_alert{ false };
			float max_hp{ 25.f };
			float max_stam{ 50.f };

			bool is_wet{ false };
			bool always_wind{ false };
			bool no_weight{ false };
			bool allow_pickup_fish{ false };
			bool map_click_teleport{ true };
			bool inventory_top_first{ true };
			bool infinite_stability{ false };
			bool ward_bypass{ false };
			bool instant_bow_draw{ false };

			bool god_mode{ false };
			bool ghost_mode{ false };
			bool infinite_durability{ false };
			bool free_crafting{ false };
			bool one_hit_resource{ false };
			float damage_multiplier{ 1.f };
			float speed_multiplier{ 1.f };
			float pickup_range{ 2.f };
			int max_food_slots{ 3 };

			// Ship & Sailing
			bool ship_ashlands_immune{ true };
			bool ship_no_wave_damage{ true };
			float ship_speed_multiplier{ 1.f };

			// Buffs & Effects
			bool auto_cleanse_debuffs{ false };
			bool keep_rested{ false };

			// Fishing
			bool fishing_instant_bite{ false };
			bool fishing_unbreakable_line{ true };
			bool fishing_no_stamina{ true };
			bool fishing_auto_catch{ false };

			// World & Events
			bool disable_raids{ false };
			bool keep_skills_on_death{ true };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(self, is_teleportable, open_all_recipe_and_free_craft, no_drop_on_dead, map_click_teleport, inventory_top_first, infinite_stability, ward_bypass, instant_bow_draw, god_mode, ghost_mode, infinite_durability, free_crafting, one_hit_resource, damage_multiplier, speed_multiplier, pickup_range, max_food_slots, ship_ashlands_immune, ship_no_wave_damage, ship_speed_multiplier, auto_cleanse_debuffs, keep_rested, fishing_instant_bite, fishing_unbreakable_line, fishing_no_stamina, fishing_auto_catch, disable_raids, keep_skills_on_death)
		} self;

		struct window
		{
			ImU32 color = 3357612055;
			float gui_scale = 1.f;
			float transparency = 1.f;
			Color custom_background{ 8, 12, 20, 255 };
			Color custom_panel{ 15, 24, 38, 255 };
			Color custom_text{ 225, 235, 250, 255 };

			ImFont* font_title = nullptr;
			ImFont* font_sub_title = nullptr;
			ImFont* font_small = nullptr;
			ImFont* font_icon = nullptr;

			int layout = 0; // 0: List, 1: Window
			int theme = 4; // Emerald, Violet, Ocean
			bool switched_view = true;
			bool mouse_active = false;
			bool input = false;
			bool censor = true;
			bool overlay = false;
			bool js_eval = false;

			uint32_t open_key{ VK_INSERT };
			uint32_t back_key{ VK_NUMPAD0 };
			uint32_t enter_key{ VK_NUMPAD5 };
			uint32_t up_key{ VK_NUMPAD8 };
			uint32_t down_key{ VK_NUMPAD2 };
			uint32_t left_key{ VK_NUMPAD4 };
			uint32_t right_key{ VK_NUMPAD6 };
			uint32_t left_tab_key{ VK_NUMPAD7 };
			uint32_t right_tab_key{ VK_NUMPAD9 };

			Vector2 m_pos = { 25.f, 25.f };
			float m_width = 450.f;
			std::size_t m_option_per_page = 11;
			bool m_sounds = true;
			// Input
			std::int32_t m_open_delay = 200;
			std::int32_t m_back_delay = 300;
			std::int32_t m_enter_delay = 300;
			std::int32_t m_vectical_delay = 120;
			std::int32_t m_horizontal_delay = 120;
			std::int32_t m_tabbar_switch = 200;

			// Submenu bar
			Color m_submenu_bar_background_color{ 24, 24, 24, 255 };
			Color m_submenu_bar_text_color{ 153, 153, 155, 255 };

			//Options
			Color m_toggle_on_color{ 255, 255, 255, 200 };
			Color m_toggle_off_color{ 0, 0, 0, 150 };
			Color m_submenu_rect_color{ 255, 255, 255, 180 };
			Color m_option_selected_text_color{ 0, 0, 0, 255 };
			Color m_option_unselected_text_color{ 255, 255, 255, 255 };
			Color m_option_selected_background_color{ 255, 255, 255, 200 };
			Color m_option_unselected_background_color{ 0, 0, 0, 150 };

			//Footer
			Color m_footer_background_color{ 0, 0, 0, 150 };

			//Tabbar
			Color m_tab_unselected_text_color{ 255, 255, 255, 255 };
			Color m_tab_selected_text_color{ 0, 0, 0, 255 };
			Color m_tab_unselected_color{ 0, 0, 0, 150 };
			Color m_tab_selected_color{ 255, 255, 255, 200 };

			//Description
			Color description_background_color{ 0, 0, 0, 150 };
			Color description_text_color{ 255, 255, 255, 255 };

			//Sliderbar
			Color m_slider_track_color = { 255, 255, 255, 255 };
			Color m_slider_knob_color = { 0, 0, 0, 255 };


			NLOHMANN_DEFINE_TYPE_INTRUSIVE(window,
				layout,
				theme,
				color,
				gui_scale,
				transparency,
				custom_background,
				custom_panel,
				custom_text,
				mouse_active,
				censor,
				overlay,
				open_key,
				back_key,
				enter_key,
				up_key,
				down_key,
				left_key,
				right_key,
				left_tab_key,
				right_tab_key,
				m_pos,
				m_width,
				m_option_per_page,
				m_sounds,
				m_open_delay,
				m_enter_delay,
				m_back_delay,
				m_horizontal_delay,
				m_vectical_delay,
				m_tabbar_switch,
				m_submenu_bar_background_color,
				m_submenu_bar_text_color,
				m_toggle_on_color,
				m_toggle_off_color,
				m_submenu_rect_color,
				m_option_selected_background_color,
				m_option_unselected_background_color,
				m_option_selected_text_color,
				m_option_unselected_text_color,
				m_footer_background_color,
				m_tab_selected_color,
				m_tab_unselected_color,
				m_tab_selected_text_color,
				m_tab_unselected_text_color,
				description_background_color,
				description_text_color
			)
		} window;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(menu_settings, window, self)
	};

	inline menu_settings g_settings{};
}
