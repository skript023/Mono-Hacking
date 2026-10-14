#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
	namespace
	{
		bool is_local_attack(MonoObject* attack)
		{
			static auto character_field = mono::get_field("Attack", "m_character", "assembly_valheim");
			MonoObject* attacker        = nullptr;
			if (attack && character_field)
			{
				mono::get_field_value(attack, character_field, &attacker);
			}
			return attacker && attacker == unity::get_local_player();
		}

		bool is_ranged_attack(MonoObject* attack)
		{
			static auto bow_draw_field        = mono::get_field("Attack", "m_bowDraw", "assembly_valheim");
			static auto requires_reload_field = mono::get_field("Attack", "m_requiresReload", "assembly_valheim");
			bool bow_draw                     = false;
			bool requires_reload              = false;
			if (attack && bow_draw_field)
			{
				mono::get_field_value(attack, bow_draw_field, &bow_draw);
			}
			if (attack && requires_reload_field)
			{
				mono::get_field_value(attack, requires_reload_field, &requires_reload);
			}
			return bow_draw || requires_reload;
		}
	}

	float hooks::attack_get_stamina(MonoObject* attack)
	{
		float cost = detour_base::get_original<attack_get_stamina>()(attack);
		if (is_local_attack(attack))
		{
			const float multiplier =
			    is_ranged_attack(attack) ? g_settings.self.ranged_stamina_multiplier : g_settings.self.melee_stamina_multiplier;
			cost *= std::clamp(multiplier, 0.f, 100.f);
		}
		return cost;
	}

	float hooks::attack_get_eitr(MonoObject* attack)
	{
		float cost = detour_base::get_original<attack_get_eitr>()(attack);
		if (is_local_attack(attack))
		{
			cost *= std::clamp(g_settings.self.attack_eitr_multiplier, 0.f, 100.f);
		}
		return cost;
	}
}
