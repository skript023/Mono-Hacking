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
		if (g_running && mono::get_field_value<"Terminal/ConsoleCommand", "Command", std::string>(command) == "confirmcheats")
		{
			static auto add_string = mono::get_method_exact("Terminal", "AddString", {"System.String"}, "assembly_valheim");
			auto terminal          = args ? mono::get_field_value<"Terminal/ConsoleEventArgs", "Context", MonoObject*>(args) : nullptr;
			if (terminal && add_string)
			{
				const char* message = _allowed_command.get_state()
				    ? "Astra blocked confirmcheats so this character keeps its achievements. Cheat commands already work while Allow all commands is on."
				    : "Astra blocked confirmcheats so this character keeps its achievements. Turn on Allow all commands in the Self menu to use cheat commands.";
				void* message_args[] = {mono::to_mono_string(message)};
				mono::invoke_method(add_string, terminal, message_args);
			}
			return;
		}

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

	bool hooks::set_key_value_is_cheat(MonoString* set_key_command)
	{
		if (g_running && _allowed_command.get_state())
		{
			return false;
		}

		return detour_base::get_original<set_key_value_is_cheat>()(set_key_command);
	}

	void hooks::global_key_add(MonoObject* zone_system, MonoString* key, bool can_save_to_server_option_keys)
	{
		if (g_running && can_save_to_server_option_keys && key && detour_base::get_original<set_key_value_is_cheat>()(key))
		{
			can_save_to_server_option_keys = false;
		}

		detour_base::get_original<global_key_add>()(zone_system, key, can_save_to_server_option_keys);
	}
}
