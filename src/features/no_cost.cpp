#include "commands/looped_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	class open_all_recepies : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			MonoObject* player_instance = unity::get_local_player();

			if (player_instance == nullptr)
			{
				LOG(WARNING) << "Gagal mendapatkan instance Local Player. Tidak dapat menyetel cheat.";
				return;
			}

			// 2. Dapatkan Class dan Field
			MonoClass* player_class = mono::get_class("Player", "assembly_valheim");
			if (player_class == nullptr) return;

			// Field internal yang dikontrol oleh NoCostCheat()
			MonoClassField* no_cost_cheat_field = mono::get_field(player_class, "m_noPlacementCost");

			if (no_cost_cheat_field == nullptr)
			{
				no_cost_cheat_field = mono::get_field(player_class, "m_noCostCheat");
				LOG(WARNING) << "Gagal menemukan field Player::m_noCostCheat. Periksa decompiler!";
				return;
			}

			if (no_cost_cheat_field == nullptr)
			{
				LOG(WARNING) << "Gagal menemukan field Player::m_noPlacementCost atau m_noCostCheat.";
				return;
			}

			// 3. Set Nilai
			// Mono merepresentasikan bool C# (saat set/get field) sebagai int32_t (4 bytes)
			int32_t value = this->get_state() ? 1 : 0;

			// Set field m_noCostCheat pada instance player
			mono::set_field_value(player_instance, no_cost_cheat_field, &value);

			LOG(INFO) << "Force Crafting Anywhere/All Recipes disetel ke: " << (value ? "TRUE" : "FALSE");
		}

		virtual void on_disable() override
		{
			MonoObject* player_instance = unity::get_local_player();

			if (player_instance == nullptr)
			{
				LOG(WARNING) << "Gagal mendapatkan instance Local Player. Tidak dapat menyetel cheat.";
				return;
			}

			// 2. Dapatkan Class dan Field
			MonoClass* player_class = mono::get_class("Player", "assembly_valheim");
			if (player_class == nullptr) return;

			// Field internal yang dikontrol oleh NoCostCheat()
			MonoClassField* no_cost_cheat_field = mono::get_field(player_class, "m_noPlacementCost");

			if (no_cost_cheat_field == nullptr)
			{
				no_cost_cheat_field = mono::get_field(player_class, "m_noCostCheat");
				LOG(WARNING) << "Gagal menemukan field Player::m_noCostCheat. Periksa decompiler!";
				return;
			}

			if (no_cost_cheat_field == nullptr)
			{
				LOG(WARNING) << "Gagal menemukan field Player::m_noPlacementCost atau m_noCostCheat.";
				return;
			}

			// 3. Set Nilai
			// Mono merepresentasikan bool C# (saat set/get field) sebagai int32_t (4 bytes)
			int32_t value = this->get_state() ? 1 : 0;

			// Set field m_noCostCheat pada instance player
			mono::set_field_value(player_instance, no_cost_cheat_field, &value);

			LOG(INFO) << "Force Crafting Anywhere/All Recipes disetel ke: " << (value ? "TRUE" : "FALSE");
		}
	};

	static open_all_recepies _open_all_recepies("open_all_recepies", "Opens all recipes and free crafting", "Opens all recipes and free crafting");
}