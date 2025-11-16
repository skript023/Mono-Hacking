#include "hooking.hpp"

namespace big
{
	void hooks::set_alerted(MonoObject* player, bool alerted)
	{
		return detour_base::get_original<set_alerted>()(player, g_settings.self.no_animal_alert ? false : alerted);
	}
}