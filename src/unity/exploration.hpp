#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace big::exploration
{
	struct location_entry
	{
		std::string location_name;
		std::string display_name;
		std::string pin_name;
		int pin_type;
		std::string biome;
	};

	struct options
	{
		bool discover_all{false};
	};

	void set_options(const options& opt);
	options get_options();

	void explore_all_map();
	void reset_map();

	bool discover_location(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all);
	void discover_all_bosses(bool discover_all);
	void discover_all_traders(bool discover_all);
	void discover_everything(bool discover_all);

	const std::vector<location_entry>& get_boss_entries();
	const std::vector<location_entry>& get_trader_entries();
}
