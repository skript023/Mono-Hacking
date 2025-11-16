#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::update(MonoObject* player)
	{
		if (g_running)
			g_script_mgr.tick();

		return detour_base::get_original<update>()(player);
	}
}