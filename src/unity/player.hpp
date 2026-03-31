#pragma once
#include "mono/mono.hpp"

namespace big
{
	class player
	{
		MonoObject* m_player{};
	public:
		player(MonoObject* player);
		~player() noexcept;

		void set_max_health(float health, bool flash);
		void set_base_health(float health);
		void set_max_eitr(float eitr);
		void set_base_stamina(float stamina);
		void set_max_stamina(float stamina, bool flash);
	};
}