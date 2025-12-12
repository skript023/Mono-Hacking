#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_amount("eitr_amount", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 0.f, 100.f, 0.f);
	class eitr : public looped_command
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
			auto m_maxEitr = mono::get_field(klass, "m_maxEitr");

			if (!klass)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			mono::set_field_value(unity::get_local_player(), m_maxEitr, &_eitr_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_maxEitr = mono::get_field(klass, "m_maxEitr");

			if (!klass)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			float default_stamina_regen = 0.f;

			mono::set_field_value(unity::get_local_player(), m_maxEitr, &default_stamina_regen);
		}
	};

	static eitr _eitr("eitr", "Max Eitr", "Max Eitr");
}