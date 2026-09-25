#pragma once
#include "mono/mono.hpp"
#include <string>
#include <vector>

namespace big
{
	class buff_tools
	{
	public:
		struct boss_power
		{
			std::string internal_name;
			std::string display_name;
			std::string boss;
			std::string effect_description;
		};

		static const std::vector<boss_power>& get_boss_powers();
		static void apply_rested(int comfort = 20);
		static void activate_guardian_power(const std::string& power_name);
		static void activate_all_guardian_powers();
		static void apply_eitr_shield(float hp = 1000.f);
		static void clear_all_debuffs();
		static void remove_status_effect(const std::string& name);
		static void update();
	};
}

