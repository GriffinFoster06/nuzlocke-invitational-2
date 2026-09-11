# Phase 10 Arc 1 — Roxanne to Brawly

## Summary

- Preserve the completed Arc 0 flow through Roxanne and the combined Rustboro theft scene.
- After defeating the Rusturf Aqua Grunt, resolve the Devon handoff in place: retain the Devon Goods, grant the Great Ball and PokéNav, activate Quick Travel, restore Briney, and bypass the Rustboro escort, President cutscene, and mandatory Match Call tutorial.
- Use the existing PokéNav region map for Quick Travel. It unlocks after Peeko’s rescue, works outdoors, and targets only towns whose existing `FLAG_VISITED_*` flag is set.
- Require the short walk from Petalburg to Briney’s house, but replace all ferry journeys with a confirmation, brief fade, and direct dock-to-dock warp.
- Make Steven optional for progression while preserving his story flag and Steel Wing reward. Badge 2 unlocks Slateport.
- No files have been modified. Implementation begins only after approval.

## Interfaces and Shared State

- Add `bool32 Ruleset_QuickTravelAvailable(void)` to `include/ruleset_field.h` and `src/ruleset_field.c`.
  - Return true only when `SETTING_QUICK_TRAVEL` is enabled and `FLAG_RECOVERED_DEVON_GOODS` is set.
  - Do not add save data, settings, flags, or migration fields.
- Extend `src/field_move.c::FieldMove_PokeRiderEnabled()` to return true for Quick Travel in addition to the existing Poké Rider and HM-free Fly conditions.
  - This automatically serves both `src/pokenav_region_map.c` and `src/field_region_map.c`.
  - Preserve outdoor-map validation and `MAPSECTYPE_CITY_CANFLY`, so unvisited towns remain unavailable.
- Rename the visible `{R_BUTTON}FLY` help text to `{R_BUTTON}TRAVEL` in `src/pokenav_main_menu.c` and `src/field_region_map.c`; do not change the region-map input contract.
- Preserve these invariants through Arc 1:
  - `FLAG_DELIVERED_DEVON_GOODS` remains unset and `ITEM_DEVON_PARTS` remains in the bag for Arc 2.
  - `FLAG_DELIVERED_STEVEN_LETTER` remains unset until Steven is actually met.
  - No physical `ITEM_LETTER` is issued on the new path.
  - Match Call remains available, but its Rustboro and first-ferry tutorials do not interrupt play.

## Area Plans

### Route 116 and Rusturf Tunnel

**Current flow**

- Arc 0’s Rustboro trigger sets `VAR_RUSTBORO_CITY_STATE=3`, `VAR_RUSTURF_TUNNEL_STATE=2`, `VAR_ROUTE116_STATE=1`, and the stolen-goods/object flags.
- Route 116’s coordinate event at `(47,9)` advances its local state after Briney explains Peeko’s kidnapping.
- Rusturf’s approach events move the Grunt and Peeko; defeating `TRAINER_GRUNT_RUSTURF_TUNNEL` gives `ITEM_DEVON_PARTS`, runs the Peeko reunion, sets the recovered flag, and leaves Rustboro in state 4.
- The player must then walk back to Rustboro, receive the Great Ball, be warped to Devon 3F, endure the President sequence, receive the Letter/PokéNav, and trigger the mandatory scientist Match Call tutorial.

**Required final flow**

- Leave Route 116, Rusturf geography, connections, encounters, trainers, items, and the Peeko/Grunt approach intact.
- In `data/maps/RusturfTunnel/scripts.inc::RusturfTunnel_EventScript_Grunt`:
  - Preserve the Aqua battle and loss/retry behavior.
  - After victory, require successful insertion of `ITEM_DEVON_PARTS` before committing progression. A defeated-trainer retry path must allow the item grant to be retried if its pocket was full.
  - Keep a short Grunt departure and Peeko reunion.
  - Add a compact Devon acknowledgement that grants the Great Ball and PokéNav without a warp or escort.
  - Set the terminal story state listed below, then return field control inside Rusturf Tunnel.
- Refactor the recovered-goods branch in `data/maps/RustboroCity/scripts.inc` into a short, non-warping fallback:
  - Its normal role is compatibility for a save already at state 4.
  - If the tunnel’s Great Ball grant failed, the still-visible employee retries that reward and hides only after success.
  - Do not alter Arc 0’s `RustboroCity_EventScript_StolenGoodsScene`.
- Set after successful rescue/handoff:
  - Clear `FLAG_DEVON_GOODS_STOLEN`.
  - Set `FLAG_RECOVERED_DEVON_GOODS` and `FLAG_RETURNED_DEVON_GOODS`.
  - Set `FLAG_SYS_POKENAV_GET`, `FLAG_RECEIVED_POKENAV`, `FLAG_HAS_MATCH_CALL`, and `FLAG_ADDED_MATCH_CALL_TO_POKENAV`.
  - Set `FLAG_HIDE_RUSTBORO_CITY_SCIENTIST` and `FLAG_HIDE_RUSTBORO_CITY_POKEMON_SCHOOL_SCOTT`.
  - Set `VAR_RUSTBORO_CITY_STATE=7` and `VAR_DEVON_CORP_3F_STATE=1`, bypassing states 4–6 and the forced President/scientist scenes.
  - Preserve the optional Rustboro rival until the player sails by clearing `FLAG_HIDE_RUSTBORO_CITY_RIVAL`; do not create a mandatory Route 104 rival interruption.
  - Set `VAR_BRINEY_HOUSE_STATE=1`, `VAR_BRINEY_LOCATION=1`, clear the Briney-house Briney/Peeko hide flags, and hide Route 116 Briney.
  - Apply the existing President visibility changes for Wanda and her boyfriend so later Rock Smash/Strength restoration still works.
- If Great Ball insertion fails, retain the Rustboro employee as the retry NPC; all story and Quick Travel state still advances.

**Warps and transitions**

- Do not change:
  - Route 116 `(47,8)` ↔ Rusturf west entrance `(4,10)`.
  - Route 116 `(65,10)` ↔ Rusturf east entrance `(18,20)`.
  - Rusturf `(29,16)` ↔ Verdanturf.
- The final rescue script performs no warp. Quick Travel becomes usable after exiting the cave to Route 116.

**Dependencies and preserved behavior**

- Arc 0 dependency: Roxanne, the combined theft scene, and its state/visibility setup remain untouched.
- Later dependency: Arc 2 receives the physical Devon Goods; Arc 3 retains Wanda, her boyfriend, Rock Smash, HM Strength, and `FLAG_RUSTURF_TUNNEL_OPENED`.
- Preserve Route 116/Rusturf wild tables, Nuzlocke locations, trainers, overworld items, tunnel access, Peeko, the Grunt battle, and the optional Rustboro rival.

**Emulator verification**

- Grunt loss and retry do not advance rescue state.
- Victory leaves the player in Rusturf, grants the Devon Goods/PokéNav/Great Ball, and never loads Devon Corp.
- Rustboro states 4–6 and the scientist tutorial cannot fire afterward.
- Save/reload and cave re-entry keep Peeko/Grunt removed and later Wanda/Rock Smash content viable.
- Full-bag reward handling leaves a functional Great Ball claim path without blocking progression.

### Briney Ferry and Quick Travel

**Current flow**

- Briney appears at his Route 104 house only after the President sequence.
- Reaching him normally requires returning through Petalburg Woods.
- The first trip stages the player on Route 104, runs long boat movement arrays, and may force Norman’s Match Call.
- Dewford, Route 104, and Route 109 repeat trips use similarly long object-movement sequences.

**Required final flow**

- The intended route is: exit Rusturf → Quick Travel from outdoor Route 116 to visited Petalburg → walk the southern Route 104 segment to Briney’s house. No Petalburg Woods revisit is required.
- Update `data/maps/Route104_MrBrineysHouse/scripts.inc`:
  - Replace letter/package-dependent first-trip dialogue with a short rescue thank-you and Dewford confirmation.
  - Set `FLAG_MR_BRINEY_SAILING_INTRO` on first acceptance.
  - Silently set `FLAG_ENABLE_NORMAN_MATCH_CALL`; do not run a ferry call or registration cutscene.
  - Replace `VAR_BOARD_BRINEY_BOAT_STATE=1` and the Route 104 staging warp with a fade and direct Dewford warp.
- Update `data/maps/DewfordTown/scripts.inc` and `data/maps/Route109/scripts.inc` so every repeat trip uses the same brief fade/direct-warp convention.
- Reuse `data/event_scripts.s` helpers `EventScript_MoveMrBrineyToHouse`, `EventScript_MoveMrBrineyToDewford`, and `EventScript_MoveMrBrineyToRoute109` to keep every Briney, Peeko, and boat hide flag synchronized.
- Set `VAR_BRINEY_LOCATION` directly to the destination and keep `VAR_BOARD_BRINEY_BOAT_STATE=0`; do not use its transient sailing state or `VAR_0x8008` backup.
- Once direct transitions are wired, remove only movement tables and Route 104 sailing handlers proven unreferenced by search.

**Exact destinations**

- Briney house → Dewford: `MAP_DEWFORD_TOWN, 11, 10`; set Briney location 2.
- Dewford → Petalburg side: `MAP_ROUTE104_MR_BRINEYS_HOUSE, 5, 4`; set location 1.
- Dewford → Slateport side: `MAP_ROUTE109, 22, 24`; set location 3.
- Route 109 → Dewford: `MAP_DEWFORD_TOWN, 11, 10`; set location 2.
- First departure also sets `VAR_RUSTBORO_CITY_STATE=8`, `VAR_ROUTE104_STATE=2`, and hides both Rustboro/Route 104 rival objects, matching the existing post-sailing terminal state.

**Dependencies and preserved behavior**

- Arc 0 supplies already-visited Petalburg and Rustboro flags.
- Quick Travel uses existing heal destinations, including Petalburg `(20,17)`; it does not create route or cave destinations.
- Arc 2 begins from the existing Route 109 landing with Briney and his boat present.
- Later Briney relocation/reset logic remains compatible because the same location values and object-hide flags are used.
- Disabling `SETTING_QUICK_TRAVEL` restores the existing Poké Rider/HM-free Fly gate; it intentionally means the player must traverse normally.
- Preserve Route 104, Route 106, and Route 109 encounters, trainers, items, buildings, and walking access.

**Emulator verification**

- The PokéNav’s `R: TRAVEL` hint fits both zoom modes.
- Route 116 permits travel after rescue; Rusturf itself rejects it because caves remain invalid Fly maps.
- Petalburg and Rustboro are selectable; Dewford and Slateport are not selectable before their visited flags.
- Every ferry direction lands on a passable tile with Briney/boat visible only at the destination.
- Repeated trips, save/reload at each endpoint, and approaching Briney from every valid facing direction do not strand or duplicate objects.
- No long sailing animation or Norman call remains.

### Dewford, Granite Cave, and Brawly

**Current flow**

- Dewford is marked visited on arrival.
- Before `FLAG_DELIVERED_STEVEN_LETTER`, Briney only offers a return to Petalburg.
- Steven removes `ITEM_LETTER`, sets the delivered flag, gives Steel Wing, registers himself, and disappears only for the current map session.
- Delivering the letter unlocks Slateport and enables Mr. Stone’s optional Exp. Share reward.
- Brawly independently gives Badge 2 and Bulk Up and advances the Petalburg Gym state.

**Required final flow**

- Change `data/maps/DewfordTown/scripts.inc::DewfordTown_EventScript_Briney`:
  - Before `FLAG_BADGE02_GET`, offer only Petalburg or cancel, with no letter reminder.
  - After `FLAG_BADGE02_GET`, use the existing Petalburg/Slateport/cancel menu.
  - Do not consult `FLAG_DELIVERED_STEVEN_LETTER` for ferry progression.
- Keep Steven optional in `data/maps/GraniteCave_StevensRoom/scripts.inc`:
  - Use one short acknowledgement with no required physical Letter.
  - If a legacy save still holds `ITEM_LETTER`, remove it during this interaction.
  - Set `FLAG_DELIVERED_STEVEN_LETTER`.
  - Give `ITEM_TM_STEEL_WING`; if the bag is full, leave Steven present and retry only the reward on the next interaction.
  - After successful reward delivery, set `FLAG_REGISTERED_STEVEN_POKENAV`, set `FLAG_HIDE_GRANITE_CAVE_STEVEN`, and remove him. This makes disappearance persistent across re-entry.
- Keep `RustboroCity_DevonCorp_3F_EventScript_MrStone` available in terminal state 1:
  - Before Steven, use brief optional guidance.
  - After Steven, retain the existing retry-safe `ITEM_EXP_SHARE` reward and `FLAG_RECEIVED_EXP_SHARE`.
  - No Devon return is needed for progression.
- Leave `data/maps/DewfordTown_Gym/scripts.inc` behavior unchanged:
  - Preserve every gym trainer, darkness progression, Brawly battle, badge fanfare, `FLAG_DEFEATED_DEWFORD_GYM`, `FLAG_BADGE02_GET`, the Petalburg Gym state increment, randomized TM Bulk Up, Match Call registration, and trainer-state updates.
  - Badge 2 therefore continues to raise the active cap from 21 to 24 and unlock HM-free Flash through existing systems.

**Warps and preserved content**

- Preserve Dewford Gym `(8,17)` and gym exits `(5,27)/(6,27)`.
- Preserve Route 106’s Granite entrance `(48,16)` and Granite 1F exit `(37,12)`.
- Preserve all Granite floors and their existing warp graph, including 1F `(5,10)` to Steven’s room and Steven-room `(7,3)` back to 1F.
- Preserve Granite’s wild encounters, items, Hiker Flash reward/flag, optional exploration, Dewford fishing, Route 106 encounters/trainers, and all town buildings.
- Arc 2 dependency: Badge 2 exposes the Route 109 ferry handoff, while the still-held Devon Goods remain for Slateport Shipyard/Museum.
- No Slateport Shipyard or Museum script is changed in Arc 1.

**Emulator verification**

- Briney refuses Slateport before Badge 2 regardless of Steven state.
- Steven can be visited before or after Brawly; Steel Wing, delivered-letter state, persistent disappearance, and full-bag retry all work.
- Skipping Steven entirely does not block Brawly or Slateport.
- Mr. Stone grants Exp. Share only after Steven and never starts the old President escort.
- Brawly loss/retry, badge/TM receipt, cap change to 24, and HM-free Flash unlock remain correct.
- Immediately after Badge 2, Slateport appears in Briney’s menu and lands at Route 109 `(22,24)` without entering an Arc 2 event.

## Implementation and Build Order

1. Implement the Quick Travel predicate, integrate it with `FieldMove_PokeRiderEnabled()`, and update region-map help text.
2. Implement the Rusturf post-Grunt handoff and Rustboro fallback/reward retry; verify all terminal flags and later Rusturf visibility.
3. Run `make -j$(sysctl -n hw.ncpu)`.
4. Replace the four Briney travel directions with direct transitions and remove only confirmed-dead movement handlers.
5. Run `make -j$(sysctl -n hw.ncpu)`.
6. Decouple Dewford ferry access from Steven, make the Steven interaction optional/retry-safe, and leave Brawly’s battle/reward script unchanged.
7. Run `make -j$(sysctl -n hw.ncpu)`.
8. Run static searches for obsolete sailing references, duplicate item grants, and all Arc 1 flag consumers; run `git diff --check`.
9. Final verification:
   - `make clean`
   - `make -j$(sysctl -n hw.ncpu)`
   - `make clean`
   - `make debug`
   - Treat only compiler/tool diagnostics as failures, not the vanilla-ROM SHA1 mismatch.

## Exact Continuous mGBA Acceptance Route

1. Start inside Rustboro Gym immediately after Roxanne’s reward dialogue. Confirm Badge 1, the Roxanne TM, cap 21, and `VAR_RUSTBORO_CITY_STATE=1`.
2. Exit and cross one of the Arc 0 theft coordinates at Rustboro x=23/y=20–24. Confirm the combined chase/help scene and state 3.
3. Walk east through Route 116. Keep Infinite Repel off long enough to confirm the Route 116 encounter table remains active; use existing trainers/items as desired.
4. Trigger Briney at `(47,9)`, then enter Rusturf through Route 116 `(47,8)`.
5. Confirm Rusturf encounters, save before the Grunt, defeat him, and complete the shortened Peeko scene.
6. Before leaving the tunnel, confirm:
   - Devon Goods are in the bag.
   - Great Ball reward was granted.
   - PokéNav and Match Call menus exist.
   - No Letter was added.
   - No warp or escort occurred.
7. Exit through Rusturf `(4,10)` to Route 116 `(47,8)`. Open PokéNav → Map:
   - Confirm `R: TRAVEL`.
   - Confirm Petalburg/Rustboro selectable.
   - Confirm Dewford/Slateport unavailable.
   - Travel to Petalburg’s existing heal destination `(20,17)`.
8. Leave Petalburg west onto southern Route 104 and walk north to Briney’s house at `(17,50)`. Do not enter Petalburg Woods.
9. Speak to Briney, accept Dewford, and verify the short fade lands at Dewford `(11,10)` with Briney/boat correctly placed and `FLAG_VISITED_DEWFORD_TOWN` set.
10. Before Brawly, speak to Briney and verify only Petalburg/cancel is available.
11. Walk north onto Route 106, verify its encounters/trainers, and enter Granite Cave at `(48,16)`.
12. Traverse the unchanged Granite floors, obtain Flash from the Hiker, verify cave encounters/items, and optionally meet Steven through 1F `(5,10)`. Confirm Steel Wing and persistent Steven removal.
13. Return to Dewford, enter the Gym at `(8,17)`, defeat the retained Gym trainers, and defeat Brawly.
14. Confirm Badge 2, the Brawly TM, cap 24, HM-free Flash availability, and the Petalburg Gym state increment.
15. Return to Briney and verify Slateport is now selectable. Accept once to validate the brief transition and Route 109 landing at `(22,24)`, then stop before entering Slateport or triggering Arc 2 content.
