#include "exploration.hpp"
#include "minimap.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"
#include "zone_system.hpp"
#include <format>

namespace big
{
	namespace
	{
		exploration::options g_options;

		const std::vector<exploration::location_entry> boss_catalog = {
		    {{"Eikthyrnir"}, "Eikthyr", "Eikthyr", 9, "Meadows"},
		    {{"GDKing"}, "The Elder", "The Elder", 9, "Black Forest"},
		    {{"Bonemass"}, "Bonemass", "Bonemass", 9, "Swamp"},
		    {{"Dragonqueen"}, "Moder", "Moder", 9, "Mountain"},
		    {{"GoblinKing"}, "Yagluth", "Yagluth", 9, "Plains"},
		    {{"Mistlands_DvergrBossEntrance1", "Mistlands_DvergrBossEntrance", "Mistlands_BossEntrance", "Mistlands_DvergrTownEntrance1", "SeekerQueen"}, "The Queen", "The Queen", 9, "Mistlands"},
		    {{"AshlandsBoss", "Ashlands_Boss", "AshlandsBoss_Altar", "Altar_EmeraldFlame", "Fader"}, "Fader", "Fader", 9, "Ashlands"}};

		const std::vector<exploration::location_entry> trader_catalog = {
		    {{"Vendor_BlackForest"}, "Haldor (Trader)", "Haldor", 2, "Black Forest"},
		    {{"Hildir_camp"}, "Hildir (Merchant)", "Hildir", 14, "Meadows"},
		    {{"BogWitch_Camp", "BogWitch"}, "Bog Witch (Alchemist)", "Bog Witch", 2, "Swamp"},
		    {{"Hildir_crypt"}, "Smouldering Tomb (Crypt)", "Hildir Crypt", 15, "Black Forest"},
		    {{"Hildir_cave"}, "Howling Cavern (Cave)", "Hildir Cave", 15, "Mountain"},
		    {{"Hildir_tower"}, "Sealed Tower", "Hildir Tower", 16, "Plains"}};

		bool try_local_discover(const std::string& name, const std::string& pin_name, int pin_type)
		{
			auto zs = zone_system::get_instance();
			if (!zs)
				return false;

			auto player = unity::get_local_player();
			Vector3 my_pos = player ? unity::get_position(player) : Vector3{0.f, 0.f, 0.f};

			Vector3 closest_pos{};
			if (zs.find_closest_location(name, my_pos, closest_pos))
			{
				auto map = minimap::get_instance();
				if (map)
				{
					map.discover_location(closest_pos, pin_type, pin_name);
					return true;
				}
			}

			return false;
		}
	}

	void exploration::set_options(const options& opt)
	{
		g_options = opt;
	}

	exploration::options exploration::get_options()
	{
		return g_options;
	}

	const std::vector<exploration::location_entry>& exploration::get_boss_entries()
	{
		return boss_catalog;
	}

	const std::vector<exploration::location_entry>& exploration::get_trader_entries()
	{
		return trader_catalog;
	}

	void exploration::explore_all_map()
	{
		auto map = minimap::get_instance();
		if (map)
			map.explore_all();
		else
			unity::explore_all_map();

		notification::success("Map Exploration", "All map fog revealed across the world!");
	}

	void exploration::reset_map()
	{
		auto map = minimap::get_instance();
		if (map)
			map.reset();
		else
			unity::reset_map();

		notification::info("Map Exploration", "Minimap fog of war reset.");
	}

	bool exploration::discover_location(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all)
	{
		try_local_discover(std::string(name), std::string(pin_name), pin_type);

		auto game = unity::get_game();
		if (!game)
		{
			notification::warning("Exploration", "Game instance not found. Join a world first!");
			return false;
		}

		unity::discover_closest_location(name, pin_name, pin_type, false, discover_all);
		return true;
	}

	bool exploration::discover_location(const location_entry& entry, bool discover_all)
	{
		for (const auto& candidate : entry.candidate_names)
		{
			try_local_discover(candidate, entry.pin_name, entry.pin_type);
		}

		auto game = unity::get_game();
		if (!game)
		{
			notification::warning("Exploration", "Game instance not found. Join a world first!");
			return false;
		}

		for (const auto& candidate : entry.candidate_names)
		{
			unity::discover_closest_location(candidate, entry.pin_name, entry.pin_type, false, discover_all);
		}
		return true;
	}

	void exploration::discover_all_bosses(bool discover_all)
	{
		int count = 0;
		for (const auto& boss : boss_catalog)
		{
			if (discover_location(boss, discover_all))
				count++;
		}
		if (count > 0)
			notification::success("Boss Tracker", std::format("Discovery requested for all {} bosses (Mode: {})!", count, discover_all ? "All World Altars" : "Closest Altar"));
	}

	void exploration::discover_all_traders(bool discover_all)
	{
		int count = 0;
		for (const auto& trader : trader_catalog)
		{
			if (discover_location(trader, discover_all))
				count++;
		}
		if (count > 0)
			notification::success("Trader Tracker", std::format("Discovery requested for {} traders & quest POIs!", count));
	}

	void exploration::discover_everything(bool discover_all)
	{
		discover_all_bosses(discover_all);
		discover_all_traders(discover_all);
	}
}
