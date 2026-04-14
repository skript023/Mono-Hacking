#include "main_worker.hpp"
#include "script.hpp"
#include "pointers.hpp"
#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"

#include "server/server_module.hpp"

namespace big
{
	static void update()
	{
		self::update();
	}
	void main_worker::run()
	{
		commands::enable_bool_commands();
		
		while (g_running)
		{
			TRY_CLAUSE
			{
				update();
				g_pointers->m_resolution.x = unity::get_screen_width();
				g_pointers->m_resolution.y = unity::get_screen_height();
			} EXCEPT_CLAUSE

			script::get_current()->yield();
		}
	}
	void main_worker::slow_run()
	{
		while (g_running)
		{
			TRY_CLAUSE
			{
				commands::run_looped_command();
			} EXCEPT_CLAUSE

			script::get_current()->yield(5s);
		}
	}
}