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

console_command = get_class(game_image, b"", b"Terminal/ConsoleCommand")
run_action = exact(console_command, b"RunAction", ["Terminal.ConsoleEventArgs"])
assert run_action == legacy_lookup(console_command, b"RunAction", 1)
assert not api("mono_method_get_flags", c.c_uint, P, P)(run_action, None) & 0x10, "native callback expects the command as this"
assert get_field(console_command, b"IsCheat")
print("PASS: console RunAction cheat gate and IsCheat field match hook ABI")

assert get_field(console_command, b"Command")
assert get_field(get_class(game_image, b"", b"Terminal/ConsoleEventArgs"), b"Context")
exact(get_class(game_image, b"", b"Terminal"), b"AddString", ["System.String"])
print("PASS: confirmcheats block reads the command name and writes to its terminal")

server_options = get_class(game_image, b"", b"ServerOptionsGUI")
key_is_cheat = exact(server_options, b"SetKeyAndValueIsCheat", ["System.String"])
assert key_is_cheat == legacy_lookup(server_options, b"SetKeyAndValueIsCheat", 1)
assert api("mono_method_get_flags", c.c_uint, P, P)(key_is_cheat, None) & 0x10, "must be static; native callback has no this argument"
zone_system = get_class(game_image, b"", b"ZoneSystem")
key_add = exact(zone_system, b"GlobalKeyAdd", ["System.String", "System.Boolean"])
assert key_add == legacy_lookup(zone_system, b"GlobalKeyAdd", 2)
assert not api("mono_method_get_flags", c.c_uint, P, P)(key_add, None) & 0x10, "native callback expects the zone system as this"
print("PASS: setkey cheat check and GlobalKeyAdd match hook ABI")

profile = get_class(game_image, b"", b"PlayerProfile")
assert get_field(profile, b"m_usedCheats")
player_stats = get_field(profile, b"m_playerStats")
field_type = api("mono_field_get_type", P, P)(player_stats)
allocated = type_name(field_type)
assert c.string_at(allocated).decode() == "PlayerProfile.PlayerStats[]"
free(allocated)
stats = get_class(game_image, b"", b"PlayerProfile/PlayerStats")
assert exact(stats, b"set_Item", ["PlayerStatType", "System.Single"]) == legacy_lookup(stats, b"set_Item", 2)
print("PASS: profile cheat flag and per-difficulty stats match the clearing code")

character_drop = get_class(game_image, b"", b"CharacterDrop")
drop_items = legacy_lookup(character_drop, b"DropItems", 4)
assert drop_items, "CharacterDrop.DropItems(drops, center, area, cheated) is missing"
assert api("mono_method_get_flags", c.c_uint, P, P)(drop_items, None) & 0x10, "must be static; native callback has no this argument"
drop_signature = signature(drop_items)
params = P()
drop_parameters = []
for _ in range(4):
    allocated = type_name(parameter(drop_signature, c.byref(params)))
    drop_parameters.append(c.string_at(allocated).decode())
    free(allocated)
assert drop_parameters[0].startswith("System.Collections.Generic.List"), drop_parameters
assert drop_parameters[1:] == ["UnityEngine.Vector3", "System.Single", "System.Boolean"], drop_parameters
print("PASS: CharacterDrop.DropItems takes the cheated flag the loot hook clears")

inventory = get_class(game_image, b"", b"Inventory")
cheated_weapon = exact(inventory, b"CheatedDamagingItemEquipped", [])
assert cheated_weapon == legacy_lookup(inventory, b"CheatedDamagingItemEquipped", 0)
assert not api("mono_method_get_flags", c.c_uint, P, P)(cheated_weapon, None) & 0x10, "native callback expects the inventory as this"
cooking_station = get_class(game_image, b"", b"CookingStation")
spawn_item = exact(cooking_station, b"SpawnItem", ["System.String", "System.Int32", "UnityEngine.Vector3", "System.Boolean"])
assert spawn_item == legacy_lookup(cooking_station, b"SpawnItem", 4)
assert not api("mono_method_get_flags", c.c_uint, P, P)(spawn_item, None) & 0x10, "native callback expects the station as this"
print("PASS: equipped-weapon cheat check and cooking station output match hook ABI")

game = get_class(game_image, b"", b"Game")
register_kill = exact(game, b"RegisterKill", ["System.Int64", "System.String", "System.Int32", "KillModifiers", "System.Int32", "System.Boolean"])
assert register_kill == legacy_lookup(game, b"RegisterKill", 6)
assert not api("mono_method_get_flags", c.c_uint, P, P)(register_kill, None) & 0x10, "native callback expects the game as this"
allocated = type_name(api("mono_class_enum_basetype", P, P)(get_class(game_image, b"", b"KillModifiers")))
assert c.string_at(allocated).decode() == "System.Int32", "hook passes KillModifiers as int"
free(allocated)
print("PASS: Game.RegisterKill matches the kill credit hook ABI")

game_camera = get_class(game_image, b"", b"GameCamera")
exact(game_camera, b"get_instance", [])
assert get_field(game_camera, b"m_camera")
camera = get_class(core, b"UnityEngine", b"Camera")
exact(camera, b"WorldToViewportPoint", ["UnityEngine.Vector3"])
exact(camera, b"get_rect", [])
print("PASS: gameplay camera and exact viewport projection ABI")
