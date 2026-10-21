# Valheim feature implementation handoff

## Goal

Continue implementing and auditing the full feature checklist from the user’s request in this repository. Keep progress visible as a checked/unchecked checklist, preserve the existing worktree changes, and report what is source-audited, build-verified, and runtime-tested separately. The user has reported crashes after testing a compiled build, so avoid claiming runtime safety without an in-game test.

## Current state

- The worktree contains extensive in-progress changes. Do not reset, revert, or clean untracked files.
- Latest Release build succeeded with the command below. This verifies compilation/linking only; no in-game validation has been done.
- `git diff --check` passed previously (line-ending conversion warnings only).
- A prior crash dump did not include the faulting exception address/module/RVA, so it was insufficient to identify the crashing feature. Exception logging was updated to capture RIP/exception/module offsets for a future dump.
- The most recent code fix corrected controlled wind direction to match the UI convention (`0° = north`, `90° = east`); the Release build passed after the fix.
- World preset follow-up: fixed the apply path so it no longer calls `ServerOptionsGUI.SetKeys` with stale UI modifier values after `SetPreset`; added the runtime enum's `Default` and `Custom` options. Release build passed after this change too.
- World action follow-up: preset and individual modifier paths now check managed exceptions and stop reporting success when parsing or applying fails. Time method/field signatures were checked against the included runtime references; Release build passed.
- Player hook follow-up: `Player.GetCollider()` returns `CapsuleCollider` in the included reference, so noclip now resolves `Collider.enabled` from `UnityEngine.PhysicsModule` rather than invoking `Behaviour.enabled` on the wrong managed type. Getter/setter calls capture managed exceptions; Release build passed. This is source-level evidence of a real binding mismatch, not proof it caused the earlier crash.
- Base Tools follow-up: Instant Finish now moves each queued smelter ore through Valheim's `QueueProcessed`, removes it from the input queue, then flushes the processed output. Previously it only called `SpawnProcessed`, which flushes output already completed and leaves queued ore untouched. Release build passed.
- Item Spawner follow-up: confirmed both spawn paths clamp quantities to 10,000. Replaced the fixed 0-20 variant range with runtime `SharedData.m_variants` count in the item browser and both inventory/world spawn paths; Release build passed.
- Creature Tools follow-up: Freeze AI now claims and verifies `ZNetView` ownership before disabling `MonsterAI`, captures managed exceptions, and only records an AI for restore after a successful change. This fixes the local-only freeze path; Release build passed.
- World/Time follow-up: `CanPause`, `FadeTimeScale`, server/time getters, time setters, and save now detect managed exceptions before updating local state or reporting success. Release build passed; game behavior remains unverified.
- World editor/event follow-up: global-key set/remove and raid start/stop now check managed invocation exceptions before reporting success; the editor also shows a failure notification. Signatures/RPC behavior were checked against `references/ZoneSystem.cs` and `references/RandEventSystem.cs`; Release build passed.
- Combat follow-up: Hostile One-Hit Damage now raises all eleven `HitData.DamageTypes` fields, including previously skipped chop and pickaxe damage; names/field order were checked against `references/HitData.cs`. Release build passed.
- Equipment UI follow-up: native and ImGui paths both require `InventoryGui.IsVisible()`; runtime reference confirms `IsVisible` is static, and native grid copies the inventory panel background. Save Loadout now reports success only after its package/custom-data writes finish without managed exceptions. Release build passed; visual alignment still needs in-game review.

## Audited or implemented at source level so far

- [x] Equipment visual re-equip on login: restore through native `Humanoid.EquipItem` and `SetupEquipment`, instead of setting only `m_equipped`. Runtime behavior still needs in-game verification.
- [x] Item spawner runtime catalog: validate ObjectDB metadata and skip non-item prefabs; preserve variants/custom data when adding to inventory; quantity capped at 10,000.
- [x] Inventory editor duplicate: preserve variant, durability, custom data, and item override fields.
- [x] Skills/powers: source audit of exact/set/reset skills, skill gain/loss hooks, guardian powers, cooldown, and status-effect browser/actions.
- [x] Creature tools: source audit of searchable spawning, count/level controls, placement/taming, nearby actions; exclude the Player prefab from creature results.
- [x] Prefab spawner: delay initial spawn position until a player reference exists; source audit of browser/position/count/history/undo.
- [x] Map/teleport: source audit of reveal/reset, pins, bookmarks, generated locations, coordinate/teleport paths.
- [x] Combat: source audit of damage, attack costs, bow/ammo, parry, and item stat override paths.
- [x] Building: source audit of key placement/build controls and GlobalKey IDs against runtime references.
- [x] Crafting: source audit of runtime recipe browser, requirement/material/station controls, unlocks, and recipe-to-spawner flow.
- [x] Weather: runtime environment browser and controlled wind paths inspected; wind-angle conversion corrected and compiled.
- [x] World presets: native preset application no longer reapplies stale modifier values; the preset chooser includes all nine values in `references/WorldPresets.cs`.
- [x] World/Time source signatures: `Game.CanPause`/`FadeTimeScale`, `ZNet.GetTimeSeconds`/`SetNetTime`, `ZNet.IsServer`, `SaveWorldAndPlayerProfiles`, and `EnvMan.m_dayLengthSec` match the included runtime references. Runtime behavior is still unverified.
- [x] Noclip collider binding: `Character.GetCollider()` returns `CapsuleCollider`; noclip now reads/writes the inherited `Collider.enabled` property from `UnityEngine.PhysicsModule`, and contains managed exceptions.
- [x] Instant smelter finish: queued ore is converted via the native processing queue path, then pending outputs are spawned; verified against `references/Smelter.cs` and Release-built.
- [x] Item spawner quantities and variants: both actions clamp to 10,000; variant choices now use each runtime item's `m_variants` count, consistent with `references/VariantDialog.cs`.
- [x] Freeze AI authority: claims/validates network ownership before disabling AI and retains prior enabled states for restoration; Release-built, runtime behavior unverified.
- [x] World/Time exception handling: time-scale, server/time reads, time writes, and save paths check managed exceptions and return failure when invocation fails; Release-built, runtime behavior unverified.
- [x] Global Key and raid event invocation paths: set/remove requests, selected raid start, and raid stop check managed exceptions; signatures/RPC flow match runtime references; Release-built, runtime behavior unverified.
- [x] Hostile One-Hit Damage covers every field in runtime `HitData.DamageTypes`, including chop and pickaxe; Release-built, runtime behavior unverified.
- [x] Equipment UI inventory visibility and persistence feedback: both views gate on inventory visibility; native panel background is copied from Valheim inventory; save action reports its actual result; Release-built, visual result unverified in game.
- [x] ESP: source/UI contains creature and dropped-item collection, localized item labels, stack counts, distance, character/item bounds anchors, screen-clamped labels, overlap spacing, target markers, and saved RGB colors.
- [x] Player and Utilities UI coverage: source contains movement/survival controls, camera/freecam/HUD controls, pickup/carry controls, and Base Tools for production/farming/weather/structures/boat helpers. Individual runtime effects still need verification.
- [x] Complete live Mono reference audit across all 93 hooks and 834 lookups against live Valheim Managed assembly:
  - `src/hooks/rpc_use_stamina.cpp` & `src/hooking.hpp`: corrected `sender` parameter from 32-bit `long` to 64-bit `int64_t`.
  - `src/utility/unity.hpp`: resolved fatal overload ambiguity in `ZDO.GetVec3` (was invoking string overload with int hash); updated `unity::show_message` to support 5-parameter `Player.Message` with 4-param fallback.
  - `src/hooks/eat_food.cpp`: updated `send_character_message` to support 5-parameter `Character.Message`.
  - `src/hooks/on_map_middle_click.cpp`: updated mouse query to `ZInput.get_pointerPosition` with fallback to `get_mousePosition`.
  - `src/hooks/player_update.cpp`: corrected `Animator` assembly to `UnityEngine.AnimationModule`.
  - `src/hooks/start_attack.cpp`: resolved `Humanoid.GetInventory`, unambiguous `ContainsItem` and `AddItem`, and supported 2-parameter `Inventory.Changed(bool, bool)`.
  - `src/unity/base_tools.cpp`: supported 5-parameter `CookingStation.GetSlot` / `SetSlot` (`bool cheated`).
  - `src/unity/zone_system.cpp`: specified `System.String` for `SetGlobalKey` / `RemoveGlobalKey` to avoid enum overload mismatch.
  - `src/unity/item_drop.cpp`: fixed `get_bounds_top` to locate `Renderer` in `UnityEngine.CoreModule` and pass `mono::reflection_type(renderer_class)` to `GetComponent(Type)`.
  - `src/unity/tameable.cpp`: fixed `is_alerted` via `m_monsterAI->BaseAI.IsAlerted()` and updated `Command(Humanoid, bool)`.
  - `src/unity/equipment_slots.cpp`: updated `SEMan.AddStatusEffect` to 5 params (`short variant`) and bound grid to `InventoryElement`.
  - `src/unity/inventory_editor.cpp`, `src/unity/item_spawner.cpp`, `src/unity/world_tools.cpp`: supported 2-parameter `Inventory.Changed` (`bool success, bool cheatedStateChanged`).
  - `src/unity/item_spawner.cpp`: updated native `Inventory.AddItem` to support 8 parameters (`bool cheated, bool pickedUp`).
- [x] Apply Anti Slop UI review (`anti-slop/audit-001-2026-10-14.md`):
  - Native inventory grid: applied rich-text Norse color coding to empty slot tags (`<color=#...>` in TextMeshProUGUI) and added hotkey cues when items are placed (`Alt+1..6`, `Shift+1..3`, `Alt+R/T/Z`).
  - ImGui Equipment & Slots window: broken out into clear, distinct category sections ("Armor & Accessories", "Utility Trinkets", "Food Buffs", "Ammunition", "Keys & Tools", "Quick Access"); added category accent titles, colored slot borders, and visible non-hover labels (`Alt+1`, `Equipped`, `Empty`).
- [x] Compilation & validation checks:
  - Clean Release build passed via CMake (`version.dll` built with 0 errors).
  - `clang-format` applied to all modified files.
  - `git diff --check` passed cleanly (0 whitespace/syntax errors).
- [ ] In-game validation in live Valheim: pending execution in game client to verify runtime stability and visual feel.

The checked items above mean source-level work/audit was done; they are not a claim that the feature has been confirmed working in game.

## Important files touched recently

- Runtime modules: `src/unity/item_spawner.cpp`, `src/unity/inventory_editor.cpp`, `src/unity/equipment_slots.cpp`, `src/unity/creature_spawner.cpp`, `src/unity/prefab_spawner.cpp`, `src/unity/crafting_tools.cpp`, `src/unity/world_time.cpp`, `src/unity/bookmark_manager.cpp`, `src/unity/camera_tools.cpp`.
- Hooks and UI: `src/hooks/`, `src/menu/submenu/`, `src/menu/esp/`, `src/menu_settings.*`.
- Crash diagnostics: `src/logger/exception_handler.cpp`, `src/logger/stack_trace.cpp`.
- Several new files are untracked; they are part of the current work and must be preserved.

## Build and checks (PowerShell)

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target Valheim -j 8
git diff --check
```

When formatting modified C++ files:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\Llvm\x64\bin\clang-format.exe' -i <changed-cpp-files>
```

## Continue this task in Antigravity

1. Open the existing folder `L:\Coding\Mono Hacking` in Antigravity. Work in this same folder so it sees the current uncommitted changes.
2. Open `ANTIGRAVITY_HANDOFF.md` and paste this prompt:

   > Continue the active Valheim feature checklist from `ANTIGRAVITY_HANDOFF.md` in this existing worktree. First inspect the current files and git status; preserve all user changes and do not reset, clean, or overwrite unrelated work. Send me a concise checked/unchecked progress update before substantial work. Continue the unchecked items one at a time, verify runtime references before changing Mono calls, and after each implementation run clang-format, `git diff --check`, and the Release build. Report source audit, build, and in-game validation separately. Do not claim in-game behavior is verified unless someone tested it in Valheim. Update the handoff checklist with concrete evidence as you go.

3. Continue from the unchecked items in the checklist above. The current Release build succeeded, but the full checklist and in-game validation remain incomplete.

## Starting in Antigravity

Open the existing folder `L:\Coding\Mono Hacking` in Antigravity. Ask it: “Read `ANTIGRAVITY_HANDOFF.md` and continue the active Valheim feature checklist from the current worktree. Preserve all existing changes, inspect the actual code before updating status, send me a concise checklist progress update before substantial work, and distinguish source audit/build/in-game validation.”
