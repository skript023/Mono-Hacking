#include "esp_projection.hpp"

namespace big
{
	template<typename T>
	static bool read_value(MonoMethod* getter, MonoObject* target, T& out)
	{
		MonoObject* exception = nullptr;
		auto boxed            = mono::invoke_method(getter, target, nullptr, &exception);
		if (!boxed || exception)
		{
			return false;
		}
		auto value = mono::object_unbox(boxed);
		if (!value)
		{
			return false;
		}
		out = *static_cast<T*>(value);
		return true;
	}

	bool esp_projection::begin_frame_impl()
	{
		m_ready                  = false;
		static auto get_instance = mono::get_method("GameCamera", "get_instance", 0, "assembly_valheim");
		static auto camera_field = mono::get_field("GameCamera", "m_camera", "assembly_valheim");
		static auto get_world_to_camera = mono::get_method("Camera", "get_worldToCameraMatrix", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_projection = mono::get_method("Camera", "get_projectionMatrix", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_rect = mono::get_method("Camera", "get_rect", 0, "UnityEngine.CoreModule", "UnityEngine");
		if (!get_instance || !camera_field || !get_world_to_camera || !get_projection || !get_rect)
		{
			return false;
		}
		MonoObject* exception = nullptr;
		auto game_camera      = mono::invoke_method(get_instance, nullptr, nullptr, &exception);
		if (!game_camera || exception)
		{
			return false;
		}
		MonoObject* camera = nullptr;
		mono::get_field_value(game_camera, camera_field, &camera);
		if (!camera)
		{
			return false;
		}
		if (!read_value(get_rect, camera, m_rect) || !read_value(get_world_to_camera, camera, m_world_to_camera) || !read_value(get_projection, camera, m_projection))
		{
			return false;
		}
		m_ready = m_rect.width > 0 && m_rect.height > 0;
		return m_ready;
	}
	bool esp_projection::project_impl(const Vector3& world, Vector3& viewport)
	{
		if (!m_ready)
		{
			return false;
		}
		const auto& view  = m_world_to_camera;
		const float x     = view[0] * world.x + view[4] * world.y + view[8] * world.z + view[12];
		const float y     = view[1] * world.x + view[5] * world.y + view[9] * world.z + view[13];
		const float z     = view[2] * world.x + view[6] * world.y + view[10] * world.z + view[14];
		const float depth = -z;
		if (!std::isfinite(depth) || depth <= 0.1f)
		{
			return false;
		}
		const auto& proj   = m_projection;
		const float clip_x = proj[0] * x + proj[4] * y + proj[8] * z + proj[12];
		const float clip_y = proj[1] * x + proj[5] * y + proj[9] * z + proj[13];
		const float clip_w = proj[3] * x + proj[7] * y + proj[11] * z + proj[15];
		if (clip_w == 0.f)
		{
			return false;
		}
		const float viewport_x = (clip_x / clip_w + 1.f) * 0.5f;
		const float viewport_y = (clip_y / clip_w + 1.f) * 0.5f;
		if (!std::isfinite(viewport_x) || !std::isfinite(viewport_y))
		{
			return false;
		}
		viewport = Vector3{m_rect.x + viewport_x * m_rect.width, 1.f - (m_rect.y + viewport_y * m_rect.height), depth};
		return true;
	}
}
