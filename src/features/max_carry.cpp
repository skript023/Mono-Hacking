#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _carry_amount("carry_amount", "", "", 100.f, 10000.f, 100.f);
	class max_carry_weight : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();
			
			if (player.get_max_carry() < _carry_amount.get_state())
				player.set_max_carry(_carry_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_max_carry(300.f);
		}
	};

	static max_carry_weight _max_carry_weight("max_carry_weight", "Max Carry", "Max Carry Weight");
}