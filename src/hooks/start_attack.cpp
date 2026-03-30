#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"

namespace big
{
    using namespace features;

    static MonoObject* find_best_target(Vector3 shooter, Vector3 forward)
    {
        float best_fov = _aimbot_fov.get_state();
        MonoObject* best = nullptr;
        static constexpr double PI = 3.1415926535897932384626433832795;

        auto characters = unity::get_all_characters();
        auto local_player = unity::get_local_player();

        for (auto c : characters)
        {
            if (!c || (uintptr_t)c < 0x10000 || c == local_player)
                continue;

            if (unity::is_dead(c))
                continue;

            Vector3 pos = unity::get_position(c);
            Vector3 dir = (pos - shooter).normalize();

            float dot = forward.dot(dir);
            float angle = acosf(dot) * (180.f / PI);

            if (angle < best_fov)
            {
                best_fov = angle;
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

            LOG(INFO) << "Original spawn point: " << spawnPoint->x << ", " << spawnPoint->y << ", " << spawnPoint->z;
            LOG(INFO) << "Original aim dir: " << aimDir->x << ", " << aimDir->y << ", " << aimDir->z;

            auto local = unity::get_local_player();
            if (!local)
                return;

            Vector3 shooter = *spawnPoint;

            Vector3 forward = unity::get_camera_forward();

            auto best = find_best_target(shooter, forward);
            if (!best)
                return;

            Vector3 target_pos = unity::get_position(best);
            Vector3 velocity   = unity::get_velocity(best);

            Vector3 predicted = predict_arrow(
                shooter,
                target_pos,
                velocity,
                100.f,
                9.8f
            );

            Vector3 dir = (predicted - shooter).normalize();

            *aimDir = dir;
        } EXCEPT_CLAUSE
	}
}