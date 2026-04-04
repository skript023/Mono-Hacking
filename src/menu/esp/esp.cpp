#include "esp.h"
#include "pointers.hpp"
#include "ui/canvas.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"
#include "worker/entity_worker.hpp"

namespace big
{
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

        float y_top = std::min(top_s.y, bottom_s.y);
        float y_bottom = std::max(top_s.y, bottom_s.y);

        float h = y_bottom - y_top;
        if (h <= 1.f)
            return;

        float w = h * 0.5f;
        float x = top_s.x - w / 2.f;
        float y = y_top;

        float ratio = std::clamp(hp / max_hp, 0.f, 1.f);
        float filled = h * ratio;

        float hx = x - 6.f;

        canvas::draw_line(hx, y, hx, y + h, {0, 0, 0, 255}, 2.f);

        Color col{
            (uint8_t)((1.f - ratio) * 255),
            (uint8_t)(ratio * 255),
            0,
            255
        };

        canvas::draw_line(
            hx,
            y + h - filled,
            hx,
            y + h,
            col,
            2.f
        );
    }

    inline void draw_fov_circle(float fov_px)
    {
        float screen_w = g_pointers->m_resolution.x;
        float screen_h = g_pointers->m_resolution.y;

        float cx = screen_w * 0.5f;
        float cy = screen_h * 0.5f;

        canvas::draw_circle(cx, cy, fov_px, { 255, 255, 255, 255 }, 64);
    }
    
    void esp::draw_esp()
	{
        using namespace features;

        float width = static_cast<float>(g_pointers->m_resolution.x / 2);
        float height = static_cast<float>(g_pointers->m_resolution.y / 2);

        static const Color white = { 255, 255, 255, 255 };

        if (!_esp_enabled.get_state())
        {
            return;
        }

        const auto view = g_esp_data.view();

        if (view.empty())
        {
            LOG(FATAL) << "View data is empty";

            return;
        }

        if (_draw_fov.get_state())
        {
            draw_fov_circle(_aimbot_fov.get_state());
        }

        for (const auto& data : view)
        {
            if (data.self)
                continue;
                
            if (_draw_health.get_state() && ( data.type == EEntityType::Character || data.type == EEntityType::Player))
            {
                draw_health(data.top, data.location, data.health, data.max_health);
            }
            if (_draw_box.get_state() && ( data.type == EEntityType::Character || data.type == EEntityType::Player))
            {
                draw_box(data.top, data.location);
            }
            if (_draw_line.get_state())
            {
                canvas::draw_line(width, 0, data.screen.x, data.screen.y, white, 1.f);
            }
            if (_draw_name.get_state())
            {
                canvas::draw_stroke_text(data.screen.x, data.screen.y, white, data.name);
            }
        }
	}
}