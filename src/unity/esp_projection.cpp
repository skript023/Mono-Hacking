#include "esp_projection.hpp"

namespace big
{
	bool esp_projection::begin_frame_impl()
	{
		m_camera = nullptr;
		MonoObject* exception = nullptr;
		auto getter = mono::get_method("GameCamera", "get_instance", 0, "assembly_valheim");
		if (!getter)
			return false;
		auto game_camera = mono::invoke_method(getter, nullptr, nullptr, &exception);
		if (!game_camera || exception)
			return false;
		auto field = mono::get_field(mono::object_get_class(game_camera), "m_camera");
		if (!field)
			return false;
		mono::get_field_value(game_camera, field, &m_camera);
		if (!m_camera)
			return false;
		m_project = mono::get_method_exact("Camera", "WorldToViewportPoint", {"UnityEngine.Vector3"}, "UnityEngine.CoreModule", "UnityEngine");
		auto rect_getter = mono::get_method("Camera", "get_rect", 0, "UnityEngine.CoreModule", "UnityEngine");
		if (!m_project || !rect_getter)
			return false;
		auto boxed = mono::invoke_method(rect_getter, m_camera, nullptr, &exception);
		if (!boxed || exception)
			return false;
		auto value = mono::object_unbox(boxed);
		if (!value)
			return false;
		m_rect = *static_cast<viewport_rect*>(value);
		return m_rect.width > 0 && m_rect.height > 0;
	}
	bool esp_projection::project_impl(const Vector3& world, Vector3& viewport)
	{
		if (!m_camera || !m_project)
			return false;
		auto point = world;
		void* args[] = {&point};
		MonoObject* exception = nullptr;
		auto boxed = mono::invoke_method(m_project, m_camera, args, &exception);
		if (!boxed || exception)
			return false;
		auto value = mono::object_unbox(boxed);
		if (!value)
			return false;
		const auto result = *static_cast<Vector3*>(value);
		if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z) || result.z <= 0.1f)
			return false;
		viewport = Vector3{m_rect.x + result.x * m_rect.width, 1.f - (m_rect.y + result.y * m_rect.height), result.z};
		return true;
	}
}
