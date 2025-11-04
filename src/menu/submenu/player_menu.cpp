#include "../view.hpp"
#include "script.hpp"
#include "mono/mono.hpp"
#include "utility/unity.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Player", SubmenuPlayer, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("Flying", nullptr, &g_settings.self.flying);
            sub->add_option<bool_option<bool>>("Allow Teleporting with Any Items", nullptr, &g_settings.self.is_teleportable);
            sub->add_option<bool_option<bool>>("No Weight", nullptr, &g_settings.self.no_weight);
            sub->add_option<bool_option<bool>>("Don't Drop Items when Dead", nullptr, &g_settings.self.no_drop_on_dead);
            sub->add_option<bool_option<bool>>("Never Wet", nullptr, &g_settings.self.is_wet);
            sub->add_option<bool_option<bool>>("Forsaken Power Always Ready", nullptr, &g_settings.self.forsaken_power_always_ready);
            sub->add_option<bool_option<bool>>("Opens all recipes and free crafting", nullptr, &g_settings.self.open_all_recipe_and_free_craft, [] {
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
                int32_t value = g_settings.self.open_all_recipe_and_free_craft ? 1 : 0;

                // Set field m_noCostCheat pada instance player
                mono::set_field_value(player_instance, no_cost_cheat_field, &value);

                LOG(INFO) << "Force Crafting Anywhere/All Recipes disetel ke: " << (g_settings.self.open_all_recipe_and_free_craft ? "TRUE" : "FALSE");
            });
            sub->add_option<bool_slider_float_option>("Set Max Health", nullptr, &g_settings.self.enable_max_hp, &g_settings.self.max_hp, 25.f, 1000.f, 10.f, 3, true, [] {
				auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");
                auto m_baseHP = mono::get_field(klass, "m_baseHP");

                if (!method || !klass)
                {
					LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

                    return;
                }

                bool flashBar = true;

                void* args[2] = { &g_settings.self.max_hp, &flashBar };
                mono::invoke_method(method, unity::get_local_player(), args);
                mono::set_field_value(unity::get_local_player(), m_baseHP, &g_settings.self.max_hp);
            });
            sub->add_option<bool_slider_float_option>("Set Max Stamina", nullptr, &g_settings.self.enable_max_stam, &g_settings.self.max_stam, 50.f, 1000.f, 10.f, 3, true, [] {
                auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");
                auto m_baseStamina = mono::get_field(klass, "m_baseStamina");
                auto m_stamina = mono::get_field(klass, "m_stamina");

                if (!method || !klass)
                {
                    LOG(WARNING) << "Failed to find method Player::GetPlayerName";

                    return;
                }

                bool flashBar = true;

                void* args[2] = { &g_settings.self.max_stam, &flashBar };
                mono::invoke_method(method, unity::get_local_player(), args);

                mono::set_field_value(unity::get_local_player(), m_baseStamina, &g_settings.self.max_stam);
                mono::set_field_value(unity::get_local_player(), m_stamina, &g_settings.self.max_stam);
            });
        });
    }
}