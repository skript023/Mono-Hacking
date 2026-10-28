#include "features/features.hpp"
#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"


namespace big
{
	using namespace features;
	bool hooks::attack_use_ammo(MonoObject* attack, MonoObject** ammo_item)
	{
		const bool used = detour_base::get_original<attack_use_ammo>()(attack, ammo_item);
		if (!used || !g_running || !g_settings.self.preserve_ammunition || !attack || !ammo_item || !*ammo_item)
		{
			return used;
		}

		static auto character_field   = mono::get_field("Attack", "m_character", "assembly_valheim");
		static auto get_inventory     = mono::get_method("Humanoid", "GetInventory", 0, "assembly_valheim");
		static auto contains_item     = mono::get_method("Inventory", "ContainsItem", 1, "assembly_valheim");
		static auto add_item          = mono::get_method("Inventory", "AddItem", 1, "assembly_valheim");
		static auto inventory_changed = []() {
			auto m = mono::get_method("Inventory", "Changed", 2, "assembly_valheim");
			if (!m)
			{
				m = mono::get_method("Inventory", "Changed", 0, "assembly_valheim");
			}
			return m;
		}();
		static auto stack_field = mono::get_field("ItemDrop/ItemData", "m_stack", "assembly_valheim");
		if (!character_field || !get_inventory || !contains_item || !add_item || !stack_field)
		{
			return used;
		}

		MonoObject* character = nullptr;
		mono::get_field_value(attack, character_field, &character);
		if (!character || character != unity::get_local_player())
		{
			return used;
		}

		auto inventory = mono::invoke_method(get_inventory, character, nullptr);
		if (!inventory)
		{
			return used;
		}

		void* contains_args[]         = {*ammo_item};
		auto contains                 = mono::invoke_method(contains_item, inventory, contains_args);
		const bool still_in_inventory = contains && *reinterpret_cast<bool*>(mono::object_unbox(contains));
		if (still_in_inventory)
		{
			int stack = 0;
			mono::get_field_value(*ammo_item, stack_field, &stack);
			mono::set_field_value(*ammo_item, stack_field, &++stack);
			if (inventory_changed)
			{
				if (mono::get_param_count(inventory_changed) == 2)
				{
					bool success  = true;
					bool cheated  = false;
					void* cargs[] = {&success, &cheated};
					mono::invoke_method(inventory_changed, inventory, cargs);
				}
				else
				{
					mono::invoke_method(inventory_changed, inventory, nullptr);
				}
			}
		}
		else
		{
			int restored_stack = 1;
			mono::set_field_value(*ammo_item, stack_field, &restored_stack);
			void* add_args[] = {*ammo_item};
			mono::invoke_method(add_item, inventory, add_args);
		}
		return used;
	}

	static MonoObject* find_best_target(Vector3 shooter)
	{
		float fov_px = unity::fov_degrees_to_pixels(_aimbot_fov.get_state());

		MonoObject* best = nullptr;
		float best_dist  = fov_px;

		auto characters   = unity::get_all_characters();
		auto local_player = unity::get_local_player();

		float screen_w = (float)unity::get_screen_width();
		float screen_h = (float)unity::get_screen_height();
		if (screen_w <= 0.f || screen_h <= 0.f)
		{
			screen_w = (float)g_pointers->m_resolution.x;
			screen_h = (float)g_pointers->m_resolution.y;
		}

		Vector3 screen_center = {screen_w * 0.5f, screen_h * 0.5f, 0.f};

		for (auto c : characters)
		{
			if (!c || (uintptr_t)c < 0x10000 || c == local_player)
			{
				continue;
			}

			if (unity::is_dead(c))
			{
				continue;
			}

			Vector3 center_world = unity::get_center_point(c);
			Vector3 head_world   = unity::get_head_point(c);
			if (head_world.is_zero())
			{
				head_world = unity::get_top_point(c);
			}

			if (center_world.is_zero())
			{
				center_world = unity::get_position(c);
				center_world.y += 1.2f;
			}

			Vector3 screen_center_pt{};
			Vector3 screen_head_pt{};

			bool has_center = unity::world_to_screen(center_world, screen_center_pt);
			bool has_head   = unity::world_to_screen(head_world, screen_head_pt);

			if (!has_center && !has_head)
			{
				continue;
			}

			float dist_center = 999999.f;
			if (has_center)
			{
				float dx    = screen_center_pt.x - screen_center.x;
				float dy    = screen_center_pt.y - screen_center.y;
				dist_center = sqrtf(dx * dx + dy * dy);
			}

			float dist_head = 999999.f;
			if (has_head)
			{
				float dx  = screen_head_pt.x - screen_center.x;
				float dy  = screen_head_pt.y - screen_center.y;
				dist_head = sqrtf(dx * dx + dy * dy);
			}

			float dist = std::min(dist_center, dist_head);

			if (!std::isfinite(dist) || dist > fov_px)
			{
				continue;
			}

			if (dist < best_dist)
			{
				best_dist = dist;
				best      = c;
			}
		}

		return best;
	}

	inline Vector3 predict_arrow(const Vector3& shooter, const Vector3& target, const Vector3& velocity, float speed, float gravity)
	{
		Vector3 to_target = target - shooter;
		float distance    = to_target.length();

		float t = distance / speed;

		Vector3 future;

		future.x = target.x + velocity.x * t;
		future.y = target.y + velocity.y * t + 0.5f * gravity * t * t; // Y-up
		future.z = target.z + velocity.z * t;

		return future;
	}

	void hooks::get_projectile_spawn_point(MonoObject* attack, Vector3* spawnPoint, Vector3* aimDir)
	{
		if (!_aimbot_enabled.get_state())
		{
			return detour_base::get_original<get_projectile_spawn_point>()(attack, spawnPoint, aimDir);
		}

		detour_base::get_original<get_projectile_spawn_point>()(attack, spawnPoint, aimDir);

		if (!spawnPoint || !aimDir)
		{
			return;
		}
#ifdef _DEBUG
		LOG(INFO) << "Original spawn point: " << spawnPoint->x << ", " << spawnPoint->y << ", " << spawnPoint->z;
		LOG(INFO) << "Original aim dir: " << aimDir->x << ", " << aimDir->y << ", " << aimDir->z;
#endif
		auto local = unity::get_local_player();
		if (!local || mono::get_field_value<"Attack", "m_character", MonoObject*>(attack) != local)
		{
			return;
		}

		Vector3 shooter = *spawnPoint;
		Vector3 forward = unity::get_forward(local);

		auto best = find_best_target(shooter);

		if (!best)
		{
			return;
		}

		Vector3 target_pos = unity::get_head_point(best);
		if (target_pos.is_zero())
		{
			target_pos = unity::get_center_point(best);
		}
		if (target_pos.is_zero())
		{
			target_pos = unity::get_top_point(best);
		}
		Vector3 velocity = unity::get_velocity(best);

		if (_teleport_projectile.get_state())
		{
			*spawnPoint = target_pos;

			return;
		}
#ifdef _DEBUG
		LOG(INFO) << "Best target is " << unity::get_hover_name(best) << " at position " << target_pos.x << ", "
		          << target_pos.y << ", " << target_pos.z;
#endif
		Vector3 dir = target_pos - shooter;

		float len = dir.length();
		if (len < 0.001f)
		{
			return;
		}

		dir = dir / len;

		*aimDir = dir;
	}
}
