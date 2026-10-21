#pragma once
#include <chrono>
#include <string>
#include <vector>

namespace big::cheat_status
{
	enum class action { inspect, character, world, nearby };
	struct snapshot
	{
		bool ready = false;
		bool host = false;
		bool character_ready = false;
		bool character_marked = false;
		bool native_bypass = false;
		bool mod_bypass = false;
		int inventory_items = 0;
		int chest_items = 0;
		int chests = 0;
		int uninspected_chests = 0;
		int marked_objects = 0;
		int marked_creatures = 0;
		int marked_pieces = 0;
		int marked_stations = 0;
		int marked_flags = 0;
		int skipped_objects = 0;
		float radius = 30.f;
		std::vector<std::string> world_keys;
		std::string error;
		std::string result;
		std::chrono::steady_clock::time_point sampled{};
	};
	snapshot get_snapshot();
	void request(action operation, float radius);
}
