#include "self.hpp"
#include "utility/unity.hpp"

namespace big
{
	self::self(): m_player(nullptr)
	{
	}
	player self::get_player_impl()
	{
		return m_player;
	}
	void self::update_impl()
	{
		if (m_player == nullptr)
			m_player = player(unity::get_local_player());
	}
}