#include "hooking.hpp"
#include <commands/bool_command.hpp>

namespace big
{
	bool_command _max_body_armor("max_body_armor", "Max Body Armor", "Sets your body armor to maximum value.", true);

	float hooks::get_body_armor(MonoObject* player)
	{
		if (true)
		{
			return 99999999.f;
		}

		return detour_base::get_original<get_body_armor>()(player);
	}
}