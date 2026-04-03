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

		mono::invoke(method, m_character, health, flash);
	}
	void player::set_base_health(float health)
	{
		if (!mono::set_field_value<"Player", "m_baseHP">(m_character, health))
		{
			LOG(FATAL) << "Failed to set base health value";
		}
	}
	void player::set_max_eitr(float eitr)
	{
		if (!mono::set_field_value<"Player", "m_maxEitr">(m_character, eitr))
		{
			LOG(FATAL) << "Failed to set max eitr value";
		}
	}
	void player::set_base_stamina(float stamina)
	{
		if (!mono::set_field_value<"Player", "m_baseStamina">(m_character, stamina))
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

		mono::invoke(method, m_character, stamina, flash);
	}
	void player::set_stamina(float stamina)
	{
		if (!mono::set_field_value<"Player", "m_stamina">(m_character, stamina))
		{
			LOG(FATAL) << "Failed to set stamina value";
		}
	}
	void player::set_stamina_regen(float regen)
	{
		if (!mono::set_field_value<"Player", "m_staminaRegen">(m_character, regen))
		{
			LOG(FATAL) << "Failed to set stamina regen value";
		}
	}
	void player::set_max_carry(float carry)
	{
		if (!mono::set_field_value<"Player", "m_maxCarryWeight">(m_character, carry))
		{
			LOG(FATAL) << "Failed to set max carry value";
		}
	}
	void player::set_adrenalin(float adrenalin)
	{
		if (!mono::set_field_value<"Player", "m_adrenaline">(m_character, adrenalin))
		{
			LOG(FATAL) << "Failed to set adrenalin value";
		}
	}
	void player::set_max_adrenalin(float adrenalin)
	{
		if (!mono::set_field_value<"Player", "m_maxAdrenaline">(m_character, adrenalin))
		{
			LOG(FATAL) << "Failed to set max adrenalin value";
		}
	}
	void player::set_no_placement_cost(BOOL cost)
	{
		if (!mono::set_field_value<"Player", "m_noPlacementCost">(m_character, cost))
		{
			LOG(FATAL) << "Failed to set no placement cost value";
		}
	}
	void player::set_max_food(int food)
	{
		if (!mono::set_field_value<"Player", "m_maxFoods">(m_character, food))
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

		mono::invoke(method, m_character, eitr);
	}
	void player::set_eitr_regen(float regen)
	{
		if (!mono::set_field_value<"Player", "m_eiterRegen">(m_character, regen))
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

		mono::invoke(method, m_character, position, rotation, distantTeleport);
	}
	float player::get_max_carry()
	{
		return mono::get_field_value<"Player", "m_maxCarryWeight", float>(m_character);
	}
	float player::get_base_health()
	{
		return mono::get_field_value<"Player", "m_baseHP", float>(m_character);
	}
	float player::get_base_stamina()
	{
		return mono::get_field_value<"Player", "m_baseStamina", float>(m_character);
	}
	float player::get_max_eitr()
	{
		return mono::get_field_value<"Player", "m_maxEitr", float>(m_character);
	}
	float player::get_stamina_regen()
	{
		return mono::get_field_value<"Player", "m_staminaRegen", float>(m_character);
	}
	float player::get_eitr_regen()
	{
		return mono::get_field_value<"Player", "m_eiterRegen", float>(m_character);
	}
	float player::get_max_adrenalin()
	{
		return mono::get_field_value<"Player", "m_maxAdrenaline", float>(m_character);
	}
	float player::get_adrenalin()
	{
		return mono::get_field_value<"Player", "m_adrenaline", float>(m_character);
	}
	int player::get_player_id()
	{
		static auto method = mono::get_method("Player", "GetPlayerName", 0, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::GetPlayerID";

			return 0;
		}

		auto result = mono::invoke(method, m_character);

		return *reinterpret_cast<int*>(mono::object_unbox(result));
	}
	bool player::is_player()
	{
		static MonoMethod* method = mono::get_method(
			"Player",
			"IsPlayer",
			0,
			"assembly_valheim"
		);

		if (!method || !m_character)
			return false;

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return false;

		return *reinterpret_cast<bool*>(mono::object_unbox(obj));
	}
	skills player::get_skills()
	{
		static MonoMethod* method = mono::get_method(
			"Player",
			"GetSkills",
			0,
			"assembly_valheim"
		);

		if (!method || !m_character)
			return nullptr;

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return nullptr;

		return obj;
	}
	std::string player::get_player_name()
	{
		static auto method = mono::get_method("Player", "GetPlayerName", 0, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Player::GetPlayerName";
			return {};
		}

		auto result = mono::invoke(method, m_character);

		return mono::from_mono_string(reinterpret_cast<MonoString*>(result));
	}
	mono_array_view<food> player::get_foods()
	{
		if (!m_character)
			return {};

		static auto method = mono::get_method("Player", "GetFoods", 0, "assembly_valheim");

		auto foods = mono::invoke(method, m_character);

		if (!foods)
		{
			LOG(WARNING) << "Failed to get food list object.";
			return {};
		}

		return mono::list<food>(foods);
	}

	mono_array_view<player> player::get_all_players()
	{
		static MonoMethod* method = mono::get_method("Player", "GetAllPlayers", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke_method(method);
#ifdef _DEBUG
		MonoClass* klass = mono::object_get_class(result);
		const char* class_name = mono::class_get_name(klass);
		const char* namespace_name = mono::class_get_namespace(klass);

		LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;
#endif
		return mono::list<player>(result);
	}

	mono_array_view<player> player::get_all_splayers()
	{
		auto result = mono::get_static_field_value<"Player", "s_players", MonoObject*>();
#ifdef _DEBUG
		MonoClass* klass = mono::object_get_class(result);
		const char* class_name = mono::class_get_name(klass);
		const char* namespace_name = mono::class_get_namespace(klass);

		LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;
#endif
		return mono::list<player>(result);
	}
}