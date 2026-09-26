#include "commands/looped_command.hpp"
#include "commands/bool_command.hpp"
#include "mono/mono.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	class disable_raids : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto res = unity::get_rand_event_system();
			if (!res)
				return;

			auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
			if (!res_class)
				return;

			static auto f_chance = mono::get_field(res_class, "m_eventChance");
			if (f_chance)
			{
				float zero = 0.f;
				mono::set_field_value(res, f_chance, &zero);
			}
		}

		virtual void on_disable() override
		{
			auto res = unity::get_rand_event_system();
			if (!res)
				return;

			auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
			if (!res_class)
				return;

			static auto f_chance = mono::get_field(res_class, "m_eventChance");
			if (f_chance)
			{
				float default_chance = 20.f;
				mono::set_field_value(res, f_chance, &default_chance);
			}
		}
	};

	static disable_raids _disable_raids("disable_raids", "Disable Base Raids", "Prevent all random monster attacks from spawning near your base.");
	static bool_command _keep_skills_on_death("keep_skills_on_death", "Retain Skills on Death", "Prevent losing 5% of skill levels upon dying.", true);
}

