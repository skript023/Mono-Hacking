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
				auto method = mono::get_method("Player", "SetMaxHealth", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");
                auto field = mono::get_field(klass, "m_baseHP");

                if (!method || !klass)
                {
					LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

                    return;
                }

                float health = 600;
                bool flashBar = true;
                void* args[2] = { &health, &flashBar };
                mono::invoke_method(method, klass, args);
                mono::set_field_value((MonoObject*)klass, field, &health);
            });
            sub->add_option<reguler_option>("Test", nullptr, [] {
                auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");
                auto klass = mono::get_class("Player", "assembly_valheim");

                if (!method || !klass)
                {
                    LOG(WARNING) << "Failed to find method Player::GetPlayerName";

                    return;
                }

                float stamina = 600;
                bool flashBar = true;
                void* args[2] = { &stamina, &flashBar };
                mono::invoke_method(method, klass, args);
            });
        });
    }
}