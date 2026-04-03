#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	class infinite_stamina : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

			auto base = player.get_base_stamina();

			player.set_stamina(base);
		}

		virtual void on_disable() override
		{

		}
	};

	static infinite_stamina _infinite_stamina("infinite_stamina", "Infinite Stamina", "Stamina Always Maximum");
}