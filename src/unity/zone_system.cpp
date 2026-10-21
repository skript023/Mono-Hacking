#include "zone_system.hpp"
#include "utility/unity.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

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

	zone_system zone_system::get_instance_impl()
	{
		static auto get_inst = mono::get_method("ZoneSystem", "get_instance", 0, "assembly_valheim");
		if (get_inst)
		{
			auto res = mono::invoke_method(get_inst, nullptr, nullptr);
			if (res)
			{
				return zone_system(res);
			}
		}
		auto klass = mono::get_class("ZoneSystem", "assembly_valheim");
		if (!klass)
		{
			return zone_system(nullptr);
		}

		auto field = mono::get_field(klass, "s_instance");
		if (!field)
		{
			field = mono::get_field(klass, "m_instance");
		}
		if (field)
		{
			auto static_data = mono::get_static_field_data(klass);
			if (static_data)
			{
				uint32_t offset = mono::get_field_offset(field);
				auto ptr_addr   = (void*)((uintptr_t)static_data + offset);
				if (ptr_addr && *(MonoObject**)ptr_addr)
				{
					return zone_system(*(MonoObject**)ptr_addr);
				}
			}
		}
		return zone_system(unity::get_zone_system());
	}

	bool zone_system::find_closest_location(std::string_view name, const Vector3& my_pos, Vector3& out_pos)
	{
		if (!m_zone_system)
		{
			return false;
		}

		static auto find_m = mono::get_method("ZoneSystem", "FindClosestLocation", 3, "assembly_valheim");
		if (!find_m)
		{
			return false;
		}

		MonoString* mono_name = mono::to_mono_string(std::string(name));

		struct LocationInstanceRaw
		{
			void* m_location;
			Vector3 m_position;
			bool m_placed;
			char pad[7];
		};
		LocationInstanceRaw closest{};

		void* args[3] = {mono_name, const_cast<Vector3*>(&my_pos), &closest};

		auto res = mono::invoke_method(find_m, m_zone_system, args);
		if (res && *static_cast<bool*>(mono::object_unbox(res)))
		{
			out_pos = closest.m_position;
			return true;
		}
		return false;
	}

	std::vector<generated_location> zone_system::get_generated_locations(const Vector3& my_pos)
	{
		std::vector<generated_location> result;
		if (!m_zone_system)
		{
			return result;
		}

		static auto get_location_list = mono::get_method("ZoneSystem", "GetLocationList", 0, "assembly_valheim");
		auto locations = get_location_list ? mono::invoke_method(get_location_list, m_zone_system, nullptr) : nullptr;
		auto collection_class = locations ? mono::object_get_class(locations) : nullptr;
		auto get_enumerator = collection_class ? mono::class_get_method_from_name(collection_class, "GetEnumerator", 0) : nullptr;
		if (!get_enumerator)
		{
			return result;
		}

		auto enumerator       = mono::invoke_method(get_enumerator, locations, nullptr);
		auto enumerator_class = enumerator ? mono::object_get_class(enumerator) : nullptr;
		auto move_next = enumerator_class ? mono::class_get_method_from_name(enumerator_class, "MoveNext", 0) : nullptr;
		auto get_current = enumerator_class ? mono::class_get_method_from_name(enumerator_class, "get_Current", 0) : nullptr;
		if (!enumerator || !move_next || !get_current)
		{
			return result;
		}

		while (true)
		{
			auto has_next = mono::invoke_method(move_next, enumerator, nullptr);
			if (!has_next || !*reinterpret_cast<bool*>(mono::object_unbox(has_next)))
			{
				break;
			}

			auto instance = mono::invoke_method(get_current, enumerator, nullptr);
			if (!instance)
			{
				continue;
			}
			auto instance_class        = mono::object_get_class(instance);
			static auto location_field = instance_class ? mono::get_field(instance_class, "m_location") : nullptr;
			static auto position_field = instance_class ? mono::get_field(instance_class, "m_position") : nullptr;
			if (!location_field || !position_field)
			{
				continue;
			}

			MonoObject* location{};
			Vector3 position{};
			mono::get_field_value(instance, location_field, &location);
			mono::get_field_value(instance, position_field, &position);
			if (!location)
			{
				continue;
			}
			auto location_class = mono::object_get_class(location);
			auto prefab_field   = location_class ? mono::get_field(location_class, "m_prefabName") : nullptr;
			auto name_field     = location_class ? mono::get_field(location_class, "m_name") : nullptr;
			MonoString* prefab_name_string{};
			MonoString* name_string{};
			if (prefab_field)
			{
				mono::get_field_value(location, prefab_field, &prefab_name_string);
			}
			if (name_field)
			{
				mono::get_field_value(location, name_field, &name_string);
			}
			const std::string prefab_name = mono::from_mono_string(prefab_name_string);
			if (prefab_name.empty())
			{
				continue;
			}

			const float dx = position.x - my_pos.x;
			const float dy = position.y - my_pos.y;
			const float dz = position.z - my_pos.z;
			result.push_back({mono::from_mono_string(name_string), prefab_name, position, std::sqrt(dx * dx + dy * dy + dz * dz)});
		}

		std::sort(result.begin(), result.end(), [](const generated_location& lhs, const generated_location& rhs) {
			return lhs.distance < rhs.distance;
		});
		return result;
	}

	std::vector<std::string> zone_system::get_global_keys()
	{
		std::vector<std::string> result;
		if (!m_zone_system)
		{
			return result;
		}
		static auto get_keys = mono::get_method("ZoneSystem", "GetGlobalKeys", 0, "assembly_valheim");
		auto list            = get_keys ? mono::invoke_method(get_keys, m_zone_system, nullptr) : nullptr;
		for (auto* key : unity::list_to_vector(list))
		{
			if (key)
			{
				result.push_back(mono::from_mono_string(reinterpret_cast<MonoString*>(key)));
			}
		}
		return result;
	}

	bool zone_system::set_global_key(const std::string& name)
	{
		if (!m_zone_system || name.empty())
		{
			return false;
		}
		static auto set_key = mono::get_method_exact("ZoneSystem", "SetGlobalKey", {"System.String"}, "assembly_valheim");
		if (!set_key)
		{
			return false;
		}
		void* args[1]         = {mono::to_mono_string(name)};
		MonoObject* exception = nullptr;
		mono::invoke_method(set_key, m_zone_system, args, &exception);
		return exception == nullptr;
	}

	bool zone_system::remove_global_key(const std::string& name)
	{
		if (!m_zone_system || name.empty())
		{
			return false;
		}
		static auto remove_key = mono::get_method_exact("ZoneSystem", "RemoveGlobalKey", {"System.String"}, "assembly_valheim");
		if (!remove_key)
		{
			return false;
		}
		void* args[1]         = {mono::to_mono_string(name)};
		MonoObject* exception = nullptr;
		mono::invoke_method(remove_key, m_zone_system, args, &exception);
		return exception == nullptr;
	}
}
