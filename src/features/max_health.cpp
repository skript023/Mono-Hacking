#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _max_hp("max_hp", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 25.f, 1000.f, 25.f);
	class max_health : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player    = nullptr;
		float m_original_health = 25.f;
		bool m_has_original     = false;

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
				m_player          = object;
				m_original_health = player.get_base_health();
				m_has_original    = true;
			}
			const float desired_health = _max_hp.get_state();
			if (player.get_base_health() != desired_health)
			{
				player.set_base_health(desired_health);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (m_has_original && player.get_object() == m_player)
			{
				player.set_base_health(m_original_health);
			}
			m_player          = nullptr;
			m_original_health = 25.f;
			m_has_original    = false;
		}
	};

	static max_health _max_health("max_health", "Max Base Health", "Increase Max Base Health");
}
