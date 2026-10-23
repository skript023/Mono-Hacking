#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	static constexpr bool get_entity(std::string const& hash)
	{
	}
	class tamed_all_deer : public command
	{
		using command::command;

		virtual void on_call() override
		{
			auto characters = character::get_all_scharacters();

			for (auto character : characters)
			{
				if (!character || (uintptr_t)character.get_object() < 0x10000)
					continue;

				if (mono::get_field_value<"Character", "m_name", std::string>(character.get_object()) == "$enemy_deer")
				{
					character.set_tamed(true);
				}
			}
		}
	};
	class tamed_all_boar : public command
	{
		using command::command;

		virtual void on_call() override
		{
			auto characters = character::get_all_scharacters();

			for (auto character : characters)
			{
				if (!character || (uintptr_t)character.get_object() < 0x10000)
					continue;

				if (mono::get_field_value<"Character", "m_name", std::string>(character.get_object()) == "$enemy_boar")
				{
					character.set_tamed(true);
				}
			}
		}
	};

	class tamed_all_wolf : public command
	{
		using command::command;

		virtual void on_call() override
		{
			auto characters = character::get_all_scharacters();

			for (auto character : characters)
			{
				if (!character || (uintptr_t)character.get_object() < 0x10000)
					continue;

				if (mono::get_field_value<"Character", "m_name", std::string>(character.get_object()) == "$enemy_wolf")
				{
					character.set_tamed(true);
				}
			}
		}
	};

	static tamed_all_deer _tamed_all_deer("tamed_all_deer", "Tamed All Dear", "Tamed All Dear");
	static tamed_all_boar _tamed_all_boar("tamed_all_boar", "Tamed All Boar", "Tamed All Boar");
	static tamed_all_wolf _tamed_all_wolf("tamed_all_wolf", "Tamed All Wolf", "Tamed All Wolf");
}
