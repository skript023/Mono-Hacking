#include "commands/looped_command.hpp"
#include "commands/bool_command.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	class disable_raids : public looped_command
	{
		using looped_command::looped_command;
		uintptr_t m_handle = 0;
		float m_original   = 0.f;

		static MonoClassField* chance_field()
		{
			static auto field = mono::get_field("RandEventSystem", "m_eventChance", "assembly_valheim");
			return field;
		}

		void restore()
		{
			if (auto res = mono::retained_object(m_handle))
			{
				mono::set_field_value(res, chance_field(), &m_original);
			}
			mono::release(m_handle);
			m_handle = 0;
		}

		virtual void on_tick() override
		{
			auto res = unity::get_rand_event_system();
			if (!res || !chance_field())
			{
				return;
			}

			if (mono::retained_object(m_handle) != res)
			{
				restore();
				m_handle = mono::retain(res);
				mono::get_field_value(res, chance_field(), &m_original);
			}

			float zero = 0.f;
			mono::set_field_value(res, chance_field(), &zero);
		}

		virtual void on_disable() override
		{
			async_run([this] {
				restore();
			});
		}
	};

	static disable_raids _disable_raids("disable_raids", "Disable Base Raids", "Prevent all random monster attacks from spawning near your base.");
	static bool_command _keep_skills_on_death("keep_skills_on_death", "Retain Skills on Death", "Prevent losing 5% of skill levels upon dying.", true);
}
