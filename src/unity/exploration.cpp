#include "exploration.hpp"
#include "utility/unity.hpp"
#include "notification/notification_service.hpp"
#include <format>

namespace big::exploration
{
	namespace
	{
		options g_options;

		const std::vector<location_entry> boss_catalog = {
		    {"Eikthyrnir", "Eikthyr", "Eikthyr", 9, "Meadows"},
		    {"GDKing", "The Elder", "The Elder", 9, "Black Forest"},
		    {"Bonemass", "Bonemass", "Bonemass", 9, "Swamp"},
		    {"Dragonqueen", "Moder", "Moder", 9, "Mountain"},
		    {"GoblinKing", "Yagluth", "Yagluth", 9, "Plains"},
		    {"SeekerQueen", "The Queen", "The Queen", 9, "Mistlands"},
		    {"Fader", "Fader", "Fader", 9, "Ashlands"}};

		const std::vector<location_entry> trader_catalog = {
		    {"Vendor_BlackForest", "Haldor (Trader)", "Haldor", 2, "Black Forest"},
		    {"Hildir_camp", "Hildir (Merchant)", "Hildir", 14, "Meadows"},
		    {"BogWitch_Camp", "Bog Witch (Alchemist)", "Bog Witch", 2, "Swamp"},
		    {"Hildir_crypt", "Smouldering Tomb (Crypt)", "Hildir Crypt", 15, "Black Forest"},
		    {"Hildir_cave", "Howling Cavern (Cave)", "Hildir Cave", 15, "Mountain"},
		    {"Hildir_tower", "Sealed Tower", "Hildir Tower", 16, "Plains"}};
	}

	void set_options(const options& opt)
	{
		g_options = opt;
	}

	options get_options()
	{
		return g_options;
	}

	const std::vector<location_entry>& get_boss_entries()
	{
		return boss_catalog;
	}

	const std::vector<location_entry>& get_trader_entries()
	{
		return trader_catalog;
	}

	void explore_all_map()
	{
		unity::explore_all_map();
		notification::success("Map Exploration", "All map fog revealed across the world!");
	}

	void reset_map()
	{
		unity::reset_map();
		notification::info("Map Exploration", "Minimap fog of war reset.");
	}

	bool discover_location(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all)
	{
		auto game = unity::get_game();
		if (!game)
		{
			notification::warning("Exploration", "Game instance not found. Join a world first!");
			return false;
		}

		unity::discover_closest_location(name, pin_name, pin_type, false, discover_all);
		return true;
	}

	void discover_all_bosses(bool discover_all)
	{
		int count = 0;
		for (const auto& boss : boss_catalog)
		{
			if (discover_location(boss.location_name, boss.pin_name, boss.pin_type, discover_all))
				count++;
		}
		if (count > 0)
			notification::success("Boss Tracker", std::format("Discovery requested for all {} bosses (Mode: {})!", count, discover_all ? "All World Altars" : "Closest Altar"));
	}

	void discover_all_traders(bool discover_all)
	{
		int count = 0;
		for (const auto& trader : trader_catalog)
		{
			if (discover_location(trader.location_name, trader.pin_name, trader.pin_type, discover_all))
				count++;
		}
		if (count > 0)
			notification::success("Trader Tracker", std::format("Discovery requested for {} traders & quest POIs!", count));
	}

	void discover_everything(bool discover_all)
	{
		discover_all_bosses(discover_all);
		discover_all_traders(discover_all);
	}
}
