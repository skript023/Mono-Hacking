#include "skills.hpp"

namespace big
{
	skills::skills(MonoObject* o): m_skills(o)
	{}
	skills::~skills() noexcept
	{
        m_skills = nullptr;
	}
	std::vector<skill_def> skills::get_all_skill_def()
	{
		auto s = mono::get_field_value<MonoObject*>(m_skills, "Skills", "m_skills");

        return mono::from_list<skill_def>(s);
	}
}