#pragma once
#include "player.hpp"

namespace big
{
	class self
	{
		player m_player;
		static self& instance()
		{
			static self value;

			return value;
		}

	private:
		self();
		self(const self&) = delete;
		self& operator=(const self&) = delete;
		player get_player_impl();
		void update_impl();

	public:
		static player get_player()
		{
			return instance().get_player_impl();
		}
		static void update()
		{
			instance().update_impl();
		}
	};
}
