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

                if (!method)
                {
					LOG(WARNING) << "Failed to find method Player::SetMaxHealth";

                    return;
                }

                float health = 600;
                bool flashBar = true;
                void* args[2] = { &health, &flashBar };
                mono::invoke_method(method, nullptr, args);
            });
            sub->add_option<reguler_option>("Test", nullptr, [] {
                auto method = mono::get_method("Player", "SetMaxStamina", 2, "assembly_valheim");

                if (!method)
                {
                    LOG(WARNING) << "Failed to find method Player::GetPlayerName";

                    return;
                }

                float stamina = 600;
                bool flashBar = true;
                void* args[2] = { &stamina, &flashBar };
                mono::invoke_method(method, nullptr, args);
            });
        });
    }
}