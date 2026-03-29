#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	class infinite_stamina : public looped_command
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
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_stamina = mono::get_field(klass, "m_stamina");
			auto m_baseStamina = mono::get_field(klass, "m_baseStamina");

			if (!klass || !m_stamina || !m_baseStamina)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			void* max_stam = nullptr;
			mono::get_field_value(unity::get_local_player(), m_baseStamina, &max_stam);

			if (max_stam)
			{
				mono::set_field_value(unity::get_local_player(), m_stamina, &max_stam);
			}
		}

		virtual void on_disable() override
		{

		}
	};

	static infinite_stamina _infinite_stamina("infinite_stamina", "Infinite Stamina", "Stamina Always Maximum");
}