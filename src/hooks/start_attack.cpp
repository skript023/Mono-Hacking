#include "hooking.hpp"
#include "utility/unity.hpp"

namespace big
{
    static MonoObject* find_best_target(Vector3 shooter, Vector3 forward)
    {
        float best_fov = 9999.f;
        MonoObject* best = nullptr;
        static constexpr double PI = 3.1415926535897932384626433832795;

        auto characters = unity::get_all_characters();
        auto local_player = unity::get_local_player();

        for (auto c : characters)
        {
            if (c == local_player)
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

	bool hooks::start_attack(MonoObject* humanoid, MonoObject* character, bool secondary)
	{
        auto local_player = unity::get_local_player();

        if (humanoid != local_player)
            return detour_base::get_original<start_attack>()(humanoid, character, secondary);

        auto cam = unity::get_camera_forward();
        auto pos = unity::get_position(humanoid);

        auto target = find_best_target(pos, cam);

        return detour_base::get_original<start_attack>()(humanoid, target, secondary);
	}
}