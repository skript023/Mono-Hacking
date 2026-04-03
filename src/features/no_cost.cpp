#include "commands/looped_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	class open_all_recepies : public bool_command
	{
		using bool_command::bool_command;

		virtual void on_enable() override
		{
			auto player = self::get_player();

			player.set_no_placement_cost(TRUE);
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_no_placement_cost(FALSE);
		}
	};

	static open_all_recepies _open_all_recepies("open_all_recepies", "Opens all recipes and free crafting", "Opens all recipes and free crafting");
}