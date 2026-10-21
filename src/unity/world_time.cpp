#include "world_time.hpp"

#include <astra/host/notification.hpp>
#include "task_mgr.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

namespace big
{
	namespace
	{
		std::atomic<bool> s_cleanup_pending{false};
		std::atomic<float> s_time_scale{1.f};

		template<typename T>
		bool invoke_unboxed(MonoMethod* method, void* object, void** args, T& value)
		{
			if (!method)
			{
				return false;
			}
			MonoObject* exception = nullptr;
			auto result           = mono::invoke_method(method, object, args, &exception);
			auto unboxed          = result ? mono::object_unbox(result) : nullptr;
			if (exception || !unboxed)
			{
				return false;
			}
			value = *static_cast<T*>(unboxed);
			return true;
		}

		bool invoke_checked(MonoMethod* method, void* object, void** args)
		{
			if (!method)
			{
				return false;
			}
			MonoObject* exception = nullptr;
			mono::invoke_method(method, object, args, &exception);
			return exception == nullptr;
		}

		bool apply_time_scale(float value)
		{
			static auto can_pause      = mono::get_method("Game", "CanPause", 0, "assembly_valheim");
			static auto set_time_scale = mono::get_method("Game", "FadeTimeScale", 2, "assembly_valheim");
			if (!set_time_scale || !can_pause)
			{
				return false;
			}
			value = std::clamp(value, 0.f, 5.f);
			if (value != 1.f)
			{
				bool can_pause_value = false;
				if (!invoke_unboxed(can_pause, nullptr, nullptr, can_pause_value) || !can_pause_value)
				{
					return false;
				}
			}
			float transition = 0.f;
			void* args[2]    = {&value, &transition};
			if (!invoke_checked(set_time_scale, nullptr, args))
			{
				return false;
			}
			s_time_scale.store(value, std::memory_order_release);
			return true;
		}

		MonoObject* get_znet()
		{
			auto klass   = mono::get_class("ZNet", "assembly_valheim");
			auto field   = klass ? mono::get_field(klass, "m_instance") : nullptr;
			auto statics = klass ? mono::get_static_field_data(klass) : nullptr;
			if (!field || !statics)
			{
				return nullptr;
			}
			return *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(statics) + mono::get_field_offset(field));
		}

		bool is_server(MonoObject* znet)
		{
			static auto method = mono::get_method("ZNet", "IsServer", 0, "assembly_valheim");
			if (!znet || !method)
			{
				return false;
			}
			bool result = false;
			return invoke_unboxed(method, znet, nullptr, result) && result;
		}

		double get_day_length()
		{
			auto env = unity::get_env_man();
			return env ? static_cast<double>(mono::get_field_value<"EnvMan", "m_dayLengthSec", int64_t>(env)) : 1200.0;
		}
	}

	bool world_time::set_time_scale(float value)
	{
		if (!apply_time_scale(value))
		{
			notification::warning("World Time", "Valheim's native time scale controls are unavailable or blocked in this session.");
			return false;
		}
		return true;
	}

	bool world_time::reset_time_scale()
	{
		return apply_time_scale(1.f);
	}

	bool world_time::is_time_paused()
	{
		return s_time_scale.load(std::memory_order_acquire) <= 0.f;
	}

	void world_time::shutdown()
	{
		s_cleanup_pending.store(true, std::memory_order_release);
		async_run([] {
			apply_time_scale(1.f);
			s_cleanup_pending.store(false, std::memory_order_release);
		});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (cleanup_pending() && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (cleanup_pending())
		{
			LOG(WARNING) << "[World Time] Game thread unavailable to restore the simulation scale before unload";
		}
	}

	bool world_time::cleanup_pending()
	{
		return s_cleanup_pending.load(std::memory_order_acquire);
	}

	bool world_time::set_day_fraction(float fraction)
	{
		auto znet = get_znet();
		if (!is_server(znet))
		{
			return false;
		}
		static auto get_time = mono::get_method("ZNet", "GetTimeSeconds", 0, "assembly_valheim");
		static auto set_time = mono::get_method("ZNet", "SetNetTime", 1, "assembly_valheim");
		if (!get_time || !set_time)
		{
			return false;
		}
		double current_seconds = 0.0;
		if (!invoke_unboxed(get_time, znet, nullptr, current_seconds))
		{
			return false;
		}
		const double day_length = std::max(1.0, get_day_length());
		const auto day          = std::floor(current_seconds / day_length);
		const double target = day * day_length + static_cast<double>(std::clamp(fraction, 0.f, 0.999f)) * day_length;
		void* args[1]       = {const_cast<double*>(&target)};
		return invoke_checked(set_time, znet, args);
	}

	bool world_time::next_morning()
	{
		auto znet = get_znet();
		if (!is_server(znet))
		{
			return false;
		}
		static auto get_time = mono::get_method("ZNet", "GetTimeSeconds", 0, "assembly_valheim");
		static auto set_time = mono::get_method("ZNet", "SetNetTime", 1, "assembly_valheim");
		if (!get_time || !set_time)
		{
			return false;
		}
		double current_seconds = 0.0;
		if (!invoke_unboxed(get_time, znet, nullptr, current_seconds))
		{
			return false;
		}
		const double day_length = std::max(1.0, get_day_length());
		const double next_day = std::floor(current_seconds / day_length) + (std::fmod(current_seconds, day_length) / day_length >= 0.15 ? 1.0 : 0.0);
		const double target = next_day * day_length + day_length * 0.15;
		void* args[1]       = {const_cast<double*>(&target)};
		return invoke_checked(set_time, znet, args);
	}

	bool world_time::save_world()
	{
		auto znet = get_znet();
		if (!znet)
		{
			return false;
		}

		static auto save_world_and_profiles = mono::get_method("ZNet", "SaveWorldAndPlayerProfiles", 0, "assembly_valheim");
		if (!save_world_and_profiles || !is_server(znet))
		{
			return false;
		}

		return invoke_checked(save_world_and_profiles, znet, nullptr);
	}
}
