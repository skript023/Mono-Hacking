# Base Tools

Open **Home > Base Tools**. Settings are session-only and start with normal speeds (1x), automatic weather, and automatic daylight. The default action radius is 30 metres; only objects currently loaded by the game are scanned.

| Menu | Behaviour |
| --- | --- |
| Quick Stack | Requests the game's StackAll flow on accessible chests in range. The receiving chest must already contain the item type. Food/potions, ammo, and hotbar items are protected by default; equipped items remain protected by the game. F7 is optional and only works while the game has focus. |
| Production Monitor | Live queue, fuel, cooking, sap, shield generator, fermentation and honey status. Shows active stations directly in the menu list as well as the inspector panel. Supports 1-click refuel and instant completion. |
| Repair Radius | Requests repair for accessible buildings in range. Optional damage labels show health on screen, up to 200 visible labels. |
| Farming Assistant | Lists plant condition and estimated growth time. Bypasses Deep North / harsh biome cold/heat/roof/space restrictions so crops can grow anywhere. Instant growth matures all nearby plants. |
| Production Speed | Independent 1–50x smelting, cooking, fermentation, sap extraction, and honey production. Processing and fuel use scale together for smelters. Only nearby objects owned by this client are accelerated. |
| Weather & Daylight | Weather choices come from active EnvMan environments and all biomes (including Deep North and Ashlands). Day fraction controls local day/night lighting; world time is not advanced. Restore Automatic Environment clears both overrides. |
| Comfort Inspector | Shows current comfort, shelter status, and furniture inside the game's own 10m comfort radius. Individual furniture values are potential contributions; duplicates in a comfort group do not necessarily stack. |

Plant growth speed reduces the total required growth time, while the bypass option allows crops to thrive even in Deep North freezing temperatures or Ashlands heat. Fermentation speed scales elapsed fermentation time, including existing batches. Cooking and sap extraction speeds accelerate food preparation and Dvergr extractor yields up to 50x. All Mono operations run in fibers, ensuring thread safety without locking mutexes. Set all speeds to 1x to use normal calculations again. Produced items, completed growth, repairs, and moved inventory remain normal game changes.

Snapshots refresh once per second while a monitor is open. Building labels also keep scanning while enabled. Multiplayer ownership can change, so a speed setting may stop applying when another peer becomes the owner. Native chest access/capacity checks still apply; a request notification does not guarantee that every chest accepted items.

## Validation

- Clang Release build completed.
- `tests/base_tools/check_game_contract.ps1` passed against the installed game assemblies: 11 game types and the Unity object-scan overload. This checks actual method signatures, enum values, and the Inventory.AddItem overload used by the hooks.
- Multiplayer behaviour and in-game visual/keyboard interaction remain unverified.

Runtime checks before considering the UI validated: quick stack into full/locked/in-use chests; verify protected food, ammo, hotbar and equipped items; test a delayed stack response; verify 1x and accelerated production; leave/re-enter object range; transfer network ownership; test healthy and unhealthy plants; restore automatic weather/daylight; inspect comfort with duplicate groups; open/close panels in both menu layouts and at a small resolution.
