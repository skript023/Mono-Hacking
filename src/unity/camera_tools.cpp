#include "camera_tools.hpp"

#include "menu_settings.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace big::camera_tools
{
	namespace
	{
		MonoObject* get_camera()
		{
			static auto get_instance = mono::get_method("GameCamera", "get_instance", 0, "assembly_valheim");
			return get_instance ? mono::invoke_method(get_instance, nullptr) : nullptr;
		}

		struct baseline
		{
			float fov{};
			float zoom{};
			float smoothness{};
			float free_fly_speed{};
			float free_fly_smoothness{};
			float camera_fov{};
			float sky_fov{};
			bool shake{};
			bool has_camera_fov{};
			bool has_sky_fov{};
			bool has_free_fly_smoothness{};
			bool valid{};
		};

		baseline s_original;
		bool s_freecam_original{};
		bool s_freecam_state_captured = false;
		std::atomic<bool> s_cleanup_pending{false};

		void restore()
		{
			auto camera = get_camera();
			if (camera && s_original.valid)
			{
				static auto klass = mono::get_class("GameCamera", "assembly_valheim");
				if (klass)
				{
					const auto set_float = [camera](const char* name, float value) {
						auto field = mono::get_field(klass, name);
						if (field)
						{
							mono::set_field_value(camera, field, &value);
						}
					};
					const auto set_bool = [camera](const char* name, bool value) {
						auto field = mono::get_field(klass, name);
						if (field)
						{
							mono::set_field_value(camera, field, &value);
						}
					};
					set_float("m_fov", s_original.fov);
					set_float("m_zoomSens", s_original.zoom);
					set_float("m_smoothness", s_original.smoothness);
					set_float("m_freeFlySpeed", s_original.free_fly_speed);
					set_bool("m_cameraShakeEnabled", s_original.shake);

					static auto camera_field = mono::get_field(klass, "m_camera");
					static auto sky_field    = mono::get_field(klass, "m_skyCamera");
					static auto set_fov = mono::get_method("Camera", "set_fieldOfView", 1, "UnityEngine.CoreModule", "UnityEngine");
					MonoObject* game_camera = nullptr;
					MonoObject* sky_camera  = nullptr;
					if (camera_field)
					{
						mono::get_field_value(camera, camera_field, &game_camera);
					}
					if (sky_field)
					{
						mono::get_field_value(camera, sky_field, &sky_camera);
					}
					if (set_fov && game_camera && s_original.has_camera_fov)
					{
						void* args[] = {&s_original.camera_fov};
						mono::invoke_method(set_fov, game_camera, args);
					}
					if (set_fov && sky_camera && s_original.has_sky_fov)
					{
						void* args[] = {&s_original.sky_fov};
						mono::invoke_method(set_fov, sky_camera, args);
					}

					static auto set_free_fly_smoothness = mono::get_method("GameCamera", "SetFreeFlySmoothness", 1, "assembly_valheim");
					if (set_free_fly_smoothness && s_original.has_free_fly_smoothness)
					{
						void* args[] = {&s_original.free_fly_smoothness};
						mono::invoke_method(set_free_fly_smoothness, camera, args);
					}
				}
			}

			if (camera && s_freecam_state_captured && freecam_active() != s_freecam_original)
			{
				static auto toggle = mono::get_method("GameCamera", "ToggleFreeFly", 0, "assembly_valheim");
				if (toggle)
				{
					mono::invoke_method(toggle, camera, nullptr);
				}
			}
			s_freecam_state_captured = false;
			s_original               = {};
			s_cleanup_pending.store(false, std::memory_order_release);
		}
	}

	bool freecam_active()
	{
		static auto in_free_fly = mono::get_method("GameCamera", "InFreeFly", 0, "assembly_valheim");
		if (!in_free_fly)
		{
			return false;
		}

		auto result = mono::invoke_method(in_free_fly, nullptr);
		return result && *static_cast<bool*>(mono::object_unbox(result));
	}

	bool toggle_freecam()
	{
		auto camera = get_camera();
		if (!camera)
		{
			return false;
		}

		static auto toggle = mono::get_method("GameCamera", "ToggleFreeFly", 0, "assembly_valheim");
		if (!toggle)
		{
			return false;
		}

		if (!s_freecam_state_captured)
		{
			s_freecam_original       = freecam_active();
			s_freecam_state_captured = true;
		}
		mono::invoke_method(toggle, camera, nullptr);
		return true;
	}

	bool apply_settings()
	{
		auto camera = get_camera();
		if (!camera)
		{
			return false;
		}

		static auto klass = mono::get_class("GameCamera", "assembly_valheim");
		if (!klass)
		{
			return false;
		}
		if (!s_original.valid)
		{
			const auto get_float = [camera](const char* name, float& value) {
				auto field = mono::get_field(klass, name);
				if (!field)
				{
					return false;
				}
				mono::get_field_value(camera, field, &value);
				return true;
			};
			const auto get_bool = [camera](const char* name, bool& value) {
				auto field = mono::get_field(klass, name);
				if (!field)
				{
					return false;
				}
				mono::get_field_value(camera, field, &value);
				return true;
			};
			const bool fields_valid = get_float("m_fov", s_original.fov) && get_float("m_zoomSens", s_original.zoom)
			    && get_float("m_smoothness", s_original.smoothness) && get_float("m_freeFlySpeed", s_original.free_fly_speed)
			    && get_bool("m_cameraShakeEnabled", s_original.shake);
			static auto camera_field = mono::get_field(klass, "m_camera");
			static auto sky_field    = mono::get_field(klass, "m_skyCamera");
			MonoObject* game_camera  = nullptr;
			MonoObject* sky_camera   = nullptr;
			if (camera_field)
			{
				mono::get_field_value(camera, camera_field, &game_camera);
			}
			if (sky_field)
			{
				mono::get_field_value(camera, sky_field, &sky_camera);
			}
			static auto get_fov = mono::get_method("Camera", "get_fieldOfView", 0, "UnityEngine.CoreModule", "UnityEngine");
			if (get_fov && game_camera)
			{
				auto value = mono::invoke_method(get_fov, game_camera);
				if (value && mono::object_unbox(value))
				{
					s_original.camera_fov     = *static_cast<float*>(mono::object_unbox(value));
					s_original.has_camera_fov = true;
				}
			}
			if (get_fov && sky_camera)
			{
				auto value = mono::invoke_method(get_fov, sky_camera);
				if (value && mono::object_unbox(value))
				{
					s_original.sky_fov     = *static_cast<float*>(mono::object_unbox(value));
					s_original.has_sky_fov = true;
				}
			}
			s_original.valid = fields_valid;
			static auto get_free_fly_smoothness = mono::get_method("GameCamera", "GetFreeFlySmoothness", 0, "assembly_valheim");
			if (get_free_fly_smoothness)
			{
				auto value = mono::invoke_method(get_free_fly_smoothness, camera);
				if (value && mono::object_unbox(value))
				{
					s_original.free_fly_smoothness     = *static_cast<float*>(mono::object_unbox(value));
					s_original.has_free_fly_smoothness = true;
				}
			}
		}

		const auto set_float = [camera](const char* name, float value) {
			auto field = mono::get_field(klass, name);
			if (!field)
			{
				return false;
			}
			mono::set_field_value(camera, field, &value);
			return true;
		};
		const auto set_bool = [camera](const char* name, bool value) {
			auto field = mono::get_field(klass, name);
			if (!field)
			{
				return false;
			}
			mono::set_field_value(camera, field, &value);
			return true;
		};

		const bool fov    = set_float("m_fov", std::clamp(g_settings.self.camera_fov, 30.f, 120.f));
		const bool zoom   = set_float("m_zoomSens", std::clamp(g_settings.self.camera_zoom_sensitivity, 0.1f, 30.f));
		const bool smooth = set_float("m_smoothness", std::clamp(g_settings.self.camera_smoothness, 0.f, 1.f));
		const bool speed  = set_float("m_freeFlySpeed", std::clamp(g_settings.self.freecam_speed, 1.f, 1000.f));
		const bool shake  = set_bool("m_cameraShakeEnabled", g_settings.self.camera_shake_enabled);

		static auto camera_field     = mono::get_field(klass, "m_camera");
		static auto sky_camera_field = mono::get_field(klass, "m_skyCamera");
		static auto set_fov = mono::get_method("Camera", "set_fieldOfView", 1, "UnityEngine.CoreModule", "UnityEngine");
		MonoObject* game_camera = nullptr;
		MonoObject* sky_camera  = nullptr;
		if (camera_field)
		{
			mono::get_field_value(camera, camera_field, &game_camera);
		}
		if (sky_camera_field)
		{
			mono::get_field_value(camera, sky_camera_field, &sky_camera);
		}
		float camera_fov = std::clamp(g_settings.self.camera_fov, 30.f, 120.f);
		if (set_fov)
		{
			void* args[] = {&camera_fov};
			if (game_camera)
			{
				mono::invoke_method(set_fov, game_camera, args);
			}
			if (sky_camera)
			{
				mono::invoke_method(set_fov, sky_camera, args);
			}
		}

		static auto set_free_fly_smoothness = mono::get_method("GameCamera", "SetFreeFlySmoothness", 1, "assembly_valheim");
		bool freecam_smooth = false;
		if (set_free_fly_smoothness)
		{
			float value  = std::clamp(g_settings.self.freecam_smoothness, 0.f, 1.f);
			void* args[] = {&value};
			mono::invoke_method(set_free_fly_smoothness, camera, args);
			freecam_smooth = true;
		}
		return fov && zoom && smooth && speed && shake && freecam_smooth;
	}

	void reset()
	{
		s_cleanup_pending.store(true, std::memory_order_release);
		async_run([] {
			restore();
		});
	}

	void shutdown()
	{
		reset();
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (cleanup_pending() && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (cleanup_pending())
		{
			LOG(WARNING) << "[Camera] Game thread unavailable for settings cleanup before unload";
		}
	}

	bool cleanup_pending()
	{
		return s_cleanup_pending.load(std::memory_order_acquire);
	}
}
