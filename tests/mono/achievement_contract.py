"""Check the installed achievement hook ABI without attaching to the game."""
from pathlib import Path
import runpy

globals().update(runpy.run_path(str(Path(__file__).with_name("hud_contract.py"))))
achievements = get_class(game_image, b"", b"Achievements")
for name, parameters in [(b"CanGetAchievements", ["System.Boolean"]), (b"IsCheatedAtAll", [])]:
    method = exact(achievements, name, parameters)
    assert method == legacy_lookup(achievements, name, len(parameters))
    flags = api("mono_method_get_flags", c.c_uint, P, P)(method, None)
    assert flags & 0x10, (name, "must be static; native callback has no this argument")
print("PASS: installed achievement eligibility methods are static and match hook ABI")

game_camera = get_class(game_image, b"", b"GameCamera")
exact(game_camera, b"get_instance", [])
assert get_field(game_camera, b"m_camera")
camera = get_class(core, b"UnityEngine", b"Camera")
exact(camera, b"WorldToViewportPoint", ["UnityEngine.Vector3"])
exact(camera, b"get_rect", [])
print("PASS: gameplay camera and exact viewport projection ABI")
