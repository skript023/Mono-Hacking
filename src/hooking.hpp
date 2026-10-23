#pragma once
#include "common.hpp"
#include "mono/mono.hpp"
#include "class/enums.hpp"

#include "unity/player.hpp"
#include "unity/skills.hpp"
#include "unity/item_drop.hpp"

#include "hooking/vmt_hook.hpp"
#include "hooking/detour_hook.hpp"
#include "hooking/swap_pointer_hook.hpp"

namespace big
{
	struct hooks
	{
		static void* convert_thread_to_fiber(void* param);

		static constexpr auto swapchain_num_funcs           = 19;
		static constexpr auto swapchain_present_index       = 8;
		static constexpr auto swapchain_draw_indexed_index  = 12;
		static constexpr auto swapchain_resizebuffers_index = 13;
		static HRESULT APIENTRY swapchain_present(IDXGISwapChain* this_, UINT sync_interval, UINT flags);
		static HRESULT APIENTRY swapchain_resizebuffers(IDXGISwapChain* this_, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);
		static void APIENTRY swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

		static LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);
		static BOOL clip_cursor(const RECT* rect);
		// Frees the cursor while the mod menu uses the mouse and gives the game its confinement back after.
		static void sync_cursor_clip(bool menu_uses_mouse);
		static bool is_teleportable(void* _this, bool allow_all_items);
		static void create_tomb_stone(MonoObject* player);
		static bool is_under_roof(Vector3 startPos);
		static void on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod);
		static float get_weight(void* _thisint, int stackOverride);
		static float item_get_armor(MonoObject* item, int quality, float world_level);
		static float item_get_max_durability(MonoObject* item, int quality);
		static float item_get_base_block_power(MonoObject* item, int quality);
		static float item_get_deflection_force(MonoObject* item, int quality);
		static void update_water(MonoObject* _this, float dt);
		static bool is_debug_flying(MonoObject* player);
		static bool is_encumbered(MonoObject* player);
		static void update_guardian_power(MonoObject* player, float dt);
		static void game_update(MonoObject* game);
		static void update(MonoObject* player);
		static void update_food(MonoObject* player, float dt, bool force_update);
		static void player_set_max_eitr(MonoObject* player, float max_eitr, bool flash);
		static void set_alerted(MonoObject* player, bool alerted);
		static bool is_wind_controll_active(MonoObject* player);
		static bool is_out_of_water(MonoObject* player);
		static void raise_skill(MonoObject* player, SkillType type, float value);
		static void lower_all_skills(MonoObject* skills, float factor);
		static bool take_input(MonoObject* player_controller, bool look);
		static void player_set_controls(MonoObject* player, Vector3 movedir, bool attack, bool attack_hold, bool secondary_attack, bool secondary_attack_hold, bool block, bool block_hold, bool jump, bool crouch, bool run, bool auto_run, bool dodge);
		static float get_body_armor(MonoObject* player);
		static bool have_empty_slot(MonoObject* inventory);
		static bool allowed_command(MonoObject* ConsoleCommand, MonoObject* Terminal, bool Boolean);
		static void console_run_action(MonoObject* command, MonoObject* args);
		static void camera_render(MonoObject* Camera);
		static void get_projectile_spawn_point(MonoObject* attack, Vector3* spawnPoint, Vector3* aimDir);
		static void on_hit(MonoObject* attack, MonoObject* spawnPoint, Vector3 hitPoint, bool water, Vector3 normal);
		static bool is_known_material(MonoObject* player, MonoString* name);
		static bool zone_get_global_key(MonoObject* zone, int key);
		static int crafting_station_get_level(MonoObject* station, bool check_extensions);
		static MonoObject* drop_item(MonoObject* item, int amount, Vector3 position, Quaternions rotation);
		static void rpc_use_stamina(MonoObject* player, int64_t sender, float v);
		static void on_map_middle_click(MonoObject* minimap, MonoObject* handler);
		static bool top_first(void* _this, void* item);
		static float wearntear_get_support(MonoObject* this_ptr);
		static bool wearntear_have_support(MonoObject* this_ptr);
		static bool private_area_check_access(Vector3 point, float radius, bool flash, bool wardCheck);
		static float get_attack_draw_percentage(MonoObject* humanoid);
		static bool attack_use_ammo(MonoObject* attack, MonoObject** ammo_item);
		static bool is_weapon_loaded(MonoObject* player);
		static void character_damage(MonoObject* character, MonoObject* hit);
		static void character_rpc_damage(MonoObject* character, int64_t sender, MonoObject* hit);
		static bool humanoid_is_blocking(MonoObject* humanoid);
		static bool humanoid_block_attack(MonoObject* humanoid, MonoObject* hit, MonoObject* attacker);
		static bool player_in_god_mode(MonoObject* player);
		static bool player_in_ghost_mode(MonoObject* player);
		static bool player_recipe_requirements(MonoObject* player, MonoObject* recipe, bool discover, int quality, int amount);
		static bool player_recipe_item_requirements(MonoObject* player, MonoObject* recipe, bool discover, int quality, int amount);
		static bool player_piece_requirements(MonoObject* player, MonoObject* piece, int mode);
		static bool player_check_can_remove_piece(MonoObject* player, MonoObject* piece);
		static bool required_crafting_station(MonoObject* player, MonoObject* recipe, int quality, bool check_level);
		static void consume_resources(MonoObject* player, MonoArray* requirements, int quality, int item_quality, int multiplier);
		static MonoObject* recipe_required_station(MonoObject* recipe, int quality);
		static MonoObject* first_required_item(MonoObject* player, MonoObject* inventory, MonoObject* recipe, int quality, int* amount, int* extra, int multiplier);
		static bool inventory_gui_can_repair(MonoObject* gui, MonoObject* item);
		static bool inventory_gui_have_repairable_items(MonoObject* gui);
		static void inventory_gui_repair_one_item(MonoObject* gui);
		static double smelter_delta(MonoObject* object);
		static double fermenter_time(MonoObject* object);
		static float hive_delta(MonoObject* object);
		static float plant_grow_time(MonoObject* object);
		static void plant_update_health(MonoObject* object, double time_since_planted);
		static float cooking_delta(MonoObject* object);
		static float sap_collector_delta(MonoObject* object);
		static MonoString* environment_override(MonoObject* object);
		static void environment_update(MonoObject* object);
		static void container_stack_response(MonoObject* object, int64_t sender, bool granted);
		static bool inventory_add_stack_item(MonoObject* inventory, MonoObject* item);
		static MonoString* item_get_tooltip(MonoObject* item, int quality, bool crafting, float world_level, int stack_override, bool appending);
		static void humanoid_drain_durability(MonoObject* humanoid, MonoObject* item, float dt);
		static float player_get_run_speed_factor(MonoObject* player);
		static float player_get_jog_speed_factor(MonoObject* player);
		static void attack_modify_damage(MonoObject* attack, MonoObject* hit_data, float damage_factor);
		static float attack_get_stamina(MonoObject* attack);
		static float attack_get_eitr(MonoObject* attack);
		static bool player_can_eat(MonoObject* player, MonoObject* item, bool show_messages);
		static bool player_eat_food(MonoObject* player, MonoObject* item);
		static bool bypass_cheat_checks();
		static bool can_get_achievements(bool cheated);
		static bool achievements_is_cheated();
		static bool inventory_any_cheated_item(MonoObject* inventory);
		static bool set_key_value_is_cheat(MonoString* set_key_command);
		static void global_key_add(MonoObject* zone_system, MonoString* key, bool can_save_to_server_option_keys);
		static void character_drop_items(MonoObject* drops, Vector3 center_pos, float drop_area, bool cheated);
		static bool inventory_cheated_damaging_item_equipped(MonoObject* inventory);
		static void cooking_station_spawn_item(MonoObject* station, MonoString* name, int slot, Vector3 user_point, bool cheated);
		static void game_register_kill(MonoObject* game, int64_t target, MonoString* name, int boss_order, int kill_modifiers, int level, bool cheats_used);
		static void drop_invalid_items(MonoObject* humanoid);
		static void hud_update_food(MonoObject* hud, MonoObject* player);
		static bool humanoid_equip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects);
		static void humanoid_unequip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects);
		static void humanoid_update_equipment_status_effects(MonoObject* humanoid);
		static void humanoid_unequip_all_items(MonoObject* humanoid);
		static void player_use_hotbar_item(MonoObject* player, int index);
		static void player_save(MonoObject* player, MonoObject* pkg);
		static bool inventory_contains_item(MonoObject* inventory, MonoObject* item);
		static bool inventory_remove_item(MonoObject* inventory, MonoObject* item, int amount);
	};

	class hooking
	{
		friend hooks;

	public:
		explicit hooking();
		~hooking();

		void enable();
		void disable();

	private:
		// Deletes every hook that came off; a hook still in place stays alive for whoever calls into it.
		static void delete_disabled_hooks();

		bool m_enabled{};


		WNDPROC m_og_wndproc;
	};

	inline hooking* g_hooking{};
}
