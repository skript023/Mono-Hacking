#include "hooking.hpp"
#include "script_mgr.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"

namespace big
{
    using namespace features;

    static MonoObject* find_best_target(Vector3 shooter)
    {
        float fov_px = unity::fov_degrees_to_pixels(_aimbot_fov.get_state());

        MonoObject* best = nullptr;
        float best_dist = fov_px;

        auto characters = unity::get_all_characters();
        auto local_player = unity::get_local_player();

        float screen_w = (float)unity::get_screen_width();
        float screen_h = (float)unity::get_screen_height();
        if (screen_w <= 0.f || screen_h <= 0.f)
        {
            screen_w = (float)g_pointers->m_resolution.x;
            screen_h = (float)g_pointers->m_resolution.y;
        }

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

            Vector3 center_world = unity::get_center_point(c);
            Vector3 head_world   = unity::get_head_point(c);
            if (head_world.is_zero())
                head_world = unity::get_top_point(c);

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
                continue;

            float dist_center = 999999.f;
            if (has_center)
            {
                float dx = screen_center_pt.x - screen_center.x;
                float dy = screen_center_pt.y - screen_center.y;
                dist_center = sqrtf(dx * dx + dy * dy);
            }

            float dist_head = 999999.f;
            if (has_head)
            {
                float dx = screen_head_pt.x - screen_center.x;
                float dy = screen_head_pt.y - screen_center.y;
                dist_head = sqrtf(dx * dx + dy * dy);
            }

            float dist = std::min(dist_center, dist_head);

            if (!std::isfinite(dist) || dist > fov_px)
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
        TRY_CLAUSE
        {
            if (!_aimbot_enabled.get_state())
            {
                return detour_base::get_original<on_hit>()(Projectile, collider, hitPoint, water, normal);
            }

            auto local = unity::get_local_player();
            if (!local)
                return detour_base::get_original<on_hit>()(Projectile, collider, hitPoint, water, normal);

            Vector3 shooter = unity::get_position(local);
            auto best = find_best_target(shooter);

            if (best)
            {
                Vector3 target_pos = unity::get_head_point(best);
                if (target_pos.is_zero())
                    target_pos = unity::get_center_point(best);
                if (target_pos.is_zero())
                    target_pos = unity::get_top_point(best);

                if (!target_pos.is_zero())
                    return detour_base::get_original<on_hit>()(Projectile, collider, target_pos, water, normal);
            }

            return detour_base::get_original<on_hit>()(Projectile, collider, hitPoint, water, normal);
        } EXCEPT_CLAUSE
	}
}
