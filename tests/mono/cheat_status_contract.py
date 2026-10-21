from pathlib import Path
import runpy

globals().update(runpy.run_path(str(Path(__file__).with_name("hud_contract.py"))))

contracts = {
    "Game": [("get_instance", []), ("GetPlayerProfile", []),
             ("SavePlayerProfile", ["System.Boolean", "System.Boolean"])],
    "ZNet": [("get_instance", []), ("get_World", []), ("IsServer", []),
             ("SaveWorldAndPlayerProfiles", [])],
    "ZoneSystem": [("get_instance", []),
                   ("GlobalKeyRemove", ["System.String", "System.Boolean"]),
                   ("SendGlobalKeys", ["System.Int64"])],
    "ServerOptionsGUI": [("GetPossibleGlobalKeyValues", [])],
    "Humanoid": [("GetInventory", [])],
    "Inventory": [("Changed", ["System.Boolean", "System.Boolean"])],
    "Container": [("IsInUse", []), ("Save", [])],
    "ZNetView": [("IsValid", []), ("IsOwner", []), ("GetZDO", [])],
    "ZDO": [("GetBool", ["System.Int32", "System.Boolean"]),
            ("Set", ["System.Int32", "System.Boolean"])],
    "PlayerProfile/PlayerStats": [("set_Item", ["PlayerStatType", "System.Single"])],
}
for class_name, methods in contracts.items():
    klass = get_class(game_image, b"", class_name.encode())
    assert klass, class_name
    for name, params in methods:
        exact(klass, name.encode(), params)
print("PASS: cheat inspection, cleanup, synchronization and save method signatures")

fields = {
    "PlayerProfile": {"m_usedCheats": "System.Boolean", "m_playerStats": "PlayerProfile.PlayerStats[]", "m_filename": "System.String"},
    "ItemDrop/ItemData": {"m_cheated": "System.Boolean"},
    "Inventory": {"m_inventory": "System.Collections.Generic.List<ItemDrop.ItemData>"},
    "Container": {"m_inventory": "Inventory"},
    "World": {"m_startingGlobalKeys": "System.Collections.Generic.List<System.String>", "m_uid": "System.Int64"},
    "CookingStation": {"m_slots": "UnityEngine.Transform[]"},
    "Fermenter": {"m_delayedTapItemCheated": "System.Boolean"},
    "ZDOVars": {"s_cheated": "System.Int32", "s_cheatedQueued": "System.Int32"},
}
for class_name, names in fields.items():
    klass = get_class(game_image, b"", class_name.encode())
    for name, expected in names.items():
        f = get_field(klass, name.encode())
        assert f, (class_name, name)
        allocated = type_name(api("mono_field_get_type", P, P)(f))
        actual = c.string_at(allocated).decode()
        free(allocated)
        assert actual == expected, (class_name, name, actual, expected)
print("PASS: raw flag fields, cooking slots, delayed fermentation and inventory storage")

stat_class = get_class(game_image, b"", b"PlayerStatType")
stat_field = get_field(stat_class, b"Cheats")
assert stat_field, "PlayerStatType.Cheats missing"
stat_value = c.c_int()
vtable = api("mono_class_vtable", P, P, P)(domain, stat_class)
api("mono_field_static_get_value", None, P, P, P)(vtable, stat_field, c.byref(stat_value))
assert stat_value.value == 4, stat_value.value
print("PASS: character cheat-use statistic enum value")
