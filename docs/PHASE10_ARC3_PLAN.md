# Phase 10 Arc 3 Plan — Wattson through Flannery

## Status and scope

This is the approved, implementation-ready plan for Phase 10 Arc 3. It is a
planning document only; writing this file does not implement Arc 3.

The inspected planning baseline is commit `501bb2c305` (`Phase 10: Arc 2 —
Brawly through Wattson`). The execution session must revalidate the baseline
and working tree rather than assuming they are unchanged.

Arc 3 begins from the completed Arc 2 handoff after Wattson and ends after
Flannery, immediate Go-Goggles delivery, desert access, and the existing
Quick Travel path back toward Petalburg have been verified. It covers only:

- Route 117 and Verdanturf
- Routes 111 and 112, Fiery Path, and the Route 113 traversal dependency
- Fallarbor Town and Route 114
- Meteor Falls
- Mt. Chimney
- Jagged Pass and Lavaridge Town

Phases 1–9.5 and Phase 10 Arcs 0–2 are complete and must not regress. Arc 4
must not begin: do not implement Norman, the Petalburg Gym challenge, Surf
progression, or any later-story work in this arc.

The Arc 3 execution session must begin by reading `AGENTS.md`, `docs/SPEC.md`,
`docs/PHASES.md`, `docs/PHASE10_PROMPTS.md`, and this plan. It must inspect
`git status`, staged and unstaged diffs, and the relevant recent commits before
editing. Unrelated user changes must be preserved.

## Approved player flow

The intended continuous progression is:

1. Defeat Wattson and leave Mauville with Badge 3, the level-29 cap, HM-free
   Rock Smash access, both bikes, and `VAR_PETALBURG_GYM_STATE == 5`.
2. Route 117 and Verdanturf remain an optional western detour. Their encounter,
   trainers, Day Care, and optional Rusturf Tunnel restoration remain intact.
   There is no Shelmet/Karrablast progression errand.
3. Travel north through Route 111 and Route 112. The cable car remains blocked
   by Team Magma, but its repeated dialogue is reduced to a concise clue.
4. Make the complete first traversal through Fiery Path, upper Route 112,
   Route 113, Fallarbor, and Route 114. This preserves every new encounter
   route and the meaningful trainers and rewards on that route.
5. Trigger the Aqua/Magma confrontation in Meteor Falls. After its state and
   persistent actor flags have been committed, offer an immediate transition
   to the Route 112 lower cable-car station. Declining leaves the player in
   Meteor Falls. Cozmo provides a retry while Mt. Chimney is unfinished.
6. Ride the cable car normally. Defeat the Mt. Chimney Magma Grunts, Tabitha,
   and Maxie. Preserve the Maxie battle and the villain conflict while
   shortening speeches.
7. Reconcile and award the Meteorite safely, then offer an immediate transition
   to the top of Jagged Pass. Declining leaves the player on Mt. Chimney.
8. Traverse Jagged Pass normally so its encounter, trainers, item, and terrain
   remain meaningful, then enter Lavaridge.
9. Defeat Flannery. Complete Badge 4 and cap progression, give the physical
   Go-Goggles immediately and safely, and retain the existing randomized TM
   reward with retry behavior.
10. Leave the Gym with desert access enabled by the physical Go-Goggles. Use
    the already-implemented Quick Travel system, when enabled, to return toward
    Petalburg. Verify Petalburg is selectable, but stop before entering Arc 4.

The two direct story transitions are explicit event choices and do not depend
on the configurable Quick Travel setting. Selecting **No** always preserves the
original walkable route.

## A. Dependency graph

```text
Arc 2 terminal state
  Badge 3 / cap 29 / HM-free Rock Smash / both bikes
  VAR_PETALBURG_GYM_STATE = 5
  existing Quick Travel system available when enabled
        |
        +--> optional Route 117 / Verdanturf / Rusturf flavor
        |
        v
Route 111 south --Rock Smash--> Route 112 south
        |                         cable car blocked by Magma
        v
Fiery Path --> Route 112 north --> Route 113 --> Fallarbor --> Route 114
                                                                  |
                                                                  v
Meteor Falls confrontation
  VAR_METEOR_FALLS_STATE = 1
  FLAG_MET_ARCHIE_METEOR_FALLS
  persistent Aqua/Magma actor cleanup
  FLAG_HIDE_ROUTE_112_TEAM_MAGMA
        |
        +--> No: remain in Meteor Falls / Cozmo retry
        |
        +--> Yes: Route 112 lower cable station, state normalized to 0
                                                                  |
                                                                  v
normal cable ride: VAR_CABLE_CAR_STATION_STATE 0 -> 1 -> 0
                                                                  |
                                                                  v
Mt. Chimney Grunts --> Tabitha --> Maxie
  villain terminal flags / Cozmo visibility / Meteorite attempt
        |
        +--> No: remain on Mt. Chimney
        |
        +--> Yes: Jagged Pass top, ash weather initialized
                                                                  |
                                                                  v
Jagged Pass encounter + trainers + descent
  VAR_JAGGED_PASS_STATE is not advanced by the shortcut
                                                                  |
                                                                  v
Lavaridge Gym trainers --> Flannery
  Badge 4 / cap 36 / Petalburg state 5 -> 6
  physical Go-Goggles confirmed immediately
  randomized Overheat TM retained
                                                                  |
                       +------------------------------------------+
                       v
desert access                  existing Quick Travel toward Petalburg
                       \                 /
                        STOP BEFORE ARC 4
```

The critical ordering constraints are:

- The Route 112 Magma blocker may disappear only when the Meteor Falls
  confrontation completes.
- The Meteor Falls transition may occur only after all story and persistent
  hide state is committed.
- The lower cable station must receive `VAR_CABLE_CAR_STATION_STATE == 0`, so
  its normal ride initializes and completes correctly.
- The Mt. Chimney completion flags must be committed before its Jagged Pass
  transition is offered.
- The Jagged Pass transition must not modify `VAR_JAGGED_PASS_STATE`; later
  Jagged Pass content owns that variable.
- Flannery's badge/Petalburg progression must occur once. The Go-Goggles
  receipt state may advance only after the physical item is present.
- Desert access depends on the physical Go-Goggles, not merely a story flag.

## B. Area implementation sub-plans

### 1. Route 117 and Verdanturf

#### Actual flow

Route 117 is already an optional westward branch from Mauville to Verdanturf.
It provides its encounter table, the Day Care, nine ordinary trainers, berries,
a Great Ball, a Revive, and Cut-dependent exploration. Verdanturf sets its
visited flag and provides access to Rusturf Tunnel and its town buildings.

There is no map NPC, trade, or mandatory progression script for an obsolete
Shelmet/Karrablast evolution exchange. Their relevant evolution handling was
already addressed by earlier phases, so Arc 3 must not introduce a replacement
errand.

Rusturf Tunnel restoration remains optional flavor. Breaking the paired
boulders advances `VAR_RUSTURF_TUNNEL_STATE`, gives Strength, sets
`FLAG_RECEIVED_HM_STRENGTH`, and eventually sets
`FLAG_RUSTURF_TUNNEL_OPENED`; it is not an Arc 3 progression prerequisite.

#### Target flow

Make no changes. Preserve the branch as worthwhile optional content while the
required story route continues north from Mauville. Do not gate Route 111,
Meteor Falls, Mt. Chimney, Flannery, or Go-Goggles behind Verdanturf or Rusturf
restoration.

#### Exact files and scripts to inspect, not edit

- `data/maps/Route117/scripts.inc`
  - `Route117_OnTransition`
  - all trainer, item, berry, Day Care, and Cut-side scripts
- `data/maps/VerdanturfTown/scripts.inc`
  - `VerdanturfTown_OnTransition`
- `data/maps/VerdanturfTown_WandasHouse/scripts.inc`
- `data/maps/RusturfTunnel/scripts.inc`
  - boulder-restoration events and the Strength reward sequence
- Corresponding `map.json` files and connections for Route 117, Verdanturf,
  Rusturf Tunnel, the Day Care, and Verdanturf buildings

#### Flags, variables, objects, warps, battles, and rewards

- Preserve `FLAG_VISITED_VERDANTURF_TOWN`.
- Preserve `VAR_RUSTURF_TUNNEL_STATE`, `FLAG_RECEIVED_HM_STRENGTH`, and
  `FLAG_RUSTURF_TUNNEL_OPENED` as optional side-content state.
- Preserve Route 117's Mauville/Verdanturf connections and the Day Care warp at
  `(51, 5)`.
- Preserve Verdanturf's Rusturf Tunnel warp at `(8, 1)` and all town-building
  warps.
- Preserve the Route 117 trainer battles for Isaac, Lydia, Dylan, Maria, Derek,
  Anna & Meg, Melina, Brandi, and Aisha.
- Preserve the Great Ball, Revive, berries, and optional Strength reward.

#### Dependencies and emulator verification

This branch depends only on Arc 2 traversal access and is not depended upon by
the Arc 3 main path. In mGBA, detour here immediately after Wattson: obtain a
Route 117 encounter, verify representative trainers and Day Care access, enter
Verdanturf, confirm there is no Shelmet/Karrablast errand, and confirm Rusturf
restoration is supplemental. If Quick Travel is enabled, verify Verdanturf is
available only after its normal visit gate.

### 2. Routes 111 and 112, Fiery Path, and Route 113

#### Actual flow

Badge 3 and HM-free Rock Smash permit the player to break the southern Route
111 rocks. Two Team Magma objects at Route 112 `(26, 30)` and `(27, 30)` block
the lower cable-car entrance. Their current interaction repeats four dialogue
boxes. The first progression route therefore uses Fiery Path, upper Route 112,
and Route 113 to reach Fallarbor and Route 114. The Meteor Falls scene later
hides the Route 112 blocker, after which the vanilla flow asks the player to
manually retrace already-cleared terrain.

#### Target flow

- Keep the entire first traversal and all new encounter access.
- Keep meaningful trainers and exploration on Routes 111, 112, and 113 and in
  Fiery Path.
- Compress `Route112_EventScript_MagmaGrunts` to one short directional clue.
- Do not open the cable car early.
- Remove only the unnecessary post-Meteor Falls retrace by the direct lower
  cable-station offer described in the Meteor Falls plan.
- Preserve normal walking as an option and preserve the cable ride itself.

#### Exact implementation file and script

- Edit only `data/maps/Route112/scripts.inc` for this area.
  - Shorten `Route112_EventScript_MagmaGrunts`.
  - Preserve both blocking object events and their shared script.

Inspect but do not edit:

- `data/maps/Route111/scripts.inc` and `map.json`
- `data/maps/Route112/map.json`
- `data/maps/FieryPath/scripts.inc` and `map.json`
- `data/maps/Route113/scripts.inc` and `map.json`
- lower and upper cable-station scripts and map data

#### Flags, variables, objects, warps, battles, and rewards

- Preserve `FLAG_HIDE_ROUTE_112_TEAM_MAGMA` as the two blocker objects' hide
  flag; Meteor Falls owns setting it. The objects remain
  `LOCALID_ROUTE112_GRUNT_1` and `LOCALID_ROUTE112_GRUNT_2`.
- Preserve `FLAG_HIDE_ROUTE_111_ROCK_SMASH_TIP_GUY`,
  `FLAG_RECEIVED_HM_ROCK_SMASH`, `FLAG_LANDMARK_FIERY_PATH`,
  `VAR_JAGGED_PASS_ASH_WEATHER`, and the applicable Scott visibility state.
- Preserve the Route 111 connection to Mauville and Route 112.
- Preserve the Route 112 cable-car doors at `(28, 27)` and `(29, 27)`, Fiery
  Path entrances at `(11, 36)` and `(22, 10)`, the Route 113 connection, and
  the lower Jagged Pass doors.
- Preserve Route 111's Winstrate sequence, Gabby & Ty, and the Drew, Heidi,
  Beau, Becky, Dusty, Travis, Irene, Daisuke, Wilton, Brooke, Hayden, Bianca,
  Tyron, Celina, Celia, Bryan, and Branden trainer content.
- Preserve Route 112's Brice, Trent, Larry, Carol, Bryant, and Shayla battles.
- Preserve Route 113's Jaylen, Dillon, Madeline, Lao, Lung, Tori & Tia, Sophie,
  Coby, Wyatt, and Lawrence battles.
- Preserve all route pickups, berries, Secret Power access, the Route 113 Rest
  Stop, Trainer Hill access, Mirage Tower/fossil access, and Fiery Path's
  Toxic, Fire Stone, and later Strength branch.

#### Dependencies and emulator verification

Arc 2 supplies Rock Smash. Meteor Falls later removes the cable blocker; no
earlier state may do so. In mGBA, verify the Route 111 rocks, the concise but
informative blocked-cable interaction, encounters on each newly reached map,
representative trainers, all map connections, and the full initial traversal
through Route 113. Revisit after Meteor Falls and confirm the blockers remain
gone across a save/reload.

### 3. Fallarbor Town and Route 114

#### Actual flow

Fallarbor sets its visited flag and has no mandatory arrival cutscene. Route
114 supplies a new encounter, twelve ordinary trainers, Rock Smash branches,
Lanette, the Fossil Maniac, items, and the Meteor Falls entrance at `(8, 63)`.
There is no required Remoraid workaround; the relevant evolution progression
is already handled by the project's earlier evolution changes.

Cozmo's later Meteorite-for-Return exchange is optional and safe but verbose.
Fallarbor story-facing NPC dialogue is also longer than needed.

#### Target flow

- Preserve Fallarbor and Route 114 access, encounters, trainers, services,
  side areas, and rewards.
- Do not add a Remoraid catch, trade, party check, or other evolution errand.
- Shorten only story-facing dialogue:
  - `FallarborTown_EventScript_ExpertM`
  - Cozmo's wife's status dialogue
  - Cozmo's no-Meteorite and Meteorite-exchange dialogue
- Make Cozmo's `NoticeMeteorite` path ask about the exchange directly.
- Preserve the existing safe order for Return: successfully give the TM before
  removing the Meteorite or setting the receipt flag. A full TM pocket must
  leave the exchange retryable.
- Leave unrelated optional lore dialogue unchanged.

#### Exact implementation files and scripts

- `data/maps/FallarborTown/scripts.inc`
  - `FallarborTown_EventScript_ExpertM`
- `data/maps/FallarborTown_CozmosHouse/scripts.inc`
  - Cozmo's wife's story-status branches
  - Cozmo's no-item branch
  - the `NoticeMeteorite`/Return exchange path and its associated text labels

Inspect but do not edit:

- `data/maps/Route114/scripts.inc` and `map.json`
- Fallarbor's other building scripts and map data
- Route 114's Lanette, Fossil Maniac, Terra Cave, and Route 115 connections

#### Flags, variables, objects, warps, battles, and rewards

- Preserve `FLAG_VISITED_FALLARBOR_TOWN`.
- Preserve `FLAG_HIDE_FALLARBOR_HOUSE_PROF_COZMO` and its Mt. Chimney-driven
  visibility change.
- Preserve `FLAG_RECEIVED_TM_RETURN` and its idempotent reward behavior.
- Preserve `FLAG_HIDE_FALLARBOR_TOWN_BATTLE_TENT_SCOTT`.
- Preserve the Route 113/Route 114 town connections, the Meteor Falls entrance
  at `(8, 63)`, Route 115 access, and all Lanette/Fossil Maniac/Terra Cave
  warps.
- Preserve Route 114's twelve ordinary trainer battles and their rematches.
- Preserve Roar, Dig, Return, the doll reward, berries, and map items.

#### Dependencies and emulator verification

Fallarbor is reached only after the complete Route 113 traversal, and Meteor
Falls remains reached through Route 114. In mGBA, verify the visit flag,
services, Route 114 encounter, trainers and branches, concise story dialogue,
absence of a Remoraid gate, and the Meteor Falls entrance. After Mt. Chimney,
test Cozmo's exchange with room and with a full TM pocket; the latter must not
consume the Meteorite or mark Return received.

### 4. Meteor Falls

#### Actual flow

The Route 114 entrance places the player in `MeteorFalls_1F_1R` near `(27, 18)`.
The story trigger near `(14, 18)` runs while `VAR_METEOR_FALLS_STATE == 0`.
It stages Cozmo, Team Magma, Archie, and Team Aqua for a dialogue-only
confrontation; no battle belongs in this scene. The current confrontation is
overlong. It runtime-removes Team Magma actors but does not persistently set
`FLAG_HIDE_METEOR_FALLS_TEAM_MAGMA`, so actors can reappear incorrectly after
map reload. The vanilla ending also requires a pointless manual retrace to
Route 112.

#### Target flow

Retain the location, encounter access, physical exploration, Aqua/Magma
confrontation, and later/deeper Meteor Falls content. Shorten the confrontation
to:

- one concise Magma statement of intent,
- Archie's and Aqua's arrival,
- a concise confrontation and Mt. Chimney direction,
- a brief departure.

After movement and departure are complete, commit all terminal story state
before offering travel:

```text
VAR_METEOR_FALLS_STATE = 1
FLAG_MET_ARCHIE_METEOR_FALLS = set
FLAG_HIDE_ROUTE_112_TEAM_MAGMA = set
FLAG_HIDE_METEOR_FALLS_TEAM_MAGMA = set
FLAG_HIDE_METEOR_FALLS_TEAM_AQUA = set
FLAG_HIDE_FALLARBOR_TOWN_BATTLE_TENT_SCOTT = set
```

Then ask whether to go directly toward Mt. Chimney:

- **Yes:** set `VAR_CABLE_CAR_STATION_STATE = 0`, fade out, and execute
  `warp MAP_ROUTE112_CABLE_CAR_STATION, 6, 9`. The approved destination is the
  lower cable-car station's safe interior tile `(6, 9)`; do not bypass the
  cable ride.
- **No:** remain in Meteor Falls with the story complete.

Because the choice is optional, Cozmo's post-event interaction must offer the
same direct transition again while `VAR_METEOR_FALLS_STATE == 1` and
`FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY` is unset. Once Mt. Chimney is complete,
Cozmo returns to the normal post-event behavior. This retry is part of the
story event and is available regardless of the Quick Travel setting.

#### Exact implementation file and scripts

- `data/maps/MeteorFalls_1F_1R/scripts.inc`
  - `MeteorFalls_1F_1R_EventScript_MagmaStealsMeteoriteScene`
  - `MeteorFalls_1F_1R_EventScript_ProfCozmo`
  - `MeteorFalls_1F_1R_EventScript_MetCozmo`
  - the confrontation movement chains and text labels directly referenced by
    those scripts
  - a new local helper for the lower-station offer/warp if that is clearer than
    sharing inline labels

Preserve `MeteorFalls_1F_1R_MapScripts`, `MeteorFalls_1F_1R_OnLoad`, and
`MeteorFalls_1F_1R_EventScript_OpenStevensCave`; they own unrelated postgame
Steven-cave behavior.

Do not edit Meteor Falls `map.json`, encounters, other Meteor Falls floors,
trainer data, or map connections. Remove movement or text labels only if an
exact repository search proves they became unreferenced after the rewrite.

#### Flags, variables, objects, warps, battles, and rewards

- Use exactly the state and flags listed above.
- Preserve the story actors `LOCALID_METEOR_FALLS_MAGMA_GRUNT_1`,
  `LOCALID_METEOR_FALLS_MAGMA_GRUNT_2`, `LOCALID_METEOR_FALLS_ARCHIE`,
  `LOCALID_METEOR_FALLS_AQUA_GRUNT_1`,
  `LOCALID_METEOR_FALLS_AQUA_GRUNT_2`, and the Cozmo object running
  `MeteorFalls_1F_1R_EventScript_ProfCozmo`.
- Preserve the physical Route 114 and Route 115 exits, deeper-area warps,
  Steven's cave access, and all Meteor Falls encounter tables.
- Preserve the later Nicolas and John & Jay battles and the postgame Steven
  battle; this confrontation adds no battle.
- Preserve all Meteor Falls items and exploration rewards.
- The transition grants no reward and must not skip a new encounter map: the
  player already traversed Routes 112/113/114 and Meteor Falls, and must still
  ride the cable car and traverse Jagged Pass.

#### Dependencies and emulator verification

The confrontation depends on state 0 and supplies the Route 112 blocker hide
flag. The direct transition depends on persistent cleanup and normalized cable
state. In mGBA:

1. Enter from Route 114, obtain/verify the location encounter, and trigger the
   scene.
2. Decline travel, save, reload, and re-enter the room. Neither Aqua nor Magma
   may reappear and the confrontation may not replay.
3. Talk to Cozmo and accept the retry; verify landing at the safe lower-station
   tile with normal player control.
4. In a separate path, accept immediately after the scene and verify the same
   result.
5. Confirm the manual Route 114/113/112 return remains possible after declining.

### 5. Mt. Chimney

#### Actual flow

The lower station begins a normal cable ride by advancing
`VAR_CABLE_CAR_STATION_STATE` from 0 to 1. The upper station arrival at `(6, 4)`
returns it to 0. Mt. Chimney contains the Aqua/Magma tableau, two required
Magma Grunts, Tabitha, and Maxie. The speeches are lengthy, but the battles and
conflict are meaningful. The terminal Maxie sequence hides the teams, marks
the evil team defeated, adjusts Cozmo and Lava Cookie visibility, and permits
the Meteorite pickup. The original flow then requires manual movement to the
Jagged Pass exits at `(20, 41)` and `(21, 41)`.

#### Target flow

- Preserve the cable ride and both stations unchanged.
- Preserve both required Grunt battles, Tabitha, Maxie, loss/retry behavior,
  and the battlefield tableau.
- Shorten Archie's warning, Maxie's plan and challenge, Maxie's retreat, and
  the post-battle thanks without removing the conflict or objective.
- After Maxie, commit the existing terminal state exactly once:

```text
FLAG_HIDE_MT_CHIMNEY_TEAM_MAGMA = set
FLAG_HIDE_MT_CHIMNEY_TEAM_AQUA = set
FLAG_DEFEATED_EVIL_TEAM_MT_CHIMNEY = set
FLAG_HIDE_FALLARBOR_HOUSE_PROF_COZMO = clear
FLAG_HIDE_METEOR_FALLS_1F_1R_COZMO = set
FLAG_HIDE_MT_CHIMNEY_LAVA_COOKIE_LADY = clear
```

- Handle the Meteorite immediately after the fight. Reconcile an already-held
  item first. Otherwise attempt to give it and set its receipt state only after
  the physical item is confirmed. If the relevant pocket is full, leave the
  Meteorite machine interaction available for retry while keeping the defeated
  villain state complete.
- Once terminal state and the Meteorite attempt are complete, ask whether to
  continue directly down Jagged Pass:
  - **Yes:** set `VAR_JAGGED_PASS_ASH_WEATHER = 1`, fade out, and warp to
    the verified safe top landing with `warp MAP_JAGGED_PASS, 13, 6`.
  - **No:** remain on Mt. Chimney.
- Do not change `VAR_JAGGED_PASS_STATE`. The player must still traverse Jagged
  Pass normally; this transition must not complete or hide later Jagged Pass
  events.
- Keep the physical Mt. Chimney-to-Jagged Pass exits usable.

#### Exact implementation file and scripts

- `data/maps/MtChimney/scripts.inc`
  - preserve `MtChimney_OnTransition` and `MtChimney_OnResume`
  - shorten text used by `MtChimney_EventScript_Archie`,
    `MtChimney_EventScript_ArchieGoStopTeamMagma`, and
    `MtChimney_EventScript_ArchieBusyFighting`
  - preserve `MtChimney_EventScript_Grunt1`,
    `MtChimney_EventScript_Grunt2`, and `MtChimney_EventScript_Tabitha`
  - revise `MtChimney_EventScript_Maxie` and its directly referenced pre-battle,
    retreat, Archie-thanks, approach, and exit labels
  - revise `MtChimney_EventScript_MeteoriteMachine`,
    `MtChimney_EventScript_LeaveMeteoriteAlone`,
    `MtChimney_EventScript_MachineOff`, and
    `MtChimney_EventScript_MachineOn` only as required for the approved reward
    reconciliation/retry behavior
  - a local post-completion Jagged Pass prompt/warp helper

Inspect but do not edit:

- `data/maps/Route112_CableCarStation/scripts.inc` and `map.json`
- `data/maps/MtChimney_CableCarStation/scripts.inc` and `map.json`
- `data/maps/MtChimney/map.json`
- `data/maps/JaggedPass/map.json`

#### Flags, variables, objects, warps, battles, and rewards

- Preserve `VAR_CABLE_CAR_STATION_STATE` and its normal `0 -> 1 -> 0` cycle.
- Preserve `FLAG_RECEIVED_METEORITE`; set it only after
  `ITEM_METEORITE` is confirmed in the Bag.
- Use the six terminal flags listed above and preserve any existing per-trainer
  defeated flags.
- Preserve all Aqua/Magma battlefield objects until Maxie's completion script
  hides them, especially `LOCALID_MT_CHIMNEY_ARCHIE`,
  `LOCALID_MT_CHIMNEY_MAXIE`, `LOCALID_MT_CHIMNEY_TABITHA`,
  `LOCALID_MT_CHIMNEY_MAGMA_GRUNT_1`, and
  `LOCALID_MT_CHIMNEY_MAGMA_GRUNT_2`.
- Preserve `TRAINER_GRUNT_MT_CHIMNEY_1`,
  `TRAINER_GRUNT_MT_CHIMNEY_2`, `TRAINER_TABITHA_MT_CHIMNEY`, and
  `TRAINER_MAXIE_MT_CHIMNEY`.
- Preserve the later Shelby, Melissa, Sheila, Shirley, Sawyer, and other
  state-gated Mt. Chimney trainer content.
- Preserve the cable-station warps, Jagged Pass doors at `(20, 41)` and
  `(21, 41)`, and the approved direct landing at Jagged Pass `(13, 6)`.
- Preserve the Meteorite and Lava Cookie rewards. The direct transition grants
  nothing and must be repeat-safe.

#### Dependencies and emulator verification

Mt. Chimney requires the Meteor Falls blocker state and a successful normal
cable ride. Jagged Pass becomes the natural forward route only after Maxie's
terminal state. In mGBA, test both cable directions and the station-state
cycle; every required villain battle, including intentional loss/retry paths;
terminal actor cleanup across save/reload; Meteorite receipt with room, already
held, and full-pocket cases; both prompt choices; the direct landing; the
physical Jagged exits; and later revisit trainer availability.

### 6. Jagged Pass and Lavaridge

#### Actual flow

Jagged Pass already has its own encounter table, five ordinary trainers, a
Burn Heal, Acro Bike routes, and later story state. It naturally descends into
Lavaridge. Lavaridge sets its visited state and controls later Mt. Chimney
trainer visibility.

The Lavaridge Gym retains eight Gym trainers and Flannery. Her victory script
awards Badge 4, performs standard badge and progression work, updates
`VAR_LAVARIDGE_TOWN_STATE` to 1, and eventually relies on a longer rival scene
to give the Go-Goggles and advance the town state to 2. That rival item path
does not sufficiently protect story advancement from an item-give failure.
Desert passage correctly checks possession of the physical Go-Goggles.

#### Target flow

Jagged Pass is unchanged. The player must arrive at its top, receive its
encounter opportunity, fight or avoid its normal trainers as map design
allows, obtain its item if desired, and descend normally.

On the first Flannery victory:

- Preserve battle fanfare, defeated/badge flags, trainer progression, cap
  progression, and the standard Wanda/Wally and Petalburg readiness updates.
- Preserve the existing Flannery registration/call behavior, but perform it
  silently if it currently adds unnecessary dialogue.
- Reconcile an already-held Go-Goggles item, otherwise give it immediately in
  the Gym.
- Perform that Go-Goggles reconciliation before attempting the optional
  randomized Overheat TM reward.
- Set the Go-Goggles receipt state and `VAR_LAVARIDGE_TOWN_STATE = 2` only after
  physical possession is confirmed.
- Keep both rival-scene objects hidden for the fresh Arc 3 path; no outdoor
  rival handoff is required after a successful Gym reward.
- Preserve the randomized Overheat TM reward and its existing safe retry
  behavior. An inventory failure for the TM must not replay Badge 4 or the
  Go-Goggles award.
- On later Flannery interactions, retry the Go-Goggles first if possession was
  not confirmed, then retry Overheat as required, without replaying the win
  sequence.
- Keep legacy `VAR_LAVARIDGE_TOWN_STATE == 1` rival scripts compilable for old
  saves, but a fresh planned path must never be left in state 1.
- Do not add a direct Petalburg warp. After the player exits the Gym, the
  existing outdoor Quick Travel system provides the streamlined return when
  enabled. Walking remains valid when it is disabled.

#### Exact implementation file and scripts

- `data/maps/LavaridgeTown_Gym_1F/scripts.inc`
  - `LavaridgeTown_Gym_1F_EventScript_Flannery`
  - `LavaridgeTown_Gym_1F_EventScript_FlanneryDefeated`
  - `LavaridgeTown_Gym_1F_EventScript_GiveOverheat`
  - `LavaridgeTown_Gym_1F_EventScript_GiveOverheat2`
  - a local Go-Goggles reconciliation/retry helper added for the approved flow
  - directly referenced Flannery victory, reward, registration, and post-battle
    text labels only where shortening is required

Inspect but do not edit:

- `data/maps/JaggedPass/scripts.inc` and `map.json`
- `data/maps/LavaridgeTown/scripts.inc` and `map.json`
- Lavaridge Gym `map.json` and trainer layouts
- Petalburg town/Gym scripts
- `src/ruleset_field.c::Ruleset_QuickTravelAvailable()` and
  `src/field_move.c::FieldMove_PokeRiderEnabled()`
- the existing region-map Fly/Quick Travel flow and visited-town destinations
- `data/maps/Route111/scripts.inc::Route111_EventScript_ViciousSandstormTrigger`
  and `Route111_EventScript_PreventRouteAccess`

#### Flags, variables, objects, warps, battles, and rewards

- Do not change `VAR_JAGGED_PASS_STATE`; preserve
  `VAR_JAGGED_PASS_ASH_WEATHER` initialization for the top landing.
- Preserve Lavaridge's visited state and normal later Mt. Chimney reveal logic.
- Preserve `FLAG_WHITEOUT_TO_LAVARIDGE`,
  `FLAG_DEFEATED_LAVARIDGE_GYM`, `FLAG_BADGE04_GET`, the cap update to 36,
  and HM-free Strength progression.
- Advance `VAR_PETALBURG_GYM_STATE` from 5 to 6 once through the existing
  `Common_EventScript_ReadyPetalburgGymForBattle` call. Preserve its greeter
  visibility update and expanded Petalburg Mart state; do not run Norman
  content. Preserve the `Common_EventScript_SetGymTrainers` call.
- Require confirmed physical `ITEM_GO_GOGGLES` possession before setting
  `FLAG_RECEIVED_GO_GOGGLES` or `VAR_LAVARIDGE_TOWN_STATE = 2`.
- Preserve `FLAG_HIDE_LAVARIDGE_TOWN_RIVAL` and
  `FLAG_HIDE_LAVARIDGE_TOWN_RIVAL_ON_BIKE` on the fresh direct-reward path;
  their objects are `LOCALID_LAVARIDGE_RIVAL` and
  `LOCALID_LAVARIDGE_RIVAL_ON_BIKE`.
- Preserve `FLAG_RECEIVED_TM_OVERHEAT` and
  `FLAG_ENABLE_FLANNERY_MATCH_CALL` as independently idempotent post-victory
  state.
- Preserve the desert's physical-item check and transient `VAR_TEMP_3` use.
  Mirage Tower remains governed by its own independent state.
- Preserve Jagged Pass trainers Eric, Diana, Ethan, Julio, and Autumn, plus its
  later state-gated guard/battle.
- Preserve Lavaridge Gym trainers Cole, Gerald, Axle, Danielle, Keegan, Jace,
  Jeff, and Eli, and preserve the Flannery boss battle.
- Preserve the Burn Heal, Lavaridge Egg, Heat Badge, randomized Overheat TM,
  and Go-Goggles rewards.
- Preserve all Jagged Pass entrances/exits, Lavaridge building warps, and Gym
  puzzle warps.

#### Dependencies and emulator verification

Jagged Pass depends on completed Mt. Chimney but must retain its independent
later state. Flannery supplies Badge 4, cap 36, Go-Goggles, desert access, and
the existing Arc 4 readiness state without beginning Arc 4. In mGBA, verify
the Jagged encounter, representative trainers, item and Acro routes, descent,
Lavaridge visit, all Gym trainers, Flannery loss/retry and victory, one-time
badge/Petalburg advancement, immediate physical Go-Goggles, randomized
Overheat, full-pocket retries, no fresh state-1 rival scene, desert passage,
and the existing Quick Travel choice for Petalburg. Cancel the Petalburg trip
or stop before entering its Gym.

## C. Recommended whole/split structure

Implement Arc 3 as one approved arc, not as separate approval sub-arcs.

The dependency graph does not require a formal split. The implementation is a
single linear state chain and the intended edit surface is six map-script
files. Splitting it into independently accepted sub-arcs would materially
increase the risk of leaving temporary inconsistent states—for example, a
completed Meteor Falls scene with no safe cable transition, or a completed
Flannery battle that still depends on the obsolete outdoor reward scene.

Use three internal implementation/checkpoint groups instead:

1. **Approach and Meteor Falls:** Route 112 dialogue, Fallarbor/Cozmo dialogue,
   Meteor Falls persistent cleanup and lower-station transition.
2. **Mt. Chimney:** shortened conflict, preserved battles, terminal state,
   Meteorite handling, and Jagged Pass transition.
3. **Flannery terminal:** immediate Go-Goggles, retry-safe TM handling, desert
   access, and Petalburg readiness.

These are build and review checkpoints, not separate arcs. The execution
session must complete and verify the continuous chain before handing it off.

## D. Exact edit boundary and implementation order

### Authorized Arc 3 implementation files

Only these six source files are authorized for Arc 3 implementation unless a
new planning/review session explicitly amends this plan:

1. `data/maps/Route112/scripts.inc`
2. `data/maps/FallarborTown/scripts.inc`
3. `data/maps/FallarborTown_CozmosHouse/scripts.inc`
4. `data/maps/MeteorFalls_1F_1R/scripts.inc`
5. `data/maps/MtChimney/scripts.inc`
6. `data/maps/LavaridgeTown_Gym_1F/scripts.inc`

No public API, save-layout, map JSON, map connection, encounter-table,
trainer-party, or C-source change is planned. If implementation proves that
one is necessary, stop and return to planning rather than expanding scope.

### Implementation order

1. Re-read the source-of-truth documents and inspect repository status/diffs.
2. Capture the exact pre-edit scripts, label references, map warp definitions,
   flag/variable constants, trainer constants, and reward calls named here.
3. Implement Route 112's concise blocker dialogue.
4. Implement Fallarbor/Cozmo dialogue shortening while preserving Return's
   give-before-consume ordering.
5. Implement Meteor Falls dialogue, persistent actor cleanup, terminal-state
   order, direct lower-station choice, and Cozmo retry.
6. Complete Checkpoint 1 static checks and build.
7. Implement Mt. Chimney dialogue shortening, preserve all battles, commit
   terminal flags, reconcile the Meteorite, and add the optional Jagged Pass
   transition without changing `VAR_JAGGED_PASS_STATE`.
8. Complete Checkpoint 2 static checks and build.
9. Implement Flannery's immediate, possession-confirmed Go-Goggles reward and
   idempotent Overheat retry while retaining all badge/Petalburg progression.
10. Complete Checkpoint 3 static checks and build.
11. Audit the total diff against the six-file allowlist and this plan.
12. Run final release and debug builds, then perform the continuous emulator
    route and supplemental failure-path tests.
13. Record implementation, static verification, build verification, and mGBA
    acceptance as distinct handoff states. Do not begin Arc 4.

## E. Build checkpoints and static checks

### Checkpoint 1: approach through Meteor Falls

After changes to Route 112, Fallarbor, Cozmo's house, and Meteor Falls:

```sh
git diff --check
make -j$(sysctl -n hw.ncpu)
```

Statically verify:

- Route 112 still has two Magma blockers tied to
  `FLAG_HIDE_ROUTE_112_TEAM_MAGMA`.
- The full Route 111/112/Fiery Path/Route 113/114 first traversal remains.
- There is no Shelmet/Karrablast or Remoraid progression errand.
- Return is given successfully before Meteorite removal/receipt state.
- Meteor Falls terminal state is written only after the actor scene completes.
- Both Meteor Falls villain groups receive persistent hide flags.
- The Route 112 blocker flag is set by the completed confrontation.
- Declining travel leaves a valid Cozmo retry while Mt. Chimney is unfinished.
- The accepted warp resolves to the lower cable station's safe arrival and
  explicitly normalizes cable state to 0.

### Checkpoint 2: Mt. Chimney through Jagged Pass handoff

After the Mt. Chimney change:

```sh
git diff --check
make -j$(sysctl -n hw.ncpu)
```

Statically verify:

- The lower/upper cable station scripts still perform their normal state cycle.
- Both required Grunts, Tabitha, and Maxie remain referenced and reachable.
- Loss paths do not set completion flags or offer forward travel.
- All six Mt. Chimney terminal visibility/progression flags are present.
- The Meteorite reward is possession-aware and failure-safe.
- The prompt occurs after terminal state/reward handling.
- The direct warp resolves to safe Jagged Pass top coordinates.
- `VAR_JAGGED_PASS_ASH_WEATHER` is initialized and
  `VAR_JAGGED_PASS_STATE` is never mutated by the new transition.
- Physical cable, Mt. Chimney, and Jagged Pass warps remain intact.

### Checkpoint 3: Flannery terminal

After the Lavaridge Gym change:

```sh
git diff --check
make -j$(sysctl -n hw.ncpu)
```

Statically verify:

- All Gym trainers and Flannery remain reachable.
- The badge/common progression path runs once.
- `VAR_PETALBURG_GYM_STATE` reaches exactly 6 from the Arc 2 value 5 and the
  existing greeter/Mart updates remain intact.
- Badge 4 produces cap 36 and existing HM-free Strength progression.
- Go-Goggles receipt and `VAR_LAVARIDGE_TOWN_STATE = 2` occur only after
  physical item possession is confirmed.
- A failed Go-Goggles award is retryable without replaying the badge.
- Overheat remains randomized and independently retryable.
- Fresh progression cannot end at `VAR_LAVARIDGE_TOWN_STATE == 1`.
- Legacy state-1 handling remains compilable for existing saves.
- Desert access still checks the physical Go-Goggles.
- Quick Travel and Petalburg scripts are unchanged.

### Final static and build gate

Before emulator acceptance:

```sh
git status --short
git diff --check
git diff --stat
make clean
make -j$(sysctl -n hw.ncpu)
make clean
make debug
```

The final static audit must confirm:

- Only the six authorized implementation scripts are changed by execution,
  in addition to this approved plan file and any separately pre-existing user
  changes.
- Every newly removed text/movement label has no remaining reference.
- No map JSON, encounter, trainer-party, C, header, save-layout, or Arc 4 file
  changed.
- Arcs 0–2 behavior and their terminal state remain intact.
- All event rewards are idempotent and failure-safe.
- Both direct story transitions are optional, occur after state commit, and do
  not skip any new encounter route.
- Release and debug builds both succeed. A non-vanilla ROM SHA-1 is expected
  for this hack and is not itself a build failure.

## F. Continuous emulator test route

Use a fresh or controlled save at the Arc 2 Wattson handoff. Record relevant
flags and variables at each numbered boundary where the emulator/debug tooling
allows it.

1. Defeat Wattson. Confirm Badge 3, cap 29, HM-free Rock Smash, both bikes, and
   `VAR_PETALBURG_GYM_STATE == 5`.
2. Detour across Route 117. Trigger its encounter opportunity, test
   representative trainers and Day Care access, and enter Verdanturf.
3. Confirm no Shelmet/Karrablast progression errand. Optionally complete or
   inspect Rusturf restoration and confirm it is not required. If Quick Travel
   is enabled, return to Mauville through the existing system.
4. Travel north on Route 111, use Rock Smash, and inspect the concise Route 112
   cable blocker clue. Confirm the cable cannot yet be ridden.
5. Traverse Fiery Path and upper Route 112, retaining new encounters and
   representative trainer/reward access.
6. Traverse Route 113 normally and reach Fallarbor. Verify its visit behavior
   and concise story-facing dialogue.
7. Traverse Route 114, obtain its encounter opportunity, and verify trainers,
   branches, side areas, and the Meteor Falls entrance.
8. Enter Meteor Falls, obtain/verify its encounter opportunity, and run the
   shortened Aqua/Magma confrontation.
9. Decline direct travel. Save/reload and revisit the room; the scene must not
   replay and neither villain group may reappear.
10. Speak to Cozmo and accept the retry. Confirm a safe landing in the lower
    Route 112 cable station with state 0 and normal control.
11. Ride the cable car normally and confirm the complete `0 -> 1 -> 0` station
    state cycle.
12. On Mt. Chimney, preserve the conflict and defeat both required Grunts,
    Tabitha, and Maxie. Separately exercise at least one intentional loss/retry
    and confirm no premature completion.
13. Verify persistent villain cleanup, Cozmo visibility changes, the Meteorite
    attempt, and the concise terminal dialogue.
14. Decline the Jagged Pass transition once and verify physical exits remain
    usable; on another pass, accept and verify the safe top landing.
15. Traverse Jagged Pass normally. Trigger its encounter opportunity, fight
    representative trainers, inspect the Burn Heal and Acro routes, and descend
    into Lavaridge. Confirm no premature later Jagged state.
16. Enter Lavaridge Gym, test its puzzle and trainers, intentionally lose once
    if practical, then defeat Flannery.
17. Confirm Badge 4, cap 36, one-time Petalburg state 6, immediate physical
    Go-Goggles, randomized Overheat behavior, and no fresh outdoor rival reward
    scene.
18. Exit the Gym. Open existing Quick Travel, verify Petalburg is now a valid
    destination when that system is enabled, and cancel without entering Arc 4.
19. Return through Quick Travel or normal walking to Route 111's desert and
    verify the physical Go-Goggles grant access. Preserve desert encounters,
    trainers, items, and Mirage Tower's independent state.

Supplement the continuous route with focused save-state tests for:

- accepting the Meteor Falls transition immediately,
- declining it and using Cozmo's retry,
- both Mt. Chimney prompt choices,
- repeat cable rides and save/reloads at both villain-state boundaries,
- already-held and full-pocket Meteorite handling,
- already-held and full-pocket Go-Goggles handling,
- full TM pocket for Return and Overheat,
- repeated Flannery interaction after every reward-state combination,
- Quick Travel disabled, proving all manual routes remain valid.

Map scripts and events cannot be accepted by code inspection or builds alone.
The full continuous mGBA route and the relevant failure-path tests are required
before Arc 3 can be marked accepted.

## Risks and mitigations

1. **Meteor Falls actors reappear after reload.** The current runtime removal
   is insufficient. Set both persistent villain hide flags before travel and
   explicitly test save/reload.
2. **A direct warp leaves the player blocked or on an invalid tile.** Resolve
   warp ids and coordinates against the actual map JSON immediately before
   implementation and test player control at each landing.
3. **Cable state is stale.** Normalize it to 0 before the lower-station warp and
   preserve the normal ride's `0 -> 1 -> 0` behavior.
4. **Reward failure corrupts story progression.** Confirm physical Meteorite or
   Go-Goggles possession before receipt state; preserve retry paths for every
   full-pocket case.
5. **The Jagged shortcut skips its content.** Land at the top, set only ash
   weather, do not change `VAR_JAGGED_PASS_STATE`, and require normal descent.
6. **Badge or Petalburg progression repeats.** Keep one-time defeated/badge
   guards and separate item/TM retries from the victory transaction.
7. **Fresh saves fall into the legacy Lavaridge state-1 scene.** Advance a
   successful Gym reward directly to state 2 while retaining state-1 code only
   for old-save compatibility.
8. **The streamlined route accidentally skips Route 113 or another encounter.**
   Offer Meteor travel only after the initial Meteor Falls confrontation,
   which itself requires the complete northwestern first traversal.
9. **Direct story travel becomes coupled to Quick Travel settings.** Keep both
   story prompts local and unconditional; use existing Quick Travel only for
   the post-Flannery return toward Petalburg.
10. **Dialogue trimming removes referenced labels or movement.** Search every
    candidate label before deletion and run the assembler/build at each group.
11. **Arc 4 begins implicitly.** Stop at Petalburg readiness state 6. Do not
    enter, edit, or test Norman's challenge beyond verifying the prerequisite.
12. **Static success hides event defects.** Require the continuous emulator
    route, reload tests, loss paths, both prompt choices, and inventory-failure
    cases before acceptance.

## Acceptance criteria

Arc 3 implementation is acceptable only when all of the following are true:

- Arcs 0–2 have not regressed, and Arc 2's Wattson handoff remains valid.
- Route 117/Verdanturf remains optional and useful; Rusturf restoration remains
  optional flavor; no obsolete Shelmet/Karrablast errand exists.
- Routes 111/112, Fiery Path, Route 113, Fallarbor, Route 114, Meteor Falls,
  Jagged Pass, and the desert retain their encounter opportunities.
- Meaningful route, villain, Gym-trainer, and boss battles remain, including
  Tabitha, Maxie, and Flannery.
- No obsolete Remoraid workaround exists, and approved story dialogue is
  materially shorter without losing direction.
- The Meteor Falls Aqua/Magma confrontation completes once, persists across
  reload, clears the Route 112 blockers, and safely offers the lower cable
  station without bypassing the cable ride.
- Mt. Chimney retains its villain conflict and Maxie battle, persists terminal
  state, safely handles the Meteorite, and makes Jagged Pass the natural
  forward path.
- The Jagged Pass transition preserves its encounter and traversal and does
  not alter `VAR_JAGGED_PASS_STATE`.
- Flannery grants Badge 4 progression once and makes the physical Go-Goggles
  available immediately with safe retry handling.
- The Go-Goggles open the desert, and the existing Quick Travel system can
  streamline return toward Petalburg after four badges when enabled.
- `VAR_PETALBURG_GYM_STATE == 6` and associated greeter/Mart readiness are
  reached exactly once, but Norman and all other Arc 4 content remain untouched.
- All specified static checks pass, release and debug builds pass, and the
  continuous mGBA route plus applicable failure-path tests receive user
  acceptance.
- Implementation, static verification, build verification, and emulator
  acceptance are recorded separately in the handoff.

Arc 4 must not begin until Arc 3 has been implemented in a fresh execution
session, reviewed in a fresh read-only review session, built successfully, and
accepted by the user in mGBA.
