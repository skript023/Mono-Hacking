#include "hooking.hpp"

#include "commands/float_command.hpp"
#include "commands/bool_command.hpp"
#include "features/features.hpp"
#include "utility/unity.hpp"

namespace big
{
	float_command _raise_skill("raise_skill", "EXP Multiplier", "Multiplies the amount of skill experience gained.", 1.f, 100.f, 1.f);
	bool_command _enable_raise_skill("enable_raise_skill", "EXP Multiplier", "Multiplies the amount of skill experience gained.", false);

	void hooks::raise_skill(MonoObject* player, SkillType type, float value)
	{
		const bool is_local_player = g_running && player && player == unity::get_local_player();
		if (is_local_player && features::_freeze_skills.get_state())
		{
			return;
		}
		if (is_local_player && features::_prevent_skill_loss.get_state() && value < 0.f)
		{
			return;
		}
		if (is_local_player && _enable_raise_skill.get_state())
		{
			return detour_base::get_original<raise_skill>()(player, type, value * _raise_skill.get_state());
		}

		return detour_base::get_original<raise_skill>()(player, type, value);
	}

	void hooks::lower_all_skills(MonoObject* skills, float factor)
	{
		static auto player_field = mono::get_field("Skills", "m_player", "assembly_valheim");
		MonoObject* player       = nullptr;
		if (player_field && skills)
		{
			mono::get_field_value(skills, player_field, &player);
		}

		if (g_running && player && player == unity::get_local_player())
		{
			if (features::_freeze_skills.get_state()
			    || ((features::_prevent_skill_loss.get_state() || features::_keep_skills_on_death.get_state()) && factor > 0.f))
			{
				return;
			}
		}

		return detour_base::get_original<lower_all_skills>()(skills, factor);
	}
}
