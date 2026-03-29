#include "hooking.hpp"
#include "commands/bool_command.hpp"

namespace big
{
    bool_command _allowed_command("allowed_command", "Allow all commands", "Allow the use of all commands in the console.", true);
    
	bool hooks::allowed_command(MonoObject* ConsoleCommand, MonoObject* Terminal, bool Boolean)
	{
        if (_allowed_command.get_state())
        {
            return true;
        }

        return detour_base::get_original<allowed_command>()(ConsoleCommand, Terminal, Boolean);
	}
}