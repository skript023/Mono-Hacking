#include "player.hpp"
#include "utility/unity.hpp"

namespace big
{
	void player::set_max_health(float health, bool flash)
	{
		auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

			return;
		}

		bool flashBar = flash;
		float max_hp = health;

		std::array<void*, 2> args{};

		args[0] = &max_hp;
		args[1] = &flashBar;

		mono::invoke_method(method, m_character, args.data());
	}
	void player::set_base_health(float health)
	{
		if (!mono::set_field_value(m_character, "Player", "m_baseHP", health))
		{
			LOG(FATAL) << "Failed to set base health value";
		}
	}
	void player::set_max_eitr(float eitr)
	{
		if (!mono::set_field_value(m_character, "Player", "m_maxEitr", eitr))
		{
			LOG(FATAL) << "Failed to set max eitr value";
		}
	}
	void player::set_base_stamina(float stamina)
	{
		if (!mono::set_field_value(m_character, "Player", "m_baseStamina", stamina))
		{
			LOG(FATAL) << "Failed to set base stamina value";
		}
	}
	void player::set_max_stamina(float stamina, bool flash)
	{
		auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::SetMaxStamina";
			return;
		}
		bool flashBar = flash;
		float max_stam = stamina;

		std::array<void*, 2> args{};

		args[0] = &max_stam;
		args[1] = &flashBar;

		mono::invoke_method(method, m_character, args.data());
	}
	void player::set_stamina(float stamina)
	{
		if (!mono::set_field_value(m_character, "Player", "m_stamina", stamina))
		{
			LOG(FATAL) << "Failed to set stamina value";
		}
	}
	void player::set_stamina_regen(float regen)
	{
		if (!mono::set_field_value(m_character, "Player", "m_staminaRegen", regen))
		{
			LOG(FATAL) << "Failed to set stamina regen value";
		}
	}
	void player::set_max_carry(float carry)
	{
		if (!mono::set_field_value(m_character, "Player", "m_maxCarryWeight", carry))
		{
			LOG(FATAL) << "Failed to set max carry value";
		}
	}
	void player::set_adrenalin(float adrenalin)
	{
		if (!mono::set_field_value(m_character, "Player", "m_adrenaline", adrenalin))
		{
			LOG(FATAL) << "Failed to set adrenalin value";
		}
	}
	void player::set_max_adrenalin(float adrenalin)
	{
		if (!mono::set_field_value(m_character, "Player", "m_maxAdrenaline", adrenalin))
		{
			LOG(FATAL) << "Failed to set max adrenalin value";
		}
	}
	void player::set_no_placement_cost(bool cost)
	{
		if (!mono::set_field_value(m_character, "Player", "m_noPlacementCost", cost))
		{
			LOG(FATAL) << "Failed to set no placement cost value";
		}
	}
	void player::set_max_food(int food)
	{
		if (!mono::set_field_value(m_character, "Player", "m_maxFoods", food))
		{
			LOG(FATAL) << "Failed to set max food value";
		}
	}
	void player::add_eitr(float eitr)
	{
		auto method = mono::get_method("Player", "AddEitr", 1, "assembly_valheim");
		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::AddEitr";
			return;
		}

		float amount = eitr;
		void* args[1] = {&amount};
		mono::invoke_method(method, m_character, args);
	}
	void player::set_eitr_regen(float regen)
	{
		if (!mono::set_field_value(m_character, "Player", "m_eiterRegen", regen))
		{
			LOG(FATAL) << "Failed to set eitr regen value";
		}
	}
	void player::teleport_to(Vector3 const& position, Quaternions const& rotation, bool distantTeleport)
	{
		static MonoMethod* method = mono::get_method("Player", "TeleportTo", 3, "assembly_valheim");

		if (!method || !m_character)
		{
			LOG(WARNING) << "Failed to find method Plyer::TeleportTo";

			return;
		}

		bool flashBar = true;

		auto pos = position;
		auto rot = rotation;
		auto distant = distantTeleport;

		std::array<void*, 3> args = {&pos, &rot, &distant};

		mono::invoke_method(method, m_character, args.data());
	}
	float player::get_max_carry()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_maxCarryWeight");
	}
	float player::get_base_health()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_baseHP");
	}
	float player::get_base_stamina()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_baseStamina");
	}
	float player::get_max_eitr()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_maxEitr");
	}
	float player::get_stamina_regen()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_staminaRegen");
	}
	float player::get_eitr_regen()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_eiterRegen");
	}
	float player::get_max_adrenalin()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_maxAdrenaline");
	}
	float player::get_adrenalin()
	{
		return mono::get_field_value<float>(m_character, "Player", "m_adrenaline");
	}
	int player::get_player_id()
	{
		static auto method = mono::get_method("Player", "GetPlayerName", 0, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::GetPlayerID";

			return 0;
		}

		auto result = mono::invoke_method(method, m_character, nullptr);

		return *reinterpret_cast<int*>(mono::object_unbox(result));
	}
	std::string player::get_player_name()
	{
		static auto method = mono::get_method("Player", "GetPlayerName", 0, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::GetPlayerName";
			return {};
		}

		auto result = mono::invoke_method(method, m_character, nullptr);

		return mono::from_mono_string(reinterpret_cast<MonoString*>(result));
	}
	std::vector<food> player::get_foods()
	{
		std::vector<food> result;
		
		if (!m_character)
			return result;

		static auto method = mono::get_method("Player", "GetFoods", 0, "assembly_valheim");

		auto foods = mono::invoke_method(method, m_character);

		if (!foods)
		{
			LOG(WARNING) << "Failed to get food list object.";
			return result;
		}

		return mono::from_list<food>(foods);
	}

	std::vector<player> player::get_all_players()
	{
		static MonoMethod* method = mono::get_method("Player", "GetAllPlayers", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke_method(method, nullptr);
#ifdef _DEBUG
		MonoClass* klass = mono::object_get_class(result);
		const char* class_name = mono::class_get_name(klass);
		const char* namespace_name = mono::class_get_namespace(klass);

		LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;
#endif
		return mono::from_list<player>(result);
	}
}