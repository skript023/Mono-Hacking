#pragma once
#include "mono/mono.hpp"
#include "class/enums.hpp"
#include <string>
#include <string_view>

namespace big
{
	class skill_def
	{
		MonoObject* m_skill_def;

	public:
		skill_def(MonoObject* o) :
		    m_skill_def(o) {};
		~skill_def() noexcept
		{
			m_skill_def = nullptr;
		}

		MonoObject* get_object() const
		{
			return m_skill_def;
		}
	};

	class skills
	{
		MonoObject* m_skills;

	public:
		skills(MonoObject* o);
		~skills() noexcept;

		MonoObject* get_object() const
		{
			return m_skills;
		}
		explicit operator bool() const
		{
			return m_skills != nullptr;
		}

		mono_array_view<skill_def> get_all_skill_def();

		void cheat_raise_skill(const std::string& name, float value, bool show_message = true);
		void cheat_raise_skill(SkillType type, float value, bool show_message = true);

		void cheat_reset_skill(const std::string& name);
		void cheat_reset_skill(SkillType type);

		float get_skill_level(SkillType type);

		static std::string_view skill_type_to_csharp_name(SkillType type);
		static std::string_view skill_type_to_display_name(SkillType type);
		static skills get_local_skills();
	};
}
