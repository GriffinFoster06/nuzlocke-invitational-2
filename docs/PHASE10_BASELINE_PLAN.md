# Phase 10 baseline remediation plan

## Purpose and authority

This is the complete, human-approved execution plan for the repository-wide
baseline remediation that must be completed before Phase 10 continues. A fresh
Codex execution session must read, follow, and remain within this file. It must
also read `AGENTS.md`, `AI_HANDOFF.md`, `docs/SPEC.md`, `docs/PHASES.md`,
`docs/PHASE9_REMEDIATION_PROMPTS.md`, and `docs/PHASE10_PROMPTS.md` before
editing. `docs/SPEC.md` is authoritative for gameplay design.

This remediation preserves all Phase 1–9.5 systems and all existing Phase 10
Arc 0 work. It does not rebuild Arc 0 from scratch. It does not plan or begin
Arc 1. It does not authorize a commit or push; implementation, build, static
review, and user mGBA playtesting must pass before completion is committed.

The only new gameplay requirement in this remediation is that a valid fishing
cast must ultimately start a fishing encounter. Catch-rate mechanics are
explicitly out of scope.

## Repository and current baseline state

At plan approval on 2026-09-11:

- The repository is `/Users/griffinfoster/Projects/pokeemerald-expansion` on
  branch `main`.
- `git status`, the unstaged diff, and the staged diff are clean.
- `HEAD` is `a86f2fa95c` (`Transfer to codex and more phase 10`) and matches
  `origin/main`.
- Relevant history is:
  - `de53064ca8` — completed Phase 9.5 checkpoint;
  - `af88cb09ad` — initial Phase 10 Arc 0 implementation for Littleroot and the
    first rival;
  - `3c9de94ab9` — accepted follow-up fixes;
  - `a86f2fa95c` — committed Petalburg, Petalburg Woods, Rustboro, workflow,
    and baseline documentation work that had previously been uncommitted.
- All known Arc 0 implementation is now committed. The stale description in
  `AI_HANDOFF.md` that Petalburg, Petalburg Woods, and Rustboro remain
  uncommitted must be corrected during execution.
- The last recorded normal build completed on 2026-09-09. Direct dependency
  scans performed during planning on the current tree also completed without
  the Rustboro diagnostic. The normal linker still emits an unrelated warning
  that `pokeemerald.elf` has a LOAD segment with RWX permissions; that linker
  warning is outside this remediation.
- `pokeemerald.ss1` and `pokeemerald.ss2` are mGBA save states in the repository
  root. `pokeemerald.ss2` is the stronger candidate for the reported
  post-capture forced-down state. Neither state matches the current ROM, so
  current symbols must not be applied to them without validating the matching
  historical ROM first.

Before editing, the execution session must capture `git status --short`,
`git diff`, `git diff --staged`, and the relevant log. It must preserve any
new user work if the tree no longer matches this snapshot.

## Scope and expected file set

The execution session is authorized to change only the following tracked files
for this remediation:

- `AGENTS.md`
- `data/maps/RustboroCity_Gym/scripts.inc`, only if the already-corrected
  comment has regressed
- `docs/PHASE10_BASELINE_PLAN.md`, only if implementation evidence requires an
  approved amendment; the approved substance must otherwise remain unchanged
- `docs/PHASE10_PROMPTS.md`
- `docs/SPEC.md`
- `include/config/fishing.h`
- `src/fishing.c`
- `docs/POST_CAPTURE_DIAGNOSTICS.md` as a new file
- `tools/debug/inspect_capture_state.py` as a new file

`AI_HANDOFF.md` is ignored local shared state and must also be updated during
execution as findings, decisions, verification results, and next steps change.

No change is planned for `docs/PHASES.md`: its current source-only build and
grouped-arc Phase 10 guidance are already suitable for current Codex work. No
change is planned for `docs/PHASE9_REMEDIATION_PROMPTS.md`: its current
historical notice prevents it from being mistaken for active Phase 10
instructions. `CLAUDE.md` must not be modified.

If implementation reveals that another tracked file must change, stop and
obtain human approval for an amended plan before editing that file.

## Workstream 1 — Rustboro dependency-scanner diagnostic

### Exact file and section

- `data/maps/RustboroCity_Gym/scripts.inc`, the explanatory comment immediately
  before the Roxanne/Norman badge-counter logic near the start of the file.
- Verification also covers `tools/scaninc/asm_file.cpp`, which defines how the
  dependency scanner parses assembly comments and strings; no change to the
  scanner is planned.

### Current and historical behavior

The build previously emitted:

```text
data/maps/RustboroCity_Gym/scripts.inc:294 unexpected EOF in string
```

The diagnostic was not caused by unterminated dialogue. The historical comment
contained this punctuation:

```text
Norman's badge counter; the Gym reserves value 1 for "Wally tutorial in progress"
```

`tools/scaninc` recognizes `;` as an assembly end-of-line comment marker in
this parsing context. It discarded the remainder of that physical line,
including the opening double quote, and later interpreted the closing double
quote on the continuation line as the start of a string that reached EOF.
Executable script commands and dialogue were valid.

The current committed file already uses a comma after `counter` instead of the
semicolon. Direct scans of the individual script and aggregate event scripts
currently succeed. This remediation must preserve that exact comment-only
correction and prove that it remains sufficient.

### Intended behavior and implementation approach

The comment must read `Norman's badge counter, ...` so `scaninc` does not split
the comment before the quoted phrase. Do not alter any command, label, constant,
branch, flag, variable, text resource, or dialogue. If the comma is already
present, make no source edit. If the semicolon has reappeared, replace only that
semicolon with a comma.

### Dependency risks

- Editing script tokens or labels could change Roxanne progression.
- Editing dialogue to silence the scanner would create an unnecessary gameplay
  change.
- Disabling or bypassing dependency scanning would conceal the parser problem.

### Static, build, and emulator verification

Run:

```sh
tools/scaninc/scaninc -I include -I "" data/maps/RustboroCity_Gym/scripts.inc
tools/scaninc/scaninc -I include -I "" data/event_scripts.s
```

Both commands must exit zero without `unexpected EOF in string`. Confirm the
aggregate scan traverses the Rustboro Gym include. If the script changed,
prove the comma is the only source change in this file. The normal build must
also complete without the diagnostic. The punctuation correction has no
independent gameplay effect; Roxanne and post-Gym progression are covered by
the Arc 0 emulator matrix.

## Workstream 2 — Phase 10 Codex workflow

### Exact file, current behavior, and intended change

Edit `docs/PHASE10_PROMPTS.md`, especially its authority, arc membership,
workflow for Arcs 1–8, Arc 0 state record, and completion gates. It already
defines nine grouped arcs and most gates. Add explicit fresh planning/review
and execution sessions with the approved role names, and update the Arc 0
baseline from the former committed/uncommitted split to the actual committed
state at `a86f2fa95c`.

Retain this exact membership:

- **Arc 0 — Littleroot through Roxanne:** Littleroot opening; First rival
  battle; Petalburg/Wally tutorial; Petalburg Woods; Rustboro/Roxanne.
- **Arc 1 — Roxanne through Brawly:** Route 116/Rusturf Tunnel; Briney ferry;
  Dewford.
- **Arc 2 — Brawly through Wattson:** Slateport; Route 110/Mauville.
- **Arc 3 — Wattson through Flannery:** Route 117/Verdanturf; Routes
  111/112/Fiery Path; Fallarbor/Route 114; Meteor Falls; Mt. Chimney; Jagged
  Pass/Lavaridge.
- **Arc 4 — Flannery through Norman:** Petalburg/Norman.
- **Arc 5 — Norman through Winona:** Routes 118/119; Weather Institute; Route
  119 rival; Fortree/Route 120/Devon Scope; Winona.
- **Arc 6 — Winona through Tate & Liza / Mossdeep:** Route 121/Lilycove; Mt.
  Pyre; Magma Hideout; Aqua Hideout; Routes 124/Mossdeep; Mossdeep Space
  Center.
- **Arc 7 — Tate & Liza through Juan:** Seafloor Cavern; Kyogre awakening;
  Sootopolis crisis; Sky Pillar; Return from Sky Pillar; Post-Rayquaza premium
  encounters; Juan/Gym 8.
- **Arc 8 — Juan through Champion:** Ever Grande; Victory Road; Pokémon League.

For every Arc 1–8, require:

1. **Planning:** Start Codex with the **Phase 10 planning/review profile**. The
   planning model inspects actual code first, creates a separate sub-plan for
   each area, and inspects scripts, flags/state variables, warps, rewards, and
   dependencies. Human approval is required. Save the exact approved plan as
   `docs/PHASE10_ARC<N>_PLAN.md`.
2. **Execution:** Start a fresh Codex session with the **Phase 10 execution
   profile**. It reads the approved plan, implements only the approved scope,
   builds, and inspects the resulting diff.
3. **Review:** Start a fresh Codex session with the **Phase 10 planning/review
   profile** and perform a read-only review against both `docs/SPEC.md` and the
   approved plan.
4. **Playtest:** The user continuously playtests the entire arc in mGBA.
   Isolated flag-driven tests may supplement but cannot replace that traversal.
5. **Completion:** Commit and push only after implementation, build, static
   review, and emulator playtesting all pass.

The profile names are role names; profile configuration is outside this
remediation. Large arcs may be divided when actual dependencies justify it.
Scope changes require renewed approval and an updated exact plan file. Arc 0
remains an existing-state record; do not make a retroactive implementation
plan or redo it. Arc 1 is prohibited until current acceptance gates pass.

### Risks and verification

Ambiguous roles can mix planning, implementation, and review; stale status can
cause existing Arc 0 work to be overwritten; missing membership can move
dependencies between arcs. Review rendered Markdown and verify every arc/area,
both exact role names, fresh-session wording, human approval, plan naming,
SPEC-and-plan review, continuous playtest, and commit/push gate. Run
`git diff --check`. This workflow text has no runtime emulator requirement.

## Workstream 3 — Minimal Codex-relevant documentation corrections

### `AGENTS.md`

**Exact section:** `Existing project infrastructure` and the Phase 10/cross-
agent workflow paragraphs.

**Current behavior:** It accurately describes most architecture, but its
randomizer bullet says `src/randomizer.c` implements ability results. Ability
generation actually lives in `src/ability_gen.c` and is consumed through
`GetSpeciesAbility` in `src/pokemon.c`; generated learnsets live in
`src/learnset_gen.c`.

**Intended behavior and approach:** Keep accurate descriptions of the settings
UI (`src/ruleset.c`, `src/data/ruleset.h`, `include/constants/ruleset.h`,
`src/ruleset_menu.c`, `src/ruleset_field.c`, `src/start_menu.c`), Nuzlocke
(`src/nuzlocke.c`, `include/nuzlocke.h`), HM-free traversal
(`src/field_move.c`, `include/field_move.h`), caps, and fair AI. State that
`src/randomizer.c`/`include/randomizer.h` own deterministic wild, trainer,
starter, gift, static, TM, Tutor, and item mappings; `src/ability_gen.c` owns
generated abilities; `src/learnset_gen.c` owns generated learnsets. These use
upstream generation helpers rather than recreating the unpublished stale
Randolocke randomizer.

Retain that existing Phase 1–9.5 systems must be inspected rather than treated
as absent; `docs/SPEC.md` is authoritative for gameplay;
`docs/PHASE10_PROMPTS.md` is authoritative for Phase 10 workflow;
`AI_HANDOFF.md` stores unfinished shared state; and the approved plan-file
handoff goes from a planning/review session to a fresh execution session. Add
no Claude-specific instructions.

**Risks and verification:** Wrong ownership guidance sends agents to the wrong
subsystem. Validate every cited path/symbol, read the rendered file, and run
`git diff --check`. This is documentation-only and has no emulator test.

### `docs/SPEC.md`

**Exact sections:** Evolution Assistance/Evolve command, hard level caps, and
a concise fishing rule in the appropriate encounter or quality-of-life area.

**Current behavior:** Evolution and cap prose already matches implementation:
Phase 9.5 found no specific-move-known evolution requirements, removed
Evolution Assistance, retained Evolve, clamps acquisition levels whenever caps
are enabled, permits later over-cap growth in soft/warning modes, and applies
configured over-cap battle ineligibility until progression catches up with the
existing no-cap-legal-battler exception. Do not rewrite those statements or
change gameplay for them. Guaranteed fishing is not yet documented.

**Intended behavior and approach:** Preserve evolution and cap text. Add the
complete fishing rule: on a fishable tile, when the current map/time of day has
a non-null fishing table, Old, Good, and Super Rod casts always bite and start
an encounter after an untimed A prompt. Invalid tiles and missing tables remain
invalid. Preserve rod tables, relative slot weights, level generation,
randomizer/Nuzlocke behavior, catch probability, and presentation except for
random no-bite and input-timing failures.

**Risks and verification:** Do not change gameplay to satisfy stale prose.
Compare the language to Workstream 5, inspect the Markdown, and run
`git diff --check`.

### Files verified without edits

Do not edit `docs/PHASES.md`; it already states source-only building and points
Phase 10 to grouped arcs. Do not edit `docs/PHASE9_REMEDIATION_PROMPTS.md`; it
is clearly historical. If either regresses, stop for approval. Do not modify
`CLAUDE.md`.

### `AI_HANDOFF.md`

Replace stale objective/state, file inventory, verification, findings, and next
step. It currently mislabels committed Arc 0 work as uncommitted and omits the
fishing/capture workstreams. Record current commits, exact objective, changed
files, commands/results, fishing decisions, capture-state evidence, unresolved
risks, emulator tests, and next step. Keep implementation, build, static
review, emulator verification, and still-needs-playtest status separate. Record
the deferred Claude cleanup. Verify it against the final tree and evidence.

## Workstream 4 — Actual Arc 0 state

Arc 0 is implemented and committed. Correct its record and test it; do not
redesign or redo it.

### Littleroot opening

- **Files/functions/scripts:** `src/new_game.c` (`WarpToNewGameStart`,
  `ApplyLittlerootOpeningState` and call sites); `src/overworld.c`
  (`CB2_NewGame`); `data/maps/LittlerootTown/scripts.inc` and
  `data/scripts/players_house.inc` (opening/clock paths);
  `data/maps/Route101/scripts.inc` (Birch rescue/starter progression).
- **Implemented behavior:** moving truck skipped; equivalent story, object, and
  gender state initialized; immediate running; optional clock preference
  retained; forced rival-house busywork removed; Birch rescue shortened;
  starter choice, nickname, and rescue battle retained.
- **Risks:** new-game gender/clock state, house objects, rescue flags, starter
  ownership, map re-entry, save/load.

### First rival battle

- **Files/functions/scripts:** `data/maps/Route103/scripts.inc`
  (`Route103_EventScript_RivalEnd` and victory/loss paths);
  `data/maps/LittlerootTown_ProfessorBirchsLab/scripts.inc` (compressed lab and
  rewards); `src/nuzlocke.c`, `include/nuzlocke.h`, `src/pokemon.c` (first-Ball
  activation/ownership); `data/maps/OldaleTown/map.json`,
  `data/maps/OldaleTown/scripts.inc`, `data/maps/OldaleTown_Mart/scripts.inc`
  (accepted optional Ball source).
- **Implemented behavior:** Route 103 battle retained; victory returns directly
  to the lab; Pokédex, Balls, and Running progression granted promptly;
  terminal rival/Oldale state prevents replay; loss remains replayable;
  Nuzlocke activates on actual Ball acquisition.
- **Risks:** outcome, warp/facing, flags, lab objects, reward duplication,
  bag-full behavior, save/reload.

### Petalburg/Wally tutorial

- **Files/scripts:** `data/maps/PetalburgCity/map.json` and
  `data/maps/PetalburgCity/scripts.inc`
  (`PetalburgCity_EventScript_SkipWallyIntro`, removed escort/tutorial
  triggers); `data/maps/PetalburgCity_Gym/scripts.inc` (Norman gate and
  `VAR_PETALBURG_GYM_STATE`).
- **Implemented behavior:** Norman's story encounter/gate retained; escort and
  Wally catching tutorial removed; required variables, flags, and visibility
  advance to the completed-tutorial state.
- **Risks:** gym/city state, Wally/rival/mother visibility, Norman dialogue and
  badge checks, both entrances, re-entry.

### Petalburg Woods

- **File/scripts:** `data/maps/PetalburgWoods/scripts.inc`, including both
  approach triggers, Devon researcher/Aqua grunt branches, loss/retry, Great
  Ball reward, and object movement/visibility.
- **Implemented behavior:** encounter area and Aqua battle retained; Devon
  filler shortened; both approach paths, loss/retry, victory, reward, and
  required movement remain.
- **Risks:** direction/triggers, object IDs, movement completion, loss restore,
  reward capacity, re-entry.

### Rustboro/Roxanne

- **Files/scripts:** `data/maps/RustboroCity_Gym/scripts.inc` (Roxanne battle,
  badge/TM, cap, Norman counter, scanner-safe comment);
  `data/maps/RustboroCity/map.json` and
  `data/maps/RustboroCity/scripts.inc`
  (`RustboroCity_EventScript_StolenGoodsScene`, Devon employee, Route
  116/Rusturf flags/variables).
- **Implemented behavior:** Roxanne and normal rewards retained; post-Gym theft
  progression compressed toward Route 116/Rusturf; downstream state and
  visibility flags advance.
- **Known uncertainty:** `RustboroCity_EventScript_StolenGoodsScene` removes the
  employee, changes permanent coordinates, and clears its hide flag, but the
  reviewed path did not visibly call `addobject` before ending. Test employee
  visibility immediately and after re-entry. A fix requires proof and an
  approved amendment; this plan does not authorize an Arc 0 script fix.
- **Risks:** badge/TM, cap, Norman counter, employee visibility/coordinates,
  theft flags, Route 116 grunt/Peeko state, Rusturf objects, re-entry.

### Arc 0 status to record independently

- **IMPLEMENTED:** yes, in `af88cb09ad`, `3c9de94ab9`, and `a86f2fa95c`.
- **BUILD VERIFIED:** passed historically on 2026-09-09; reverify after this
  remediation.
- **STATIC REVIEW VERIFIED:** baseline review completed; recheck final diff and
  retain the employee uncertainty.
- **EMULATOR VERIFIED:** no current evidence establishes full acceptance.
- **STILL NEEDS PLAYTEST:** every Arc 0 case in the emulator matrix below.

Build or static inspection cannot upgrade emulator status.

## Workstream 5 — Valid fishing casts always start an encounter

### Complete requirements

When a rod is used on a valid fishing tile and the current map/time of day has
a valid fishing encounter table, the attempt must always bite and, after the
player responds to an untimed A-button prompt, start a fishing encounter.
Early/mistimed A presses and waiting too long cannot lose the encounter.

This applies to Old, Good, and Super Rod. Preserve:

1. invalid fishing locations;
2. the requirement for a non-null fishing table;
3. rod-specific encounter tables;
4. relative species/slot distributions;
5. encounter-level generation;
6. project randomizer behavior;
7. Nuzlocke location and Dupes behavior;
8. battle-start/end ownership of Nuzlocke consumption—casting alone consumes
   nothing;
9. catch probability;
10. fresh normal slot selection rather than a fixed species;
11. rod, dots, bite, hook, transition, put-away, and invalid presentation except
    where removing timing failure requires a state change.

No-bite/got-away strings may remain for invalid or unrelated paths, but cannot
result from random odds, early A, or timeout on a valid cast.

### Actual implementation findings

- `src/item_use.c`: `ItemUseOutOfBattle_Rod`, rod field callback, tile
  validation, `StartFishing`. It validates the facing tile but does not select
  a table or consume a Nuzlocke location.
- `src/fishing.c`: `StartFishing`, `Task_Fishing`, `sFishingStateFuncs`, and
  states from `Fishing_Init` through `Fishing_EndNoMon`. `Fishing_Init` locks
  controls/sets `preventStep`; `Fishing_StartEncounter` restores avatar state,
  unlocks, calls `FishingWildEncounter`, records TV data, destroys the task.
- `Fishing_CheckForBite` first calls `DoesCurrentMapHaveFishingMons`. Current
  `GEN_LATEST` bite odds then use `Fishing_RollForBite` and
  `CalculateFishingBiteOdds`: 25% Old, 50% Good, 75% Super before boosts.
- Current `I_FISHING_MINIGAME == GEN_3`: `Fishing_ShowDots` allows early A to
  cancel; `Fishing_WaitForA` times out after 36/33/30 frames; Good/Super can
  require more timed rounds via `Fishing_CheckMoreDots`.
- Gen 1/2 selection makes `DoesFishingMinigameAllowCancel` false and uses
  `Fishing_APressNoMinigame`, which waits indefinitely for A before
  `Fishing_MonOnHook`.
- `src/wild_encounter.c`: `DoesCurrentMapHaveFishingMons`,
  `ChooseWildMonIndex_Fishing`, `NuzlockeRerollDupeSlot`,
  `GenerateFishingWildMon`, `ChooseWildMonLevel`, `FishingWildEncounter`,
  Feebas branch, `BattleSetup_StartWildBattle`. Slot weights remain Old 70/30
  (0–1), Good 60/20/20 (2–4), Super 40/40/15/4/1 (5–9); Lure may mirror slots.
- Generation selects a fresh slot, applies Nuzlocke Dupes reroll, resolves
  `Randomizer_WildSlotSpecies`, chooses that slot's level, updates the chain,
  and creates the Pokémon.
- `src/randomizer.c`/`include/randomizer.h` own
  `Randomizer_WildSlotSpecies`; do not bypass/reseed it.
- `src/battle_setup.c::BattleSetup_StartWildBattle` calls
  `Nuzlocke_NoteWildEncounterStart(FALSE)` at battle start. Casting does not.
- `src/nuzlocke.c`/`include/nuzlocke.h` preserve start/end, Dupes, and obtained
  behavior. `src/pokemon.c::GiveCapturedMonToPlayer` calls
  `Nuzlocke_OnMonObtained` then assigns party/PC.

### Exact implementation

1. Add `I_FISHING_ALWAYS_BITE TRUE` to `include/config/fishing.h`, documented
   to apply only after `DoesCurrentMapHaveFishingMons` succeeds.
2. In `src/fishing.c::Fishing_CheckForBite`, keep the missing-table check first.
   With the switch true, take the existing bite path without bite-odds or
   Sticky Hold/Suction Cups RNG. With it false, retain upstream odds. Keep the
   bite animation/downstream path.
3. Set `I_FISHING_MINIGAME` to `GEN_1` in `include/config/fishing.h` to use the
   existing no-cancel, untimed A path. This keeps casting, initial dots, bite
   animation/text, hook text, and battle transition while removing early
   cancellation, timeout, and repeated timed rounds.
4. Do not edit slot, generation, battle setup, randomizer, Nuzlocke, Ball, or
   catch-formula code.

### Risks and verification

Table validation must precede forced bite to prevent null access. Slot RNG must
remain. Battle start must remain after the prompt. The disabled configuration
must retain upstream odds. Feebas and environment behavior must remain. Task
cleanup must not lock controls.

Statically prove valid casts have no random/early/timeout failure and invalid
casts still fail. Confirm slot, level, Feebas, randomizer, Nuzlocke, battle,
capture, and catch files are unchanged. Run `git diff --check`, build, and the
full fishing emulator matrix.

## Workstream 6 — Intermittent forced-down movement after capture

### Full bug description and path

After some successful captures, overworld return can make the player walk Down
continuously as if held. Directional input does not recover control; menus and
saving may be unavailable; restart is the only known recovery. It is
intermittent and not assumed fishing-related.

Trace: wild encounter → battle → capture → Pokédex/nickname → party/storage →
battle cleanup → field callback → task/script restoration → avatar/input
restoration → control. Compare grass/fishing/static; free/full party;
nickname yes/no/mandatory; Nuzlocke off/on; randomizer off/on. Never implement
a speculative global input reset.

### Exact files/functions

- `data/battle_scripts_2.s`: `BattleScript_SuccessBallThrow`, Pokédex,
  `trygivecaughtmonnick`, `givecaughtmon`, outcome.
- `src/battle_script_commands.c`: dex commands, `Cmd_trygivecaughtmonnick`,
  `Cmd_givecaughtmon`, full-party selection, screen callbacks/messages.
- `src/naming_screen.c`: `DoNamingScreen`, task cleanup, caught nickname,
  `BattleMainCB2`/alternate returns.
- `src/pokemon.c`: `GiveCapturedMonToPlayer`, `CopyMonToPC`, party/storage,
  `Nuzlocke_OnMonObtained`.
- `src/pokemon_storage_system.c`, `src/party_menu.c`: full-party swap/storage.
- `src/nuzlocke.c`: obtained/end/death hooks, nickname, whiteout callbacks.
- `src/randomizer.c`: verify no input/field mutation and randomized branches.
- `src/battle_main.c`: end/evolution/cleanup, `TryEvolvePokemon`,
  `ReturnFromBattleToOverworld`.
- `src/battle_util2.c`: `FreeBattleResources` and lifetimes.
- `src/battle_setup.c`: battle start, saved callbacks, `CB2_EndWildBattle`,
  `CB2_EndScriptedWildBattle`, field-return selection.
- `src/overworld.c`: `CB2_ReturnToField`, `CB2_ReturnToFieldLocal`,
  `ReturnToFieldLocal`, `InitObjectEventsReturnToField`, map scripts,
  `CB2_Overworld`, field callbacks, task/sprite/effect reconstruction,
  `ScriptContext_Init`, unlocks.
- `src/field_control_avatar.c`, `src/field_player_avatar.c`: keys, transitions,
  forced/held movement, actions, `preventStep`, locks.
- `src/main.c`, `include/main.h`: `ReadKeys`, `gMain` raw/filtered/repeated keys,
  masks, optimized-loop persistent registers.
- `src/task.c`, `src/script.c`, `src/event_object_movement.c`, field effects:
  tasks, contexts, synthetic input, held movement/effects.
- `src/fishing.c`: task/control restoration as a correlation branch.

### Current evidence — B. ROOT CAUSE NOT PROVEN

- `ss1` (2026-09-07 16:45) shows a battle move-summary screen and embeds ROM
  CRC `0x5d0b39f8`, matching `test-0907-1634.gba`.
- `ss2` (2026-09-08 16:02) shows overworld grass and embeds CRC `0xfd4c0fe9`,
  matching `checkpoint-after-phase9.gba`; it is the stronger stuck candidate.
- Current ROM CRC was `0x2a1b70e9`; never apply current symbols without ROM
  validation.
- Decompressed `ss2` hardware `KEYINPUT=0x03fd`: B is pressed, Down is not.
- Saved PC `0x08179ee6` is in matching optimized `ReadKeys`. `r4=0x030066e0`
  is validated `gMain`, but `r6=0x03006698` is not `REG_KEYINPUT`, `r7=1` is
  not `KEYS_MASK=0x3ff`, and `r8=0x02001004` points into battle-global storage.
  `r9=0x080dbadd` falls inside matching `FreeBattleResources` code.
- `ss2` has `heldKeysRaw=0x72bd`, `newKeysRaw=0`, `heldKeys=0x72bd`, `newKeys=0`,
  `newAndRepeatedKeys=0`. Wrong source/mask computation yields `0x72bd`, which
  contains `DPAD_DOWN`.

This proves the immediate mechanism: corrupted persistent registers in the
optimized input loop repeatedly write a false held-key value containing Down.
It does not prove which capture transition corrupts registers or stack.

### Narrowed candidates

1. ABI/saved-register/stack/return corruption across capture cleanup and
   `FreeBattleResources`, especially damage to `r6`–`r10`.
2. Capture-only Pokédex, nickname, full-party swap, storage, screen restore.
3. Lifetime/bounds/use-after-free/double-free in battle resources, tasks,
   naming, copies, Nuzlocke hooks, randomized species.
4. Script, forced movement, locks, avatar, fishing, or resumed map state only if
   input-loop registers are intact in a new occurrence.

### Instrumentation plan

Create read-only standard-library `tools/debug/inspect_capture_state.py` to
parse PNG `gbAs`, validate version/decompressed length/ROM CRC, and print hash,
CRC, CPU registers/CPSR, hardware `KEYINPUT`, and offsets. Print `gMain` keys
only with a validated matching address. Support both states; fail on mismatch;
never modify states.

Create `docs/POST_CAPTURE_DIAGNOSTICS.md` with symptoms/evidence, tool usage,
ROM matching, mGBA/GDB reproduction, break/watchpoints derived from the actual
build, the capture matrix, and prohibition on permanent fixes before the first
corrupting transition/write is observed.

Use `/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb` if available; it lacks
Python, so parse externally. Make a clean optimized `DINFO=1` build so register
allocation remains representative. Use matching historical ROM/disassembly
for `ss2`; reproduce on current symbols if no matching ELF exists.

Capture at battle start, each capture branch, before/after resource cleanup,
callback handoff, field init, first overworld frame, and first bad movement:
registers `r0`–`r12`, `sp/lr/pc/cpsr`; stack/saved-register slots; `KEYINPUT`
and all `gMain` key fields; callbacks/inBattle/state; battle function/outcome/
flags/script/command states; resource pointers/frees; tasks; script contexts;
field callbacks; avatar flags/transition/preventStep/object/facing/movement;
coordinates/effects; encounter/map/fishing; randomizer seed/settings; Nuzlocke
state; party/storage/nickname/dex/species/evolution branches.

Preserve/hash states; open only with matching ROM; verify `ss2`; make current
instrumented fresh save; test matrix branches; save pre-capture and stuck
states; locate earliest corrupt register/stack/input write using watchpoints. If
proven, amend this plan with exact transition/files/functions/minimal fix/risks
and obtain approval before implementing.

### Risks and verification

A key clear masks corruption; unoptimized debug may hide it; mismatched ROM
creates false evidence; broad logging changes timing. Static acceptance requires
a reviewable inspector, documented validation/offsets, reproducible output for
both states, unchanged hashes, and usable diagnostics. Build normal and clean
optimized `DINFO=1`. Emulator acceptance validates `ss2` and runs the matrix;
root cause may honestly remain unproven without inventing a fix.

## Implementation order

1. Re-read governing files; capture status/diffs/log/state hashes/scanner;
   update `AI_HANDOFF.md`.
2. Verify/preserve Rustboro comma and run both scans.
3. Correct only `AGENTS.md`, `docs/PHASE10_PROMPTS.md`, `docs/SPEC.md`; update
   Arc 0 and fishing documentation. Do not touch deferred/no-change files.
4. Implement fishing only in `include/config/fishing.h`, `src/fishing.c`;
   inspect diff and build.
5. Add inspector and diagnostic document; validate states; diagnostic build.
   Do not add a permanent capture fix.
6. Read-only review against SPEC and this plan; ensure no catch-rate/Arc 1 work.
7. Run full mGBA matrix; record each status separately.
8. Update `AI_HANDOFF.md`; leave changes uncommitted for human inspection.

## Build checkpoints and static verification

### A — baseline/scanner

Run both direct scans, `git diff --check`, and
`make -j$(sysctl -n hw.ncpu)`. Require zero exit/no Rustboro diagnostic. Record
but do not remediate RWX warning.

### B — fishing/documentation

Run `git diff --check`; inspect staged/unstaged diffs; build normally; inspect
ROM timestamp/exit; trace valid/invalid casts; confirm by file diff that slot,
level, randomizer, Nuzlocke, battle, and catch-rate code did not change.

### C — diagnostics

Run inspector on both states with matching CRC expectations; verify
deterministic output/unchanged hashes; `make clean`, then an optimized `DINFO=1`
build using supported make options; verify breakpoint symbols; rebuild normally
after diagnostic validation; inspect final diff and `git diff --check`.

The project builds from source; do not use vanilla SHA1 mismatch as a failure.

## Complete emulator test matrix

Record ROM commit/hash, mGBA version, save source, settings, route, expected,
actual, pass/fail for each case.

### Arc 0 continuous traversal

| Area | Cases | Acceptance |
|---|---|---|
| New game/Littleroot | Both genders; clock preference on/off; save/reload and building re-entry | Intended starting state, immediate running, clock path works, skipped scenes do not replay, correct objects/warps. |
| Birch/starter | Starters where practical; nickname completed/cancelled/required; victory | Granted once, nickname rules, rescue progression, no duplicate/skipped state. |
| First rival | Victory; loss/retry; Route 103/lab re-entry | Battle retained, loss recoverable, direct lab return, rewards once, no replay. |
| First-Ball gate | Lab Balls and optional Ball source; before/after | Nuzlocke latches only on actual acquisition. |
| Petalburg | Both approaches; first visit/re-entry; Norman early | No escort/tutorial, state/visibility advances, Norman gate remains. |
| Woods | Both approaches; lose/retry/win; reward normal/full bag if applicable; re-entry | Encounters/battle/reward remain, shortened scene, correct movement, no repeat/softlock. |
| Roxanne/Rustboro | Lose/retry/win; badge/TM/cap/counter; theft; employee immediate/re-entry; Route 116/Rusturf approach | Rewards/state once, employee correct, intended handoff without beginning/skipping Arc 1. |

The user must continuously play opening through Arc 0 endpoint; isolated tests
only supplement it.

### Guaranteed fishing

| Case | Coverage | Acceptance |
|---|---|---|
| Old Rod | At least 30 valid casts across two tables if available; early A during dots; delayed prompt A | Every cast bites, waits indefinitely at prompt, starts after A; no random/early/timeout loss. |
| Good Rod | Same | Same, with Good Rod slots. |
| Super Rod | Same | Same, with Super Rod slots. |
| Invalid tile | Land, wall/object, other non-fishable facing | Existing rejection; no battle. |
| Missing table | Fishable behavior on map/time with null table if available | Existing no-mon path; no null access/battle. |
| Species/slot | Repeated multi-species tables plus static weights | Variety where table permits; no fixed slot. Short-run exact percentages not required. |
| Levels | Repeated variable-level slots | Within configured ranges and variable where permitted. |
| Randomizer | Each rod off/on with recorded seed | Vanilla and deterministic mapping work; normal slots/seed stability. |
| Nuzlocke fresh location | Prompt, then flee/KO/catch under rules | Cast/prompt consume nothing; battle end applies designed rule. |
| Dupes/Shiny | Owned-family slots and clauses | Existing randomizer-aware rerolls/clauses unchanged. |
| Feebas | Valid coordinate if practical | Special species/level/battle path intact. |
| Return | Flee/KO/capture; Surf if available | Graphics/facing/surf/control/tasks/menu/save/movement normal. |

### Forced-down capture diagnostics

| Dimension | Cases |
|---|---|
| Source | Grass; each fishing rod; static/scripted |
| Party/storage | Free slot; full party direct PC; swap accept; decline/cancel; box rollover/full if practical |
| Nickname | Yes/completed; No; B decline; mandatory; cancellation behavior |
| Pokédex | New entry; owned species |
| Rules | Nuzlocke off/on; Dupes/Shiny outcomes |
| Randomizer | Off/on with seed |
| Aftermath | No level/evolution; catch EXP level; evolution if reachable |
| Controls | Flee/KO controls; after capture stand/directions/Start/save/interact/map transition/60 seconds |

For every capture, note continuous Down, physical Down, working inputs, exact
branch. On reproduction save/hash pre-capture and stuck states and run
diagnostics.

## Acceptance criteria

1. Only authorized files differ; unrelated work is preserved.
2. Rustboro scans/build pass without EOF; only scanner-safe comment punctuation
   is involved; executable script/dialogue unchanged.
3. Current Codex architecture/workflow/Arc 0/fishing docs match this plan;
   `docs/PHASES.md`, `docs/PHASE9_REMEDIATION_PROMPTS.md`, `CLAUDE.md` unchanged.
4. Arc 0 remains implemented rather than rewritten; statuses stay separate;
   emulator cases pass before Arc 0 is complete.
5. Every valid rod cast survives odds/early A/timeout, retains animation and
   untimed A, and preserves tables/weights/levels/randomizer/Nuzlocke/Feebas/
   battle/capture. Fishing matrix passes.
6. Catch-rate formulas and Ball behavior are unchanged.
7. Capture diagnosis ends honestly as either **ROOT CAUSE IDENTIFIED** only
   after an approved amendment names exact transition/files/functions/fix/
   risks, or **ROOT CAUSE NOT PROVEN** with candidates, inspector, exact values,
   procedure, and evidence. No speculative fix.
8. Builds and `git diff --check` pass; static review passes; `AI_HANDOFF.md` is
   accurate.
9. Arc 1 has not been planned, edited, or started.
10. No commit/push until user review and required mGBA acceptance.

## Gameplay impact and expected repository state

The only new runtime behavior is guaranteed fishing on valid tiles/tables with
animations and untimed A retained. Tables, weights, levels, randomizer,
Nuzlocke, and catch chance remain. Rustboro is comment-only. Diagnostics/docs
do not affect gameplay. Existing Arc 0 is preserved/tested.

After execution, Phase 1–9.5 remain intact; Arc 0 has explicit independent
statuses; scans/build are clean of the EOF; Codex grouped-arc workflow is
complete; valid casts always reach encounters; the forced-down issue is either
proven in an approved amendment or remains unresolved with usable diagnostics;
catch rates are untouched; Arc 1 remains unstarted; remediation changes remain
uncommitted for review.

## Explicitly out of scope

- Catch probability, Ball modifiers, shake calculations, critical capture, or
  any catch-rate mechanics.
- Arc 1 planning/implementation, including Route 116/Rusturf Tunnel, Briney
  ferry, or Dewford gameplay. Existing handoff flags may only be tested.
- Speculative input resets or unproven capture fixes.
- Unrelated linker RWX warning.
- Gameplay changes solely for stale prose.
- Commit/push before acceptance.

## Deferred cleanup

The following will be handled later and is not part of this remediation:

- `CLAUDE.md` modernization;
- Claude-specific workflow documentation or Claude Code configuration;
- historical Claude model names, shortcuts, prompts, and instruction cleanup.

Record this in `AI_HANDOFF.md`. OpenAI Codex is the exclusive environment for
the current multi-day Phase 10 effort; Claude compatibility work is not needed
before Phase 10 resumes.
