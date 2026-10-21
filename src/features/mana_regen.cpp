#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_regen_amount("eitr_regen_amount", "Eitr Regen Amount", "Amount of eitr to regenerate per second.", 0.f, 100.f, 5.f);
	class eitr_regen : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player   = nullptr;
		float m_original_regen = 5.f;
		bool m_has_original    = false;

		virtual void on_tick() override
		{
			auto player  = self::get_player();
			auto* object = player.get_object();
			if (!object)
			{
				return;
			}

			if (object != m_player)
			{
				m_player         = object;
				m_original_regen = player.get_eitr_regen();
				m_has_original   = true;
			}
			const float desired_regen = _eitr_regen_amount.get_state();
			if (player.get_eitr_regen() != desired_regen)
			{
				player.set_eitr_regen(desired_regen);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (m_has_original && player.get_object() == m_player)
			{
				player.set_eitr_regen(m_original_regen);
			}
			m_player         = nullptr;
			m_original_regen = 5.f;
			m_has_original   = false;
		}
	};

	static eitr_regen _eitr_regen("eitr_regen", "Eitr Regen", "Eitr Regeneration");
}
