#include "hooking.hpp"
#include "astra/host/canvas.hpp"

#include "commands/float_command.hpp"
#include "commands/bool_command.hpp"

#include <imgui.h>

namespace big
{
	bool_command _disable_input("disable_input", "Disable Input", "Disables player input when enabled.");
	bool hooks::take_input(MonoObject* player_controller, bool look)
	{
		// A cursor over the menu only stops the mouse look; movement stops while a menu text field is typing.
		if (canvas::captures_game_input() && (look || ImGui::GetIO().WantTextInput))
			return false;
		return detour_base::get_original<take_input>()(player_controller, look);
	}

	// Movement still reaches the player while the cursor is over the menu, so clicks there must not attack or block.
	void hooks::player_set_controls(MonoObject* player, Vector3 movedir, bool attack, bool attack_hold, bool secondary_attack, bool secondary_attack_hold, bool block, bool block_hold, bool jump, bool crouch, bool run, bool auto_run, bool dodge)
	{
		if (canvas::captures_game_input())
			attack = attack_hold = secondary_attack = secondary_attack_hold = block = block_hold = false;
		detour_base::get_original<player_set_controls>()(player, movedir, attack, attack_hold, secondary_attack, secondary_attack_hold, block, block_hold, jump, crouch, run, auto_run, dodge);
	}
}
