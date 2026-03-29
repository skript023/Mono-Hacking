#include "esp.h"
#include "pointers.hpp"
#include "ui/canvas.hpp"
#include "worker/entity_worker.hpp"

namespace big
{
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
            canvas::draw_line(width, 0, data.screen.x, data.screen.y, white, 1.f);
            canvas::draw_stroke_text(data.screen.x, data.screen.y, white, data.name);
        }
	}
}