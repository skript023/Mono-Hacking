# Base Tools

Open **Home > Base Tools**. Settings are session-only and start with normal speeds (1x), automatic weather, and automatic daylight. The default action radius is 30 metres; only objects currently loaded by the game are scanned.

| Menu | Behaviour |
| --- | --- |
| Quick Stack | Requests the game's StackAll flow on accessible chests in range. The receiving chest must already contain the item type. Food/potions, ammo, and hotbar items are protected by default; equipped items remain protected by the game. F7 is optional and only works while the game has focus. |
| Production Monitor | Shows smelter queue/fuel, fermenter state and estimated remaining time, and honey stock. |
| Repair Radius | Requests repair for accessible buildings in range. Optional damage labels show health on screen, up to 200 visible labels. |
| Farming Assistant | Lists plant condition and estimated growth time. Instant growth rechecks health and requires local network ownership and access. |
| Production Speed | Independent 1–20x smelting, fermentation, and honey production. Processing and fuel use scale together for smelters. Only nearby objects owned by this client are accelerated. |
| Weather & Daylight | Weather choices come from the active EnvMan environment list. Day fraction controls local day/night lighting; world time is not advanced. Restore Automatic Environment clears both overrides. |
| Comfort Inspector | Shows current comfort, shelter status, and furniture inside the game's own 10m comfort radius. Individual furniture values are potential contributions; duplicates in a comfort group do not necessarily stack. |

Plant growth speed reduces the total required growth time. Fermentation speed scales elapsed fermentation time, including existing batches. Raising these values can make an existing plant or batch immediately ready; lowering them can make an unfinished batch appear less complete. Set all speeds to 1x to use normal calculations again. Produced items, completed growth, repairs, and moved inventory remain normal game changes.

Snapshots refresh once per second while a monitor is open. Building labels also keep scanning while enabled. Multiplayer ownership can change, so a speed setting may stop applying when another peer becomes the owner. Native chest access/capacity checks still apply; a request notification does not guarantee that every chest accepted items.

## Validation

- Clang Release build completed.
- `tests/base_tools/check_game_contract.ps1` passed against the installed game assemblies: 11 game types and the Unity object-scan overload. This checks actual method signatures, enum values, and the Inventory.AddItem overload used by the hooks.
- Multiplayer behaviour and in-game visual/keyboard interaction remain unverified.

Runtime checks before considering the UI validated: quick stack into full/locked/in-use chests; verify protected food, ammo, hotbar and equipped items; test a delayed stack response; verify 1x and accelerated production; leave/re-enter object range; transfer network ownership; test healthy and unhealthy plants; restore automatic weather/daylight; inspect comfort with duplicate groups; open/close panels in both menu layouts and at a small resolution.
