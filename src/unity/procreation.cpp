#include "procreation.hpp"
#include <algorithm>
#include <unordered_map>

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

		static auto klass = mono::get_class("Procreation", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_lovePoints") : nullptr;
		if (!field)
		{
			return 0;
		}

		int pts = 0;
		mono::get_field_value(m_procreation, field, &pts);
		return pts;
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
			float duration{};
			float interval{};
			bool has_duration{};
			bool has_interval{};
		};
		static std::unordered_map<MonoObject*, base_timing> originals;
		auto [it, inserted] = originals.try_emplace(m_procreation);
		auto& base          = it->second;
		if (inserted)
		{
			if (duration_field)
			{
				mono::get_field_value(m_procreation, duration_field, &base.duration);
				base.has_duration = base.duration > 0.f;
			}
			if (interval_field)
			{
				mono::get_field_value(m_procreation, interval_field, &base.interval);
				base.has_interval = base.interval > 0.f;
			}
		}

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
			changed = true;
		}
		return changed;
	}
}
