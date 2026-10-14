#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _carry_amount("carry_amount", "", "", 100.f, 10000.f, 100.f);
	class max_carry_weight : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player   = nullptr;
		float m_original_carry = 300.f;
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
				m_original_carry = player.get_max_carry();
				m_has_original   = true;
			}

			const float desired_carry = _carry_amount.get_state();
			if (player.get_max_carry() != desired_carry)
			{
				player.set_max_carry(desired_carry);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (m_has_original && player.get_object() == m_player)
			{
				player.set_max_carry(m_original_carry);
			}
			m_player         = nullptr;
			m_original_carry = 300.f;
			m_has_original   = false;
		}
	};

	static max_carry_weight _max_carry_weight("max_carry_weight", "Max Carry", "Max Carry Weight");
}
