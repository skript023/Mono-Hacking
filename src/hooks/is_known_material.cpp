#include "hooking.hpp"
#include "unity/crafting_tools.hpp"

namespace big
{
	bool hooks::is_known_material(MonoObject* player, MonoString* name)
	{
		if (player && crafting_tools::active(player))
			return true;
		return detour_base::get_original<is_known_material>()(player, name);
	}
}
