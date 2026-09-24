#pragma once
#include "class/vector.hpp"
#include "mono/mono.hpp"
#include <string_view>

namespace big
{
	class zone_system
	{
	private:
		MonoObject* m_zone_system{};

	public:
		zone_system(MonoObject* obj = nullptr);
		~zone_system() noexcept;

		MonoObject* get_object() const
		{
			return m_zone_system;
		}
		static zone_system get_instance();

		bool find_closest_location(std::string_view name, const Vector3& my_pos, Vector3& out_pos);

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
