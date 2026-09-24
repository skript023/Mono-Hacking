#pragma once
#include "class/vector.hpp"
#include "mono/metadata/object-forward.h"
#include <string>
#include <vector>

namespace big::base_tools
{
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
	options get_options();
	void set_options(options value);
	snapshot get_snapshot();
	std::vector<marker> get_markers();
	void request_scan();
	void update();
	void hotkey_tick();
	void quick_stack();
	void repair_nearby();
	void grow_nearby();
	float multiplier(MonoObject* object, float value);
	bool begin_stack_response(MonoObject* container);
	bool protected_item(MonoObject* item);
	inline thread_local bool filtering_stack = false;
}
