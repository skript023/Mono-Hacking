#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"

namespace big
{
    using namespace features;

    static MonoObject* find_best_target(Vector3 shooter)
    {
        float fov_px = _aimbot_fov.get_state(); // pixel radius

        MonoObject* best = nullptr;
        float best_dist = fov_px;

        auto characters = unity::get_all_characters();
        auto local_player = unity::get_local_player();

        float screen_w = unity::get_screen_width();
        float screen_h = unity::get_screen_height();

        Vector3 screen_center = {
            screen_w * 0.5f,
            screen_h * 0.5f,
            0.f
        };

        for (auto c : characters)
        {
            if (!c || (uintptr_t)c < 0x10000 || c == local_player)
                continue;

            if (unity::is_dead(c))
                continue;

            Vector3 world = unity::get_position(c);

            Vector3 screen{};

            if (!unity::world_to_screen(world, screen))
                continue;
            
            float dx = screen.x - screen_center.x;
            float dy = screen.y - screen_center.y;
            
            float dist = sqrtf(dx * dx + dy * dy);
            
            // LOG(INFO) << "screen: " << screen.x << ", " << screen.y << ", " << screen.z << " dist: " << dist;

            if (!std::isfinite(dist))
                continue;

            if (dist > fov_px)
                continue;

            if (dist < best_dist)
            {
                best_dist = dist;
                best = c;
            }
        }

        return best;
    }

	void hooks::on_hit(MonoObject* Projectile, MonoObject* collider, Vector3 hitPoint, bool water, Vector3 normal)
	{
		if (!_aimbot_enabled.get_state())
		{
			auto local = unity::get_local_player();
			if (!local)
				return;

			Vector3 shooter = unity::get_position(local);
			Vector3 forward = unity::get_forward(local);

			auto best = find_best_target(shooter);

			if (!best)
			{
				LOG(FATAL) << "No target found within FOV";
				return;
			}

			Vector3 target_pos = unity::get_top_point(best);
			Vector3 velocity = unity::get_velocity(best);

			return detour_base::get_original<on_hit>()(Projectile, collider, target_pos, water, normal);
		}

		return detour_base::get_original<on_hit>()(Projectile, collider, hitPoint, water, normal);
	}
}