#include "hotkeys.hpp"
#include "bool_command.hpp"
#include "commands.hpp"
#include "notification/notification_service.hpp"
#include "pointers.hpp"
#include "utility/joaat.hpp"
#include <imgui.h>

namespace big
{
	// Registers before settings.json is read, so the bindings load with everything else.
	static hotkeys& g_hotkeys_registration = hotkeys::instance();

	hotkeys::hotkeys() :
	    state_serializer("hotkeys")
	{
	}

	void hotkeys::load_state_impl(nlohmann::json& state)
	{
		m_bindings.clear();
		for (auto& [name, value] : state.items())
		{
			if (!value.is_number_integer())
				continue;
			const auto vk = value.get<std::int64_t>();
			if (vk > 0 && vk < 255)
				m_bindings[name] = static_cast<std::uint32_t>(vk);
		}
	}

	void hotkeys::save_state_impl(nlohmann::json& state)
	{
		state = nlohmann::json::object();
		for (auto& [name, vk] : m_bindings)
			state[name] = vk;
	}

	void hotkeys::tick_impl()
	{
		// Works with the mod menu open too: Astra refuses menu keys as hotkeys. Typing into a menu text box
		// is not a hotkey, and Alt+R/T/Z/Y belong to the equipment slot hotkeys.
		const bool accept = GetForegroundWindow() == g_pointers->m_hwnd && !ImGui::GetIO().WantTextInput
		    && (GetAsyncKeyState(VK_MENU) & 0x8000) == 0;

		for (auto& [name, vk] : m_bindings)
		{
			const bool down = (GetAsyncKeyState(static_cast<int>(vk)) & 0x8000) != 0;
			bool& was_down = m_was_down[name];
			if (down && !was_down && accept)
			{
				if (auto command = commands::get_bool_command(joaat(name)))
				{
					command->call();
					notification::info(command->get_label(), command->get_state() ? "On" : "Off");
				}
			}
			was_down = down;
		}
	}
}
