#pragma once
#include "class/vector.hpp"
#include <optional>
#include <string>
#include <vector>

namespace big
{
	class online_players
	{
	public:
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

		static void update();
		static snapshot get_snapshot();
		static bool teleport_to(const std::string& id);
	};
}
