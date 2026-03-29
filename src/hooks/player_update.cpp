#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::update(MonoObject* player)
	{
		TRY_CLAUSE
		{
			if (g_running)
			g_script_mgr.tick();

			return detour_base::get_original<update>()(player);
		} EXCEPT_CLAUSE
	}
}