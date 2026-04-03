#include "skills.hpp"

namespace big
{
	skills::skills(MonoObject* o): m_skills(o)
	{}
	skills::~skills() noexcept
	{
        m_skills = nullptr;
	}
	mono_array_view<skill_def> skills::get_all_skill_def()
	{
		auto s = mono::get_field_value<"Skills", "m_skills", MonoObject*>(m_skills);

        return mono::list<skill_def>(s);
	}
}