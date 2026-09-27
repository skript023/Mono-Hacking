#include "hud.hpp"
#include "fiber_pool.hpp"
#include "utility/unity.hpp"
#include <algorithm>
#include <cmath>

namespace big::unity
{
	bool hud_manager::cleanup_pending_impl() const
	{
		return m_cleanup_pending.load(std::memory_order_acquire);
	}

	void hud_manager::shutdown_impl()
	{
		if (!g_fiber_pool)
			return;
		m_cleanup_pending.store(true, std::memory_order_release);
		g_fiber_pool->queue_job([this] {
			reset_impl();
			m_cleanup_pending.store(false, std::memory_order_release);
		});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (cleanup_pending_impl() && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (cleanup_pending_impl())
			LOG(WARNING) << "[HUD] Game thread unavailable for food slot cleanup before unload";
	}

	void hud_manager::queue_update_impl()
	{
		if (m_pending || !g_fiber_pool)
			return;
		m_pending = true;
		g_fiber_pool->queue_job([this] {
			m_pending = false;
			if (g_running)
				update_impl();
			else
				reset_impl();
		});
	}
	void hud_manager::queue_reset_impl()
	{
		if (g_fiber_pool)
			g_fiber_pool->queue_job([this] {
				reset_impl();
			});
	}
	MonoMethod* hud_manager::method_impl(const char* type, const char* name, int count)
	{
		auto method = mono::get_method(type, name, count, "UnityEngine.CoreModule", "UnityEngine");
		if (!method)
			throw std::runtime_error(std::format("Missing {}.{}", type, name));
		return method;
	}
	MonoObject* hud_manager::call_impl(MonoMethod* method, MonoObject* object, void** args)
	{
		if (!method)
			throw std::runtime_error("Missing HUD method");
		MonoObject* exception = nullptr;
		auto result = mono::invoke_method(method, object, args, &exception);
		if (exception)
			throw std::runtime_error("Managed HUD operation failed");
		return result;
	}
	uintptr_t hud_manager::retain_impl(MonoObject* object)
	{
		if (!object)
			throw std::runtime_error("Missing HUD object");
		auto handle = mono::retain(object);
		if (!handle)
			throw std::runtime_error("Cannot retain HUD object");
		return handle;
	}
	MonoObject* hud_manager::element_impl(MonoArray* array, int index)
	{
		if (!array || index < 0 || index >= mono::array_length(array))
			throw std::runtime_error("Invalid HUD array index");
		auto object = *static_cast<MonoObject**>(mono::array_with_size(array, sizeof(MonoObject*), index));
		if (!object)
			throw std::runtime_error("Empty HUD food slot");
		return object;
	}
	MonoObject* hud_manager::clone_impl(MonoObject* source, const Vector3& offset)
	{
		auto get_transform = method_impl("Component", "get_transform", 0);
		auto transform = call_impl(get_transform, source);
		if (!transform)
			throw std::runtime_error("Missing food slot transform");
		auto parent = call_impl(method_impl("Transform", "get_parent", 0), transform);
		if (!parent)
			throw std::runtime_error("Missing food slot parent");
		// Instantiate(Component, parent, false) preserves the component type and avoids GetComponent overloads.
		auto instantiate = mono::get_method_exact("Object", "Instantiate", {"UnityEngine.Object", "UnityEngine.Transform", "System.Boolean"}, "UnityEngine.CoreModule", "UnityEngine");
		bool world_position = false;
		void* args[] = {source, parent, &world_position};
		auto clone = call_impl(instantiate, nullptr, args);
		m_clones.push_back(retain_impl(clone));
		auto clone_transform = call_impl(get_transform, clone);
		auto position_box = call_impl(method_impl("Transform", "get_localPosition", 0), transform);
		if (!clone_transform || !position_box)
			throw std::runtime_error("Missing cloned food slot transform");
		auto position = *static_cast<Vector3*>(mono::object_unbox(position_box));
		position.x += offset.x;
		position.y += offset.y;
		position.z += offset.z;
		void* position_args[] = {&position};
		call_impl(method_impl("Transform", "set_localPosition", 1), clone_transform, position_args);
		return clone;
	}
	void hud_manager::restore_impl()
	{
		auto hud = mono::retained_object(m_hud);
		for (size_t i = 0; i < m_fields.size(); ++i)
		{
			auto original = mono::retained_object(m_originals[i]);
			if (hud && original)
				mono::set_field_value(hud, m_fields[i], original);
		}
		// Restore all three arrays before destroying components used by UpdateFood.
		if (!m_clones.empty())
		{
			auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
			auto destroy = mono::get_method("Object", "Destroy", 1, "UnityEngine.CoreModule", "UnityEngine");
			for (auto handle : m_clones)
			{
				auto component = mono::retained_object(handle);
				auto go = component ? mono::invoke_method(get_go, component) : nullptr;
				if (go)
				{
					void* args[] = {go};
					mono::invoke_method(destroy, nullptr, args);
				}
				mono::release(handle);
			}
		}
		m_clones.clear();
		for (auto handle : m_roots)
			mono::release(handle);
		m_roots.clear();
		m_target = 3;
	}
	void hud_manager::reset_impl()
	{
		if (m_hud)
			restore_impl();
		for (auto& handle : m_originals)
		{
			mono::release(handle);
			handle = 0;
		}
		mono::release(m_hud);
		m_hud = 0;
		m_failed = false;
	}
	void hud_manager::resize_impl(MonoObject* hud, int target)
	{
		restore_impl();
		std::array<MonoArray*, 3> arrays{};
		for (size_t group = 0; group < arrays.size(); ++group)
		{
			auto original = reinterpret_cast<MonoArray*>(mono::retained_object(m_originals[group]));
			int count = mono::array_length(original);
			auto array = mono::new_reference_array(original, target);
			m_roots.push_back(retain_impl(reinterpret_cast<MonoObject*>(array)));
			arrays[group] = array;
			auto get_transform = method_impl("Component", "get_transform", 0);
			auto get_position = method_impl("Transform", "get_localPosition", 0);
			auto last = element_impl(original, count - 1);
			auto previous = element_impl(original, count - 2);
			auto last_transform = call_impl(get_transform, last);
			auto previous_transform = call_impl(get_transform, previous);
			if (!last_transform || !previous_transform)
				throw std::runtime_error("Missing food slot transform");
			auto last_box = call_impl(get_position, last_transform);
			if (!last_box)
				throw std::runtime_error("Missing food slot position");
			auto last_position = *static_cast<Vector3*>(mono::object_unbox(last_box));
			auto previous_box = call_impl(get_position, previous_transform);
			if (!previous_box)
				throw std::runtime_error("Missing food slot position");
			auto previous_position = *static_cast<Vector3*>(mono::object_unbox(previous_box));
			Vector3 spacing{last_position.x - previous_position.x, last_position.y - previous_position.y, 0.f};
			// Icons under separate containers can have identical local positions.
			if (std::abs(spacing.x) + std::abs(spacing.y) < 0.01f)
				spacing.y = -32.f;
			for (int index = 0; index < target; ++index)
			{
				MonoObject* component = nullptr;
				if (index < count)
					component = element_impl(original, index);
				else
				{
					float step = static_cast<float>(index - count + 1);
					component = clone_impl(last, {spacing.x * step, spacing.y * step, 0.f});
				}
				mono::set_array_reference(array, index, component);
			}
		}
		// No yielding: UpdateFood must never observe unequal lengths or incomplete slots.
		for (size_t i = 0; i < arrays.size(); ++i)
			mono::set_field_value(hud, m_fields[i], arrays[i]);
		m_target = target;
	}
	void hud_manager::update_impl()
	{
		try
		{
			auto getter = mono::get_method("Hud", "get_instance", 0, "assembly_valheim");
			auto hud = call_impl(getter);
			if (!hud || !unity::get_local_player())
			{
				if (m_hud)
					reset_impl();
				return;
			}
			if (hud != mono::retained_object(m_hud))
			{
				reset_impl();
				m_hud = retain_impl(hud);
				const char* names[] = {"m_foodBars", "m_foodIcons", "m_foodTime"};
				for (size_t i = 0; i < m_fields.size(); ++i)
				{
					m_fields[i] = mono::get_field("Hud", names[i], "assembly_valheim");
					MonoArray* array = nullptr;
					mono::get_field_value(hud, m_fields[i], &array);
					if (!array || mono::array_length(array) < 3)
						throw std::runtime_error("HUD food arrays are not ready");
					m_originals[i] = retain_impl(reinterpret_cast<MonoObject*>(array));
				}
			}
			if (m_failed)
				return;
			int target = std::clamp(g_settings.self.max_food_slots, 3, 10);
			if (target != m_target)
			{
				if (target == 3)
					restore_impl();
				else
					resize_impl(hud, target);
			}
		}
		catch (const std::exception& error)
		{
			if (!m_failed)
			{
				m_failed = true;
				restore_impl();
				LOG(WARNING) << "[HUD] Food slot expansion stopped for this HUD: " << error.what();
			}
		}
	}
}
