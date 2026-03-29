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
            sub->add_option<bool_option<bool>>("No Alert Animal", nullptr, &g_settings.self.no_animal_alert);
            sub->add_option<bool_option<bool>>("Allow Teleporting with Any Items", nullptr, &g_settings.self.is_teleportable);
            sub->add_option<bool_option<bool>>("Always Have Wind Facing Boat", nullptr, &g_settings.self.always_wind);
            sub->add_option<bool_option<bool>>("Allow Picking Up Fish in Water", nullptr, &g_settings.self.allow_pickup_fish);
            sub->add_option<bool_option<bool>>("No Weight", nullptr, &g_settings.self.no_weight);
            sub->add_option<bool_option<bool>>("Don't Drop Items when Dead", nullptr, &g_settings.self.no_drop_on_dead);
            sub->add_option<bool_option<bool>>("Never Wet", nullptr, &g_settings.self.is_wet);
            sub->add_option<bool_option<bool>>("Forsaken Power Always Ready", nullptr, &g_settings.self.forsaken_power_always_ready);
            sub->add_option<bool_option<bool>>("open_all_recepies"_hash);
            sub->add_option<bool_option<bool>>("infinite_stamina"_hash);
            sub->add_option<bool_option<bool>>("allowed_command"_hash);
            sub->add_option<bool_slider_int_option>("inventory_size"_hash, "inventory_height"_hash, 1);
            sub->add_option<bool_slider_float_option>("enable_raise_skill"_hash, "raise_skill"_hash, 1.f);
            sub->add_option<bool_slider_float_option>("max_food_health"_hash, "food_hp"_hash, 1000.f);
            sub->add_option<bool_slider_float_option>("max_body_armor"_hash, "num_body_armor"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("max_carry_weight"_hash, "carry_amount"_hash, 100.f);
            sub->add_option<bool_slider_float_option>("max_health"_hash, "max_hp"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("max_stamina"_hash, "max_stam"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("eitr"_hash, "eitr_amount"_hash, 5.f);
            sub->add_option<bool_slider_float_option>("stamina_regen"_hash, "stamina_regen_amount"_hash, 5.f);
            sub->add_option<bool_slider_float_option>("eitr_regen"_hash, "eitr_regen_amount"_hash, 5.f);
        });
    }
}