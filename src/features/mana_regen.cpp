#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_regen_amount("eitr_regen_amount", "Eitr Regen Amount", "Amount of eitr to regenerate per second.", 5.f, 100.f, 5.f);
	class eitr_regen : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

			if (player.get_eitr_regen() < _eitr_regen_amount.get_state())
				player.set_eitr_regen(_eitr_regen_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_eitr_regen(5.f);
		}
	};

	static eitr_regen _eitr_regen("eitr_regen", "Eitr Regen", "Eitr Regeneration");
}