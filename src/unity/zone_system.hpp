#pragma once
#include "class/vector.hpp"
#include "mono/mono.hpp"
#include <string_view>
#include <string>
#include <vector>

namespace big
{
	struct generated_location
	{
		std::string name;
		std::string prefab_name;
		Vector3 position{};
		float distance{};
	};

	class zone_system
	{
	private:
		static zone_system& instance()
		{
			static zone_system value;
			return value;
		}
		zone_system get_instance_impl();
		MonoObject* m_zone_system{};

	public:
		zone_system(MonoObject* obj = nullptr);
		~zone_system() noexcept;

		MonoObject* get_object() const
		{
			return m_zone_system;
		}
		static zone_system get_instance()
		{
			return instance().get_instance_impl();
		}

		bool find_closest_location(std::string_view name, const Vector3& my_pos, Vector3& out_pos);
		std::vector<generated_location> get_generated_locations(const Vector3& my_pos);
		std::vector<std::string> get_global_keys();
		bool set_global_key(const std::string& name);
		bool remove_global_key(const std::string& name);

		bool operator==(const zone_system& other) const
		{
			return m_zone_system == other.m_zone_system;
		}
		operator bool() const
		{
			return m_zone_system != nullptr;
		}
	};
}
