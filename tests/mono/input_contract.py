"""Check the player input hook ABI without attaching to the game."""
from pathlib import Path
import runpy

globals().update(runpy.run_path(str(Path(__file__).with_name("hud_contract.py"))))
method_flags = api("mono_method_get_flags", c.c_uint, P, P)

controller = get_class(game_image, b"", b"PlayerController")
take_input = exact(controller, b"TakeInput", ["System.Boolean"])
assert take_input == legacy_lookup(controller, b"TakeInput", 1)
assert not method_flags(take_input, None) & 0x10, "native callback expects the controller as this"
print("PASS: PlayerController.TakeInput matches hook ABI")

player = get_class(game_image, b"", b"Player")
set_controls = exact(player, b"SetControls", ["UnityEngine.Vector3"] + ["System.Boolean"] * 11)
assert set_controls == legacy_lookup(player, b"SetControls", 12)
assert not method_flags(set_controls, None) & 0x10, "native callback expects the player as this"
print("PASS: Player.SetControls matches hook ABI")
