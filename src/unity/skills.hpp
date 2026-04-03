#pragma once
#include "mono/mono.hpp"

namespace big
{
    class skill_def
    {
        MonoObject* m_skill_def;
    public:
        skill_def(MonoObject* o): m_skill_def(o) {};
        ~skill_def() noexcept { m_skill_def = nullptr; }
    };

    class skills
    {
        MonoObject* m_skills;
    public:
        skills(MonoObject* o);
        ~skills() noexcept;

        mono_array_view<skill_def> get_all_skill_def();
    };
}