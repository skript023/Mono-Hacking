#include "zone_system.hpp"
#include "utility/unity.hpp"

namespace big
{
	zone_system::zone_system(MonoObject* obj) :
	    m_zone_system(obj)
	{
	}

	zone_system::~zone_system() noexcept
	{
		m_zone_system = nullptr;
	}

	zone_system zone_system::get_instance()
	{
		static auto get_inst = mono::get_method("ZoneSystem", "get_instance", 0, "assembly_valheim");
		if (get_inst)
		{
			auto res = mono::invoke_method(get_inst, nullptr, nullptr);
			if (res)
				return zone_system(res);
		}
		auto klass = mono::get_class("ZoneSystem", "assembly_valheim");
		if (!klass)
			return zone_system(nullptr);

		auto field = mono::get_field(klass, "s_instance");
		if (!field)
			field = mono::get_field(klass, "m_instance");
		if (field)
		{
			auto static_data = mono::get_static_field_data(klass);
			if (static_data)
			{
				uint32_t offset = mono::get_field_offset(field);
				auto ptr_addr = (void*)((uintptr_t)static_data + offset);
				if (ptr_addr && *(MonoObject**)ptr_addr)
					return zone_system(*(MonoObject**)ptr_addr);
			}
		}
		return zone_system(unity::get_zone_system());
	}

	bool zone_system::find_closest_location(std::string_view name, const Vector3& my_pos, Vector3& out_pos)
	{
		if (!m_zone_system)
			return false;

		static auto find_m = mono::get_method("ZoneSystem", "FindClosestLocation", 3, "assembly_valheim");
		if (!find_m)
			return false;

		MonoString* mono_name = mono::to_mono_string(std::string(name));

		struct LocationInstanceRaw
		{
			void* m_location;
			Vector3 m_position;
			bool m_placed;
			char pad[7];
		};
		LocationInstanceRaw closest{};

		void* args[3] = {
		    mono_name,
		    const_cast<Vector3*>(&my_pos),
		    &closest};

		auto res = mono::invoke_method(find_m, m_zone_system, args);
		if (res && *static_cast<bool*>(mono::object_unbox(res)))
		{
			out_pos = closest.m_position;
			return true;
		}
		return false;
	}
}
