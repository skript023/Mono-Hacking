#include "hooking.hpp"

namespace big
{
	bool hooks::is_known_material(MonoObject* player, MonoString* name)
	{
		//return detour_base::get_original<is_known_material>()(player, name);

        return true;
	}
}