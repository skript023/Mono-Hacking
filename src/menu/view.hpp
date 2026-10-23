#pragma once
#include "imgui.h"
#include "submenu.hpp"
#include "misc/cpp/imgui_stdlib.h"

namespace big
{
	class view
	{
	public:
		static void register_submenu()
		{
			player_submenu();
			base_tools_submenu();
			cheat_status_submenu();
			teleport_submenu();
			//esp_submenu();
			setting_submenu();
		}

	public:
		static void js_scripts();
		static void draw_input();
		static void draw_overlay();
		static void notifications();
		static void online_player_panel();
		static void generated_location_browser();
		static void toggle_generated_location_browser();
		static void base_tools_submenu();
		static void base_tools_panel();
		static void cheat_status_submenu();
		static void cheat_status_panel();

	public:
		//static void esp_submenu();
		static void player_submenu();
		static void teleport_submenu();
		static void setting_submenu();
	};
}
