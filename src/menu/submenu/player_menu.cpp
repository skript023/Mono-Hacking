#include "../view.hpp"
#include "script.hpp"
#include "mono/mono.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Player", SubmenuPlayer, [](regular_submenu* sub)
        {
            sub->add_option<reguler_option>("Set Max Health", nullptr, [] {
                auto get_local_player_instance = [=] () ->MonoObject*
                {
                    // 1. Cari Class Player
                    MonoClass* player_class = mono::get_class("Player", "assembly_valheim", "");
                    if (player_class == nullptr) return nullptr;

                    // 2. Cari Static Field m_localPlayer
                    MonoClassField* local_player_field = mono::get_field(player_class, "m_localPlayer");
                    if (local_player_field == nullptr) return nullptr;

                    // 3. Dapatkan Base Address Static Field Data
                    void* static_field_data_addr = mono::get_static_field_data(player_class);
                    if (static_field_data_addr == nullptr) return nullptr;

                    // 4. Hitung Offset dan Baca Nilai (MonoObject*)
                    uint32_t offset = mono::get_field_offset(local_player_field);
                    void* local_player_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

                    // Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
                    MonoObject* local_player_instance = *(MonoObject**)local_player_ptr_addr;

                    return local_player_instance;
                };

				auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");
                auto m_baseHP = mono::get_field(klass, "m_baseHP");

                if (!method || !klass)
                {
					LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

                    return;
                }

                float health = 600;
                bool flashBar = true;
                void* args[2] = { &health, &flashBar };
                mono::invoke_method(method, get_local_player_instance(), args);
                mono::set_field_value(get_local_player_instance(), m_baseHP, &health);
            });
            sub->add_option<reguler_option>("Test", nullptr, [] {
                auto get_local_player_instance = [=]() ->MonoObject*
                {
                    // 1. Cari Class Player
                    MonoClass* player_class = mono::get_class("Player", "assembly_valheim", "");
                    if (player_class == nullptr) return nullptr;

                    // 2. Cari Static Field m_localPlayer
                    MonoClassField* local_player_field = mono::get_field(player_class, "m_localPlayer");
                    if (local_player_field == nullptr) return nullptr;

                    // 3. Dapatkan Base Address Static Field Data
                    void* static_field_data_addr = mono::get_static_field_data(player_class);
                    if (static_field_data_addr == nullptr) return nullptr;

                    // 4. Hitung Offset dan Baca Nilai (MonoObject*)
                    uint32_t offset = mono::get_field_offset(local_player_field);
                    void* local_player_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

                    // Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
                    MonoObject* local_player_instance = *(MonoObject**)local_player_ptr_addr;

                    return local_player_instance;
                };

                auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");
                auto m_baseStamina = mono::get_field(klass, "m_baseStamina");
                auto m_stamina = mono::get_field(klass, "m_stamina");

                if (!method || !klass)
                {
                    LOG(WARNING) << "Failed to find method Player::GetPlayerName";

                    return;
                }

                float stamina = 600;
                bool flashBar = true;
                void* args[2] = { &stamina, &flashBar };
                auto result = mono::invoke_method(method, get_local_player_instance(), args);
                LOG(INFO) << "Success invoke Player::SetMaxStamina " << result;

                mono::set_field_value(get_local_player_instance(), m_baseStamina, &stamina);
                mono::set_field_value(get_local_player_instance(), m_stamina, &stamina);
            });
        });
    }
}