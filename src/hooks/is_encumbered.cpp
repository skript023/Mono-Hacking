#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::is_encumbered(MonoObject* player)
	{
		if (g_settings.self.no_encumbrance && player && player == unity::get_local_player())
		{
			return false;
		}

		return detour_base::get_original<is_encumbered>()(player);
	}
}
