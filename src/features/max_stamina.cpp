#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

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
			auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_baseStamina = mono::get_field(klass, "m_baseStamina");

			if (!method || !klass || !m_baseStamina)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			bool flashBar = true;

			void* args[2] = { &_max_stam.get_state(), &flashBar};
			mono::invoke_method(method, unity::get_local_player(), args);

			mono::set_field_value(unity::get_local_player(), m_baseStamina, &_max_stam.get_state());
		}

		virtual void on_disable() override
		{
			auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_baseStamina = mono::get_field(klass, "m_baseStamina");

			if (!method || !klass)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			bool flashBar = true;
			float default_max_stam = 50.f;

			void* args[2] = { &default_max_stam, &flashBar };
			mono::invoke_method(method, unity::get_local_player(), args);

			mono::set_field_value(unity::get_local_player(), m_baseStamina, &default_max_stam);
		}
	};

	static max_stamina _max_stamina("max_stamina", "Max Stamina", "Increase Maximum Stamina");
}