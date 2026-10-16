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

	void hooks::console_run_action(MonoObject* command, MonoObject* args)
	{
		if (!_allowed_command.get_state() || !mono::get_field_value<"Terminal/ConsoleCommand", "IsCheat", bool>(command))
		{
			return detour_base::get_original<console_run_action>()(command, args);
		}

		// RunAction refuses cheats until confirmcheats sets PlayerProfile.m_usedCheats, which locks achievements.
		// Running the command as a non-cheat skips that gate and never sets the flag.
		struct restore_cheat
		{
			MonoObject* command;
			~restore_cheat()
			{
				mono::set_field_value<"Terminal/ConsoleCommand", "IsCheat">(command, true);
			}
		} restore{command};
		mono::set_field_value<"Terminal/ConsoleCommand", "IsCheat">(command, false);
		detour_base::get_original<console_run_action>()(command, args);
	}
}
