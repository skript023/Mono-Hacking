#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _max_stam("max_stam", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 50.f, 1000.f, 50.f);
	class max_stamina : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_call() override
		{
			if (m_state)
				on_enable();
			else
				on_disable();
		}

		virtual void on_tick() override
		{
			auto player = self::get_player();

			if (player.get_base_stamina() < _max_stam.get_state())
			{
				player.set_base_stamina(_max_stam.get_state());
				player.set_max_stamina(_max_stam.get_state(), true);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

			player.set_base_stamina(50.f);
			player.set_max_stamina(50.f, true);
		}
	};

	static max_stamina _max_stamina("max_stamina", "Max Stamina", "Increase Maximum Stamina");
}