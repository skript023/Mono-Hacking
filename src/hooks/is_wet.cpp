#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	bool hooks::is_under_roof(Vector3 startPos)
	{
		if (g_settings.self.is_wet)
		{
			LOG(INFO) << "Pointer of roof is " << startPos.x;

			return false;
		}

		return detour_base::get_original<is_under_roof>()(startPos);
	}
}