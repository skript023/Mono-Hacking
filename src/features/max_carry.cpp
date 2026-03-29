#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _carry_amount("carry_amount", "", "", 100.f, 10000.f, 100.f);
	class max_carry_weight : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_maxCarryWeight = mono::get_field(klass, "m_maxCarryWeight");

			if (!klass || !m_maxCarryWeight)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			mono::set_field_value(unity::get_local_player(), m_maxCarryWeight, &_carry_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_maxCarryWeight = mono::get_field(klass, "m_maxCarryWeight");

			if (!klass || !m_maxCarryWeight)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			float default_stamina_regen = 0.f;

			mono::set_field_value(unity::get_local_player(), m_maxCarryWeight, &default_stamina_regen);
		}
	};

	static max_carry_weight _max_carry_weight("max_carry_weight", "Max Carry", "Max Carry Weight");
}