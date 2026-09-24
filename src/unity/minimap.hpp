#pragma once
#include "class/vector.hpp"
#include "mono/mono.hpp"
#include <string_view>

namespace big
{
	class minimap
	{
	private:
		MonoObject* m_minimap{};

	public:
		minimap(MonoObject* obj = nullptr);
		~minimap() noexcept;

		MonoObject* get_object() const
		{
			return m_minimap;
		}
		static minimap get_instance();

		void explore_all();
		void reset();
		bool discover_location(const Vector3& pos, int pin_type, std::string_view pin_name, bool show_map = false);
		bool is_open();

		bool operator==(const minimap& other) const
		{
			return m_minimap == other.m_minimap;
		}
		operator bool() const
		{
			return m_minimap != nullptr;
		}
	};
}
