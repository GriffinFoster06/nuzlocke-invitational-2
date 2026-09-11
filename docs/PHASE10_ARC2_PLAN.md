# Phase 10 Arc 2 — Slateport, Route 110, and Mauville

## Status and boundaries

This is the approved, implementation-ready plan for Phase 10 Arc 2. It is based on repository state at commit `09528c46bbdc`.

- Phase 10 Arcs 0 and 1 are complete and must not regress.
- Arc 2 covers only Slateport and Route 110/Mauville, ending with the Wattson badge.
- Arc 3 must not begin in this work.
- Catch-rate mechanics are out of scope.
- This planning document does not implement gameplay changes.
- A fresh execution session must inspect `AGENTS.md`, `docs/SPEC.md`, this plan, `git status`, `git diff`, and `git diff --staged` before editing.
- Preserve unrelated user changes. During implementation, edit only the files explicitly listed under **Implementation edit set** unless a build error proves that a directly related declaration file is required; obtain approval before expanding scope.

## Intended Arc 2 result

The continuous required path is:

`Brawly badge → Route 109 landing → direct Museum availability → two Aqua Museum battles → automatic Devon Goods handoff to Stern → Route 110 rival → Mauville Wally → both bikes → Wattson → Dynamo Badge, cap 29, and Rock Smash`

Slateport remains fully accessible. Its beach, encounters, trainers, buildings, market, Battle Tent, Shipyard, and Museum exhibits remain available. The Shipyard visit is no longer a progression prerequisite: entering Slateport with the recovered Devon Goods prepares the Museum immediately. Stern and Archie remain in the meaningful Museum event, both Aqua battles remain, and the Devon Goods resolve there without a return trip.

Route 110 remains an encounter route. The rival battle remains mandatory on the main lower path, while Trick House, Cycling Road, and ordinary trainers remain optional. Wally remains the meaningful mandatory Mauville gate and Wattson remains the mandatory gym leader. Rydel gives both bikes in one successful interaction, and selecting either Key Item becomes the intended bike-switching mechanism. Wattson’s successful first victory grants Rock Smash automatically while preserving badge, cap, Match Call, gym, and downstream story state.

## Arc 1 entry contract

Arc 2 must consume, not recreate, the completed Arc 1 handoff:

- `FLAG_BADGE02_GET` and `FLAG_DEFEATED_DEWFORD_GYM` are set.
- The progression cap is 24.
- `VAR_PETALBURG_GYM_STATE` is 4 after Roxanne and Brawly.
- `FLAG_RECOVERED_DEVON_GOODS` and `FLAG_RETURNED_DEVON_GOODS` are set.
- `FLAG_DELIVERED_DEVON_GOODS` is unset.
- `ITEM_DEVON_PARTS` is physically present in the Key Items pocket.
- PokéNav, Match Call, and optional Quick Travel are already available.
- Briney’s direct ferry lands on Route 109 at `(22,24)` with `VAR_BRINEY_LOCATION=3`.
- Steven and the Letter are not prerequisites for Slateport.
- No Arc 0 or Arc 1 script, ferry transition, Quick Travel function, Roxanne/Brawly state, or Rustboro/Rusturf state may be altered.

## Slateport

### Current actual flow

1. Arc 1 lands the player on Route 109 at `(22,24)`. Route 109 remains a normal water/fishing encounter area with optional beach trainers and Seashore House.
2. In Slateport, the Aqua queue outside the Oceanic Museum is visible because `FLAG_HIDE_SLATEPORT_CITY_TEAM_AQUA` is initially clear. The queue blocks prompt Museum access.
3. The player must enter Stern’s Shipyard and speak to Dock through `SlateportCity_SternsShipyard_1F_EventScript_Dock`.
4. Dock sets `FLAG_DOCK_REJECTED_DEVON_GOODS` and `FLAG_HIDE_SLATEPORT_CITY_TEAM_AQUA`; only then does the queue disappear and the Museum become usable.
5. Museum 1F charges the normal fee, with its existing no-money compatibility branch, and retains the familiar Rusturf Grunt’s optional `ITEM_TM_THIEF` reward.
6. Speaking to Stern on Museum 2F starts a long scene, then battles `TRAINER_GRUNT_MUSEUM_1` and `TRAINER_GRUNT_MUSEUM_2` consecutively.
7. Archie enters, delivers exposition, and leaves with both Grunts. Stern then takes `ITEM_DEVON_PARTS`, heals the party, and leaves.
8. The completion script sets the Aqua-removal, Devon-delivery, Route 110, Birch, Rustboro employee, and outside-Museum states. Its current `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=1` causes a forced Scott scene outside, and `VAR_REGISTER_BIRCH_STATE=1` causes a forced Birch scene on southern Route 110.
9. Much later, Magma Hideout sets `VAR_SLATEPORT_CITY_STATE=1` and `VAR_SLATEPORT_HARBOR_STATE=1` for Stern’s interview and the submarine theft. Those later states are not owned by Arc 2.

### Required final flow

1. On any Slateport transition, if `FLAG_RECOVERED_DEVON_GOODS` is set and `FLAG_DELIVERED_DEVON_GOODS` is unset, prepare the city for Museum progression immediately:
   - set `FLAG_DOCK_REJECTED_DEVON_GOODS` for compatibility with existing Slateport dialogue;
   - set `FLAG_HIDE_SLATEPORT_CITY_TEAM_AQUA` so the exterior queue disappears.
2. Do not warp the player or force a cutscene. The whole city remains explorable and the player may enter the Museum promptly or visit optional content first.
3. Dock remains present and meaningful, but his first-progress dialogue becomes a short direction to Stern at the Museum. Speaking to Dock must not set any additional required progression state. Preserve his delivered-goods, Badge 7, game-clear, scientists, Briney, and ferry-building branches.
4. Preserve the Museum entrance fee, no-money branch, exhibits, patrons, and familiar Grunt/Thief reward.
5. Preserve `SlateportCity_OceanicMuseum_2F_EventScript_CaptStern`, its approach/facing support, both Aqua battles, loss/retry behavior, Archie’s appearance, Stern’s role, party heal, follower refresh, and the visual departure of all four story objects.
6. Compress only the pre-battle, between-battle, Archie, and Stern text/movement pauses. The concise scene must still communicate: the player delivers Devon’s parts to Stern; Aqua tries to steal them; the player defeats two Grunts; Archie identifies Team Aqua’s goal and warns the player; Stern accepts the parts and leaves.
7. After the second battle and concise Archie/Stern resolution, hand over `ITEM_DEVON_PARTS` through the existing automatic item-handoff mechanism. The player must not return to Dock, Devon Corp, or another NPC.
8. Commit the terminal state once, after the successful battles. Skip the forced Scott and Birch interruptions while preserving their registrations and state progression silently.

### Exact implementation files and labels

Edit only these Slateport files:

- `data/maps/SlateportCity/scripts.inc`
  - `SlateportCity_OnTransition`
  - add one clearly named helper called from this transition for the recovered-but-undelivered state;
  - leave `SlateportCity_EventScript_ScottScene` and `SlateportCity_EventScript_ScottBattleTentScene` available for their other/optional uses, but prevent the post-Museum forced scene by advancing its state directly.
- `data/maps/SlateportCity_SternsShipyard_1F/scripts.inc`
  - `SlateportCity_SternsShipyard_1F_EventScript_Dock`
  - `SlateportCity_SternsShipyard_1F_EventScript_GoFindStern`
  - related first-progress text only.
- `data/maps/SlateportCity_OceanicMuseum_2F/scripts.inc`
  - `SlateportCity_OceanicMuseum_2F_EventScript_CaptStern`
  - `SlateportCity_OceanicMuseum_2F_EventScript_ReadyRegisterBirch`, if retained after the terminal-state rewrite;
  - only text and movement data used by the condensed Stern/Aqua/Archie scene.

Inspect but do not edit unless separately approved:

- `data/maps/Route109/map.json`, `data/maps/Route109/scripts.inc`, and Route 109 encounter data.
- `data/maps/Route109_SeashoreHouse/map.json` and `scripts.inc`.
- `data/maps/SlateportCity/map.json`.
- `data/maps/SlateportCity_SternsShipyard_1F/map.json`.
- `data/maps/SlateportCity_SternsShipyard_2F/map.json` and `scripts.inc`.
- `data/maps/SlateportCity_OceanicMuseum_1F/map.json` and `scripts.inc`.
- `data/maps/SlateportCity_OceanicMuseum_2F/map.json`.
- `data/maps/SlateportCity_Harbor/map.json` and `scripts.inc`.
- `data/maps/MagmaHideout_4F/scripts.inc`.
- `src/pokenav_match_call_data.c`.

### Flags, variables, and object state

The Museum completion must leave this exact compatible state:

- Set `FLAG_DOCK_REJECTED_DEVON_GOODS` on Slateport transition before delivery. Although the Shipyard is bypassed, `SlateportCity_EventScript_Woman1` reads this flag to select the post-queue dialogue.
- Set `FLAG_HIDE_SLATEPORT_CITY_TEAM_AQUA` before Museum entry. It controls the exterior queue objects, not the Museum battles.
- Set `FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_AQUA_GRUNTS` after the Museum battle sequence. It hides the 1F Aqua population and must not be set early.
- Keep the Museum 2F object flags and local IDs intact:
  - `FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_2F_CAPTAIN_STERN` / `LOCALID_OCEANIC_MUSEUM_2F_CAPT_STERN`;
  - `FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_2F_ARCHIE` / `LOCALID_OCEANIC_MUSEUM_2F_ARCHIE`;
  - `FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_2F_AQUA_GRUNT_1` / `LOCALID_OCEANIC_MUSEUM_2F_GRUNT_1`;
  - `FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_2F_AQUA_GRUNT_2` / `LOCALID_OCEANIC_MUSEUM_2F_GRUNT_2`.
  Their `addobject`/`removeobject` staging remains the scene mechanism.
- Set `FLAG_DELIVERED_DEVON_GOODS` only after the two wins and handoff.
- Set `FLAG_HIDE_ROUTE_110_TEAM_AQUA` so the five Route 110 Aqua blocker objects disappear.
- Clear `FLAG_HIDE_ROUTE_116_DEVON_EMPLOYEE` and set `FLAG_HIDE_RUSTBORO_CITY_DEVON_CORP_3F_EMPLOYEE`, matching the existing delivery terminal state.
- If `VAR_REGISTER_BIRCH_STATE` is 0, set `FLAG_ENABLE_PROF_BIRCH_MATCH_CALL` and set `VAR_REGISTER_BIRCH_STATE=2` directly. Never leave it at 1, because Route 110 coordinates `(7–10,85)` key on state 1 and run the long Birch scene.
- Set `FLAG_ENABLE_SCOTT_MATCH_CALL` without fanfare if not already set. Advance `VAR_SCOTT_STATE` exactly once for the skipped outside-Museum Scott beat, and set `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=2` directly. Never leave it at 1, which triggers `SlateportCity_EventScript_ScottScene`.
- Preserve `VAR_SLATEPORT_MUSEUM_1F_STATE` initialization and the Museum fee behavior.
- Do not change `VAR_SLATEPORT_CITY_STATE` or `VAR_SLATEPORT_HARBOR_STATE`. Their later values are set by `MagmaHideout_4F` and drive the Arc 3+ Stern interview/submarine theft.
- Do not touch `FLAG_HIDE_SLATEPORT_CITY_HARBOR_CAPTAIN_STERN`, `FLAG_HIDE_SLATEPORT_CITY_HARBOR_AQUA_GRUNT`, `FLAG_HIDE_SLATEPORT_CITY_HARBOR_ARCHIE`, `FLAG_HIDE_SLATEPORT_CITY_HARBOR_SUBMARINE_SHADOW`, `FLAG_HIDE_SLATEPORT_CITY_HARBOR_SS_TIDAL`, or `FLAG_HIDE_SLATEPORT_CITY_HARBOR_PATRONS`.
- Preserve `FLAG_HIDE_SLATEPORT_MUSEUM_POPULATION`; normal later logic continues to reveal patrons.

Any `VAR_SCOTT_STATE` increment must be guarded by the state transition (`VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE < 2`) so re-entering Slateport cannot increment it repeatedly.

### Warps and transitions to preserve

No map connection, warp event, coordinate event, layout, or collision change is authorized.

- Preserve the Route 109 ↔ Slateport south connection and Slateport ↔ Route 110 north connection.
- Preserve the Shipyard entrance at Slateport `(26,38)` and its internal stairs.
- Preserve the Museum entrance tiles at Slateport `(30,26)` and `(31,26)`, 1F exit tiles `(9,8)` and `(10,8)`, and stairs to 2F at `(6,1)`.
- Preserve the Harbor entrance at Slateport `(28,12)` and ferry-related access at `(40,7)`.
- `SlateportCity_OnTransition` changes flags only; it must not auto-warp, auto-walk, or seize input.

### Battles, encounters, rewards, and content to preserve

- Preserve Aqua Museum battles `TRAINER_GRUNT_MUSEUM_1` and `TRAINER_GRUNT_MUSEUM_2` exactly, including randomized trainer behavior and defeat flags.
- Preserve the Museum familiar Grunt’s retry-safe `ITEM_TM_THIEF` reward; do not hide him until the TM is successfully received.
- Preserve the Museum fee and its insufficient-money story exception.
- Preserve `ITEM_DEVON_PARTS` until Stern’s post-battle handoff; consume it exactly once with `Common_EventScript_PlayerHandedOverTheItem`.
- Preserve the post-scene party heal (`HealPlayerParty`) and following-Pokémon update (`UpdateFollowingPokemon`).
- Preserve Route 109 wild, surfing, and fishing encounter access and its Nuzlocke location identity.
- Preserve optional Route 109 trainers `TRAINER_DAVID`, `TRAINER_ALICE`, `TRAINER_HUEY`, `TRAINER_EDMOND`, `TRAINER_RICKY_1`, `TRAINER_LOLA_1`, `TRAINER_AUSTINA`, `TRAINER_GWEN`, `TRAINER_CARTER`, `TRAINER_MEL_AND_PAUL`, `TRAINER_CHANDLER`, `TRAINER_HAILEY`, and `TRAINER_ELIJAH`, including rematches and Match Call registration.
- Preserve Seashore House trainers `TRAINER_DWAYNE`, `TRAINER_JOHANNA`, and `TRAINER_SIMON`, and its six-Soda-Pop reward.
- Preserve Soft Sand, visible and hidden beach items, fishing NPCs, Slateport shops, Name Rater, Fan Club, Contest/Battle Tent content, Harbor, and all meaningful optional NPCs.

### Slateport downstream dependency audit

The sequence may be bypassed only because every downstream consumer receives the same terminal state:

- `FLAG_DELIVERED_DEVON_GOODS` advances rival Match Call text in `src/pokenav_match_call_data.c`.
- `FLAG_HIDE_ROUTE_110_TEAM_AQUA` opens the lower Route 110 route.
- `VAR_REGISTER_BIRCH_STATE=2` and `FLAG_ENABLE_PROF_BIRCH_MATCH_CALL` preserve Birch’s Match Call availability without the coordinate interruption.
- `FLAG_ENABLE_SCOTT_MATCH_CALL`, `VAR_SCOTT_STATE`, and `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=2` preserve Scott’s progression without the forced scene. Optional `SlateportCity_EventScript_ScottBattleTentScene` may still advance the outside-Museum state to 3.
- Wattson later sets `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=3`; this remains valid when Arc 2 begins from value 2.
- The Route 116/Rustboro employee flags retain the vanilla delivery cleanup expected by later revisits.
- Later Magma Hideout, Slateport interview, Harbor, Aqua, Archie, submarine, Mossdeep, and S.S. Tidal state remains driven by separate city/harbor variables and object flags and must remain untouched.

## Route 110 and Mauville

### Current actual flow

1. Museum completion hides the Aqua objects that block Route 110’s lower road.
2. The current Museum script leaves `VAR_REGISTER_BIRCH_STATE=1`, so crossing `(7–10,85)` triggers a long Birch Match Call registration scene before the route proper.
3. Route 110 retains wild encounters, lower-road trainers and items, optional Trick House, optional Cycling Road, and the Route 103 connection.
4. Coordinate triggers at `(33,56)`, `(34,56)`, and `(35,56)` start the rival scene. One of six May/Brendan starter-dependent trainers is selected. Victory grants `ITEM_ITEMFINDER` and sets `VAR_ROUTE110_STATE=1`.
5. Wally and his uncle block Mauville Gym. The current event contains a yes/no loop, a long post-battle exit, a forced Scott scene, and a delayed Wally phone registration after 250 steps.
6. Rydel gives only one bike. Getting the other requires returning to the shop and exchanging the first bike.
7. Rock Smash Dude in `MauvilleCity_House1` gives `ITEM_HM_ROCK_SMASH`, sets `FLAG_RECEIVED_HM_ROCK_SMASH`, and hides the Route 111 tip NPC. The field move itself is already unlocked by Badge 3 in `src/field_move.c`.
8. Wattson’s gym, trainers, switch puzzle, badge, Shock Wave reward, Match Call registration, Petalburg story increment, and cap progression are intact.

### Required final flow

1. Route 110 opens directly after the Museum. No forced Birch scene occurs.
2. Preserve all encounter access and optional content.
3. Retain the mandatory main-path rival battle and `ITEM_ITEMFINDER`; shorten only its dialogue and pauses.
4. Retain Wally as a mandatory meaningful battle before gym access. Remove the decline loop, forced Scott appearance, long exit choreography, and delayed forced call. After victory, resolve his visibility and Match Call state immediately and silently.
5. In one successful Rydel interaction, give both `ITEM_MACH_BIKE` and `ITEM_ACRO_BIKE`. Do not require exchange trips.
6. Let the player switch by selecting/registering the desired bike Key Item. Selecting the other bike while already riding directly changes bike type; selecting the active bike dismounts.
7. Preserve Wattson as mandatory. After the first successful victory, automatically give physical `ITEM_HM_ROCK_SMASH` if it has not already been obtained, then set its state flags. This retains compatibility when HM-free traversal is disabled while eliminating the pre-gym HM errand.
8. Preserve the normal Dynamo Badge state and cap transition from 24 to 29.

### Exact implementation files and labels/functions

Edit only these Route 110/Mauville files:

- `data/maps/Route110/scripts.inc`
  - `Route110_EventScript_RivalScene`
  - the May/Brendan starter-selection branches and shared `Route110_EventScript_RivalExit`
  - rival text/movements only as needed for compression;
  - leave the four Birch scene labels compiled but unreachable in normal Arc 2 flow through `VAR_REGISTER_BIRCH_STATE=2`.
- `data/maps/MauvilleCity/scripts.inc`
  - `MauvilleCity_EventScript_Wally`
  - `MauvilleCity_EventScript_WallysUncle`
  - `MauvilleCity_EventScript_BattleWallyPrompt`, `MauvilleCity_EventScript_BattleWally`, and `MauvilleCity_EventScript_DefeatedWally`
  - north/east exit support and Wally/Scott text or movement tables only as needed;
  - `MauvilleCity_EventScript_RegisterWallyCall` remains as legacy compatibility but must not be scheduled by the new completion path.
- `data/maps/MauvilleCity_BikeShop/scripts.inc`
  - `MauvilleCity_BikeShop_EventScript_Rydel`
  - the first-grant branches beginning at `MauvilleCity_BikeShop_EventScript_YesFar`
  - retire normal calls to `MauvilleCity_BikeShop_EventScript_AskSwitchBikes`, `..._SwitchBikes`, `..._SwitchAcroForMach`, and `..._SwitchMachForAcro` without changing unrelated handbook scripts.
- `src/bike.c`
  - `GetOnOffBike(u8 transitionFlags)` only.
- `data/maps/MauvilleCity_Gym/scripts.inc`
  - `MauvilleCity_Gym_EventScript_WattsonDefeated`
  - add a small retry-safe helper for `ITEM_HM_ROCK_SMASH` and related text;
  - preserve `MauvilleCity_Gym_EventScript_Wattson`, `..._GiveShockWave`, and `..._GiveShockWave2` behavior except for the deliberate Rock Smash call placement.

Inspect but do not edit unless separately approved:

- `data/maps/Route110/map.json` and Route 110 encounter data.
- `data/maps/Route110_TrickHouseEntrance`, all Trick House puzzle maps, and their scripts.
- `data/maps/Route110_SeasideCyclingRoadNorthEntrance`, `...SouthEntrance`, and their scripts.
- `data/maps/MauvilleCity/map.json`.
- `data/maps/MauvilleCity_BikeShop/map.json`.
- `data/maps/MauvilleCity_Gym/map.json`.
- `data/maps/MauvilleCity_House1/map.json` and `scripts.inc`.
- `data/maps/Route111/map.json` and `scripts.inc`.
- `data/maps/VerdanturfTown_WandasHouse/map.json` and `scripts.inc`.
- `src/field_move.c`, `src/caps.c`, item-use code, Match Call code, and trainer/encounter data.

### Rival battle and Route 110 state

- Preserve coordinate triggers `Route110_EventScript_RivalTrigger1`, `...Trigger2`, and `...Trigger3` at x `33–35`, y `56`.
- Preserve `Route110_EventScript_RivalScene` selection of all six trainers:
  - `TRAINER_MAY_ROUTE_110_TREECKO`, `...TORCHIC`, `...MUDKIP`;
  - `TRAINER_BRENDAN_ROUTE_110_TREECKO`, `...TORCHIC`, `...MUDKIP`.
- Preserve battle loss/retry behavior and starter/gender selection.
- Preserve successful `ITEM_ITEMFINDER` insertion before completing the scene. If the relevant pocket is full, retain a retry path rather than losing the reward or prematurely hiding the rival.
- Preserve `VAR_ROUTE110_STATE=1` only after the battle/reward completes, so rival objects and coordinate events stay resolved across reload.
- Compression may reduce dialogue boxes and movement waits but must retain a brief challenge and a brief post-win Itemfinder handoff.

### Wally state and object handling

- Preserve `TRAINER_WALLY_MAUVILLE` and its defeat flag/retry behavior.
- Wally or his uncle initiates a short mandatory challenge; do not present `MSGBOX_YESNO` and do not use `MauvilleCity_EventScript_DeclineWallyBattle` in the normal path.
- After victory, use a short acknowledgement, then support both player facings used by the existing event. Remove or fade Wally and his uncle without trapping the player.
- Set `FLAG_DEFEATED_WALLY_MAUVILLE`.
- Set `FLAG_HIDE_MAUVILLE_CITY_WALLY` and `FLAG_HIDE_MAUVILLE_CITY_WALLYS_UNCLE` so removal persists rather than relying only on `removeobject`.
- Clear `FLAG_HIDE_VERDANTURF_TOWN_WANDAS_HOUSE_WALLY` and `FLAG_HIDE_VERDANTURF_TOWN_WANDAS_HOUSE_WALLYS_UNCLE` so their downstream Wanda’s House state remains valid.
- Set `FLAG_ENABLE_WALLY_MATCH_CALL` immediately and silently.
- Clear `FLAG_ENABLE_FIRST_WALLY_POKENAV_CALL` and keep `VAR_WALLY_CALL_STEP_COUNTER=0`, preventing the delayed forced call.
- Advance `VAR_SCOTT_STATE` exactly once for the skipped Mauville Scott beat, but do not add/show `LOCALID_MAUVILLE_SCOTT` and do not clear `FLAG_HIDE_MAUVILLE_CITY_SCOTT`.
- `FLAG_DECLINED_WALLY_BATTLE_MAUVILLE` may remain allocated for old saves/scripts, but the new normal flow neither reads nor sets it.

These values preserve rival Match Call text progression and Wally’s later Verdanturf location without retaining the long interruption.

### Bike grant and switching mechanism

Rydel’s grant must be idempotent and bag-safe:

1. Preserve the initial greeting/“came from far away” choice and `FLAG_DECLINED_BIKE` behavior.
2. On acceptance, attempt to give both `ITEM_MACH_BIKE` and `ITEM_ACRO_BIKE`.
3. Set `FLAG_RECEIVED_BIKE` only after both items are confirmed in the bag.
4. If one insertion fails, retain the already received bike and make the next Rydel interaction retry only the missing bike. Never remove either bike.
5. Replace exchange dialogue with concise text explaining that both are available and the desired Key Item can be selected or registered.
6. Preserve Rydel, the assistant, and both bike handbooks. Calls to `SwapRegisteredBike` may remain defined for compatibility but must not be part of the new normal shop flow.

`src/bike.c::GetOnOffBike(u8 transitionFlags)` keeps its signature and call contract:

- From foot, `transitionFlags` mounts the selected bike normally.
- While riding the same selected type, selecting it dismounts and restores field music.
- While riding one bike, selecting the other bike directly calls the avatar transition for the requested type and keeps cycling music active. It must reset any bike state needed to avoid carrying Mach acceleration or Acro trick state across types.
- A call with `transitionFlags == 0` or another forced-off path must still dismount; do not accidentally remount or switch.
- Preserve Dowsing shutdown, saved music behavior, follower/avatar compatibility, Cycling Road checks, and item-use restrictions. In particular, an Acro selection must not become a way to enter or remain on Cycling Road where the current item-use rules forbid it.

### Rock Smash and Wattson terminal state

`src/field_move.c` already defines `FIELD_MOVE_ROCK_SMASH` as `BADGE_UNLOCK` keyed to `FLAG_BADGE03_GET` with `hmFree = TRUE`; do not modify it. Badge 3 is the appropriate automatic unlock point.

On the first successful `MauvilleCity_Gym_EventScript_WattsonDefeated` path:

- Preserve the badge fanfare and all current gym progression.
- If `FLAG_RECEIVED_HM_ROCK_SMASH` is unset, give `ITEM_HM_ROCK_SMASH` and only after successful insertion:
  - set `FLAG_RECEIVED_HM_ROCK_SMASH`;
  - set `FLAG_HIDE_ROUTE_111_ROCK_SMASH_TIP_GUY`.
- If the TM/HM pocket is full, Wattson must retain a post-victory retry path. Do not replay the battle or badge fanfare, do not duplicate Shock Wave, and do not set the Rock Smash receipt flag until the item exists in the bag.
- `MauvilleCity_House1_EventScript_RockSmashDude` remains unchanged as a legacy/pre-obtained fallback. After the automatic grant he uses `MauvilleCity_House1_EventScript_ReceivedRockSmash` and gives no duplicate.

Preserve these existing Wattson effects exactly:

- `FLAG_DEFEATED_MAUVILLE_GYM`.
- `FLAG_BADGE03_GET`.
- `FLAG_RECEIVED_TM_SHOCK_WAVE` and `ITEM_TM_SHOCK_WAVE`, including full-bag retry.
- `FLAG_ENABLE_WATTSON_MATCH_CALL`.
- `VAR_MAUVILLE_GYM_STATE`, `FLAG_MAUVILLE_GYM_BARRIERS_STATE`, gym trainer flags, puzzle deactivation, and `Common_EventScript_SetGymTrainers` with index 3.
- `VAR_PETALBURG_GYM_STATE` increments from 4 to 5 exactly once. Flannery remains responsible for advancing it from 5 to 6 in Arc 3.
- `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=3` and clearing `FLAG_HIDE_VERDANTURF_TOWN_SCOTT`.
- Existing `VAR_NEW_MAUVILLE_STATE`, Basement Key, New Mauville, Wattson rematch, and Thunderbolt paths.
- `FLAG_BADGE03_GET` changes `src/caps.c::GetProgressionLevelCap()` from 24 to 29 and enables Rock Smash through the existing field-move table.

### Route 110/Mauville warps and transitions to preserve

No map geometry, connection, warp, or coordinate-event edit is authorized.

- Preserve Route 110 connections to Slateport, Mauville, and Route 103.
- Preserve Trick House entrance `(11,66)`.
- Preserve Cycling Road gates at `(15,16)`, `(18,16)`, `(16,88)`, and `(19,88)`, plus challenge-end coordinates `(28,92)` and `(29,92)`.
- Preserve the New Mauville entrance at Route 110 `(35,24)`.
- Preserve Mauville connections north to Route 111, south to Route 110, west to Route 117, and east to Route 118.
- Preserve Mauville Gym entrance `(8,5)` and Bike Shop entrance `(35,5)`.
- Preserve all town buildings and their exits.

### Encounters, trainers, rewards, and optional content to preserve

- Preserve all Route 110 land/water/fishing encounter tables and its Nuzlocke location identity.
- Preserve lower-road and Cycling Road trainers `TRAINER_EDWARD`, `TRAINER_JACLYN`, `TRAINER_EDWIN_1`, `TRAINER_DALE`, `TRAINER_JACOB`, `TRAINER_ANTHONY`, `TRAINER_BENJAMIN_1`, `TRAINER_JASMINE`, `TRAINER_ABIGAIL_1`, `TRAINER_ISABEL_1`, `TRAINER_TIMMY`, `TRAINER_KALEB`, `TRAINER_JOSEPH`, and `TRAINER_ALYSSA`, including all existing rematches/Match Call registration.
- Preserve Route 110 visible items `ITEM_DIRE_HIT`, `ITEM_RARE_CANDY`, and `ITEM_ELIXIR`, and hidden `ITEM_REVIVE`, `ITEM_GREAT_BALL`, `ITEM_POKE_BALL`, and `ITEM_FULL_HEAL`.
- Preserve Trick House and Cycling Road as optional side content, including records, rewards, trainers, signs, music, and bike restrictions.
- Preserve the rival battle and `ITEM_ITEMFINDER` as required main-path content.
- Preserve `TRAINER_WALLY_MAUVILLE` as required story content.
- Preserve Gym trainers `TRAINER_KIRK`, `TRAINER_SHAWN`, `TRAINER_BEN`, `TRAINER_VIVIAN`, and `TRAINER_ANGELO`; the electric-barrier puzzle; `TRAINER_WATTSON_1`; Dynamo Badge; `ITEM_TM_SHOCK_WAVE`; Match Call; statues; guide; and rematches.
- Preserve New Mauville’s later Basement Key, generator state, and `ITEM_TM_THUNDERBOLT` reward unchanged.

## Arc 3 downstream contract

Arc 2 stops immediately after Wattson. It must hand Arc 3 this state without entering or rewriting Arc 3 content:

- `FLAG_BADGE03_GET` and `FLAG_DEFEATED_MAUVILLE_GYM` set.
- Progression cap 29.
- `VAR_PETALBURG_GYM_STATE=5`.
- `FLAG_RECEIVED_HM_ROCK_SMASH` set only when `ITEM_HM_ROCK_SMASH` was successfully received.
- `FLAG_HIDE_ROUTE_111_ROCK_SMASH_TIP_GUY` set with that receipt.
- Existing HM-free Rock Smash permission derives from Badge 3; Route 111 rock objects and `EventScript_RockSmash` remain unchanged.
- Wally removed persistently from Mauville, available in Wanda’s House, and registered for Match Call without a delayed call.
- Both bikes remain in the bag and usable for later Mach slopes and Acro rails.
- `VAR_SLATEPORT_CITY_STATE` and `VAR_SLATEPORT_HARBOR_STATE` remain 0 at this point; later villain scripts own their advancement.
- All Slateport Harbor Stern/Archie/Aqua/submarine flags remain untouched.
- No Route 111, Verdanturf, Route 112, Fiery Path, Fallarbor, Meteor Falls, Mt. Chimney, Jagged Pass, Lavaridge, or Flannery story sequence is started or modified.

## Dependency chain

1. Arc 1 Badge 2 and Route 109 landing provide access to Slateport with `ITEM_DEVON_PARTS`.
2. `SlateportCity_OnTransition` sees recovered, undelivered goods and hides only the exterior Aqua queue while setting Dock compatibility state.
3. Museum access becomes prompt; optional Slateport/Route 109 content remains available.
4. Stern’s Museum event retains both Aqua battles and concise Archie exposition.
5. Successful handoff consumes the Devon Goods, records delivery, clears Museum and Route 110 blockers, and silently advances skipped Birch/Scott registration state.
6. Route 110 remains an encounter route; the rival remains the required main-path battle and grants the Itemfinder.
7. Wally remains the required Mauville Gym gate; his victory state is resolved immediately and persistently.
8. Rydel grants both bikes; Key Item use supplies switching without repeat exchange trips.
9. Wattson remains mandatory; victory sets Badge 3, cap 29, Petalburg state 5, Shock Wave, Match Call, and retry-safe Rock Smash receipt.
10. Stop. Arc 3 begins later from the intact Route 111/Rusturf/Flannery dependency state.

## Implementation order and build checkpoints

The execution session must use this order:

1. Re-read the sources of truth and confirm the worktree. Record unrelated changes before editing.
2. Implement Slateport transition preparation, Dock compatibility dialogue, Museum exposition compression, and exact Museum terminal state.
3. Run `git diff --check`, inspect the three Slateport diffs, search every changed flag/variable globally, then run `make -j$(sysctl -n hw.ncpu)`.
4. Implement Route 110 rival compression and Wally’s mandatory concise resolution.
5. Run `git diff --check`, inspect those two script diffs, verify trainer/reward/state labels remain referenced, then build again.
6. Implement the idempotent two-bike grant and `GetOnOffBike` type switching.
7. Run static bike call-site checks and build again.
8. Implement Wattson’s retry-safe Rock Smash grant without changing his existing badge/TM/story effects.
9. Run static flag/state audits, `git diff --check`, and build again.
10. Review the total diff against this plan. Confirm no Arc 0–1 file, Arc 3 file, encounter table, trainer party, map JSON, warp, catch-rate, or unrelated system changed.
11. Run final release and debug checkpoints:
    - `make clean`
    - `make -j$(sysctl -n hw.ncpu)`
    - `make clean`
    - `make debug`
12. Record implementation, static verification, release build, debug build, and emulator acceptance as separate states. A non-vanilla ROM SHA1 is expected and is not a build failure.

## Static verification checklist

- `git status --short`, `git diff`, and `git diff --staged` show only authorized implementation files plus this approved plan.
- `git diff --check` passes.
- Global searches confirm every bypassed flag/variable receives its terminal value and no downstream use was orphaned.
- `FLAG_DOCK_REJECTED_DEVON_GOODS` remains because Slateport dialogue reads it.
- `FLAG_DELIVERED_DEVON_GOODS` is not set before both Museum battles and item handoff.
- `FLAG_HIDE_ROUTE_110_TEAM_AQUA` remains tied to Museum completion.
- `VAR_REGISTER_BIRCH_STATE` cannot remain 1 after Museum completion.
- `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE` cannot remain 1 after Museum completion.
- Every `VAR_SCOTT_STATE` increment added or retained is single-shot.
- Both Museum Grunt trainer constants and all six Route 110 rival trainer constants remain referenced.
- `VAR_ROUTE110_STATE=1` and Itemfinder grant remain in the successful rival path.
- Wally’s successful path sets persistent Mauville/Wanda visibility and immediate Match Call state; normal play cannot schedule the 250-step call.
- Bike grant cannot set `FLAG_RECEIVED_BIKE` with only one bike, cannot remove either bike, and can recover from a partial/full bag.
- `GetOnOffBike` preserves same-bike dismount, other-bike switching, forced-off behavior, music, and state reset.
- Rock Smash flags are set only after successful item insertion and Wattson’s post-victory interaction can retry.
- Wattson still sets `FLAG_BADGE03_GET`, `FLAG_DEFEATED_MAUVILLE_GYM`, `VAR_PETALBURG_GYM_STATE=5`, `VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE=3`, Shock Wave, and Match Call.
- `src/caps.c` and `src/field_move.c` remain unchanged and continue to derive cap 29/Rock Smash from Badge 3.
- Map JSON, warps, encounter data, trainer parties, Arc 0–1 files, Arc 3 scripts, and catch-rate code are unchanged.
- Release and debug builds complete without compiler, assembler, or linker errors.

## Continuous mGBA verification route

Use one continuous save beginning immediately after Brawly and ending immediately after Wattson. Do not start Arc 3.

1. Load immediately after Brawly. Verify Badge 2, Bulk Up receipt, cap 24, and `VAR_PETALBURG_GYM_STATE=4` through normal visible behavior/debug inspection.
2. Take Briney’s Arc 1 direct ferry to Route 109 `(22,24)`.
3. Before leaving Route 109, confirm fishing/surfing or a land encounter remains possible as applicable. Confirm beach trainers can be walked around and are optional.
4. Optionally battle one beach trainer and enter Seashore House; confirm these do not gate Slateport. Do not require clearing every optional trainer.
5. Enter Slateport. Confirm free movement, shops/buildings, and city exits. Confirm the exterior Aqua queue is already gone and the Museum can be entered without visiting the Shipyard.
6. Visit the Shipyard optionally. Dock should give concise Museum direction; leaving/re-entering or saving/reloading must not restore a progression requirement. Confirm later/post-delivery dialogue remains coherent.
7. Enter the Museum. Verify the fee, insufficient-funds exception where practical, exhibits, and optional familiar Grunt/Thief reward.
8. Speak to Stern from each supported facing in supplemental saves if needed. Confirm concise exposition and fight `TRAINER_GRUNT_MUSEUM_1` then `TRAINER_GRUNT_MUSEUM_2`. Test at least one loss/retry: delivery state must not advance on loss.
9. Win both battles. Confirm Archie and Stern remain meaningful, Devon Goods are removed exactly once, the party is healed, and the scene returns control without backtracking.
10. Exit and re-enter the Museum/Shipyard/Slateport. Confirm Aqua/Stern story objects stay resolved, no forced Scott scene occurs, and the Museum remains accessible.
11. Enter Route 110. Confirm no Aqua blocker and no Birch interruption. Trigger a wild encounter and verify ordinary trainers, items, Trick House entrance, Cycling Road entrance, and Route 103 access remain present/optional.
12. Cross the rival trigger. Confirm the correct May/Brendan starter-dependent battle, concise dialogue, loss/retry, Itemfinder reward, and persistent `VAR_ROUTE110_STATE=1` after save/reload.
13. Enter Mauville and verify normal city/building access. Wally and his uncle must still block gym progression meaningfully.
14. Visit Rydel. Accept the bikes and verify both Mach and Acro Bikes are in Key Items without leaving the shop or exchanging one. Register/select Mach, mount, select Acro while riding, verify direct type change, select Acro again to dismount, then reverse the sequence. Confirm save/reload persistence and Cycling Road restrictions.
15. Trigger Wally from both supported approach directions across saves. Verify no decline loop, defeat `TRAINER_WALLY_MAUVILLE`, receive the concise resolution, and confirm Wally/uncle disappear persistently with no forced Scott scene.
16. Walk at least 250 steps and change maps. Confirm no forced Wally registration call occurs and Wally is already present in Match Call. Confirm Wanda’s House placement in a supplemental check without advancing Arc 3 story.
17. Enter Mauville Gym. Verify all five optional Gym trainers, switch barriers, guide/statues, Wattson access, and a Wattson loss/retry.
18. Defeat Wattson. Verify Dynamo Badge, Shock Wave, Wattson Match Call, Gym completion, cap 29, and automatic physical Rock Smash receipt. Visit Rock Smash Dude and confirm he gives no duplicate. Save/reload and confirm all state persists.
19. Stop immediately after the Wattson acceptance checks. Do not proceed north into Arc 3.

### Required edge-case emulator checks

- Museum: lose to either Grunt, reload between steps, use all supported Stern facings, and verify doors/objects after re-entry.
- Museum Thief: fill the TM pocket, verify the familiar Grunt remains for retry, then accept the reward.
- Scott: optional Battle Tent interaction may still occur only through optional content; skipped mandatory scenes must not corrupt later Scott state.
- Rival: test both player genders and all three starter branches across targeted saves.
- Wally: test north/east approaches, loss/retry, save/reload, and 250+ post-victory steps.
- Bikes: test a full Key Items pocket and a partial state containing exactly one bike. Rydel must retry only the missing item and never take one away.
- Bike switching: test field music, followers, map transitions, Mach acceleration, Acro tricks, same-bike dismount, cross-bike switch, and Cycling Road denial/exit.
- Wattson: test a full TM/HM pocket. Badge/story state must remain coherent, Shock Wave and Rock Smash must each remain independently claimable without battle/fanfare replay or duplication.
- Rock Smash: an isolated post-Badge-3 Route 111 rock check may verify the unlock and physical HM compatibility, but it is a test-only probe, not authorization to begin or alter Arc 3.

## Risks and uncertainties

- Script item pockets may make it difficult to force a partial two-bike grant naturally. Use debugger inventory setup for that edge case; the script still must be idempotent by explicit `checkitem` checks.
- `GetOnOffBike` serves several item-use and forced-transition callers. The implementation must distinguish a requested different bike from a forced dismount and reset avatar bike state without breaking music/followers.
- Wattson currently commits badge state before giving Shock Wave. Adding another retryable item requires a post-victory branch that cannot replay badge state or strand one reward behind the other.
- Removing the delayed Wally call changes timing, not availability. Immediate `FLAG_ENABLE_WALLY_MATCH_CALL` plus clearing the delayed-call flag is the approved terminal state.
- Skipping Scott/Birch scenes must advance both their flags and their associated variables. Setting only one half risks later forced scenes, wrong dialogue, or Battle Frontier reward-count drift.
- Museum 2F story actors use hidden object flags plus runtime `addobject`/`removeobject`; do not “simplify” them into map JSON visibility changes.
- `VAR_SLATEPORT_CITY_STATE` and `VAR_SLATEPORT_HARBOR_STATE` look related to Stern but belong to later villain progression. Any Arc 2 change to them is a regression.
- No map-script change is accepted by code inspection alone. Continuous mGBA play and the edge cases above are mandatory.

## Acceptance criteria

Arc 2 is accepted only when all of the following are true:

- Arcs 0 and 1 pass regression checks from the established post-Brawly save and no Arc 0–1 file changed.
- Slateport is fully accessible; Route 109 encounters/trainers and beach content remain optional.
- The Shipyard is not required, Museum access is prompt, both Aqua Museum battles remain, Stern and Archie retain meaningful state/exposition, and Devon Goods resolve automatically after the battles without backtracking.
- All listed Museum, Route 110, Birch, Scott, Devon, and later villain flags/variables end in their compatible values and remain stable after save/reload.
- Route 110 encounters, trainers, rival battle, Itemfinder, Trick House, Cycling Road, items, and transitions remain intact.
- Wally’s meaningful Mauville battle and Wattson’s Gym battle remain mandatory, retryable, and persistent.
- Both bikes are granted without a repeat exchange trip; selecting Key Items correctly mounts, dismounts, and switches types.
- Wattson grants the correct badge/TM/Match Call/story state, raises the cap from 24 to 29, and provides retry-safe automatic Rock Smash progression.
- Release and debug builds pass, all static checks pass, and the continuous mGBA route plus required edge cases are accepted by the user.
- No Arc 3 content has begun and no catch-rate mechanic has changed.

