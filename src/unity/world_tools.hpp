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
		static const std::vector<raid_info>& get_available_raids();
		static std::string get_current_raid_name();
		static void trigger_raid(const std::string& internal_name);
		static void stop_current_raid();
		static void update_raid_system();

		// Death & Tombstone
		static bool has_death_point();
		static Vector3 get_death_point();
		static bool teleport_to_tombstone();
		static bool loot_nearby_tombstone(float radius = 100.f);

		// Remote Trader
		static bool open_trader_gui();
	};
}

