#include "hooking.hpp"

#include "commands/float_command.hpp"
#include "commands/bool_command.hpp"

namespace big
{
	float_command _raise_skill("raise_skill", "Raise Skill Multiplier", "Multiplies the amount of skill experience gained.", 1.f, 100.f, 1.f);
	bool_command _enable_raise_skill("enable_raise_skill", "Raise Skill Multiplier", "Multiplies the amount of skill experience gained.", false);

	void hooks::raise_skill(MonoObject* player, SkillType type, float value)
	{
		if (_enable_raise_skill.get_state())
			return detour_base::get_original<raise_skill>()(player, type, value * _raise_skill.get_state());

		return detour_base::get_original<raise_skill>()(player, type, value);
	}
}