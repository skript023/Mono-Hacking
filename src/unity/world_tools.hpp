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
		static void update_raid_system()
		{
			return instance().update_raid_system_impl();
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
		static bool open_trader_gui()
		{
			return instance().open_trader_gui_impl();
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
		void update_raid_system_impl();
		bool has_death_point_impl();
		Vector3 get_death_point_impl();
		bool teleport_to_tombstone_impl();
		bool loot_nearby_tombstone_impl(float radius);
		bool open_trader_gui_impl();
	};
}
