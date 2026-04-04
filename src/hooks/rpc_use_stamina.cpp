#include "hooking.hpp"
#include "commands/bool_command.hpp"

namespace big
{
    bool_command _infinite_stamina("infinite_stamina", "Infinite Stamina", "Stamina Always Maximum", false);

	void hooks::rpc_use_stamina(MonoObject* player, long sender, float v)
	{
        if (_infinite_stamina.get_state())
        {
            return detour_base::get_original<hooks::rpc_use_stamina>()(player, sender, 0);
        }

        return detour_base::get_original<hooks::rpc_use_stamina>()(player, sender, v);
	}
}