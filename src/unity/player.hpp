#pragma once
#include "mono/mono.hpp"
#include "humanoid.hpp"
#include "skills.hpp"
#include "food.hpp"

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
		void set_stamina(float stamina);
		void set_stamina_regen(float regen);
		void set_max_carry(float carry);
		void set_adrenalin(float adrenalin);
		void set_max_adrenalin(float adrenalin);
		void set_no_placement_cost(BOOL cost);
		void set_max_food(int food);
		void add_eitr(float eitr);
		void set_eitr_regen(float regen);
		void teleport_to(Vector3 const& position, Quaternions const& rotation, bool distantTeleport);
		float get_max_carry();
		float get_base_health();
		float get_base_stamina();
		float get_max_eitr();
		float get_stamina_regen();
		float get_eitr_regen();
		float get_max_adrenalin();
		float get_adrenalin();
		int get_player_id();
		bool is_player() override;
		skills get_skills();
		std::string get_player_name();
		mono_array_view<food> get_foods();
		static mono_array_view<player> get_all_players();

		bool operator==(player const& c) const { return m_character == c.m_character; }

		;
	};
}