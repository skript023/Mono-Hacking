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

		static constexpr auto swapchain_num_funcs = 19;
		static constexpr auto swapchain_present_index = 8;
		static constexpr auto swapchain_draw_indexed_index = 12;
		static constexpr auto swapchain_resizebuffers_index = 13;
		static HRESULT APIENTRY swapchain_present(IDXGISwapChain* this_, UINT sync_interval, UINT flags);
		static HRESULT APIENTRY swapchain_resizebuffers(IDXGISwapChain* this_, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);
		static void APIENTRY swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

		static LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);
		static bool is_teleportable(void* _this, bool allow_all_items);
		static void create_tomb_stone(MonoObject* player);
		static bool is_under_roof(Vector3 startPos);
		static void on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod);
		static float get_weight(void* _thisint, int stackOverride);
		static void update_water(MonoObject* _this, float dt);
		static bool is_debug_flying(MonoObject* player);
		static void update_guardian_power(MonoObject* player, float dt);
		static void update(MonoObject* player);
		static void set_alerted(MonoObject* player, bool alerted);
		static bool is_wind_controll_active(MonoObject* player);
		static bool is_out_of_water(MonoObject* player);
		static void raise_skill(MonoObject* player, SkillType type, float value);
		static bool take_input(MonoObject* player_controller, bool look);
		static float get_body_armor(MonoObject* player);
		static bool have_empty_slot(MonoObject* inventory);
		static bool allowed_command(MonoObject* ConsoleCommand, MonoObject* Terminal, bool Boolean);
		static void camera_render(MonoObject* Camera);
		static void get_projectile_spawn_point(MonoObject* attack, Vector3* spawnPoint, Vector3* aimDir);
		static void on_hit(MonoObject* attack, MonoObject* spawnPoint, Vector3 hitPoint, bool water, Vector3 normal);
		static bool is_known_material(MonoObject* player, MonoString* name);
		static MonoObject* drop_item(MonoObject* item, int amount, Vector3 position, Quaternions rotation);
		static void rpc_use_stamina(MonoObject* player, long sender, float v);
		static void on_map_middle_click(MonoObject* minimap, MonoObject* handler);
		static bool top_first(void* _this, void* item);
		static float wearntear_get_support(MonoObject* this_ptr);
		static bool wearntear_have_support(MonoObject* this_ptr);
		static bool private_area_check_access(Vector3 point, float radius, bool flash, bool wardCheck);
		static float get_attack_draw_percentage(MonoObject* humanoid);
		static bool is_weapon_loaded(MonoObject* player);
		static void character_rpc_damage(MonoObject* character, int64_t sender, MonoObject* hit);
		static bool player_in_god_mode(MonoObject* player);
		static bool player_in_ghost_mode(MonoObject* player);
		static bool player_no_cost_cheat(MonoObject* player);
		static double smelter_delta(MonoObject* object);
		static double fermenter_time(MonoObject* object);
		static float hive_delta(MonoObject* object);
		static float plant_grow_time(MonoObject* object);
		static MonoString* environment_override(MonoObject* object);
		static void environment_update(MonoObject* object);
		static void container_stack_response(MonoObject* object, int64_t sender, bool granted);
		static bool inventory_add_stack_item(MonoObject* inventory, MonoObject* item);
		static MonoString* item_get_tooltip(MonoObject* item, int quality, bool crafting, float world_level, int stack_override, bool appending);
		static void humanoid_drain_durability(MonoObject* humanoid, MonoObject* item, float dt);
		static float player_get_run_speed_factor(MonoObject* player);
		static float player_get_jog_speed_factor(MonoObject* player);
		static void attack_modify_damage(MonoObject* attack, MonoObject* hit_data, float damage_factor);
		static bool player_can_eat(MonoObject* player, MonoObject* item, bool show_messages);
		static bool player_eat_food(MonoObject* player, MonoObject* item);
	};

	struct minhook_keepalive
	{
		minhook_keepalive();
		~minhook_keepalive();
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
		bool m_enabled{};


		WNDPROC m_og_wndproc;
	};

	inline hooking* g_hooking{};
}
