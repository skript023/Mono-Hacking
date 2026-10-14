#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace big
{
	class creature_spawner
	{
	public:
		struct entry
		{
			std::string prefab;
			std::string name;
		};

		static void toggle_window();
		static void draw_window();
		static void refresh();
		static bool spawn(const std::string& prefab, int count, int level, float distance, int placement, bool tamed, const std::string& pet_name = {});

	private:
		static void scan_worker();
		static void spawn_worker(std::string prefab, int count, int level, float distance, int placement, bool tamed, std::string pet_name);
	};
}
