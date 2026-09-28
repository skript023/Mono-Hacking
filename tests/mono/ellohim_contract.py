"""Exercise Ellohim against real Mono JIT code in this test process only.

Usage: python tests/mono/ellohim_contract.py <Valheim directory> <bridge DLL>
"""
from pathlib import Path
import runpy
import sys

bridge_path = Path(sys.argv[2]).resolve()
globals().update(runpy.run_path(str(Path(__file__).with_name("hud_contract.py"))))
inventory = get_class(game_image, b"", b"Inventory")
method = exact(inventory, b"IsTeleportable", ["System.Boolean"])
compile_method = api("mono_compile_method", P, P)
entry = compile_method(method)
assert entry
bridge = c.CDLL(str(bridge_path))
bridge.test_mono_hook.argtypes = [P]
bridge.test_mono_hook.restype = c.c_char_p
error = bridge.test_mono_hook(entry)
assert error is None, error
print("PASS: real JIT Inventory.IsTeleportable create/enable/call/disable repeated three times")
