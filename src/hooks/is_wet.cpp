#include "hooking.hpp"

namespace big
{
	bool hooks::is_under_roof(Vector3 startPos)
	{
		return detour_base::get_original<is_under_roof>()(startPos);
	}
}
