#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
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

		static void update()
		{
			return instance().update_impl();
		}
		static snapshot get_snapshot()
		{
			return instance().get_snapshot_impl();
		}
		static bool teleport_to(const std::string& id)
		{
			return instance().teleport_to_impl(id);
		}

	private:
		online_players() = default;
		online_players(const online_players&) = delete;
		online_players& operator=(const online_players&) = delete;
		static online_players& instance()
		{
			static online_players value;
			return value;
		}

		void update_impl();
		snapshot get_snapshot_impl();
		bool teleport_to_impl(const std::string& id);

		std::mutex mutex;
		snapshot current;
		std::chrono::steady_clock::time_point last_update;
	};
}
