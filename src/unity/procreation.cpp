#include "procreation.hpp"
#include <algorithm>
#include <iterator>
#include <vector>

namespace big
{
	procreation::procreation(MonoObject* obj) :
	    m_procreation(obj)
	{
	}

	procreation::~procreation() noexcept
	{
		m_procreation = nullptr;
	}

	bool procreation::is_pregnant()
	{
		if (!m_procreation)
		{
			return false;
		}

		static auto method = mono::get_method("Procreation", "IsPregnant", 0, "assembly_valheim");
		if (!method)
		{
			return false;
		}

		auto res = mono::invoke_method(method, m_procreation, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}

	int procreation::get_love_points()
	{
		if (!m_procreation)
		{
			return 0;
		}

		static auto method = mono::get_method("Procreation", "GetLovePoints", 0, "assembly_valheim");
		auto result        = method ? mono::invoke_method(method, m_procreation, nullptr) : nullptr;
		return result ? *static_cast<int*>(mono::object_unbox(result)) : 0;
	}

	int procreation::get_required_love_points()
	{
		if (!m_procreation)
		{
			return 4;
		}

		static auto klass = mono::get_class("Procreation", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_requiredLovePoints") : nullptr;
		if (!field)
		{
			return 4;
		}

		int pts = 4;
		mono::get_field_value(m_procreation, field, &pts);
		return pts;
	}

	bool procreation::accelerate_timing(float multiplier)
	{
		if (!m_procreation)
		{
			return false;
		}

		multiplier                 = std::clamp(multiplier, 1.f, 100.f);
		static auto klass          = mono::get_class("Procreation", "assembly_valheim");
		static auto duration_field = klass ? mono::get_field(klass, "m_pregnancyDuration") : nullptr;
		static auto interval_field = klass ? mono::get_field(klass, "m_updateInterval") : nullptr;
		if (!duration_field && !interval_field)
		{
			return false;
		}

		struct base_timing
		{
			uintptr_t handle{};
			float duration{};
			float interval{};
			bool has_duration{};
			bool has_interval{};
		};
		static auto is_alive = mono::get_method_exact("Object", "op_Implicit", {"UnityEngine.Object"}, "UnityEngine.CoreModule", "UnityEngine");
		static std::vector<base_timing> originals;
		std::erase_if(originals, [](const base_timing& entry) {
			auto object         = mono::retained_object(entry.handle);
			void* alive_args[1] = {object};
			auto alive          = object && is_alive ? mono::invoke_method(is_alive, nullptr, alive_args) : nullptr;
			if (alive && *static_cast<bool*>(mono::object_unbox(alive)))
			{
				return false;
			}
			mono::release(entry.handle);
			return true;
		});
		auto found = std::ranges::find_if(originals, [this](const base_timing& entry) {
			return mono::retained_object(entry.handle) == m_procreation;
		});
		if (found == originals.end())
		{
			base_timing recorded{mono::retain(m_procreation)};
			if (duration_field)
			{
				mono::get_field_value(m_procreation, duration_field, &recorded.duration);
				recorded.has_duration = recorded.duration > 0.f;
			}
			if (interval_field)
			{
				mono::get_field_value(m_procreation, interval_field, &recorded.interval);
				recorded.has_interval = recorded.interval > 0.f;
			}
			originals.push_back(recorded);
			found = std::prev(originals.end());
		}
		const auto& base = *found;

		bool changed = false;
		if (duration_field && base.has_duration)
		{
			float duration = std::max(0.1f, base.duration / multiplier);
			mono::set_field_value(m_procreation, duration_field, &duration);
			changed = true;
		}
		if (interval_field && base.has_interval)
		{
			float interval = std::max(0.1f, base.interval / multiplier);
			mono::set_field_value(m_procreation, interval_field, &interval);

			static auto cancel_invoke = mono::get_method_exact("MonoBehaviour", "CancelInvoke", {"System.String"}, "UnityEngine.CoreModule", "UnityEngine");
			static auto invoke_repeating = mono::get_method("MonoBehaviour", "InvokeRepeating", 3, "UnityEngine.CoreModule", "UnityEngine");
			if (cancel_invoke && invoke_repeating)
			{
				auto procreate       = mono::to_mono_string("Procreate");
				void* cancel_args[1] = {procreate};
				void* repeat_args[3] = {procreate, &interval, &interval};
				mono::invoke_method(cancel_invoke, m_procreation, cancel_args);
				mono::invoke_method(invoke_repeating, m_procreation, repeat_args);
			}
			changed = true;
		}
		return changed;
	}
}
