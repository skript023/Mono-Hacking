#pragma once
#include "player.hpp"

namespace big
{
	class self
	{
		player m_player;
		static self& get()
		{
			static self instance;

			return instance;
		}

	private:
		player get_player_impl();
		void update_impl();
	public:
		self();
		static player get_player()
		{
			return get().get_player_impl();
		}
		static void update()
		{
			get().update_impl();
		}
	};
}