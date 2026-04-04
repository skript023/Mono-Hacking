#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _max_hp("max_hp", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 25.f, 1000.f, 25.f);
	class max_health : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

			player.set_base_health(_max_hp.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			
			player.set_base_health(25.f);
		}
	};

	static max_health _max_health("max_health", "Max Base Health", "Increase Max Base Health");
}