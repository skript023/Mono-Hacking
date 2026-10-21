# Cheat Status (A1 / A2)

Open **Self > Cheat Status**. The submenu includes status rows, cleanup buttons,
and an inspector panel that is visible only while this submenu is open.
It refreshes once per second, with a selectable 5–100 metre radius.

## A1: inspect

- Reads `PlayerProfile.m_usedCheats` and each inventory item's `m_cheated` directly.
- Checks `World.m_startingGlobalKeys` against the original game's allowed modifier
  list, including its case-sensitive matching and `preset` exception. The hooked
  `SetKeyAndValueIsCheat` and `AnyCheatedItem` are not used for inspection.
- Lists the world keys responsible for the mark.
- Counts marked loaded ZNetViews and `cheated` / `cheatedQueued` flags, including
  individual cooking slots and pending fermentation output. Creature, piece, and
  station counts are shown; a station may also be a piece.
- Counts cheated item **stacks** in the player inventory and closed, locally owned
  chests. Busy/non-owned chests are explicitly uninspected, not assumed clean.
- Shows the mod's bypass state and the original native bypass getter separately.

The panel reads current in-memory data, not save files on disk. Existing automatic
protection can already have cleared the character and inventory flags. Missing APIs,
exceptions, and old snapshots are shown as incomplete/waiting rather than clean.
Objects outside the loaded area are not inspected, and the panel does not promise
achievement eligibility (other mods and other game conditions can still matter).

## A2: clean

- **Clean Character & Inventory** clears the profile flag, cheat-use counters for
  each difficulty, and inventory item flags; requests a character save.
- **Clean World Keys (Host)** removes only keys classified as cheat modifiers from
  both active global keys and saved starting keys, including stale/duplicate saved
  entries. This also resets the corresponding gameplay modifiers. It broadcasts
  the updated keys and requests the normal game world/profile save.
- **Clean Nearby Area (Host)** clears the raw flags in owned loaded ZDOs and cooking
  slots, pending fermentation output, and items in closed owned chests. Inventory
  change notifications and container serialization update the saved ZDO contents.
  Requests the normal game world/profile save after cleanup.

World and area cleanup require the host. No ownership is forcibly taken from other
peers. Skipped non-owned marked objects and busy/non-owned chests are reported.
Changes become durable after a **successful game save**; a save request is not proof
that disk/cloud persistence succeeded. Check the game's save result before exiting.
An interrupted/failed action can have applied some changes; inspect again.

All Mono reads/writes are queued onto Game.Update. Rendering consumes native
snapshots protected by a mutex and does not access managed objects.

## Validation

Run `python tests/mono/cheat_status_contract.py <Valheim directory>` for exact
method signatures, raw field types, and the cheat-use statistic enum. Run the
existing achievement contract as a regression check. These isolated tests do not
attach to the game or access user saves.

In-game acceptance test, on a disposable copied save:

1. Open Self > Cheat Status; confirm the side panel disappears on leaving it.
2. Seed a profile flag, cheated item stack, one invalid world modifier, marked
   creature/piece, queued smelter/fermenter item, and multiple cooking slots.
   Verify the raw status despite active achievement bypasses.
3. Exercise each cleanup separately. Verify normal modifiers, item quantities,
   cooking progress, and unrelated world progress remain intact.
4. With a second peer, confirm world/area cleanup is refused on the client and
   remote-owned/busy chests are reported as uninspected/skipped on the host.
5. Save successfully, exit, remove the mod, and reload the copied world/profile.
   Confirm cleaned flags remain clear. Move outside the scanned area and verify
   the UI never certified those unloaded objects as clean.
6. Leave/rejoin, change radius, and test unavailable APIs: stale/partial data must
   never appear as a fresh clean result.

Build and metadata checks do not replace this final persistence test.
