#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"
#include "unity/ship_tools.hpp"

#include <algorithm>

namespace big::features
{
	class ship_ashlands_immune : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto ship = ship_tools::get_local_ship();
			if (!ship)
				return;

			auto ship_class = mono::get_class("Ship", "assembly_valheim");
			if (!ship_class)
				return;

			static auto f_ashlands = mono::get_field(ship_class, "m_ashlandsReady");
			if (f_ashlands)
			{
				bool ready = true;
				mono::set_field_value(ship, f_ashlands, &ready);
			}
		}
	};

	class ship_no_wave_damage : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto ship = ship_tools::get_local_ship();
			if (!ship)
				return;

			auto ship_class = mono::get_class("Ship", "assembly_valheim");
			if (!ship_class)
				return;

			static auto f_impact_dmg = mono::get_field(ship_class, "m_waterImpactDamage");
			static auto f_impact_force = mono::get_field(ship_class, "m_minWaterImpactForce");
			static auto f_upside_dmg = mono::get_field(ship_class, "m_upsideDownDmg");

			if (f_impact_dmg)
			{
				float zero = 0.f;
				mono::set_field_value(ship, f_impact_dmg, &zero);
			}
			if (f_impact_force)
			{
				float max_force = 999999.f;
				mono::set_field_value(ship, f_impact_force, &max_force);
			}
			if (f_upside_dmg)
			{
				float zero = 0.f;
				mono::set_field_value(ship, f_upside_dmg, &zero);
			}
		}

		virtual void on_disable() override
		{
			auto ship = ship_tools::get_local_ship();
			if (!ship)
				return;

			auto ship_class = mono::get_class("Ship", "assembly_valheim");
			if (!ship_class)
				return;

			static auto f_impact_dmg = mono::get_field(ship_class, "m_waterImpactDamage");
			static auto f_impact_force = mono::get_field(ship_class, "m_minWaterImpactForce");
			static auto f_upside_dmg = mono::get_field(ship_class, "m_upsideDownDmg");

			if (f_impact_dmg)
			{
				float default_dmg = 1.f;
				mono::set_field_value(ship, f_impact_dmg, &default_dmg);
			}
			if (f_impact_force)
			{
				float default_force = 2.5f;
				mono::set_field_value(ship, f_impact_force, &default_force);
			}
			if (f_upside_dmg)
			{
				float default_upside = 10.f;
				mono::set_field_value(ship, f_upside_dmg, &default_upside);
			}
		}
	};

	float_command _ship_speed_multiplier("ship_speed_multiplier", "Ship Speed Multiplier", "Multiply sail speed, reverse thrust, and rudder responsiveness.", 1.f, 5.f, 2.f);

	class ship_speed : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto ship = ship_tools::get_local_ship();
			if (!ship)
				return;

			auto ship_class = mono::get_class("Ship", "assembly_valheim");
			if (!ship_class)
				return;

			float mult = _ship_speed_multiplier.get_state();
			static auto f_sail_force = mono::get_field(ship_class, "m_sailForceFactor");
			static auto f_stear_force = mono::get_field(ship_class, "m_stearForce");
			static auto f_back_force = mono::get_field(ship_class, "m_backwardForce");
			static auto f_rudder_spd = mono::get_field(ship_class, "m_rudderSpeed");

			float sail_force = 0.1f * mult;
			float stear_force = 0.5f * std::min(mult, 3.f);
			float back_force = 50.f * mult;
			float rudder_spd = 0.5f * std::min(mult, 2.5f);

			if (f_sail_force)
				mono::set_field_value(ship, f_sail_force, &sail_force);
			if (f_stear_force)
				mono::set_field_value(ship, f_stear_force, &stear_force);
			if (f_back_force)
				mono::set_field_value(ship, f_back_force, &back_force);
			if (f_rudder_spd)
				mono::set_field_value(ship, f_rudder_spd, &rudder_spd);
		}

		virtual void on_disable() override
		{
			auto ship = ship_tools::get_local_ship();
			if (!ship)
				return;

			auto ship_class = mono::get_class("Ship", "assembly_valheim");
			if (!ship_class)
				return;

			static auto f_sail_force = mono::get_field(ship_class, "m_sailForceFactor");
			static auto f_stear_force = mono::get_field(ship_class, "m_stearForce");
			static auto f_back_force = mono::get_field(ship_class, "m_backwardForce");
			static auto f_rudder_spd = mono::get_field(ship_class, "m_rudderSpeed");

			float sail_force = 0.1f;
			float stear_force = 0.5f;
			float back_force = 50.f;
			float rudder_spd = 0.5f;

			if (f_sail_force)
				mono::set_field_value(ship, f_sail_force, &sail_force);
			if (f_stear_force)
				mono::set_field_value(ship, f_stear_force, &stear_force);
			if (f_back_force)
				mono::set_field_value(ship, f_back_force, &back_force);
			if (f_rudder_spd)
				mono::set_field_value(ship, f_rudder_spd, &rudder_spd);
		}
	};

	static ship_ashlands_immune _ship_ashlands_immune("ship_ashlands_immune", "Ashlands Ocean Boiling Immunity", "Normal wooden ships can sail safely in Ashlands waters without burning.");
	static ship_no_wave_damage _ship_no_wave_damage("ship_no_wave_damage", "No Wave & Collision Damage", "Ships take zero impact damage from crashing waves, rocks, or capsizing.");
	static ship_speed _ship_speed("ship_speed", "Ship Speed Boost", "Multiply sail speed, reverse thrust, and rudder responsiveness.");
}

