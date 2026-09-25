#include "ship_tools.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

namespace big
{
	MonoObject* ship_tools::get_local_ship()
	{
		static auto method = mono::get_method("Ship", "GetLocalShip", 0, "assembly_valheim");
		if (!method)
			return nullptr;

		return mono::invoke_method(method, nullptr, nullptr);
	}

	void ship_tools::update()
	{
		auto ship = get_local_ship();
		if (!ship)
			return;

		auto ship_class = mono::get_class("Ship", "assembly_valheim");
		if (!ship_class)
			return;

		// Ashlands boiling water immunity
		if (g_settings.self.ship_ashlands_immune)
		{
			static auto f_ashlands = mono::get_field(ship_class, "m_ashlandsReady");
			if (f_ashlands)
			{
				bool ready = true;
				mono::set_field_value(ship, f_ashlands, &ready);
			}
		}

		// Wave impact & collision damage immunity
		if (g_settings.self.ship_no_wave_damage)
		{
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

		// Speed & Steering multipliers
		float mult = g_settings.self.ship_speed_multiplier;
		if (mult > 1.05f)
		{
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
	}

	ship_tools::ship_status ship_tools::get_status()
	{
		ship_status st;
		auto ship = get_local_ship();
		if (!ship)
			return st;

		st.in_ship = true;
		st.position = unity::get_position(ship);

		auto ship_class = mono::get_class("Ship", "assembly_valheim");
		if (ship_class)
		{
			static auto f_ashlands = mono::get_field(ship_class, "m_ashlandsReady");
			if (f_ashlands)
				mono::get_field_value(ship, f_ashlands, &st.ashlands_ready);

			static auto f_rudder = mono::get_field(ship_class, "m_rudderValue");
			if (f_rudder)
				mono::get_field_value(ship, f_rudder, &st.rudder);

			static auto get_speed_method = mono::get_method("Ship", "GetSpeed", 0, "assembly_valheim");
			if (get_speed_method)
			{
				auto ret = mono::invoke_method(get_speed_method, ship, nullptr);
				if (ret)
				{
					auto val = mono::object_unbox(ret);
					if (val)
						st.speed = *reinterpret_cast<float*>(val);
				}
			}
		}

		return st;
	}

	void ship_tools::emergency_anchor()
	{
		auto ship = get_local_ship();
		if (!ship)
		{
			notification::warning("Ship Tools", "You are not on a boat.");
			return;
		}

		auto ship_class = mono::get_class("Ship", "assembly_valheim");
		if (!ship_class)
			return;

		static auto f_body = mono::get_field(ship_class, "m_body");
		if (f_body)
		{
			MonoObject* body = nullptr;
			mono::get_field_value(ship, f_body, &body);
			if (body)
			{
				static auto set_lin_vel = mono::get_method("Rigidbody", "set_linearVelocity", 1, "UnityEngine.PhysicsModule", "UnityEngine");
				static auto set_ang_vel = mono::get_method("Rigidbody", "set_angularVelocity", 1, "UnityEngine.PhysicsModule", "UnityEngine");

				Vector3 zero{0.f, 0.f, 0.f};
				void* args[1] = {&zero};
				if (set_lin_vel)
					mono::invoke_method(set_lin_vel, body, args);
				if (set_ang_vel)
					mono::invoke_method(set_ang_vel, body, args);
			}
		}

		notification::success("Ship Tools", "Emergency Anchor deployed! Boat halted.");
	}

	void ship_tools::boost_forward(float force)
	{
		auto ship = get_local_ship();
		if (!ship)
		{
			notification::warning("Ship Tools", "You are not on a boat.");
			return;
		}

		auto ship_class = mono::get_class("Ship", "assembly_valheim");
		if (!ship_class)
			return;

		static auto f_body = mono::get_field(ship_class, "m_body");
		if (f_body)
		{
			MonoObject* body = nullptr;
			mono::get_field_value(ship, f_body, &body);
			if (body)
			{
				Vector3 forward = unity::get_forward(ship);
				Vector3 impulse = forward * force;

				static auto add_force = mono::get_method("Rigidbody", "AddForce", 2, "UnityEngine.PhysicsModule", "UnityEngine");
				if (add_force)
				{
					int mode = 1; // ForceMode.Impulse
					void* args[2] = {&impulse, &mode};
					mono::invoke_method(add_force, body, args);
					notification::success("Ship Tools", "Forward thruster applied!");
					return;
				}
			}
		}
	}

	void ship_tools::repair_ship()
	{
		auto ship = get_local_ship();
		if (!ship)
		{
			notification::warning("Ship Tools", "You are not on a boat.");
			return;
		}

		static auto comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		auto wearntear_class = mono::get_class("WearNTear", "assembly_valheim");

		if (comp_method && wearntear_class)
		{
			auto mono_type = mono::reflection_type(wearntear_class);
			void* args[1] = {mono_type};
			auto wnt = mono::invoke_method(comp_method, ship, args);
			if (wnt)
			{
				static auto repair_method = mono::get_method("WearNTear", "Repair", 0, "assembly_valheim");
				if (repair_method)
				{
					mono::invoke_method(repair_method, wnt, nullptr);
					notification::success("Ship Tools", "Ship fully repaired!");
					return;
				}
			}
		}

		notification::info("Ship Tools", "Repair command sent.");
	}
}
