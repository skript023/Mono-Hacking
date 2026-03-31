#pragma once
#include "player.hpp"

namespace big
{
	class self
	{
		player m_player;
		static self& get()
		{
			static self instance{};

			return instance;
		}

	private:
		player get_player_impl();
		void set_max_health_impl(float health, bool flash);
		void set_base_health_impl(float health);
		void set_max_eitr_impl(float eitr);
	public:
		self();
		static player get_player()
		{
			return get().get_player_impl();
		}
		static void set_max_health(float health, bool flash)
		{
			get().set_max_health_impl(health, flash);
		}
	};
}