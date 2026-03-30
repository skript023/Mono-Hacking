#include "main_worker.hpp"
#include "script.hpp"
#include "pointers.hpp"
#include "utility/unity.hpp"

#include "commands/commands.hpp"
#include "commands/bool_command.hpp"

#include "server/server_module.hpp"

namespace big
{
	void main_worker::run()
	{
		commands::enable_bool_commands();
		while (g_running)
		{
			TRY_CLAUSE
			{
				g_pointers->m_resolution.x = unity::get_screen_width();
				g_pointers->m_resolution.y = unity::get_screen_height();

				commands::run_looped_command();
			} 
			EXCEPT_CLAUSE
			script::get_current()->yield();
		}
	}
}