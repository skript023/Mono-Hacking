#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _stamina_regen_amount("stamina_regen_amount", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 5.f, 100.f, 5.f);
	class stamina_regen : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_staminaRegen = mono::get_field(klass, "m_staminaRegen");

			if (!klass)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			mono::set_field_value(unity::get_local_player(), m_staminaRegen, &_stamina_regen_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_staminaRegen = mono::get_field(klass, "m_staminaRegen");

			if (!klass)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			float default_stamina_regen = 5.f;

			mono::set_field_value(unity::get_local_player(), m_staminaRegen, &default_stamina_regen);
		}
	};

	static stamina_regen _stamina_regen("stamina_regen", "Stamina Regen", "Stamina Regeneration");
}