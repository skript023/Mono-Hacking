#include "esp.h"
#include "pointers.hpp"
#include "ui/canvas.hpp"
#include "utility/unity.hpp"
#include "worker/entity_worker.hpp"

namespace big
{
    struct screen_box
    {
        float x, y, w, h;
    };

    inline bool build_screen_box(const Vector3& min, const Vector3& max, screen_box& out)
    {
        Vector3 points[8] = {
            {min.x, min.y, min.z},
            {min.x, min.y, max.z},
            {min.x, max.y, min.z},
            {min.x, max.y, max.z},
            {max.x, min.y, min.z},
            {max.x, min.y, max.z},
            {max.x, max.y, min.z},
            {max.x, max.y, max.z},
        };

        float min_x = FLT_MAX, min_y = FLT_MAX;
        float max_x = -FLT_MAX, max_y = -FLT_MAX;

        for (int i = 0; i < 8; i++)
        {
            Vector3 screen;
            if (!unity::world_to_screen(points[i], screen))
                continue;

            min_x = std::min(min_x, screen.x);
            max_x = std::max(max_x, screen.x);
            min_y = std::min(min_y, screen.y);
            max_y = std::max(max_y, screen.y);
        }

        if (min_x >= max_x || min_y >= max_y)
            return false;

        out.x = min_x;
        out.y = min_y;
        out.w = max_x - min_x;
        out.h = max_y - min_y;

        return true;
    }

    inline void draw_box(Vector3 const& top, Vector3 const& bottom)
    {
        Vector3 top_s, bottom_s;

        if (!unity::world_to_screen(top, top_s)) return;
        if (!unity::world_to_screen(bottom, bottom_s)) return;

        float h = fabsf(bottom_s.y - top_s.y);
        float w = h * 0.5f;

        float x = top_s.x - w / 2.f;
        float y = top_s.y;

        canvas::draw_box(x, y, w, h, 2.f, {255, 255, 255, 255});
    }

    static void draw_health(const Vector3& top, const Vector3& bottom, float hp, float max_hp)
    {
        if (max_hp <= 0.f)
            return;

        Vector3 top_s{}, bottom_s{};

        if (!unity::world_to_screen(top, top_s))
            return;

        if (!unity::world_to_screen(bottom, bottom_s))
            return;

        // 🔥 pastikan urutan bener
        float y_top = std::min(top_s.y, bottom_s.y);
        float y_bottom = std::max(top_s.y, bottom_s.y);

        float h = y_bottom - y_top;
        if (h <= 1.f)
            return;

        float w = h * 0.5f;
        float x = top_s.x - w / 2.f;
        float y = y_top;

        // 🔥 health ratio
        float ratio = std::clamp(hp / max_hp, 0.f, 1.f);
        float filled = h * ratio;

        // posisi health bar (kiri box)
        float hx = x - 6.f;

        // background
        canvas::draw_line(hx, y, hx, y + h, {0, 0, 0, 255}, 2.f);

        // warna gradient
        Color col{
            (uint8_t)((1.f - ratio) * 255),
            (uint8_t)(ratio * 255),
            0,
            255
        };

        // health fill
        canvas::draw_line(
            hx,
            y + h - filled,
            hx,
            y + h,
            col,
            2.f
        );
    }

    void esp::draw_esp()
	{
        float width = static_cast<float>(g_pointers->m_resolution->x / 2);
        float height = static_cast<float>(g_pointers->m_resolution->y / 2);

        static const Color white = { 255, 255, 255, 255 };

        const auto view = g_esp_data.view();

        if (!view)
        {
            LOG(FATAL) << "View data is empty";

            return;
        }

        for (const auto& data : *view)
        {
            draw_health(data.top, data.location, data.health, data.max_health);
            draw_box(data.top, data.location);
            canvas::draw_line(width, 0, data.screen.x, data.screen.y, white, 1.f);
            canvas::draw_stroke_text(data.screen.x, data.screen.y, white, data.name);
        }
	}
}