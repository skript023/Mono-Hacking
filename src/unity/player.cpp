#include "player.hpp"
#include "utility/unity.hpp"

namespace big
{
	player::player(MonoObject* player): m_player(player)
	{
		
	}
	player::~player() noexcept
	{
		m_player = nullptr;
	}
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

		mono::invoke_method(method, m_player, args.data());
	}
	void player::set_base_health(float health)
	{
		if (!mono::set_field_value(m_player, "Player", "m_baseHP", health))
		{
			LOG(FATAL) << "Failed to set base health value";
		}
	}
	void player::set_max_eitr(float eitr)
	{
		if (!mono::set_field_value(m_player, "Player", "m_maxEitr", eitr))
		{
			LOG(FATAL) << "Failed to set max eitr value";
		}
	}
	void player::set_base_stamina(float stamina)
	{
		if (!mono::set_field_value(m_player, "Player", "m_baseStamina", stamina))
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

		mono::invoke_method(method, m_player, args.data());
	}
}