#include "main_worker.hpp"
#include "script.hpp"
#include "pointers.hpp"
#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "unity/online_players.hpp"
#include "unity/base_tools.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"


namespace big
{
	static void update()
	{
		self::update();
		base_tools::hotkey_tick();
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
				online_players::update();
				base_tools::update();
			} EXCEPT_CLAUSE

			script::get_current()->yield(1s);
		}
	}
}
