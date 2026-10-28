#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "features/field_override.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"
#include "unity/ship_tools.hpp"

#include <algorithm>

namespace big::features
{
	class ship_ashlands_immune : public looped_command
	{
		using looped_command::looped_command;
		field_override<bool, 1> m_override{"Ship", {"m_ashlandsReady"}};

		virtual void on_tick() override
		{
			auto ship = ship_tools::get_local_ship();
			if (m_override.original(ship))
			{
				m_override.write(ship, {true});
			}
		}

		virtual void on_disable() override
		{
			async_run([this] {
				m_override.restore();
			});
		}
	};

	class ship_no_wave_damage : public looped_command
	{
		using looped_command::looped_command;
		field_override<float, 3> m_override{"Ship", {"m_waterImpactDamage", "m_minWaterImpactForce", "m_upsideDownDmg"}};

		virtual void on_tick() override
		{
			auto ship = ship_tools::get_local_ship();
			if (m_override.original(ship))
			{
				m_override.write(ship, {0.f, 999999.f, 0.f});
			}
		}

		virtual void on_disable() override
		{
			async_run([this] {
				m_override.restore();
			});
		}
	};

	float_command _ship_speed_multiplier("ship_speed_multiplier", "Ship Speed Multiplier", "Multiply sail speed, reverse thrust, and rudder responsiveness.", 1.f, 5.f, 2.f);

	class ship_speed : public looped_command
	{
		using looped_command::looped_command;
		field_override<float, 4> m_override{"Ship", {"m_sailForceFactor", "m_stearForce", "m_backwardForce", "m_rudderSpeed"}};

		virtual void on_tick() override
		{
			auto ship     = ship_tools::get_local_ship();
			auto original = m_override.original(ship);
			if (!original)
			{
				return;
			}

			const auto& [sail_force, stear_force, back_force, rudder_spd] = *original;

			const float mult = _ship_speed_multiplier.get_state();
			m_override.write(ship, {sail_force * mult, stear_force * std::min(mult, 3.f), back_force * mult, rudder_spd * std::min(mult, 2.5f)});
		}

		virtual void on_disable() override
		{
			async_run([this] {
				m_override.restore();
			});
		}
	};

	static ship_ashlands_immune _ship_ashlands_immune("ship_ashlands_immune", "Ashlands Ocean Boiling Immunity", "Normal wooden ships can sail safely in Ashlands waters without burning.");
	static ship_no_wave_damage _ship_no_wave_damage("ship_no_wave_damage", "No Wave & Collision Damage", "Ships take zero impact damage from crashing waves, rocks, or capsizing.");
	static ship_speed _ship_speed("ship_speed", "Ship Speed Boost", "Multiply sail speed, reverse thrust, and rudder responsiveness.");
}
