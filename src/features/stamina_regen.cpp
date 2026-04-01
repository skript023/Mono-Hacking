#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _stamina_regen_amount("stamina_regen_amount", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 5.f, 100.f, 5.f);
	class stamina_regen : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

			player.set_stamina_regen(_stamina_regen_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_stamina_regen(5.f);
		}
	};

	static stamina_regen _stamina_regen("stamina_regen", "Stamina Regen", "Stamina Regeneration");
}