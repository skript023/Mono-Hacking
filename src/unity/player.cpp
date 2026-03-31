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
	void player::set_max_carry(float carry)
	{
		if (!mono::set_field_value(m_character, "Player", "m_maxCarryWeight", carry))
		{
			LOG(FATAL) << "Failed to set max carry value";
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
	std::vector<MonoObject*> player::get_foods()
	{
		std::vector<MonoObject*> result;
		auto player = unity::get_local_player(); // sesuaikan fungsi kamu
		if (!player)
			return result;

		// Panggil GetFoods()
		auto method = mono::get_method("Player", "GetFoods", 0, "assembly_valheim");

		auto foodsList = mono::invoke_method(method, player);
		if (!foodsList)
		{
			LOG(WARNING) << "Failed to get food list object.";
			return result;
		}

		return unity::list_to_vector(foodsList);
	}

	std::vector<MonoObject*> player::get_all_players()
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
		return unity::list_to_vector(result);
	}
}