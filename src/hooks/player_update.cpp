#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"

namespace big
{
	void hooks::update(MonoObject* player)
	{
		TRY_CLAUSE
		{
			auto local_player = unity::get_local_player();
			
			if (g_running && local_player)
				g_script_mgr.tick();
				
			return detour_base::get_original<update>()(player);
		} EXCEPT_CLAUSE
	}
}