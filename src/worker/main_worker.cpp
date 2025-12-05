#include "main_worker.hpp"
#include "script.hpp"

#include "commands/commands.hpp"
#include "commands/bool_command.hpp"

#include "server/server_module.hpp"

namespace big
{
	void main_worker::run()
	{
		//commands::enable_bool_commands();
		while (g_running)
		{
			TRY_CLAUSE
			{
				//commands::run_looped_command();
			} 
			EXCEPT_CLAUSE
			script::get_current()->yield();
		}
	}
}