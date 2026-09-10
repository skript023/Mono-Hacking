#include "hooking.hpp"
#include "menu_settings.hpp"

namespace big
{
	bool hooks::top_first(void* _this, void* item)
	{
		if (g_settings.self.inventory_top_first)
		{
			return true;
		}

		return detour_base::get_original<top_first>()(_this, item);
	}
}

