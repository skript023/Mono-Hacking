#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"

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
		MonoObject* m_camera{};
		MonoMethod* m_project{};
		struct viewport_rect
		{
			float x, y, width, height;
		} m_rect{0, 0, 1, 1};
	};
}
