#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _max_stam("max_stam", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 50.f, 1000.f, 50.f);
	class max_stamina : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player     = nullptr;
		float m_original_stamina = 50.f;
		bool m_has_original      = false;

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
				m_player           = object;
				m_original_stamina = player.get_base_stamina();
				m_has_original     = true;
			}
			const float desired_stamina = _max_stam.get_state();
			if (player.get_base_stamina() != desired_stamina)
			{
				player.set_base_stamina(desired_stamina);
			}
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (m_has_original && player.get_object() == m_player)
			{
				player.set_base_stamina(m_original_stamina);
			}
			m_player           = nullptr;
			m_original_stamina = 50.f;
			m_has_original     = false;
		}
	};

	static max_stamina _max_stamina("max_stamina", "Max Base Stamina", "Increase Maximum Base Stamina");
}
