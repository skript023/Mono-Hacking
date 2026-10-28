#include "commands/looped_command.hpp"
#include "features/features.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

#include <algorithm>

namespace big::features
{
	class eitr : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_player = nullptr;

		virtual void on_tick() override
		{
			auto player  = self::get_player();
			auto* object = player.get_object();
			if (!object)
			{
				return;
			}

			const float desired_eitr = _eitr_amount.get_state();
			const float current_eitr = player.get_max_eitr();
			if (object != m_player)
			{
				m_player = object;
				if (current_eitr != desired_eitr)
				{
					_food_max_eitr = current_eitr;
				}
			}
			if (current_eitr != desired_eitr)
			{
				player.set_max_eitr(desired_eitr);
			}
		}

		virtual void on_disable() override
		{
			async_run([this] {
				auto player           = self::get_player();
				const float food_eitr = _food_max_eitr.exchange(-1.f);
				if (food_eitr >= 0.f && player.get_object() && player.get_object() == m_player)
				{
					player.set_max_eitr(food_eitr);
					player.set_eitr(std::clamp(player.get_eitr(), 0.f, food_eitr));
				}
				m_player = nullptr;
			});
		}
	};

	static eitr _eitr("eitr", "Max Eitr", "Max Eitr");
}
