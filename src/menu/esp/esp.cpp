#include "esp.h"
#include "pointers.hpp"
#include "astra/host/canvas.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"
#include "worker/entity_worker.hpp"

namespace big
{
	inline void draw_box(Vector3 const& top_s, Vector3 const& bottom_s)
	{
		float h = fabsf(bottom_s.y - top_s.y);
		float w = h * 0.5f;

		float x = top_s.x - w / 2.f;
		float y = top_s.y;

		canvas::draw_box(x, y, w, h, 2.f, {255, 255, 255, 255});
	}

	static void draw_health(const Vector3& top_s, const Vector3& bottom_s, float hp, float max_hp)
	{
		if (max_hp <= 0.f)
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
		    255};

		canvas::draw_line(
		    hx,
		    y + h - filled,
		    hx,
		    y + h,
		    col,
		    2.f);
	}

	inline void draw_fov_circle(float fov_deg)
	{
		float screen_w = 0.f;
		float screen_h = 0.f;

		if (ImGui::GetCurrentContext())
		{
			screen_w = ImGui::GetIO().DisplaySize.x;
			screen_h = ImGui::GetIO().DisplaySize.y;
		}

		if (screen_w <= 0.f || screen_h <= 0.f)
		{
			if (g_pointers)
			{
				screen_w = (float)g_pointers->m_resolution.x;
				screen_h = (float)g_pointers->m_resolution.y;
			}
		}

		if (screen_w <= 0.f || screen_h <= 0.f)
		{
			screen_w = 1920.f;
			screen_h = 1080.f;
		}

		float fov_px = unity::fov_degrees_to_pixels(fov_deg, screen_h);
		if (fov_px <= 0.f || !std::isfinite(fov_px))
			fov_px = 150.f;

		float cx = screen_w * 0.5f;
		float cy = screen_h * 0.5f;

		auto* draw_list = ImGui::GetForegroundDrawList();
		if (!draw_list)
			return;

		// Outer dark outline for high contrast against any background
		draw_list->AddCircle(ImVec2(cx, cy), fov_px, IM_COL32(0, 0, 0, 220), 64, 3.0f);
		// Bright white circle
		draw_list->AddCircle(ImVec2(cx, cy), fov_px, IM_COL32(255, 255, 255, 255), 64, 1.5f);
		// Small center dot for crosshair reference
		draw_list->AddCircleFilled(ImVec2(cx, cy), 2.5f, IM_COL32(0, 0, 0, 220));
		draw_list->AddCircleFilled(ImVec2(cx, cy), 1.5f, IM_COL32(255, 255, 255, 255));
	}

	void esp::draw_esp()
	{
		using namespace features;

		// FOV circle draws if Draw FOV is enabled OR Silent Aimbot is enabled
		if (_draw_fov.get_state() || _aimbot_enabled.get_state())
		{
			float fov = _aimbot_fov.get_state();
			if (fov < 1.0f)
				fov = 20.0f;
			draw_fov_circle(fov);
		}

		if (!_esp_enabled.get_state())
		{
			return;
		}

		const auto view = g_esp_data.view();

		if (view.empty())
		{
			return;
		}

		float width = static_cast<float>(g_pointers->m_resolution.x / 2);
		float height = static_cast<float>(g_pointers->m_resolution.y / 2);

		static const Color white = {255, 255, 255, 255};

		for (const auto& data : view)
		{
			if (data.self)
				continue;

			if (_draw_health.get_state() && data.top_visible && (data.type == EEntityType::Character || data.type == EEntityType::Player))
			{
				draw_health(data.top_screen, data.screen, data.health, data.max_health);
			}
			if (_draw_box.get_state() && data.top_visible && (data.type == EEntityType::Character || data.type == EEntityType::Player))
			{
				draw_box(data.top_screen, data.screen);
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
