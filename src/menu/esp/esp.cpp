#include "esp.h"
#include "pointers.hpp"
#include "astra/host/canvas.hpp"
#include "utility/unity.hpp"
#include "features/features.hpp"
#include "worker/entity_worker.hpp"

#include <limits>

namespace big
{
	namespace
	{
		bool s_show_esp_colors = false;
	}

	inline void draw_box(Vector3 const& top_s, Vector3 const& bottom_s, Color color)
	{
		float h = fabsf(bottom_s.y - top_s.y);
		float w = h * 0.5f;

		float x = top_s.x - w / 2.f;
		float y = std::min(top_s.y, bottom_s.y);

		canvas::draw_box(x, y, w, h, 2.f, color);
	}

	static void draw_health(const Vector3& top_s, const Vector3& bottom_s, float hp, float max_hp)
	{
		if (max_hp <= 0.f)
		{
			return;
		}

		float y_top    = std::min(top_s.y, bottom_s.y);
		float y_bottom = std::max(top_s.y, bottom_s.y);

		float h = y_bottom - y_top;
		if (h <= 1.f)
		{
			return;
		}

		float w = h * 0.5f;
		float x = top_s.x - w / 2.f;
		float y = y_top;

		float ratio  = std::clamp(hp / max_hp, 0.f, 1.f);
		float filled = h * ratio;

		float hx = x - 6.f;

		canvas::draw_line(hx, y, hx, y + h, {0, 0, 0, 255}, 2.f);

		Color col{(uint8_t)((1.f - ratio) * 255), (uint8_t)(ratio * 255), 0, 255};

		canvas::draw_line(hx, y + h - filled, hx, y + h, col, 2.f);
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
		{
			fov_px = 150.f;
		}

		float cx = screen_w * 0.5f;
		float cy = screen_h * 0.5f;

		auto* draw_list = ImGui::GetForegroundDrawList();
		if (!draw_list)
		{
			return;
		}

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
			{
				fov = 20.0f;
			}
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

		const auto display = ImGui::GetIO().DisplaySize;
		if (display.x <= 0.f || display.y <= 0.f)
		{
			return;
		}
		const float width = display.x * 0.5f;

		auto to_color = [](const ImVec4& value) {
			return Color{static_cast<uint8_t>(value.x * 255.f),
			    static_cast<uint8_t>(value.y * 255.f),
			    static_cast<uint8_t>(value.z * 255.f),
			    static_cast<uint8_t>(value.w * 255.f)};
		};
		const Color character_color = to_color(_esp_character_color.get_state());
		ImVec2 target_marker{};
		float target_marker_distance = std::numeric_limits<float>::max();
		bool has_target_marker       = false;
		struct label_entry
		{
			float x;
			float y;
			std::string text;
			bool item;
		};
		std::vector<label_entry> labels;

		for (const auto& data : view)
		{
			if (data.self)
			{
				continue;
			}
			const Vector3 bottom{data.screen.x * display.x, data.screen.y * display.y, data.screen.z};
			const Vector3 top{data.top_screen.x * display.x, data.top_screen.y * display.y, data.top_screen.z};

			const bool is_character = data.type == EEntityType::Character || data.type == EEntityType::Player;
			if (_esp_target_marker.get_state() && is_character && bottom.z > 0.f && bottom.x >= 0.f
			    && bottom.x <= display.x && bottom.y >= 0.f && bottom.y <= display.y)
			{
				const float dx       = bottom.x - display.x * 0.5f;
				const float dy       = bottom.y - display.y * 0.5f;
				const float distance = dx * dx + dy * dy;
				if (distance < target_marker_distance)
				{
					target_marker_distance = distance;
					target_marker          = {bottom.x, bottom.y};
					has_target_marker      = true;
				}
			}
			if (_draw_health.get_state() && data.top_visible && is_character)
			{
				draw_health(top, bottom, data.health, data.max_health);
			}
			if (_draw_box.get_state() && data.top_visible && is_character)
			{
				draw_box(top, bottom, character_color);
			}
			if (_draw_line.get_state() && is_character)
			{
				canvas::draw_line(width, 0, bottom.x, bottom.y, character_color, 1.f);
			}
			if (_draw_name.get_state() || _esp_distance.get_state() || data.type == EEntityType::ItemDrop)
			{
				const float anchor_y = data.top_visible ? top.y : bottom.y;
				labels.push_back({bottom.x, anchor_y - ImGui::GetFontSize() - 2.f, data.name, data.type == EEntityType::ItemDrop});
			}
		}

		std::stable_sort(labels.begin(), labels.end(), [](const label_entry& left, const label_entry& right) {
			return left.y < right.y;
		});
		auto* draw_list   = ImGui::GetForegroundDrawList();
		float font_height = ImGui::GetFontSize();
		std::vector<label_entry> placed;
		for (auto& label : labels)
		{
			ImVec2 text_size = ImGui::CalcTextSize(label.text.c_str());
			label.x = std::clamp(label.x - text_size.x * 0.5f, 4.f, std::max(4.f, display.x - text_size.x - 4.f));
			label.y = std::clamp(label.y, 4.f, std::max(4.f, display.y - font_height - 4.f));
			for (const auto& prior : placed)
			{
				const bool horizontal_overlap =
				    label.x < prior.x + ImGui::CalcTextSize(prior.text.c_str()).x && label.x + text_size.x > prior.x;
				if (horizontal_overlap && std::abs(label.y - prior.y) < font_height)
				{
					label.y = std::clamp(prior.y + font_height, 4.f, std::max(4.f, display.y - font_height - 4.f));
				}
			}
			ImVec2 pos(label.x, label.y);
			constexpr ImU32 outline = IM_COL32(0, 0, 0, 230);
			const ImVec4 color      = label.item ? _esp_item_color.get_state() : _esp_character_color.get_state();
			const ImU32 text_color  = ImGui::ColorConvertFloat4ToU32(color);
			draw_list->AddText(ImVec2(pos.x - 1.f, pos.y), outline, label.text.c_str());
			draw_list->AddText(ImVec2(pos.x + 1.f, pos.y), outline, label.text.c_str());
			draw_list->AddText(ImVec2(pos.x, pos.y - 1.f), outline, label.text.c_str());
			draw_list->AddText(ImVec2(pos.x, pos.y + 1.f), outline, label.text.c_str());
			draw_list->AddText(pos, text_color, label.text.c_str());
			placed.push_back(label);
		}
		if (has_target_marker)
		{
			const ImVec2 center(target_marker.x, target_marker.y);
			const ImU32 marker_color = ImGui::ColorConvertFloat4ToU32(_esp_character_color.get_state());
			constexpr float radius   = 9.f;
			draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 230), 16, 3.f);
			draw_list->AddCircle(center, radius, marker_color, 16, 1.5f);
			draw_list->AddLine({center.x - 14.f, center.y}, {center.x - 5.f, center.y}, marker_color, 1.5f);
			draw_list->AddLine({center.x + 5.f, center.y}, {center.x + 14.f, center.y}, marker_color, 1.5f);
			draw_list->AddLine({center.x, center.y - 14.f}, {center.x, center.y - 5.f}, marker_color, 1.5f);
			draw_list->AddLine({center.x, center.y + 5.f}, {center.x, center.y + 14.f}, marker_color, 1.5f);
		}
	}

	void esp::toggle_color_settings()
	{
		s_show_esp_colors = !s_show_esp_colors;
	}

	void esp::draw_color_settings()
	{
		if (!s_show_esp_colors)
		{
			return;
		}

		if (canvas::begin_panel("ESP Colors", &s_show_esp_colors, {.size = {0.f, 0.f}, .collapsible = false, .fit_content = true}))
		{
			auto character_color = features::_esp_character_color.get_state();
			if (ImGui::ColorEdit4("Creatures and players", &character_color.x, ImGuiColorEditFlags_NoInputs))
			{
				features::_esp_character_color.set_color_state(character_color);
			}
			auto item_color = features::_esp_item_color.get_state();
			if (ImGui::ColorEdit4("Dropped items", &item_color.x, ImGuiColorEditFlags_NoInputs))
			{
				features::_esp_item_color.set_color_state(item_color);
			}
			ImGui::TextDisabled("Colors are saved with the trainer settings.");
		}
		canvas::end_panel();
	}
}
