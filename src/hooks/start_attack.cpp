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

    inline Vector3 predict_arrow(
        const Vector3& shooter,
        const Vector3& target,
        const Vector3& velocity,
        float speed,
        float gravity
    )
    {
        Vector3 to_target = target - shooter;
        float distance = to_target.length();

        float t = distance / speed;

        Vector3 future;

        future.x = target.x + velocity.x * t;
        future.y = target.y + velocity.y * t + 0.5f * gravity * t * t; // Y-up
        future.z = target.z + velocity.z * t;

        return future;
    }

	void hooks::get_projectile_spawn_point(MonoObject* attack, Vector3* spawnPoint, Vector3* aimDir)
	{
        TRY_CLAUSE
        {
            if (!_aimbot_enabled.get_state())
            {
                return detour_base::get_original<get_projectile_spawn_point>()(attack, spawnPoint, aimDir);
            }

            detour_base::get_original<get_projectile_spawn_point>()(attack, spawnPoint, aimDir);

            if (!spawnPoint || !aimDir)
                return;
#ifdef _DEBUG
            LOG(INFO) << "Original spawn point: " << spawnPoint->x << ", " << spawnPoint->y << ", " << spawnPoint->z;
            LOG(INFO) << "Original aim dir: " << aimDir->x << ", " << aimDir->y << ", " << aimDir->z;
#endif
            auto local = unity::get_local_player();
            if (!local)
                return;

            Vector3 shooter = *spawnPoint;
            Vector3 forward = unity::get_forward(local);

            auto best = find_best_target(shooter);

            if (!best)
            {
                LOG(FATAL) << "No target found within FOV";
                return;
            }

            Vector3 target_pos = unity::get_top_point(best);
            Vector3 velocity   = unity::get_velocity(best);

            if (_teleport_projectile.get_state())
            {
                *spawnPoint = target_pos;

                return;
            }
#ifdef _DEBUG
            LOG(INFO) << "Best target is " << unity::get_hover_name(best) << " at position " << target_pos.x << ", " << target_pos.y << ", " << target_pos.z;
#endif
            Vector3 dir = target_pos - shooter;

            float len = dir.length();
            if (len < 0.001f)
                return;

            dir = dir / len;

            *aimDir = dir;
        } EXCEPT_CLAUSE
	}
}