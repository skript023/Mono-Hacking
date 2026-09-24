#pragma once
#include "class/vector.hpp"
#include "mono/metadata/object-forward.h"
#include <string>
#include <vector>

namespace big
{
	class base_tools
	{
	public:
		struct options
		{
			float radius = 30.f;
			bool keep_food = true, keep_ammo = true, keep_hotbar = true;
			bool hotkey = false, damaged_markers = false;
			float smelter_speed = 1.f, fermenter_speed = 1.f, honey_speed = 1.f, plant_speed = 1.f;
			bool lock_daylight = false;
			float daylight = .5f;
			std::string weather;
		};
		struct row
		{
			std::string name, detail;
			Vector3 position{};
		};
		struct marker
		{
			std::string text;
			Vector3 screen{};
		};
		struct snapshot
		{
			bool ready = false;
			std::vector<row> production, plants, buildings, comfort;
			std::vector<std::string> weather;
			int comfort_level = 0;
			bool sheltered = false;
		};
		static options get_options();
		static void set_options(options value);
		static snapshot get_snapshot();
		static std::vector<marker> get_markers();
		static void request_scan();
		static void update();
		static void hotkey_tick();
		static void quick_stack();
		static void repair_nearby();
		static void grow_nearby();
		static float multiplier(MonoObject* object, float value);
		static bool begin_stack_response(MonoObject* container);
		static bool protected_item(MonoObject* item);
		static inline thread_local bool filtering_stack = false;
	};
}
