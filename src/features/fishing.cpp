#include "commands/looped_command.hpp"
#include "features/field_override.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"
#include "unity/fishing_tools.hpp"

namespace big::features
{
	class fishing_unbreakable_line : public looped_command
	{
		using looped_command::looped_command;
		field_override<float, 2> m_override{"FishingFloat", {"m_breakDistance", "m_maxDistance"}};

		virtual void on_tick() override
		{
			auto floats = fishing_tools::get_floats();
			m_override.forget_missing(floats);
			for (auto* fl : floats)
			{
				if (m_override.original(fl))
				{
					m_override.write(fl, {99999.f, 99999.f});
				}
			}
		}

		virtual void on_disable() override
		{
			async_run([this] {
				m_override.restore();
			});
		}
	};

	class fishing_no_stamina : public looped_command
	{
		using looped_command::looped_command;
		field_override<float, 2> m_override{"FishingFloat", {"m_pullStaminaUse", "m_hookedStaminaPerSec"}};

		virtual void on_tick() override
		{
			auto floats = fishing_tools::get_floats();
			m_override.forget_missing(floats);
			for (auto* fl : floats)
			{
				if (m_override.original(fl))
				{
					m_override.write(fl, {0.f, 0.f});
				}
			}
		}

		virtual void on_disable() override
		{
			async_run([this] {
				m_override.restore();
			});
		}
	};

	class fishing_auto_catch : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto floats = fishing_tools::get_floats();
			if (floats.empty())
				return;

			auto float_class = mono::get_class("FishingFloat", "assembly_valheim");
			if (!float_class)
				return;

			static auto get_catch_method = mono::get_method("FishingFloat", "GetCatch", 0, "assembly_valheim");
			static auto f_line_len = mono::get_field(float_class, "m_lineLength");
			static auto f_pull_speed = mono::get_field(float_class, "m_pullLineSpeed");

			if (!get_catch_method || !f_line_len)
				return;

			for (auto* fl : floats)
			{
				if (!fl)
					continue;

				auto catch_fish = mono::invoke_method(get_catch_method, fl, nullptr);
				if (catch_fish)
				{
					float short_len = 0.2f;
					mono::set_field_value(fl, f_line_len, &short_len);

					if (f_pull_speed)
					{
						float fast_speed = 30.f;
						mono::set_field_value(fl, f_pull_speed, &fast_speed);
					}
				}
			}
		}
	};

	static fishing_unbreakable_line _fishing_unbreakable_line("fishing_unbreakable_line", "Unbreakable Fishing Line", "Fishing line never snaps regardless of fish strength or distance.");
	static fishing_no_stamina _fishing_no_stamina("fishing_no_stamina", "Zero Stamina Fishing", "Reeling and holding hooked fish consumes 0 stamina.");
	static fishing_auto_catch _fishing_auto_catch("fishing_auto_catch", "Auto Reel-in & Catch", "Automatically reel in and catch fish the instant they bite.");
}

