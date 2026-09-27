#include "hooking.hpp"

namespace big
{
	void hooks::hud_update_food(MonoObject* hud, MonoObject* player)
	{
		detour_base::get_original<hooks::hud_update_food>()(hud, player);
	}
}
