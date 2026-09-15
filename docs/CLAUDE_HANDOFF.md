# Claude session handoff

Concise current-state snapshot. This is deliberately volatile and short —
detailed phase scope, acceptance criteria, and copy/paste prompts live in
[`../PROMPTS.md`](../PROMPTS.md). Historical Phase 0-10 record lives in
[PHASES.md](PHASES.md). Product-behavior authority is
[SPEC.md](SPEC.md). Read all three before starting work; this file is not a
substitute for any of them.

## Repository state

- Branch: `main`.
- HEAD (at last check): `e181197953` ("11c done"). Everything after it —
  the targeted AI-switching remediation, Phase 11D, Phase 11E, and the
  Phase 11F gate fixes — is implemented and build-verified in the working
  tree but **not yet committed**.
- Working tree: not clean (see `git status`); see "Exact next phase" below.
  Preserve any unrelated user work (e.g. the untracked `/export` transcript
  file in the repo root).

A fresh session must re-run `git status`, `git log --oneline -10`, and
`git diff` (staged and unstaged) rather than trusting this snapshot — it is
a point-in-time record, not a live query.

## Completed phases

Phases 1-10, 11A, 11B, and 11A.6 (== "Phase 11C" in
[`../PROMPTS.md`](../PROMPTS.md)'s taxonomy — Generation Architecture, Seed
Quality, Pre-Run Settings, and Performance) are complete and committed
(`e181197953`, "11c done"). The AI-switching remediation, Phase 11D (world,
Premium statics, core QoL), Phase 11E (Terminal Run Reports) and the
Phase 11F correctness gate are **implemented and build-verified** on top of
that, but not yet committed and not yet mGBA-acceptance-tested; see
"Phase 11F: correctness gate" and "Unresolved runtime-verification items"
below for exactly what remains. See [PHASES.md](PHASES.md) for the Phase 0-10
historical record and [`../PROMPTS.md`](../PROMPTS.md)'s "Phase taxonomy"
table for full current status.

## Exact next phase

Phase 11 is code-complete (Phase 11F closed every known required defect).
The immediate next step for any session picking this up is: (1) run the
mGBA acceptance checklist in "Phase 11F: correctness gate" below (it
includes the 7 Phase 11E items), (2) commit, then (3) Phase 12 per
[`../PROMPTS.md`](../PROMPTS.md).

Phase 11A.6/11C is complete and was committed as `e181197953` ("11c done").

### Phase 11F: correctness gate (fixes implemented, awaiting mGBA acceptance)

The broad cross-system audit found these blockers; all are fixed and
build-verified (`make clean && make`, `git diff --check`, affected test TUs
compile-checked — `make check` still cannot run on this host):

- **Guaranteed Master Ball** — the 11D grant sat in a Maxie/Archie talk script
  the linearized story never reaches; it now runs from
  `SootopolisCity_EventScript_ResolveCrisis` right after `CompleteCrisis`
  (`SootopolisCity_EventScript_GiveGuaranteedMasterBall`, one-shot flag).
- **New Game wizard** — `NewGameInitData()`'s `ClearSav3()` erased the
  wizard's preset/seed/Gen mask; the ruleset is now carried across it
  (`src/new_game.c`).
- **Generation lock** — `runStarted` is now set at the end of
  `NewGameInitData()` (it used to wait for the starter), so RULES never shows
  generation settings as editable in-run.
- **Premium statics/roamers** — uniform draw over the eligible Premium pool
  (no power ladder) with the original species always eligible, so the Regis
  (below the curated 600-power floor) can stay Regis (`PickPremiumStatic`,
  `src/randomizer.c`); folded into the uncommitted `RANDOMIZER_VERSION` 4.
  `test/run_rng.c` golden values updated to v4; two new `test/randomizer.c`
  cases.
- **Move Reminder** — Disabled now really disables level-up relearning
  (Fallarbor NPC included); Normal = Fallarbor Heart Scale NPC only;
  Free/Previously-learned add the free summary relearner
  (`src/move_relearner.c`).
- **Type icons** — party-menu icons now slide/swap with their mon; the battle
  HUD recreates icons instead of re-animating across the two icon sheets.
- **Wipe reports** — wild-battle wipes/deaths read
  `gBattleResults.lastOpponentSpecies` (the enemy party is already zeroed).
- **Standard AI** — every project-retuned chance in `include/config/ai.h` is
  `AI_TUNED(project, upstream)`; Standard gets upstream values.
- **Level to Cap for PC Pokémon** — new PC menu entry "LV TO CAP" for box
  mons (Withdraw / Move Pokémon modes), `Task_LevelToCap` in
  `src/pokemon_storage_system.c`: level jump, then every skipped move in
  order, forget-a-move via the summary screen; never evolves.

**mGBA acceptance still required before commit:**
1. New Game wizard: non-default preset, manual seed `0x12345678`, Gen 1+9
   only — RULES shows exactly that, generation settings marked (L) before the
   starter; starter/Route 101 species are Gen 1/9; a whiteout retry reuses
   the settings.
2. Sootopolis after Rayquaza: "found a MASTER BALL" + bag +1 before the warp;
   no second grant on re-entry.
3. Regi chambers: each result is Premium or the original species.
4. Move Reminder per mode: Disabled (Fallarbor refuses, keeps the Heart
   Scale, no summary relearner), Normal (Fallarbor works, no summary
   relearner), Free (summary relearner).
5. Type icons: party swap in every layout; battle switch Water ↔ Normal in
   singles and doubles; PC icons/palettes; summary and starter screens.
6. PC Level to Cap: offered only for eligible box mons (not eggs, graveyard
   mons, at-target mons, or with the setting off); level + panel update;
   moves offered in order; YES → summary → replace → back to the PC and
   continues; cancel/NO skip; never evolves; Run Report counter increments.
7. Wild-battle wipe: exported `wipe.opponentKind` = wild with the right
   species.
8. AI: Standard trainers behave like upstream; Pro Fair shows no A→B→A
   switch oscillation; no gimmick buttons.
9. The 7 Phase 11E items below, plus the bike in-place switch.

### Phase 11E: Terminal Run Reports (implemented, awaiting mGBA acceptance)

Every strict run finalizes exactly one Run Report at Victory (Champion / Hall
of Fame) or Wipe (no usable Pokémon remain, independent of
`SETTING_WHITEOUT_BEHAVIOR`), idempotently. Architecture:

- **New flash-only save area**: `include/run_report.h` / `src/run_report.c`
  own a two-slot `struct RunReportBank` written via the existing
  `TryWriteSpecialSaveSector`/`TryReadSpecialSaveSector` helpers into
  `SECTOR_ID_RUN_REPORT` (aliased to `SECTOR_ID_TRAINER_HILL`, `include/save.h`
  — an e-Reader-only sector this hack can never otherwise reach). 0 EWRAM
  cost; survives reset and a subsequent New Game (only the title-screen
  "erase all" and `SAVE_OVERWRITE_DIFFERENT_FILE`'s Hall-of-Fame erase, which
  now explicitly skips this sector, touch sector 30).
- **New EWRAM**: `struct RunReportState` (live per-attempt counters + a
  bounded 24-entry death history) appended to `SaveBlock3`
  (`include/global.h`), +320 B, cleared for free by the existing
  `ClearSav3()` on New Game.
- **Terminal hooks**: `RunReport_Finalize()` called from
  `src/post_battle_event_funcs.c` (`GameClear`/`EnterHallOfFame`, Victory) and
  `src/nuzlocke.c` `Nuzlocke_ProcessPostBattleDeaths()` /
  `src/field_poison.c` (Wipe) — both wipe paths now snapshot the complete
  party via `RunReport_WipeConditionMet()`/`RunReport_Finalize()` *before*
  the shared `Nuzlocke_FinalizeDeadMons()` cleanup (extracted from the old
  battle-only cleanup so the field-poison path no longer moves mons to the
  graveyard one at a time mid-sequence — approved plan decision).
- **Statistics/death instrumentation**: small counter bumps added at existing
  choke points (`src/nuzlocke.c`, `src/wild_encounter.c`,
  `src/battle_script_commands.c`, `src/battle_setup.c`, `src/party_menu.c`);
  no new hot-path work, no per-frame counters.
- **Exporters**: `tools/export_run.py` (manual, required, read-only, versioned
  JSON with names expanded from the repo's own headers) and
  `tools/watch_run_export.py` (optional `.sav` poller, host-side JSON ledger
  by report ID — no ROM-side mutation, core gameplay never depends on it).
- **Tests**: `test/run_report.c` covers struct sizes, live-counter
  gating/saturation, `RunReport_WipeConditionMet()`, death-history bound +
  truncation, and the field-poison death path. It deliberately does **not**
  call `RunReport_Finalize()` — no test in this tree exercises real flash
  writes, so the finalize-to-flash round trip is mGBA-only (see acceptance
  items below). `test/save.c`'s `T_SAVEBLOCK3_SIZE` updated 568 → 896.

Static/build verification done: `make clean && make -j$(sysctl -n hw.ncpu)`
succeeds, `git diff --check` clean, the compile-checked test TU builds
cleanly, and the exporter leaves both a real and a synthetic `.sav`
byte-identical (SHA256-verified) across multiple runs including error paths.

**mGBA acceptance still required before commit** (cannot be established
statically):
1. Victory: reach the Champion/Hall of Fame; export and check the HOF team
   matches the winning party field-for-field.
2. Wipe via battle: lose with no living Pokémon anywhere; exported party must
   be the pre-cleanup six, not empty/compacted.
3. Wipe via field poison: walk a poisoned party to death; same completeness,
   and dead mons still land in the HEAVEN box (only the timing changed).
4. Persistence: soft-reset and fully close/reopen mGBA after each outcome;
   re-export and confirm identical JSON.
5. Retention: finalize a second attempt; confirm both reports are in the bank
   and `--all` exports both.
6. Replay/seed independence: same manual seed, two attempts — `report_id`s
   differ, and generation (starters/wild/trainer) is unaffected by a prior
   finalized report sitting in flash.
7. Idempotence: save repeatedly on the run-over/Hall-of-Fame screen; confirm
   only one record per attempt.

### Phase 11A.6/11C summary (for a session that hasn't read the diff)

- **Performance:** `PickReplacementCoreExcluding` (randomizer.c) and
  `GenerateWeighted` (learnset_gen.c) now use single-pass reservoir sampling
  instead of two-pass count-then-pick; `LearnsetGen_EnsureBuilt()` is
  hoisted out of `GenerateWeighted`'s hot loop; a small EWRAM cache
  (`src/randomizer.c`, `WILD_SLOT_CACHE_*`) memoizes
  `Randomizer_WildSlotSpecies` per encounter table, invalidated at every
  existing `Randomizer_InvalidateTms()` call site plus the species-ban
  setters; `ability_gen.c`'s `sFamilyRoot` table is now shared with
  `nuzlocke.c` (via `AbilityGen_FamilyRoot`) instead of a second `O(N²)`
  fixpoint closure in `Nuzlocke_MarkFamilyOwned`. `RANDOMIZER_VERSION` 2→3
  absorbs the resulting RNG-draw-order change.
- **Seed quality:** `GenerateAutomaticRunSeed()` (ruleset.c) mixes
  `Random32()`, `gMain.vblankCounter1`, raw RTC fields, and a new
  persistent `SaveBlock2.newRunCounter` through `Crc32B`, replacing a bare
  `Random32()` call. Run seed stays 32-bit by design (`RunRng_Seed` always
  funnels through a `u32`-returning CRC regardless of stored seed width) —
  see SPEC.md "Run seed" for the full justification. Golden-value test:
  `test/run_rng.c`.
- **Generation filtering:** new `SETTING_GEN_1_ENABLED`..`SETTING_GEN_9_ENABLED`
  (default ON), classified per-form (not family root) by
  `GetSpeciesGeneration()` (new `src/species_generation.c`, ROM-only, 0
  EWRAM). Gates `power_score.c`'s eligibility cache and
  `pokemon.c GetSpeciesEvolutions()`'s output, so evolution can never bypass
  a disabled generation. `RULESET_VERSION` 4→5. See SPEC.md "Generation
  filtering".
- **New Game settings wizard:** a dedicated, New-Game-only screen
  (`RulesetMenu_EnterNewGameWizard`, `src/ruleset_menu.c`), wired in from
  `src/main_menu.c`'s `Task_NewGameBirchSpeech_Cleanup` only — the Nuzlocke
  whiteout-retry path is untouched. Shows only generation-locked settings
  plus the preset row; START opens a confirmation summary that blocks
  starting with zero generations enabled. See SPEC.md "New Game settings
  wizard".

## Last known memory baseline

From the Phase 11A.6/11C build (see `pokeemerald.map`/build output at the
time of this update):

- EWRAM: 241088 B / 256 KB (91.97%) — up from a 240200 B (91.63%) baseline
  immediately before this phase's changes (+888 B net: the wild-slot cache
  and `newRunCounter` cost more than the family-root-sharing and
  eligibility-bit reuse saved). The New Game wizard itself costs 0 permanent
  EWRAM (its state struct is heap-allocated on open, freed on close).
- IWRAM: 28412 B / 32 KB (86.71%) — unchanged.
- ROM: 26748560 B / 32 MB (79.72%) — up from 26745584 B (79.71%) baseline
  (+2976 B, negligible against the 32 MB budget).

After landing Phase 11E's changes on top of this working tree (which also
carries other uncommitted, unrelated in-flight work per `git status` — this
delta is not Phase 11E in isolation):

- EWRAM: 241408 B / 256 KB (92.09%), +320 B — exactly `sizeof(struct
  RunReportState)`, matching the plan's budget.
- IWRAM: 28424 B / 32 KB (86.74%), +12 B — negligible.
- ROM: 26754320 B / 32 MB (79.73%), +5760 B — negligible against the 32 MB
  budget.

After the Phase 11F gate fixes (clean build): EWRAM 241520 B (92.13%,
+112 B — the PC Level to Cap working state), IWRAM 28424 B (86.74%), ROM
26758224 B (79.75%).

A later session should re-measure rather than trust these numbers once
further changes land.

## Known test-runner limitation

`make check` does not build on this machine: its helper tools
(`tools/patchelf`, `tools/mgba-rom-test-hydra`) fail because the recipe
passes GNU `ld`-specific flags (`-Map`, `--print-memory-usage`,
`--gc-sections`) through to this host's `clang`. This is a host
toolchain/Makefile-environment issue, not a source defect.

**Workaround:** compile-check individual test translation units directly
instead of running the suite, e.g.:

```
make -j build/emerald-test/test/<name>.o build/emerald-test/src/<name>.o TEST=1
```

This exercises the `#if TESTING` compile path without the runner. The
normal `make -j$(sysctl -n hw.ncpu)` build works fine and is the real gate
for whether a change compiles. Actual test execution happens in CI, not in
this local environment.

## Stale document warning: `AI_HANDOFF.md`

`AI_HANDOFF.md` at the repo root is **untracked and gitignored**
(`.gitignore` line ~69) — it exists only in this particular local working
copy, not in git history and not on any other checkout. It records a
Phase 10 Arc 0 baseline-remediation snapshot from `2026-09-09` and is
historical scratch state, not current status. Do not read it as evidence of
present implementation state, and do not treat its "Exact Next Step"
section as live instructions — Phase 10 (including Arc 0's emulator
acceptance) is complete per [PHASES.md](PHASES.md) and
[`../PROMPTS.md`](../PROMPTS.md). It has been left untouched; see
`../PROMPTS.md`'s Phase 13A scope for when to reassess whether it should be
deleted.

## Unresolved runtime-verification items

These are implemented and build-verified but have not had dedicated mGBA
acceptance recorded since landing:

- Phase 11B's permadeath-revival closures (in-battle bag-item HP restore,
  the in-battle bag menu's item-eligibility check, and Battle Pike's
  between-room heal) — logic and audit are complete, but no dedicated
  emulator playtest session has been recorded against them specifically.
- **Phase 11A.6/11C, all mGBA-only (not establishable by static inspection):**
  - New Game wizard end-to-end: the Recommended-defaults fast path reaches
    gameplay with minimal interaction, and the manual-config path (change
    preset, edit/reroll seed, toggle generations, confirm) works and its
    confirmation summary matches what was actually configured.
  - The wizard's "at least one generation enabled" guard actually blocks
    Start Game when every `SETTING_GEN_N_ENABLED` is off, and un-blocks the
    moment one is re-enabled.
  - A restrictive mask (e.g. only Gen 1 + Gen 9 enabled) produces a
    playable, non-stalling run across wild encounters, trainers, and
    evolution — not just a code-level fallback argument.
  - Soft-resetting mid-run does not change anything already generated
    (Deterministic World Principle) — spot-check a wild slot and a trainer
    party before and after a reset.
  - Perceptible reduction in wild/trainer battle-start latency versus the
    pre-Phase-11A.6 baseline (the original motivating complaint) — the fix
    is reasoned and unit-tested for correctness, but the actual felt latency
    improvement has not been measured on hardware/emulator.
  - The Nuzlocke whiteout-retry path is confirmed unaffected: it still
    reaches `CB2_NewGame` directly (never the wizard) and silently reuses
    its stashed settings.
- Terminal Run Reports (Phase 11E) are now implemented and build-verified —
  see the dedicated section above for the 7 mGBA acceptance items still
  needed before commit.
- Phase 11D (Premium statics incl. Mew/Deoxys/Ho-Oh/Lugia, post-Rayquaza
  progression, both bikes, type icons, Move Reminder, Quick Travel) and the
  Phase 11F fixes are implemented and build-verified — see "Phase 11F:
  correctness gate" above for the combined mGBA checklist.

## Pointers

- [SPEC.md](SPEC.md) — authoritative product-behavior specification.
- [PHASES.md](PHASES.md) — Phase 0-10 historical record and current
  high-level roadmap.
- [`../PROMPTS.md`](../PROMPTS.md) — authoritative runbook for all
  remaining phases (11C through 13B), with full scope, acceptance criteria,
  and copy/paste prompts per phase.
