#include "../view.hpp"
#include "script.hpp"
#include "server/server_module.hpp"

namespace big
{
	void view::setting_submenu()
	{
		canvas::add_tab<regular_submenu>("Settings", SubmenuSettings, [](regular_submenu* sub) {
			static const std::vector<const char*> layouts{"List", "Window"};
			static const std::vector<const char*> themes{"Emerald", "Violet", "Ocean", "Custom", "Studio", "Snow"};
			sub->add_option<choose_option<const char*, int>>("Menu Layout", "Switch between keyboard list and clickable window.", &layouts, &g_settings.window.layout, true);
			sub->add_option<choose_option<const char*, int>>("Theme", "Shared palette for both menu layouts.", &themes, &g_settings.window.theme, true);

			sub->add_option<number_option<float>>("Transparency", "Opacity of the List and Window UI.", &g_settings.window.transparency, 0.15f, 1.f, 0.05f, 2);
			sub->add_option<number_option<std::uint8_t>>("Accent Red", "Used by the Custom theme.", &g_settings.window.m_tab_selected_color.r, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Accent Green", "Used by the Custom theme.", &g_settings.window.m_tab_selected_color.g, 0, 255);
					sub->add_option<number_option<std::uint8_t>>("Accent Blue", "Used by the Custom theme.", &g_settings.window.m_tab_selected_color.b, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Background R", "Custom theme background red channel.", &g_settings.window.custom_background.r, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Background G", "Custom theme background green channel.", &g_settings.window.custom_background.g, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Background B", "Custom theme background blue channel.", &g_settings.window.custom_background.b, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Panel R", "Custom theme panel red channel.", &g_settings.window.custom_panel.r, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Panel G", "Custom theme panel green channel.", &g_settings.window.custom_panel.g, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Panel B", "Custom theme panel blue channel.", &g_settings.window.custom_panel.b, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Text R", "Custom theme text red channel.", &g_settings.window.custom_text.r, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Text G", "Custom theme text green channel.", &g_settings.window.custom_text.g, 0, 255);
			sub->add_option<number_option<std::uint8_t>>("Custom Text B", "Custom theme text blue channel.", &g_settings.window.custom_text.b, 0, 255);
			sub->add_option<sub_option>("Input", nullptr, SubmenuSettingsInput);
			sub->add_option<number_option<float>>("List X Position", nullptr, &g_settings.window.m_pos.x, 0.f, 2000.f, 25.f, 2);
			sub->add_option<number_option<float>>("List Y Position", nullptr, &g_settings.window.m_pos.y, 0.f, 2000.f, 25.f, 2);
			sub->add_option<number_option<float>>("List Width", nullptr, &g_settings.window.m_width, 0.f, 1000.f, 50.f, 2);
			sub->add_option<bool_option<bool>>("Show JS Script", nullptr, &g_settings.window.js_eval);
			sub->add_option<bool_option<bool>>("Sounds", nullptr, &g_settings.window.m_sounds);
			sub->add_option<bool_option<bool>>("Draw Mouse", nullptr, &g_settings.window.mouse_active);
			sub->add_option<reguler_option>("Unload", nullptr, [] {
				g_running = false;
			});
			sub->add_option<reguler_option>("Exit", nullptr, [] {
				exit(0);
			});
		});


		canvas::add_submenu<regular_submenu>("Input", SubmenuSettingsInput, [](regular_submenu* sub) {
			sub->add_option<number_option<std::int32_t>>("Open Delay", nullptr, &g_settings.window.m_open_delay, 10, 1000, 10, 0);
			sub->add_option<number_option<std::int32_t>>("Back Delay", nullptr, &g_settings.window.m_back_delay, 10, 1000, 10, 0);
			sub->add_option<number_option<std::int32_t>>("Enter Delay", nullptr, &g_settings.window.m_enter_delay, 10, 1000, 10, 0);
			sub->add_option<number_option<std::int32_t>>("Vertical Delay", nullptr, &g_settings.window.m_vectical_delay, 10, 1000, 10, 0);
			sub->add_option<number_option<std::int32_t>>("Horizontal Delay", nullptr, &g_settings.window.m_horizontal_delay, 10, 1000, 10, 0);
		});
	}
}
