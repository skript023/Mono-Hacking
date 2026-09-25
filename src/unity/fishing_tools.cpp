#include "fishing_tools.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

namespace big
{
	namespace
	{
		std::vector<MonoObject*> get_floats()
		{
			auto float_class = mono::get_class("FishingFloat", "assembly_valheim");
			if (!float_class)
				return {};

			auto instances_field = mono::get_field(float_class, "m_allInstances");
			if (!instances_field)
				return {};

			void* static_data = mono::get_static_field_data(float_class);
			if (!static_data)
				return {};

			uint32_t offset = mono::get_field_offset(instances_field);
			MonoObject* list_obj = *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(static_data) + offset);
			if (!list_obj)
				return {};

			return unity::list_to_vector(list_obj);
		}
	}

	void fishing_tools::update_impl()
	{
		auto floats = get_floats();
		if (floats.empty())
			return;

		auto float_class = mono::get_class("FishingFloat", "assembly_valheim");
		if (!float_class)
			return;

		static auto f_break_dist = mono::get_field(float_class, "m_breakDistance");
		static auto f_max_dist = mono::get_field(float_class, "m_maxDistance");
		static auto f_pull_stam = mono::get_field(float_class, "m_pullStaminaUse");
		static auto f_hook_stam = mono::get_field(float_class, "m_hookedStaminaPerSec");
		static auto f_pull_speed = mono::get_field(float_class, "m_pullLineSpeed");
		static auto f_line_len = mono::get_field(float_class, "m_lineLength");
		static auto get_catch_method = mono::get_method("FishingFloat", "GetCatch", 0, "assembly_valheim");

		for (auto* fl : floats)
		{
			if (!fl)
				continue;

			if (g_settings.self.fishing_unbreakable_line)
			{
				float max_dist = 99999.f;
				if (f_break_dist)
					mono::set_field_value(fl, f_break_dist, &max_dist);
				if (f_max_dist)
					mono::set_field_value(fl, f_max_dist, &max_dist);
			}

			if (g_settings.self.fishing_no_stamina)
			{
				float zero = 0.f;
				if (f_pull_stam)
					mono::set_field_value(fl, f_pull_stam, &zero);
				if (f_hook_stam)
					mono::set_field_value(fl, f_hook_stam, &zero);
			}

			// Fast reel speed
			float fast_reel = 25.f;
			if (f_pull_speed)
				mono::set_field_value(fl, f_pull_speed, &fast_reel);

			if (g_settings.self.fishing_auto_catch && get_catch_method && f_line_len)
			{
				auto catch_fish = mono::invoke_method(get_catch_method, fl, nullptr);
				if (catch_fish)
				{
					// Reel to catch threshold (< 0.5m)
					float short_len = 0.2f;
					mono::set_field_value(fl, f_line_len, &short_len);
				}
			}
		}
	}

	fishing_tools::fishing_status fishing_tools::get_status_impl()
	{
		fishing_status st;
		auto floats = get_floats();
		if (floats.empty())
			return st;

		st.rod_active = true;
		auto fl = floats.front();
		auto float_class = mono::get_class("FishingFloat", "assembly_valheim");
		if (float_class && fl)
		{
			static auto f_line_len = mono::get_field(float_class, "m_lineLength");
			if (f_line_len)
				mono::get_field_value(fl, f_line_len, &st.line_length);

			static auto get_catch_method = mono::get_method("FishingFloat", "GetCatch", 0, "assembly_valheim");
			if (get_catch_method)
			{
				auto fish = mono::invoke_method(get_catch_method, fl, nullptr);
				st.fish_hooked = (fish != nullptr);
			}
		}

		return st;
	}

	void fishing_tools::instant_catch_impl()
	{
		auto floats = get_floats();
		if (floats.empty())
		{
			notification::warning("Fishing Assistant", "No active fishing float found.");
			return;
		}

		auto fl = floats.front();
		auto float_class = mono::get_class("FishingFloat", "assembly_valheim");
		if (!float_class || !fl)
			return;

		static auto f_line_len = mono::get_field(float_class, "m_lineLength");
		static auto get_catch_method = mono::get_method("FishingFloat", "GetCatch", 0, "assembly_valheim");

		if (f_line_len)
		{
			float snap_zero = 0.1f;
			mono::set_field_value(fl, f_line_len, &snap_zero);
			notification::success("Fishing Assistant", "Reeled in instantly!");
		}
	}
}
