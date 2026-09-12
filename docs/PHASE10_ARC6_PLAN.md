# Phase 10 Arc 6 — Mt. Pyre through Seafloor Cavern Access

## Summary

Split Arc 6 into two gated implementation units within one approved
`docs/PHASE10_ARC6_PLAN.md`:

- **Arc 6A:** Route 121/Lilycove → Mt. Pyre → Magma Hideout → Aqua Hideout.
  End with `FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE` set and eastern Lilycove open.
- **Arc 6B:** Route 124/Mossdeep → Tate & Liza → Space Center → Dive. End at
  accessible Seafloor Cavern Entrance without starting Arc 7.

Build, review, and mGBA-test 6A before implementing 6B. This split follows the
natural persistent-state boundary, limits regression scope, and makes both
hideout warp changes independently testable.

No new public C APIs, types, flags, or vars are required.

## Area Sub-Plans

### Arc 6A

1. **Route 121 and Lilycove — preserve existing implementation**

   - Make no gameplay changes to `data/maps/Route121/scripts.inc`,
     `data/maps/LilycoveCity/scripts.inc`, or `src/data/wild_encounters.json`.
   - Preserve the Route 121 Aqua movement, Lilycove rival battle, Route
     121/Lilycove encounters, Route 122 access, and the Lilycove `(70,5)` Aqua
     Hideout entrance.
   - Retain the existing three Evolution Clerk menus in
     `data/maps/LilycoveCity_DepartmentStore_3F/scripts.inc`; they already
     cover stones, trade items, regional items, sweets, apples, armor, pots,
     teacups, and scrolls.

2. **Mt. Pyre — shorter scene and direct Magma Hideout handoff**

   - In `data/maps/MtPyre_Summit/scripts.inc`, shorten
     `MtPyre_Summit_EventScript_TeamAquaExits` to one Archie departure beat, a
     fade-removal of Archie/grunts, and one Old Lady reward explanation.
   - Preserve all four Summit grunt battles, every interior/exterior floor and
     warp, and the shared `MAPSEC_MT_PYRE` encounter identity.
   - Add a shared retry-safe Magma Emblem award helper:
     - Check bag and PC before giving `ITEM_MAGMA_EMBLEM`.
     - Set `FLAG_RECEIVED_RED_OR_BLUE_ORB`,
       `FLAG_HIDE_JAGGED_PASS_MAGMA_GUARD`, and `VAR_JAGGED_PASS_STATE = 2`
       only after the item is confirmed.
     - If the bag is full, retain `VAR_MT_PYRE_STATE = 1` and let
       `MtPyre_Summit_EventScript_OldLady` retry without replaying Team Aqua.
   - After successful reconciliation, offer optional direct travel to
     `MAP_JAGGED_PASS (16,19)`, immediately below the already-open hideout warp
     `(16,18) -> MAP_MAGMA_HIDEOUT_1F warp 0`.
   - Preserve the later `VAR_MT_PYRE_STATE = 2/3` orb-return flow and
     `FLAG_RETURNED_RED_OR_BLUE_ORB` unchanged.

3. **Magma Hideout — direct critical path with optional floors retained**

   - In `data/maps/MagmaHideout_1F/map.json`:
     - Rewire warp 1 at `(25,34)` from `MagmaHideout_2F_1R warp 1` to
       `MagmaHideout_3F_1R warp 2`.
     - Remove the three Strength boulders at `(5,22)`, `(7,22)`, and `(6,23)`.
   - In `data/maps/MagmaHideout_3F_1R/map.json`, make warp 2 at `(23,3)` return
     to `MagmaHideout_1F warp 1`.
   - Leave every other warp unchanged. The direct route becomes
     `1F → 3F_1R → 4F`; the 1F upper branches still expose the skipped floors,
     encounters, trainers, and items as optional content.
   - In `data/maps/MagmaHideout_4F/scripts.inc`:
     - Keep one orb activation, Groudon awakening/departure sequence,
       `TRAINER_MAXIE_MAGMA_HIDEOUT`, and loss/retry behavior.
     - Trim repeated dialogue, long waits, and Maxie's look-around
       choreography.
     - On victory set both Groudon object hide flags and add an `ON_TRANSITION`
       reconciliation keyed by `FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT`.
     - Skip the Slateport interview/theft detour by committing its terminal
       state directly:
       - `VAR_SLATEPORT_CITY_STATE = 2`
       - `VAR_SLATEPORT_HARBOR_STATE = 2`
       - `FLAG_MET_TEAM_AQUA_HARBOR`
       - `FLAG_HIDE_LILYCOVE_MOTEL_SCOTT`
       - both `FLAG_HIDE_AQUA_HIDEOUT_1F_GRUNT_*_BLOCKING_ENTRANCE`
       - outside Stern/Gabby/Ty hidden
       - Harbor Archie, grunt, and submarine hidden
       - Harbor Stern visible and patrons visible
     - Offer optional travel to `MAP_LILYCOVE_CITY (70,6)`, directly below the
       Aqua Hideout entrance. Declining leaves the player in 4F with the
       shortened reciprocal exit available.

4. **Aqua Hideout — direct basement route**

   - In `data/maps/AquaHideout_B1F/map.json`, redirect warp 1 at `(18,1)` to
     `AquaHideout_B2F warp 9`.
   - In `data/maps/AquaHideout_B2F/map.json`, redirect warp 9 at `(32,20)` back
     to `AquaHideout_B1F warp 1`.
   - Preserve every other teleporter, trainer, Electrode, and item so the
     Master Ball and original maze remain optional and reachable.
   - In `data/maps/AquaHideout_B2F/scripts.inc`:
     - Preserve Matt as the mandatory admin battle and retain loss/retry.
     - Shorten the submarine departure dialogue while keeping its movement.
     - Set `FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE`,
       `FLAG_HIDE_LILYCOVE_CITY_AQUA_GRUNTS`, and
       `FLAG_HIDE_AQUA_HIDEOUT_B2F_SUBMARINE_SHADOW`.
     - Extend `AquaHideout_B2F_EventScript_PreventMattNoticing` to reconcile
       the submarine hide flag on reload.
     - Offer optional return to `MAP_LILYCOVE_CITY (70,6)`. Lilycove's
       existing load script then removes the eastern Wailmer barrier.

### Arc 6B

5. **Route 124, Mossdeep, and Tate & Liza**

   - Leave Route 124, Mossdeep encounters, Mossdeep Gym layout, and
     `MossdeepCity_Gym_EventScript_TateAndLiza` unchanged.
   - Preserve the double-battle identity, two-Pokémon requirement, Gym
     trainers/puzzle, Calm Mind TM retry, `FLAG_BADGE07_GET`, and the resulting
     level cap of 50.
   - In `data/maps/MossdeepCity/scripts.inc`, compress
     `MossdeepCity_EventScript_TeamMagmaEnterSpaceCenter` to one Maxie gesture
     followed by a fade/removal. It must still set `VAR_MOSSDEEP_CITY_STATE =
     2` and persistently hide the outside Magma group.
   - Preserve the Gym's handoff of `VAR_MOSSDEEP_CITY_STATE = 1` and
     `VAR_MOSSDEEP_SPACE_CENTER_STATE = 1`.

6. **Space Center, Steven, and Dive**

   - In `data/maps/MossdeepCity_SpaceCenter_1F/scripts.inc`:
     - Keep grunts 1, 3, and 4 optional and grunt 2 at the `(13,1)` stair
       mandatory.
     - After grunt 2, advance `VAR_MOSSDEEP_SPACE_CENTER_STATE` from 1 to 2.
     - Apply the same reconciliation from
       `MossdeepCity_SpaceCenter_1F_EventScript_Grunt2Defeated` for an existing
       save whose trainer flag is already set.
   - In `data/maps/MossdeepCity_SpaceCenter_2F/map.json` and
     `data/maps/MossdeepCity_SpaceCenter_2F/scripts.inc`:
     - Remove grunt object events 5, 6, and 7 and their forced three-battle
       `ON_FRAME` sequence.
     - Remove the now-dead positioning, movement, and dialogue scripts, but
       leave their global trainer definitions untouched.
     - Make Steven's first interaction show one concise conflict explanation
       and immediately offer the existing party-selection flow.
     - Preserve cancel/decline behavior, `TRAINER_MAXIE_MOSSDEEP +
       TRAINER_TABITHA_MOSSDEEP`, partner Steven, whiteout handling, and full
       loss/retry.
     - On victory commit `VAR_MOSSDEEP_CITY_STATE = 3`,
       `VAR_MOSSDEEP_SPACE_CENTER_STATE = 3`, and all existing Magma cleanup
       flags before attempting the Dive reward.
     - Award `ITEM_HM_DIVE` retry-safely using bag and PC checks. On failure,
       leave Steven visible on 2F so talking to him retries Dive without
       replaying the boss battle.
     - On confirmed possession set `FLAG_RECEIVED_HM_DIVE`,
       `FLAG_OMIT_DIVE_FROM_STEVEN_LETTER`,
       `FLAG_HIDE_SEAFLOOR_CAVERN_ENTRANCE_AQUA_GRUNT`,
       `VAR_STEVENS_HOUSE_STATE = 2`, both Steven hide flags, and remove the 2F
       Steven object.
   - In `data/maps/MossdeepCity_StevensHouse/scripts.inc`, retain
     Beldum/postgame content but harden the old state-1 Dive event as a legacy
     retry-safe fallback. Fresh Arc 6 progression never sets house state 1.
   - In `src/field_specials.c`, make `ShouldDoRivalRayquazaCall` suppress and
     clear the legacy `FLAG_DEFEATED_MAGMA_SPACE_CENTER` /
     `VAR_RIVAL_RAYQUAZA_CALL_STEP_COUNTER` latch so no forced
     traversal-interrupting rival call occurs.
   - Do not edit `field_move.c`, `caps.c`, Route 128, Underwater Route 128, or
     Seafloor Cavern maps. Existing Badge 7 Dive permission and the warps
     `Underwater_Route128 (38,26) → Underwater_SeafloorCavern` and Dive
     surfacing to `SeafloorCavern_Entrance (10,17)` remain authoritative.

## Dependency and Save-State Contract

```text
Route 121 / Lilycove
  → Mt. Pyre grunts and orb scene
  → Magma Emblem + Jagged Pass state 2
  → Magma Hideout / Groudon / Maxie
  → completed Slateport theft state + Aqua Hideout open
  → Aqua Hideout / Matt / submarine escape
  → Lilycove east barrier removed
  → Route 124 / Mossdeep
  → Tate & Liza / Badge 7 / cap 50
  → Mossdeep state 1 → takeover trigger state 2
  → Space Center stair grunt / center state 2
  → Steven double battle / city and center state 3
  → Dive + Seafloor entrance blocker hidden
```

Canonical checkpoints:

| Checkpoint | Required state |
|---|---|
| Mt. Pyre complete | `VAR_MT_PYRE_STATE=1`, Magma Emblem present, received-orb flag set, `VAR_JAGGED_PASS_STATE=2` |
| Magma Hideout complete | Groudon-awakened flag set; Slateport city/harbor both state 2; Aqua entrance blockers hidden |
| Arc 6A complete | Aqua-escaped flag set; Lilycove Aqua grunts and B2F submarine hidden; eastern route open |
| Gym complete | defeated-gym and Badge 7 flags set; Mossdeep/Space Center state 1; cap 50 |
| Space Center infiltrated | Mossdeep city state 2; center state 2 after stair grunt |
| Arc 6B complete | Mossdeep city/center state 3; Dive present and received flag set; Steven house state 2; Seafloor entrance grunt hidden |
| Arc 7 untouched | `VAR_SEAFLOOR_CAVERN_STATE` remains 0, Kyogre-escaped flag remains unset, Mt. Pyre remains state 1 |

## Implementation Order and Build Gates

1. After approval, record this plan in `docs/PHASE10_ARC6_PLAN.md`; execution
   begins in a fresh session after rechecking status, staged/unstaged diffs,
   and recent commits.
2. Implement 6A in dependency order: Mt. Pyre reward/travel, Magma warps and
   completion state, then Aqua warps and completion state.
3. Run `git diff --check`, validate edited JSON through the normal map build,
   and run `make -j$(sysctl -n hw.ncpu)`.
4. Perform fresh read-only SPEC/plan review and targeted mGBA acceptance for
   6A before beginning 6B.
5. Implement 6B in order: Mossdeep takeover, 1F stair handoff, 2F
   encounter/boss/Dive, Steven-house fallback, forced-call suppression.
6. Repeat `git diff --check` and the incremental build, then run separately:
   - `make clean`
   - `make debug`
7. Perform fresh read-only Arc 6 review and full mGBA acceptance. Stop at
   Seafloor Cavern Entrance; do not begin Room 1 or Arc 7.

## Test Plan

- **6A path:** Route 121 → Lilycove rival/vendor check → Route 122 → all Mt.
  Pyre floors → four Summit grunts → emblem → Jagged Pass → shortened Magma
  path → Maxie → direct Lilycove handoff → Aqua Hideout → Matt → eastern
  Lilycove.
- Verify Magma Emblem bag-full retry, both direct-travel Yes/No branches, boss
  loss/retry, save/reload object persistence, and no Slateport interview/theft
  retrigger.
- Traverse every optional Magma branch and Aqua teleporter branch; confirm all
  trainers/items remain reachable, including Magma floor items and Aqua's
  Master Ball, Nugget, Max Elixir, Nest Ball, and Electrodes.
- Confirm Mt. Pyre and Magma Hideout retain their location-based Nuzlocke
  encounter identity despite shortcuts.
- **6B path:** Route 124 → Mossdeep → Tate & Liza → city takeover → Space
  Center stair grunt → Steven/Maxie/Tabitha battle → Dive → Route 128
  underwater → stolen submarine → surface inside Seafloor Cavern Entrance.
- Verify the Gym rejects a one-Pokémon party, grants Badge 7/TM/cap 50 on
  victory, and the removed 2F trio never triggers.
- Verify Steven decline, party-selection cancel, boss loss, and Dive bag-full
  retry paths; the boss must not replay after victory.
- Walk more than 250 outdoor steps after Space Center and confirm no forced
  rival call.
- Save/reload after each boss and after Dive; confirm no sleeping Groudon,
  departed submarine, defeated team, or duplicate Steven reappears.
- Confirm the Seafloor entrance grunt is absent, the Room 1 warp is reachable,
  and no Arc 7 state changes occur.

## Risks and Defaults

- The rewired hideout warps must be emulator-tested for collision, elevation,
  reciprocal exit behavior, and optional-room reachability.
- Synthesizing Slateport's terminal state risks missing a side effect; the
  exact city/harbor flags above and Harbor Stern/patron visibility are
  mandatory acceptance checks.
- Item awards are transactional: story cleanup may persist before a reward
  succeeds, but the awarding NPC must remain available until physical
  possession is confirmed.
- Existing mid-event saves are supported through object/reward/stair
  reconciliation; no save migration or new state constants are introduced.
- The approved choices are direct Slateport-state synthesis and the direct
  Aqua critical path. Direct travel prompts remain optional, with declining
  never blocking progression.
- The worktree was clean at `64cd215ec9`; unrelated changes and all completed
  Arc 0–5 behavior must remain untouched.
