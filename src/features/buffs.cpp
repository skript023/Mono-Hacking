#include "commands/looped_command.hpp"
#include "unity/buff_tools.hpp"

#include <chrono>

namespace big::features
{
	class auto_cleanse_debuffs : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			buff_tools::clear_all_debuffs(false);
		}
	};

	class keep_rested : public looped_command
	{
		using looped_command::looped_command;

		std::chrono::steady_clock::time_point m_last_applied{};

		virtual void on_enable() override
		{
			buff_tools::apply_rested(25, false);
			m_last_applied = std::chrono::steady_clock::now();
		}

		virtual void on_tick() override
		{
			auto now = std::chrono::steady_clock::now();
			if (now - m_last_applied > std::chrono::seconds(10))
			{
				m_last_applied = now;
				buff_tools::apply_rested(25, false);
			}
		}
	};

	static auto_cleanse_debuffs _auto_cleanse_debuffs("auto_cleanse_debuffs", "Auto-Purge Harmful Debuffs", "Automatically cleanse Wet, Poison, Burning, Freeze, etc.");
	static keep_rested _keep_rested("keep_rested", "Keep Rested (Comfort 25+)", "Automatically maintain maximum Rested buff.");
}

