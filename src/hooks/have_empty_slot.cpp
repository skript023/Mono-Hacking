#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::have_empty_slot(MonoObject* _this)
	{
		if (true)
		{
			LOG(VERBOSE) << "Inventory::HaveEmptySlot hooked: returning true";
			return true;
		}

		return detour_base::get_original<have_empty_slot>()(_this);
	}
}