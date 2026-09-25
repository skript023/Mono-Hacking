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

		static const std::vector<boss_power>& get_boss_powers()
		{
			return instance().get_boss_powers_impl();
		}
		static void apply_rested(int comfort = 20)
		{
			return instance().apply_rested_impl(comfort);
		}
		static void activate_guardian_power(const std::string& power_name)
		{
			return instance().activate_guardian_power_impl(power_name);
		}
		static void activate_all_guardian_powers()
		{
			return instance().activate_all_guardian_powers_impl();
		}
		static void apply_eitr_shield(float hp = 1000.f)
		{
			return instance().apply_eitr_shield_impl(hp);
		}
		static void clear_all_debuffs()
		{
			return instance().clear_all_debuffs_impl();
		}
		static void remove_status_effect(const std::string& name)
		{
			return instance().remove_status_effect_impl(name);
		}
		static void update()
		{
			return instance().update_impl();
		}

	private:
		buff_tools() = default;
		buff_tools(const buff_tools&) = delete;
		buff_tools& operator=(const buff_tools&) = delete;
		static buff_tools& instance()
		{
			static buff_tools value;
			return value;
		}

		const std::vector<boss_power>& get_boss_powers_impl();
		void apply_rested_impl(int comfort);
		void activate_guardian_power_impl(const std::string& power_name);
		void activate_all_guardian_powers_impl();
		void apply_eitr_shield_impl(float hp);
		void clear_all_debuffs_impl();
		void remove_status_effect_impl(const std::string& name);
		void update_impl();
	};
}
