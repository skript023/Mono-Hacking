#pragma once
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"

namespace big::features
{
	inline bool_command _esp_enabled("esp_activate", "Enable ESP", "Enable ESP", false);
	inline bool_command _draw_line("draw_line", "ESP Line", "ESP must be enabled", false);
	inline bool_command _draw_name("draw_name", "ESP Name", "ESP must be enabled", false);
	inline bool_command _draw_skeleton("draw_skeleton", "ESP Skeleton", "ESP must be enabled", false);
	inline bool_command _draw_health("draw_health", "ESP Health Bar", "ESP must be enabled", false);
	inline bool_command _draw_box("draw_box", "ESP Box", "ESP must be enabled", false);
	inline bool_command _draw_corner_box("draw_corner_box", "ESP Corner Box", "ESP must be enabled", false);
	inline bool_command _draw_box_3d("draw_box_3d", "ESP Box 3D", "ESP must be enabled", false);
	inline bool_command _draw_team("draw_team", "ESP Team", "ESP must be enabled", false);
	inline bool_command _draw_fov("draw_fov", "Draw FOV", "Draw max aim angle", false);
	inline bool_command _draw_anim("draw_anim", "Draw Animation", "Draw animation", false);

    inline bool_command _aimbot_enabled("aimbot", "Silent Aimbot", "Silent Aimbot", false);
	inline number_command<int> _aimbot_trigger("aimbot_trigger", "Aimbot Trigger Type", "0=MouseOnly 1=Aiming 2=Hotkey 3=Mouse+Aiming", 0, 3, 0);
	inline number_command<float> _aimbot_fov("aimbot_fov", "Aimbot FOV", "Max aim angle", 1.f, 180.f, 20.f);
	inline number_command<float> _aimbot_smooth("aimbot_smooth", "Aimbot Smooth", "Rotation interpolation", 1.f, 100.f, 10.f );
	inline bool_command _triggerbot("triggerbot", "Triggerbot", "Auto shoot when enemy is in crosshair", false);
	inline number_command<float> _trigger_fov("trigger_fov", "Trigger FOV", "Shoot angle tolerance", 0.1f, 5.f, 1.0f);
}