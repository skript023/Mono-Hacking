#include "self.hpp"
#include "utility/unity.hpp"

namespace big
{
	self::self(): m_player(unity::get_local_player())
	{
	}
	player self::get_player_impl()
	{
		return m_player;
	}
	void self::set_max_health_impl(float health, bool flash)
	{
		m_player.set_max_health(health, flash);
	}
	void self::set_base_health_impl(float health)
	{
		m_player.set_base_health(health);
	}
	void self::set_max_eitr_impl(float eitr)
	{
		m_player.set_max_eitr(eitr);
	}
}