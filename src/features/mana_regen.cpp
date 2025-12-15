#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_regen_amount("eitr_regen_amount", "Eitr Regen Amount", "Amount of eitr to regenerate per second.", 5.f, 100.f, 5.f);
	class eitr_regen : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_eiterRegen = mono::get_field(klass, "m_eiterRegen");

			if (!klass)
			{
				return;
			}

			mono::set_field_value(unity::get_local_player(), m_eiterRegen, &_eitr_regen_amount.get_state());
		}

		virtual void on_disable() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_eiterRegen = mono::get_field(klass, "m_eiterRegen");

			if (!klass)
			{
				return;
			}

			float default_stamina_regen = 5.f;

			mono::set_field_value(unity::get_local_player(), m_eiterRegen, &default_stamina_regen);
		}
	};

	static eitr_regen _eitr_regen("eitr_regen", "Eitr Regen", "Eitr Regeneration");
}