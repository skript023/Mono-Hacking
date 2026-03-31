#pragma once
#include "mono/mono.hpp"
#include "humanoid.hpp"

namespace big
{
	class player : public humanoid
	{
	public:
		using humanoid::humanoid;
		
		void set_max_health(float health, bool flash);
		void set_base_health(float health);
		void set_max_eitr(float eitr);
		void set_base_stamina(float stamina);
		void set_max_stamina(float stamina, bool flash);
		void set_max_carry(float carry);
		void set_no_placement_cost(bool cost);
		void set_max_food(int food);
		void add_eitr(float eitr);
		std::vector<MonoObject*> get_foods();
	};
}