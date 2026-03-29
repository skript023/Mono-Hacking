#include "hooking.hpp"
#include <commands/bool_command.hpp>
#include <commands/float_command.hpp>

namespace big
{
	bool_command _max_body_armor("max_body_armor", "Max Body Armor", "Sets your body armor to maximum value.", true);
	float_command _num_body_armor("num_body_armor", "Max Body Armor", "Sets your body armor to maximum value.", 100.f, 100000.f, 999.f);

	float hooks::get_body_armor(MonoObject* player)
	{
		if (_max_body_armor.get_state())
		{
			return _num_body_armor.get_state();
		}

		return detour_base::get_original<get_body_armor>()(player);
	}
}