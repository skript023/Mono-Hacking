#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
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
		static options get_options()
		{
			return instance().get_options_impl();
		}
		static void set_options(options value)
		{
			return instance().set_options_impl(value);
		}
		static snapshot get_snapshot()
		{
			return instance().get_snapshot_impl();
		}
		static std::vector<marker> get_markers()
		{
			return instance().get_markers_impl();
		}
		static void request_scan()
		{
			return instance().request_scan_impl();
		}
		static void update()
		{
			return instance().update_impl();
		}
		static void hotkey_tick()
		{
			return instance().hotkey_tick_impl();
		}
		static void quick_stack()
		{
			return instance().quick_stack_impl();
		}
		static void repair_nearby()
		{
			return instance().repair_nearby_impl();
		}
		static void grow_nearby()
		{
			return instance().grow_nearby_impl();
		}
		static float multiplier(MonoObject* object, float value)
		{
			return instance().multiplier_impl(object, value);
		}
		static bool begin_stack_response(MonoObject* container)
		{
			return instance().begin_stack_response_impl(container);
		}
		static bool protected_item(MonoObject* item)
		{
			return instance().protected_item_impl(item);
		}
		static inline thread_local bool filtering_stack = false;

	private:
		base_tools() = default;
		base_tools(const base_tools&) = delete;
		base_tools& operator=(const base_tools&) = delete;
		static base_tools& instance()
		{
			static base_tools value;
			return value;
		}

		options get_options_impl();
		void set_options_impl(options value);
		snapshot get_snapshot_impl();
		std::vector<marker> get_markers_impl();
		void request_scan_impl();
		void update_impl();
		void hotkey_tick_impl();
		void quick_stack_impl();
		void repair_nearby_impl();
		void grow_nearby_impl();
		float multiplier_impl(MonoObject* object, float value);
		bool begin_stack_response_impl(MonoObject* container);
		bool protected_item_impl(MonoObject* item);

		std::mutex mutex;
		options config;
		snapshot data;
		std::atomic_bool scan_requested = false;
		std::chrono::steady_clock::time_point scanned;
		std::vector<marker> markers;
		std::chrono::steady_clock::time_point projected;
		std::unordered_map<int, std::chrono::steady_clock::time_point> pending_stacks;
	};
}
