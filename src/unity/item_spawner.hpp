#pragma once
#include "mono/mono.hpp"
#include <string>
#include <vector>

namespace big
{
	class item_spawner
	{
	public:
		struct item_entry
		{
			std::string prefab_name;
			std::string display_name;
			std::string category;
			int max_stack{1};
			int max_quality{1};
		};

		static void initialize();
		static void refresh();
		static bool is_refreshing();
		static const std::vector<item_entry>& get_items();
		static std::vector<std::string> get_categories();
		static bool spawn_to_inventory(const std::string& prefab_name, int amount, int quality);
		static bool spawn_in_world(const std::string& prefab_name, int amount, int level = 1);
		static void draw_menu_ui();
		static void draw_standalone_window();
		static void toggle_standalone_window();
		static bool is_standalone_window_open();
	};
}
