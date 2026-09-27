#include "self.hpp"
#include "utility/unity.hpp"

namespace big
{
	self::self() :
	    m_player(nullptr)
	{
	}
	player self::get_player_impl()
	{
		if (!m_player)
			m_player = player(unity::get_local_player());
		return m_player;
	}
	void self::update_impl()
	{
		m_player = player(unity::get_local_player());
	}
}
