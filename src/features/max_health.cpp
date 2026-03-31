#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _max_hp("max_hp", "Stamina Regen Amount", "Amount of stamina to regenerate per second.", 25.f, 1000.f, 25.f);
	class max_health : public looped_command
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
			auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_baseHP = mono::get_field(klass, "m_baseHP");

			if (!method || !klass || !m_baseHP)
			{
				LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

				return;
			}

			bool flashBar = true;

			std::array<void*, 2> args{};

			args[0] = &_max_hp.get_state();
			args[1] = &flashBar;

			mono::invoke_method(method, unity::get_local_player(), args.data());
			mono::set_field_value(unity::get_local_player(), m_baseHP, &_max_hp.get_state());
		}

		virtual void on_disable() override
		{
			auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_baseHP = mono::get_field(klass, "m_baseHP");

			if (!method || !klass)
			{
				LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

				return;
			}

			bool flashBar = true;
			float default_max_hp = 25.f;

			void* args[2] = { &default_max_hp, &flashBar };
			mono::invoke_method(method, unity::get_local_player(), args);
			mono::set_field_value(unity::get_local_player(), m_baseHP, &default_max_hp);
		}
	};

	static max_health _max_health("max_health", "Max Health", "Increase Max Health");
}