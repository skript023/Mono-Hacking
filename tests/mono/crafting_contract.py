"""Validate crafting hook ABI against the installed game, in a separate Mono process.

Usage: python tests/mono/crafting_contract.py <Valheim directory>
"""
from pathlib import Path
import runpy

globals().update(runpy.run_path(str(Path(__file__).with_name("hud_contract.py"))))

player_class = get_class(game_image, b"", b"Player")
recipe_class = get_class(game_image, b"", b"Recipe")
gui_class = get_class(game_image, b"", b"InventoryGui")
contracts = [
    (player_class, b"HaveRequirements", ["Recipe", "System.Boolean", "System.Int32", "System.Int32"]),
    (player_class, b"HaveRequirements", ["Piece", "Player.RequirementMode"]),
    (player_class, b"RequiredCraftingStation", ["Recipe", "System.Int32", "System.Boolean"]),
    (player_class, b"ConsumeResources", ["Piece.Requirement[]", "System.Int32", "System.Int32", "System.Int32"]),
    (player_class, b"GetFirstRequiredItem", ["Inventory", "Recipe", "System.Int32", "System.Int32&", "System.Int32&", "System.Int32"]),
    (player_class, b"UpdateKnownRecipesList", []),
    (recipe_class, b"GetRequiredStation", ["System.Int32"]),
    (gui_class, b"get_instance", []),
    (gui_class, b"UpdateCraftingPanel", ["System.Boolean"]),
]
for klass, name, parameters in contracts:
    method = exact(klass, name, parameters)
    assert legacy_lookup(klass, name, len(parameters)) == method, (name, parameters)
for klass, fields in [
    (get_class(game_image, b"", b"ObjectDB"), [b"m_recipes"]),
    (recipe_class, [b"m_resources"]),
    (get_class(game_image, b"", b"Piece/Requirement"), [b"m_resItem", b"m_extraAmountOnlyOneIngredient"]),
    (get_class(game_image, b"", b"ItemDrop"), [b"m_itemData"]),
    (gui_class, [b"m_tabUpgrade"]),
]:
    for name in fields:
        assert get_field(klass, name), name
print("PASS: crafting hook overloads, by-reference ABI, discovery, UI and ingredient fields")
