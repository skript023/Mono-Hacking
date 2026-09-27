#include "procreation.hpp"

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
			return false;

		static auto method = mono::get_method("Procreation", "IsPregnant", 0, "assembly_valheim");
		if (!method)
			return false;

		auto res = mono::invoke_method(method, m_procreation, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}

	int procreation::get_love_points()
	{
		if (!m_procreation)
			return 0;

		static auto klass = mono::get_class("Procreation", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_lovePoints") : nullptr;
		if (!field)
			return 0;

		int pts = 0;
		mono::get_field_value(m_procreation, field, &pts);
		return pts;
	}

	int procreation::get_required_love_points()
	{
		if (!m_procreation)
			return 4;

		static auto klass = mono::get_class("Procreation", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_requiredLovePoints") : nullptr;
		if (!field)
			return 4;

		int pts = 4;
		mono::get_field_value(m_procreation, field, &pts);
		return pts;
	}
}
