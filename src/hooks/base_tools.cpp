#include "hooking.hpp"
#include "unity/base_tools.hpp"

namespace big
{
	double hooks::smelter_delta(MonoObject* object)
	{
		return detour_base::get_original<smelter_delta>()(object) * base_tools::multiplier(object, base_tools::get_options().smelter_speed);
	}
	double hooks::fermenter_time(MonoObject* object)
	{
		return detour_base::get_original<fermenter_time>()(object) * base_tools::multiplier(object, base_tools::get_options().fermenter_speed);
	}
	float hooks::hive_delta(MonoObject* object)
	{
		return detour_base::get_original<hive_delta>()(object) * base_tools::multiplier(object, base_tools::get_options().honey_speed);
	}
	float hooks::plant_grow_time(MonoObject* object)
	{
		return detour_base::get_original<plant_grow_time>()(object) / base_tools::multiplier(object, base_tools::get_options().plant_speed);
	}
	MonoString* hooks::environment_override(MonoObject* object)
	{
		auto cfg = base_tools::get_options();
		if (!cfg.weather.empty()) return mono::to_mono_string(cfg.weather);
		return detour_base::get_original<environment_override>()(object);
	}
	void hooks::environment_update(MonoObject* object)
	{
		auto cfg = base_tools::get_options();
		if (!cfg.lock_daylight)
		{
			detour_base::get_original<environment_update>()(object);
			return;
		}
		struct restore_time
		{
			MonoObject* object;
			bool enabled;
			float time;
			~restore_time()
			{
				mono::set_field_value<"EnvMan", "m_debugTimeOfDay">(object, enabled);
				mono::set_field_value<"EnvMan", "m_debugTime">(object, time);
			}
		} restore{object, mono::get_field_value<"EnvMan", "m_debugTimeOfDay", bool>(object), mono::get_field_value<"EnvMan", "m_debugTime", float>(object)};
		mono::set_field_value<"EnvMan", "m_debugTimeOfDay">(object, true);
		mono::set_field_value<"EnvMan", "m_debugTime">(object, cfg.daylight);
		detour_base::get_original<environment_update>()(object);
	}
	void hooks::container_stack_response(MonoObject* object, int64_t sender, bool granted)
	{
		struct restore_filter
		{
			bool previous = base_tools::filtering_stack;
			~restore_filter() { base_tools::filtering_stack = previous; }
		} restore;
		base_tools::filtering_stack = base_tools::begin_stack_response(object) && granted;
		detour_base::get_original<container_stack_response>()(object, sender, granted);
	}
	bool hooks::inventory_add_stack_item(MonoObject* inventory, MonoObject* item)
	{
		if (base_tools::filtering_stack && base_tools::protected_item(item)) return false;
		return detour_base::get_original<inventory_add_stack_item>()(inventory, item);
	}
}
