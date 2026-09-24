#pragma once
#include "minimap.hpp"
#include "zone_system.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace big
{
	class exploration
	{
	public:
		struct location_entry
		{
			std::vector<std::string> candidate_names;
			std::string display_name;
			std::string pin_name;
			int pin_type;
			std::string biome;
		};

		struct options
		{
			bool discover_all{false};
		};

		static void set_options(const options& opt);
		static options get_options();

		static void explore_all_map();
		static void reset_map();

		static bool discover_location(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all);
		static bool discover_location(const location_entry& entry, bool discover_all);
		static void discover_all_bosses(bool discover_all);
		static void discover_all_traders(bool discover_all);
		static void discover_everything(bool discover_all);

		static const std::vector<location_entry>& get_boss_entries();
		static const std::vector<location_entry>& get_trader_entries();
	};
}
