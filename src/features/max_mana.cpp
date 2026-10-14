#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _eitr_amount("eitr_amount", "Maximum Eitr", "Maximum eitr capacity while the override is enabled.", 0.f, 10000.f, 0.f);
	class eitr : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player  = nullptr;
		float m_original_eitr = 0.f;
		bool m_has_original   = false;

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
				m_player        = object;
				m_original_eitr = player.get_max_eitr();
				m_has_original  = true;
			}
			const float desired_eitr = _eitr_amount.get_state();
			if (player.get_max_eitr() != desired_eitr)
			{
				player.set_max_eitr(desired_eitr);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (m_has_original && player.get_object() == m_player)
			{
				player.set_max_eitr(m_original_eitr);
			}
			m_player        = nullptr;
			m_original_eitr = 0.f;
			m_has_original  = false;
		}
	};

	static eitr _eitr("eitr", "Max Eitr", "Max Eitr");
}
