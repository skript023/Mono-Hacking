#include "../view.hpp"
#include "server/server_module.hpp"

namespace big
{
	void view::setting_submenu()
	{
		canvas::add_settings_tab(SubmenuSettings, SubmenuSettingsInput, [](regular_submenu* sub) {
			sub->add_option<bool_option<bool>>("Show JS Script", nullptr, &g_settings.window.js_eval);
			sub->add_option<reguler_option>("Unload", nullptr, [] {
				g_running = false;
			});
			sub->add_option<reguler_option>("Exit", nullptr, [] {
				exit(0);
			});
		});
	}
}
