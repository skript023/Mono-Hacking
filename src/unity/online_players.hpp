#pragma once
#include "class/vector.hpp"
#include <optional>
#include <string>
#include <vector>

namespace big::online_players
{
	struct entry
	{
		std::string id;
		std::string name;
		std::string platform;
		std::string account_id;
		bool local = false;
		bool loaded = false;
		bool public_position = false;
		std::optional<Vector3> position;
		std::optional<float> distance;
		std::optional<float> health;
		std::optional<float> max_health;
	};

	struct snapshot
	{
		bool ready = false;
		std::vector<entry> players;
	};

	// Called on the game thread; snapshots contain no managed pointers.
	void update();
	snapshot get_snapshot();
	bool teleport_to(const std::string& id);
}
