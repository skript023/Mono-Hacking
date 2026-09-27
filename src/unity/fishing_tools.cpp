#include "fishing_tools.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

namespace big
{
	std::vector<MonoObject*> fishing_tools::get_floats_impl()
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
