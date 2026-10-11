#include "main_worker.hpp"
#include "pointers.hpp"
#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "unity/hud.hpp"
#include "unity/crafting_tools.hpp"
#include "unity/online_players.hpp"
#include "unity/base_tools.hpp"
#include "unity/animal_tools.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"
#include "commands/hotkeys.hpp"
#include "settings/settings.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
	static void update()
	{
		self::update();
		unity::hud_manager::update();
		crafting_tools::update();
		base_tools::hotkey_tick();
		animal_tools::update();
		animal_tools::hotkey_tick();
		settings::apply_external();
		hotkeys::tick();
	}

	static void safe_update()
	{
		TRY_CLAUSE
		{
			update();
			commands::run_looped_command();
			g_pointers->m_resolution.x = unity::get_screen_width();
			g_pointers->m_resolution.y = unity::get_screen_height();
		}
		EXCEPT_CLAUSE
	}

	task<void> main_worker::run()
	{
		commands::enable_bool_commands();

		while (g_running)
		{
			safe_update();
			co_await this_task::wait();
		}

		co_return;
	}

	static void safe_slow_update()
	{
		TRY_CLAUSE
		{
			online_players::update();
			base_tools::update();
		}
		EXCEPT_CLAUSE
	}

	task<void> main_worker::slow_run()
	{
		while (g_running)
		{
			safe_slow_update();
			co_await this_task::wait(1s);
		}

		co_return;
	}
}
