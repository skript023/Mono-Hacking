#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"

#include <array>

namespace big
{
	class esp_projection
	{
	public:
		static bool begin_frame()
		{
			return instance().begin_frame_impl();
		}
		static bool project(const Vector3& world, Vector3& viewport)
		{
			return instance().project_impl(world, viewport);
		}

	private:
		static esp_projection& instance()
		{
			static esp_projection value;
			return value;
		}
		esp_projection() = default;
		esp_projection(const esp_projection&) = delete;
		esp_projection& operator=(const esp_projection&) = delete;
		bool begin_frame_impl();
		bool project_impl(const Vector3& world, Vector3& viewport);
		bool m_ready{};
		std::array<float, 16> m_world_to_camera{};
		std::array<float, 16> m_projection{};
		struct viewport_rect
		{
			float x, y, width, height;
		} m_rect{0, 0, 1, 1};
	};
}
