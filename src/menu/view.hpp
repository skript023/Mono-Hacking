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
			home();
			player_submenu();
			base_tools_submenu();
			//stats_submenu();
			teleport_submenu();
			//esp_submenu();
			setting_submenu();
		}

	public:
		static void home();
		static void js_scripts();
		static void draw_input();
		static void draw_overlay();
		static void notifications();
		static void online_player_panel();
		static void base_tools_submenu();
		static void base_tools_panel();

	public:
		//static void esp_submenu();
		static void player_submenu();
		static void stats_submenu();
		static void teleport_submenu();
		static void setting_submenu();
	};
}
