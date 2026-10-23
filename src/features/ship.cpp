#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"
#include "task_mgr.hpp"
#include "unity/ship_tools.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace big::features
{
	template<typename T, size_t N>
	class ship_override
	{
		struct entry
		{
			uintptr_t handle{};
			std::array<T, N> original{};
		};

		std::array<const char*, N> m_names;
		std::array<MonoClassField*, N> m_fields{};
		bool m_resolved = false;
		std::vector<entry> m_entries;

		bool resolve()
		{
			if (!m_resolved)
			{
				auto ship_class = mono::get_class("Ship", "assembly_valheim");
				if (!ship_class)
				{
					return false;
				}
				for (size_t i = 0; i < N; ++i)
				{
					m_fields[i] = mono::get_field(ship_class, m_names[i]);
				}
				m_resolved = true;
			}
			return std::ranges::all_of(m_fields, [](MonoClassField* field) {
				return field != nullptr;
			});
		}

	public:
		explicit ship_override(std::array<const char*, N> names) :
		    m_names(names)
		{
		}

		const std::array<T, N>* original(MonoObject* ship)
		{
			if (!ship || !resolve())
			{
				return nullptr;
			}
			for (const auto& recorded : m_entries)
			{
				if (mono::retained_object(recorded.handle) == ship)
				{
					return &recorded.original;
				}
			}

			entry recorded{mono::retain(ship)};
			if (!recorded.handle)
			{
				return nullptr;
			}
			for (size_t i = 0; i < N; ++i)
			{
				mono::get_field_value(ship, m_fields[i], &recorded.original[i]);
			}
			return &m_entries.emplace_back(recorded).original;
		}

		void write(MonoObject* ship, std::array<T, N> values)
		{
			for (size_t i = 0; i < N; ++i)
			{
				mono::set_field_value(ship, m_fields[i], &values[i]);
			}
		}

		void restore()
		{
			for (const auto& recorded : m_entries)
			{
				if (auto ship = mono::retained_object(recorded.handle))
				{
					write(ship, recorded.original);
				}
				mono::release(recorded.handle);
			}
			m_entries.clear();
		}
	};

	class ship_ashlands_immune : public looped_command
	{
		using looped_command::looped_command;
		ship_override<bool, 1> m_override{{"m_ashlandsReady"}};

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
		ship_override<float, 3> m_override{{"m_waterImpactDamage", "m_minWaterImpactForce", "m_upsideDownDmg"}};

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
		ship_override<float, 4> m_override{{"m_sailForceFactor", "m_stearForce", "m_backwardForce", "m_rudderSpeed"}};

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
