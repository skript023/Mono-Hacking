#include "../view.hpp"
#include "script.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Home", SubmenuHome, [](regular_submenu* sub)
        {
            sub->add_option<sub_option>("Self", nullptr, "SubmenuSelf"_hash);
            sub->add_option<sub_option>("Inventory", nullptr, "SubmenuInventory"_hash);
            sub->add_option<sub_option>("World", nullptr, "SubmenuWorld"_hash);
            sub->add_option<sub_option>("Online Players", nullptr, SubmenuPlayerList);
            sub->add_option<sub_option>("ESP", nullptr, "SubmenuESP"_hash);
            sub->add_option<sub_option>("Aimbot", nullptr, "SubmenuAimbot"_hash);
        });

        canvas::add_submenu<regular_submenu>("Self", "SubmenuSelf"_hash, [](regular_submenu* sub)
        {
            sub->add_option<sub_option>("Combat & Survival", "God mode, damage multiplier, instant bow, stamina, health.", "SubmenuCombat"_hash);
            sub->add_option<sub_option>("Movement & Flight", "Fly mode, speed multiplier, wind direction, teleportation.", "SubmenuMovement"_hash);
            sub->add_option<sub_option>("Building & Crafting", "Free crafting & building, infinite durability, stability, ward bypass.", "SubmenuBuilding"_hash);
            sub->add_option<sub_option>("Farming & World QoL", "Loot vacuum magnet, 1-hit tree/mine breaker, animal alerts.", "SubmenuFarmingQoL"_hash);
        });

        canvas::add_submenu<regular_submenu>("Combat & Survival", "SubmenuCombat"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("God Mode", "Invulnerable to all incoming damage, fall, hazards, and debuffs.", &g_settings.self.god_mode);
            sub->add_option<bool_option<bool>>("Ghost Mode", "Enemies and bosses cannot detect, target, or hear you.", &g_settings.self.ghost_mode);
            sub->add_option<number_option<float>>("Damage Multiplier", "Multiplier for player attacks (set high for 1-hit kill).", &g_settings.self.damage_multiplier, 1.f, 50.f, 1.f, 1);
            sub->add_option<bool_option<bool>>("Instant Bow & Crossbow", "Instant full bow draw and instant crossbow reload.", &g_settings.self.instant_bow_draw);
            sub->add_option<number_option<int>>("Max Food Slots", "Allow eating more than 3 foods simultaneously (HP, Stamina, Eitr buffs stack!).", &g_settings.self.max_food_slots, 3, 10, 1, 0);
            sub->add_option<bool_option<bool>>("Don't Drop Items when Dead", nullptr, &g_settings.self.no_drop_on_dead);
            sub->add_option<bool_option<bool>>("Never Wet", nullptr, &g_settings.self.is_wet);
            sub->add_option<bool_option<bool>>("Forsaken Power Always Ready", nullptr, &g_settings.self.forsaken_power_always_ready);
            sub->add_option<bool_option<bool>>("infinite_stamina"_hash);
            sub->add_option<bool_option<bool>>("allowed_command"_hash);
            sub->add_option<bool_slider_float_option>("max_health"_hash, "max_hp"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("max_stamina"_hash, "max_stam"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("stamina_regen"_hash, "stamina_regen_amount"_hash, 5.f);
            sub->add_option<bool_slider_float_option>("eitr"_hash, "eitr_amount"_hash, 5.f);
            sub->add_option<bool_slider_float_option>("eitr_regen"_hash, "eitr_regen_amount"_hash, 5.f);
            sub->add_option<bool_slider_float_option>("max_food_health"_hash, "food_hp"_hash, 1000.f);
            sub->add_option<bool_slider_float_option>("max_body_armor"_hash, "num_body_armor"_hash, 10.f);
            sub->add_option<bool_slider_float_option>("enable_raise_skill"_hash, "raise_skill"_hash, 1.f);
        });

        canvas::add_submenu<regular_submenu>("Movement & Flight", "SubmenuMovement"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("Flying", "Toggle creative free flight mode.", &g_settings.self.flying);
            sub->add_option<number_option<float>>("Speed Multiplier", "Sprint & jog speed multiplier (no armor penalty).", &g_settings.self.speed_multiplier, 1.f, 10.f, 0.5f, 1);
            sub->add_option<bool_option<bool>>("Always Have Wind Facing Boat", "Wind always blows from behind your boat.", &g_settings.self.always_wind);
            sub->add_option<bool_option<bool>>("Allow Teleporting with Any Items", "Teleport through portals with ores and metals.", &g_settings.self.is_teleportable);
        });

        canvas::add_submenu<regular_submenu>("Building & Crafting", "SubmenuBuilding"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("Free Crafting & Building", "Craft anywhere with 0 cost, no station/roof/fire, free placement.", &g_settings.self.free_crafting);
            sub->add_option<bool_option<bool>>("Infinite Durability", "Weapons, tools, shields, and armor never degrade.", &g_settings.self.infinite_durability);
            sub->add_option<bool_option<bool>>("Infinite Building Stability", "Pieces never collapse from lack of support.", &g_settings.self.infinite_stability);
            sub->add_option<bool_option<bool>>("Ward / Guard Stone Bypass", "Bypass all player wards and guard stones.", &g_settings.self.ward_bypass);
            sub->add_option<bool_option<bool>>("open_all_recepies"_hash);
        });

        canvas::add_submenu<regular_submenu>("Farming & World QoL", "SubmenuFarmingQoL"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("One-Hit Tree & Mine Breaker", "Instantly fell trees and shatter rocks / ore veins in one hit.", &g_settings.self.one_hit_resource);
            sub->add_option<number_option<float>>("Vacuum Pickup Range", "Item magnet pickup radius in meters (default: 2m).", &g_settings.self.pickup_range, 2.f, 50.f, 2.f, 1);
            sub->add_option<bool_option<bool>>("No Alert Animal", "Animals never become alarmed or flee.", &g_settings.self.no_animal_alert);
            sub->add_option<bool_option<bool>>("Allow Picking Up Fish in Water", nullptr, &g_settings.self.allow_pickup_fish);
            sub->add_option<bool_option<bool>>("No Weight", nullptr, &g_settings.self.no_weight);
            sub->add_option<bool_slider_float_option>("max_carry_weight"_hash, "carry_amount"_hash, 100.f);
            sub->add_option<bool_option<bool>>("Fill Inventory Top First", "Items added to inventory will fill slots from the top down instead of bottom.", &g_settings.self.inventory_top_first);
            sub->add_option<bool_slider_int_option>("inventory_size"_hash, "inventory_height"_hash, 1);
        });
        
        canvas::add_submenu<regular_submenu>("Inventory", "SubmenuInventory"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("Fill Inventory Top First", "Items added to inventory will fill slots from the top down instead of bottom.", &g_settings.self.inventory_top_first);
            sub->add_option<bool_slider_int_option>("override_drop"_hash, "drop_amount"_hash);
            sub->add_option<bool_option<bool>>("override_selected"_hash);
            if (features::_override_selected.get_state())
            {
                sub->add_option<number_option<float>>("durability"_hash);
				sub->add_option<number_option<int>>("quality"_hash);
				sub->add_option<number_option<int>>("stack"_hash);
				sub->add_option<number_option<int>>("variant"_hash);
            }
        });
        
        
        canvas::add_submenu<regular_submenu>("World", "SubmenuWorld"_hash, [](regular_submenu* sub)
        {
            sub->add_option<reguler_option>("tamed_all_deer"_hash);
            sub->add_option<reguler_option>("tamed_all_boar"_hash);
            sub->add_option<reguler_option>("tamed_all_wolf"_hash);
            sub->add_option<reguler_option>("Explore Entire Map", "Reveal all fog of war on the minimap.", [] { unity::explore_all_map(); });
            sub->add_option<reguler_option>("Reset Map Fog", "Reset map exploration fog.", [] { unity::reset_map(); });
            sub->add_option<reguler_option>("Reveal All Bosses & Traders", "Pin all 7 boss altars and traders to the minimap.", [] { unity::discover_bosses_and_traders(); });
        });
        
        canvas::add_submenu<regular_submenu>("Online Players", SubmenuPlayerList, [](regular_submenu* sub)
        {
            auto players = player::get_all_splayers();
            
            for (auto p : players)
            {
                sub->add_option<sub_option>(p.get_player_name().c_str(), nullptr, SubmenuSelectedPlayer, [=]{
                    
                });
            }
        });
        
        canvas::add_submenu<regular_submenu>("Online Players", SubmenuSelectedPlayer, [](regular_submenu* sub)
        {

        });

        canvas::add_submenu<regular_submenu>("ESP", "SubmenuESP"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("esp_activate"_hash);
            sub->add_option<bool_option<bool>>("draw_anim"_hash);
            sub->add_option<bool_option<bool>>("draw_line"_hash);
            sub->add_option<bool_option<bool>>("draw_name"_hash);
            sub->add_option<bool_option<bool>>("draw_health"_hash);
            sub->add_option<bool_option<bool>>("draw_box"_hash);
            sub->add_option<bool_option<bool>>("draw_fov"_hash);
        });

        canvas::add_submenu<regular_submenu>("Aimbot", "SubmenuAimbot"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("aimbot"_hash);
            sub->add_option<bool_option<bool>>("tp"_hash);
            sub->add_option<bool_option<bool>>("draw_fov"_hash);
            sub->add_option<number_option<float>>("aimbot_fov"_hash);
            sub->add_option<number_option<float>>("aimbot_smooth"_hash);
            sub->add_option<number_option<int>>("aimbot_trigger"_hash);
            sub->add_option<bool_option<bool>>("triggerbot"_hash);
            sub->add_option<number_option<float>>("trigger_fov"_hash);
        });
    }
}
