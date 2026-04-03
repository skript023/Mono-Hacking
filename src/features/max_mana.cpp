#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_amount("eitr_amount", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 0.f, 10000.f, 0.f);
	class eitr : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

			player.set_max_eitr(_eitr_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_max_eitr(0.f);
		}
	};

	static eitr _eitr("eitr", "Max Eitr", "Max Eitr");
}