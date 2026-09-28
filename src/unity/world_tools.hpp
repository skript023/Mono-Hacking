#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"
#include <string>
#include <vector>

namespace big
{
	class world_tools
	{
	public:
		struct raid_info
		{
			std::string internal_name;
			std::string display_name;
			std::string enemies;
		};

		// Raids & Events
		static const std::vector<raid_info>& get_available_raids()
		{
			return instance().get_available_raids_impl();
		}
		static std::string get_current_raid_name()
		{
			return instance().get_current_raid_name_impl();
		}
		static void trigger_raid(const std::string& internal_name)
		{
			return instance().trigger_raid_impl(internal_name);
		}
		static void stop_current_raid()
		{
			return instance().stop_current_raid_impl();
		}

		// Death & Tombstone
		static bool has_death_point()
		{
			return instance().has_death_point_impl();
		}
		static Vector3 get_death_point()
		{
			return instance().get_death_point_impl();
		}
		static bool teleport_to_tombstone()
		{
			return instance().teleport_to_tombstone_impl();
		}
		static bool loot_nearby_tombstone(float radius = 100.f)
		{
			return instance().loot_nearby_tombstone_impl(radius);
		}

		// Remote Trader
		static bool open_trader_gui(const std::string& trader_name = "Haldor")
		{
			return instance().open_trader_gui_impl(trader_name);
		}

		// Repair Anywhere
		static bool repair_all_inventory()
		{
			return instance().repair_all_inventory_impl();
		}

		// World Upgrade & Old World Maintenance
		static bool generate_missing_locations()
		{
			return instance().generate_missing_locations_impl();
		}
		static bool upgrade_terrain()
		{
			return instance().upgrade_terrain_impl();
		}
		static bool upgrade_worldgen_version()
		{
			return instance().upgrade_worldgen_version_impl();
		}
		static bool force_spawn_location(const std::string& location_name)
		{
			return instance().force_spawn_location_impl(location_name);
		}

	private:
		world_tools() = default;
		world_tools(const world_tools&) = delete;
		world_tools& operator=(const world_tools&) = delete;
		static world_tools& instance()
		{
			static world_tools value;
			return value;
		}

		const std::vector<raid_info>& get_available_raids_impl();
		std::string get_current_raid_name_impl();
		void trigger_raid_impl(const std::string& internal_name);
		void stop_current_raid_impl();
		bool has_death_point_impl();
		Vector3 get_death_point_impl();
		bool teleport_to_tombstone_impl();
		bool loot_nearby_tombstone_impl(float radius);
		bool open_trader_gui_impl(const std::string& trader_name);
		bool repair_all_inventory_impl();
		bool generate_missing_locations_impl();
		bool upgrade_terrain_impl();
		bool upgrade_worldgen_version_impl();
		bool force_spawn_location_impl(const std::string& location_name);
	};
}
