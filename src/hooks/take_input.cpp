#include "hooking.hpp"

#include "commands/float_command.hpp"
#include "commands/bool_command.hpp"

namespace big
{
	bool_command _disable_input("disable_input", "Disable Input", "Disables player input when enabled.");
	bool hooks::take_input(MonoObject* player_controller, bool look)
	{
		return detour_base::get_original<take_input>()(player_controller, look);
	}
}