# Claude session handoff

Concise current-state snapshot. This is deliberately volatile and short —
detailed phase scope, acceptance criteria, and copy/paste prompts live in
[`../PROMPTS.md`](../PROMPTS.md). Historical Phase 0-10 record lives in
[PHASES.md](PHASES.md). Product-behavior authority is
[SPEC.md](SPEC.md). The full player-facing feature set is
[PLAYER_GUIDE.md](PLAYER_GUIDE.md). Read all four before starting work; this
file is not a substitute for any of them.

## Repository state

- Branch: `main`.
- HEAD (at last check): `8939c6e651` ("Phase 12 done") — Phases 1-12B are
  **committed**.
- On top of that commit, this working tree carries **Phase 13A (final
  documentation & repository cleanup)** changes, implemented and
  build-verified but **not yet committed** — see "Phase 13A" below for
  exactly what changed and why. Phase 13B (final release gate) has also run
  as a static audit in this same session; see that section for its outcome
  and the outstanding mGBA checklist.
- Working tree: not clean (see `git status`) until this session's changes are
  reviewed and committed.

A fresh session must re-run `git status`, `git log --oneline -10`, and
`git diff` (staged and unstaged) rather than trusting this snapshot — it is
a point-in-time record, not a live query.

## Completed phases

Phases 1-12B are complete and committed (`8939c6e651`, "Phase 12 done"). See
[PHASES.md](PHASES.md) for the Phase 0-10 historical record and
[`../PROMPTS.md`](../PROMPTS.md)'s "Phase taxonomy" table for full current
status.

## Exact next phase

Phase 13A is implemented and build-verified in the working tree (see below)
awaiting user review of the documentation diff and the fixes it required,
then commit. Phase 13B's static audit has been performed in this same
session (see below); its terminal verdict is withheld pending the user's
full mGBA playthrough, per the standing commit-gate rule. There is no
Phase 14 — once 13B's runtime checklist passes and is confirmed, the
product is complete.

### Phase 11F: correctness gate (historical - fixes verified before the Phase 11 commit)

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

**mGBA acceptance checklist (verified before the Phase 11 commit):**
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

### Phase 11E: Terminal Run Reports (historical - verified before the Phase 11 commit)

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

**mGBA acceptance checklist (verified before the Phase 11 commit; not
establishable by static inspection alone):**
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

`AI_HANDOFF.md` at the repo root is **untracked and gitignored** — it exists
only in this particular local working copy, not in git history and not on
any other checkout. It records a Phase 10 Arc 0 baseline-remediation
snapshot from `2026-09-09` and is historical scratch state, not current
status. As of Phase 13A it now carries its own SUPERSEDED/HISTORICAL banner
at the top of the file saying exactly this, so a future agent that opens it
directly (rather than this handoff) still gets the warning. Per user
decision during Phase 13A it was marked rather than deleted, since it may
still have archival value; safe to delete if that's ever reassessed.

## Unresolved runtime-verification items (historical, pre-Phase-11-commit)

The Phase 11A/11B/11A.6/11D/11E/11F mGBA acceptance checklists above were the
gate for the `7930527ace` commit per `PROMPTS.md`'s standing commit-gate rule
(runtime playtest → user review → commit). They are retained here as the
historical record of what that gate covered, not as a live pending list. A
regression investigation into any of that surface should still start from
these checklists.

## Phase 12A + 12B (historical - committed in `8939c6e651` "Phase 12 done")

Findings, decisions, and changes from this session's Phase 12A (Fun /
Accessibility / Recommended Defaults / Settings Simplification / UX / Game
Feel) and Phase 12B (Final Technical & Presentation Polish) per
`PROMPTS.md`. Full review reasoning lives in the approved plan this session
worked from; this is the durable summary.

**Conclusion:** the Recommended defaults already matched `docs/SPEC.md` on
every named review item (generation mask, power matching, catch rate,
trainer levels/caps, Level to Cap, Move Reminder, nicknames, Pro Fair AI,
QoL toggles). Nothing in randomizer balance or intentional variance was
touched. The real defects were in *presentation*: a mislabelled control, dead
settings still shown, the level cap never displayed anywhere, and terminal
flows that didn't tell the player what had happened.

**Phase 12A changes:**
- New Game wizard's per-row controls hint now correctly says `START begin`
  in wizard mode instead of the ordinary RULES menu's `START preset` (the two
  screens bind START differently; the hint used to be wrong for the wizard).
- Eight settings with zero gameplay consumers are now `SETTING_FLAG_HIDDEN`:
  `SETTING_BATTLE_SPEED`, `SETTING_OLDALE_MONEY_NPC`, `SETTING_SHOP_RANDOMIZATION`
  (genuinely unimplemented), and `SETTING_SHOW_IVS/EVS/NATURE_EFFECT/CAP_LEGALITY/DEAD_MARKER`
  (the features are real but were already unconditional - the summary skills
  page already cycles Stats/IVs/EVs, and the DEAD/`↑CAP` markers and nature
  boost/drop indicator already always display). `SETTING_CAT_DISPLAY` was
  removed from `enum SettingCategory` since every row it held is now hidden;
  the seven affected settings were repointed to `SETTING_CAT_PRESET_SEED`
  (already-visible, so hiding rows there can't empty the page). No
  `RULESET_VERSION` bump - hiding a setting doesn't renumber stored indices.
- New read-only **Run Information** overlay (`RfMenu_ShowRunInfo` /
  `RfMenu_DrawRunInfo`, `src/ruleset_field.c`), reached from a new "RUN INFO"
  row in the RULES field menu (split out of the old combined
  "RUN INFO / SETTINGS" row, which is now "SETTINGS"). Shows preset, seed,
  enabled generations, badges, **the active level cap** (previously had no UI
  call site anywhere - `docs/SPEC.md` "Hard level caps" requires it visible),
  deaths, locations caught, encounters, bosses defeated, and whether this
  attempt's Run Report has finalized to the save. New accessor
  `RunReport_LiveStats()` (`src/run_report.c`) exposes the existing live
  counters read-only; `RulesetMenu_BuildEnabledGenString` promoted from
  static so both screens share it.
- The Run-Over (Wipe) screen (`data/scripts/nuzlocke.inc`) now tells the
  player a Run Report was saved to the save file and names
  `tools/export_run.py` for exporting it - it used to say nothing. The
  YES/NO confirm's NO branch used to loop the same prompt silently forever;
  it now explains that saying no lets you save-and-reset to export first,
  then re-asks.
- Quick Travel unlock (`data/maps/RusturfTunnel/scripts.inc`, right after
  `FLAG_RECOVERED_DEVON_GOODS` is set) now shows a one-time message when the
  setting is on, via a new `Ruleset_CheckQuickTravelJustUnlocked` special -
  previously nothing told the player it had turned on.
- The Ball-shortcut hint (hold R + D-Pad to cycle, tap R to throw - the
  shipped Gen 7+ "last used Ball" binding) is now mentioned once, appended to
  the Oldale 999-Ball NPC's dialogue at the moment Balls arrive.
- `sLbl_LrnComp[0]` ("7 / 7 / 7") renamed to "Fixed quotas" - `docs/SPEC.md`
  "Learnset composition" is explicit that the *default* is weighted random,
  not literal 7/7/7; the old label read like the default was quota-based.
- `docs/SPEC.md` updated in three places to match already-correct code
  rather than the other way around (all discussed with and approved by the
  user before editing): "New Game settings wizard" now says it shows every
  generation-locked setting (not just preset/seed/mask - the code was always
  right, since generation-locked settings can only ever be set there); "Ball
  shortcut" now describes the shipped hold-R+D-Pad/tap-R binding instead of
  an L-cycle/R-throw split that was never implemented; "IV / EV / Nature
  display" now says these are unconditional rather than player-toggleable.
- Pro Fair AI stays the Recommended default - its move prediction predicts
  only from already-revealed move history (`AI_UsesFairKnowledge()` path,
  `src/battle_ai_main.c`), never the player's current input, so "maximum
  prediction" here is materially gentler than upstream's omniscience-backed
  version. No change made.

**Phase 12B changes:**
- `src/learnset_gen.c`'s `Generate()`: removed the `comp == LRNCOMP_FULLY_RANDOM`
  branches that were dead code (that mode already returns via
  `GenerateWeighted` earlier in the function, so `Generate()`'s own body only
  ever runs for `LRNCOMP_777`).
- Corrected a comment in `src/battle_ai_main.c` that claimed
  `AI_FLAG_PREDICTION` and `AI_FLAG_ASSUMPTIONS` "are included" in the
  project's AI tiers - only `AI_FLAG_PREDICT_MOVE` is; the rest are never
  granted and are stripped from a trainer's authored flags too for every
  non-Standard difficulty. Annotated (not changed) `PREDICT_SWITCH_CHANCE`
  and the three `ASSUME_STATUS_*_ODDS` values in `include/config/ai.h` as
  currently unreachable for the same reason.
- Rewrote the two hidden, unread `SETTING_HOF_SPECIES_EXCLUSION` /
  `SETTING_FINAL_TEAM_LOCK` descriptions off "Tournament: ..." phrasing,
  which directly contradicted `docs/SPEC.md`'s opening declaration that this
  project has no Tournament mode.
- RWX LOAD-segment linker warning: investigated and left as-is.
  `ld_script_modern.ld` declares `EWRAM (rwx)` / `IWRAM (rwx)`, which is
  correct and required on GBA (code is copied into IWRAM and executed from
  there) - not a defect.
- No compiler warnings from this session's changes (`-Werror -Wall` already
  gates the build); no project `TODO`/`FIXME` found in project-owned
  sources.

**Verification done this session:** `make clean && make -j$(sysctl -n hw.ncpu)`
clean (only the pre-existing RWX warning), `git diff --check` clean,
`test/ruleset.c` / `test/learnset_gen.c` / `test/run_report.c` and the
touched source TUs compile-checked (`make -j <objs> TEST=1`; `make check`
still cannot run on this host - see "Known test-runner limitation" below).
Memory: EWRAM 241520 B (92.13%, unchanged), IWRAM 28424 B (86.74%,
unchanged), ROM 26759696 B (79.75%, +1136 B against 32 MB - negligible).

**mGBA acceptance checklist (verified before the Phase 12 commit):**
1. New Game wizard: the settings-list hint reads "START begin" in wizard
   mode and "START preset" in the ordinary RULES menu.
2. Display & Records page is gone from RULES; no page renders empty when
   paging with L/R.
3. RULES field menu shows both "RUN INFO" and "SETTINGS" as separate rows;
   RUN INFO opens a read-only box (no clipping at width 27/height 16) showing
   correct preset/seed/gens/badges/level cap/deaths/caught/encounters/bosses
   and an accurate Run Report status line; B or A returns to the RULES list
   cleanly, list state (scroll/selection) not required to persist.
4. SETTINGS still opens the full ordinary settings browser exactly as
   before.
5. A whiteout Run-Over screen shows the new "Run Report ... saved" text and
   the export mention; answering NO shows the new reminder text once, then
   re-asks (does not loop silently or corrupt the flow); YES still proceeds
   to a fresh attempt.
6. Rescuing Peeko (Rusturf Tunnel) shows the new Quick Travel message when
   Quick Travel is on in the active ruleset, and shows nothing extra when it
   is off.
7. The Oldale 999-Ball NPC's dialogue shows the added Ball-shortcut lines
   without clipping or scroll issues.
8. A save from before this session's changes still loads correctly
   (save-compatibility spot check - no `RULESET_VERSION` /
   `RANDOMIZER_VERSION` bump was made, so this should be a non-event, but
   confirm).

## Phase 13A (final documentation & repository cleanup; implemented and
build-verified in the working tree, awaiting user review + commit)

Full inventory came from three parallel read-only code-audit agents plus
direct verification of every finding before acting on it. Full detail is in
the session's approved plan; this is the durable summary.

**Documentation rewritten from verified source**, not from prior session
summaries: [`README.md`](../README.md) (was 100% unmodified upstream RHH
boilerplate — did not mention this hack exists at all) and the new
[`PLAYER_GUIDE.md`](PLAYER_GUIDE.md) (full player-facing behavior). The
original upstream README is preserved verbatim at
[`UPSTREAM_README.md`](UPSTREAM_README.md). `docs/PHASES.md`, `PROMPTS.md`
(root), `CLAUDE.md`, `AGENTS.md` brought current (all previously said Phase
11C was "next"). `docs/PROMPTS.md` (a second, contradictory, stale runbook)
and `AI_HANDOFF.md` (untracked, gitignored, local-only) both marked with an
unmistakable SUPERSEDED/HISTORICAL banner rather than deleted, per user
decision. `FEATURES.md` (root, unmodified upstream engine-capability list)
got a one-line note that Mega/Z-Move/Dynamax/Tera are engine capabilities,
not this hack's defaults.

**Three genuine code/doc contradictions were found during verification and
fixed, per explicit user approval on each before touching source:**

1. **Ball field-item leak (fixed, `RANDOMIZER_VERSION` 4→5).**
   `ItemIsPoolEligible()` (`src/randomizer.c`) admitted the entire
   `POCKET_POKE_BALLS` pocket to the randomized field-item pool, so a lucky
   roll could hand the player an early Ultra/Timer/etc. Ball outside the
   intended shop progression — directly contradicting `docs/SPEC.md`'s "Poké
   Ball availability" ("the Master Ball is the only Poké Ball permitted").
   Now restricted to `ITEM_MASTER_BALL` only. This changes what a given seed
   generates for field items, so it needed the version bump (same precedent
   as the Phase 11D Premium-pool fix).
2. **Quick Travel unlock text (fixed, no version bump).** The Rusturf Tunnel
   unlock message told the player to "Use the TOWN MAP", but `ITEM_TOWN_MAP`
   is never granted anywhere in this campaign — the feature only ever
   worked via PokéNav → MAP → R, which the player does receive at the same
   story beat. Corrected to say POKéNAV.
3. **Two dead-setting groups hidden (no version bump — hiding a row doesn't
   renumber stored indices):**
   - `SETTING_SHINY_ODDS` (Normal/Boosted/Disabled) had zero consumers; the
     shiny rate is the fixed vanilla constant regardless.
   - The five `SETTING_ALLOW_LEGENDARY`/`MYTHICAL`/`SUB_LEGENDARY`/
     `ULTRA_BEAST`/`PARADOX` toggles were verified to have no effect on any
     pool: the Premium category they gate is *already* unconditionally
     excluded from the ordinary pool via `IsSpeciesPremium()`, and the
     Premium pool itself never reads them either. Both matched the exact
     precedent Phase 12A set for other zero-consumer settings. `docs/SPEC.md`
     "Species-pool settings" corrected to match.

**Findings verified but left as documentation-only (no code change, per
explicit user decision or because no fix was warranted):**
- `NICK_STRICT` and `NICK_MANDATORY` are behaviorally identical
  (`Nuzlocke_StrictNicknamesOn()` is a literal alias) — documented as one
  "required" behavior rather than two.
- `TRLEVEL_FLAT_OFFSET` ("Flat offset" trainer-level mode) is defined but
  unreachable from the menu (`maxv` caps the row at two values) — not
  mentioned in player docs.
- Progression-based Ball vendors are Emerald's own existing, unmodified shop
  schedule, not bespoke project work — `docs/SPEC.md` reworded to say so
  plainly rather than imply new logic that was never written.
- `SETTING_ENCOUNTER_LEVEL_MODE`, `SETTING_ALLOW_DUPLICATE_PREMIUM`,
  `SETTING_SHOP_RANDOMIZATION` remain hidden/unread exactly as before —
  already correctly excluded from player-facing docs.

**⚠️ Carried forward, not addressed this session — candidate Phase 13B
blocker:** [`POST_CAPTURE_DIAGNOSTICS.md`](POST_CAPTURE_DIAGNOSTICS.md)
records an **unresolved, unreproduced-since, "ROOT CAUSE NOT PROVEN"**
input-soft-lock symptom from around Phase 9 (post-capture, the player can
end up stuck walking Down with menus/saving potentially unavailable). It was
committed once (Phase 10 Arc 0) and never mentioned again in any later
handoff — nothing in this session's audit found evidence it was fixed, and
nothing found evidence it's stale/no-longer-reproducible either. Its
evidence files (`pokeemerald.ss1`/`.ss2`, `checkpoint-after-phase9.gba`,
`test-0907-1634.gba`) are all untracked/local-only and exist only on the
machine that produced them. This needs an explicit decision before release:
either reproduce and fix it, or deliberately determine (and record) that
it's stale. Do not silently drop this from a future handoff without that
determination.

**Repository cleanup:**
- Removed two accidentally-tracked files: `checkpoint-after-phase9.gba.pre9_5.bak`
  (a 33 MB full ROM, `.bak` defeated the `*.gba` gitignore rule) and
  `2026-09-15-142502-local-command-caveatcaveat-the-messages-below.txt` (a
  pasted terminal transcript). Added `*.bak` and `*.sav` to `.gitignore`.
- Zero `TODO`/`FIXME`/`XXX`/`HACK` and zero project-introduced debug
  instrumentation found across every project-owned source file (confirmed by
  a repo-wide grep, reconfirming the same finding recorded after Phase 12B).

**Verification done this session:** `make -j$(sysctl -n hw.ncpu)` clean
(only the pre-existing, upstream-correct RWX warning); `git diff --check`
clean. Memory: EWRAM 241520 B (92.13%, unchanged), IWRAM 28424 B (86.74%,
unchanged), ROM 26759632 B (79.75%, -64 B against the pre-session baseline —
negligible).

## Phase 13B (final release gate — static audit performed this session)

Three parallel read-only audit tracks (randomizer/Nuzlocke/battle/AI;
progression/Premium/QoL/UX; Run Reports/save/export/documentation) plus main
agent reconciliation. See the session's audit output for the full
scope-item-by-scope-item pass/fail; the material outcome is the three fixes
already folded into "Phase 13A" above (found during this audit, fixed with
approval, and re-verified by the same build).

**No other genuine release blocker was found by static inspection**, with
one explicit exception carried forward rather than resolved: the unresolved
post-capture soft-lock investigation above. Whether that counts as a release
blocker is the user's call, not a default assumption either way.

**mGBA playthrough still required before any final verdict** — a complete
run from New Game through either Champion victory or a deliberate wipe,
covering (at minimum): the New Game wizard end-to-end; every enumerated
Premium static slot's location/event/progression behavior; a randomized
field item never producing a non-Master Ball; the corrected Quick Travel
message; that hiding the six dead settings didn't empty any settings page or
change any other row's index; a Victory and a Wipe Run Report exporting
correctly; and a second fresh run afterward confirming the first run's
completion left no trace in the new run's randomization. The terminal
`READY FOR FINAL RELEASE` / `PRODUCT COMPLETE` verdict is withheld until the
user reports this playthrough passed.

## Randomizer performance + RNG audit (uncommitted, build-verified)

Player report: stutter entering trainer/gym battles and on fresh encounters,
plus a suspicion that different seeds were producing the same Pokémon. Full
detail is in the session's approved plan; this is the durable summary.

**RNG audit: the generator itself is sound.** Host-replicated the exact
pipeline (`Crc32B` seed mix → SFC32 → the reservoir-sampling selector) and ran
chi-squared tests over 200k trials at several candidate-pool sizes, with both
scattered and sequential run seeds — every cell tracked its degrees of
freedom. No change was made to the selection algorithm's fairness. The real
defect: `Randomizer_WildSlotSpecies`'s wild-slot LRU cache
(`src/randomizer.c`) was keyed on `slotSeed` alone, unlike every other
generation cache in the project (Tm/Learnset/Ability all fold `GetRunSeed()`
into their signature) — so loading a save with a different run seed in the
same emulator session kept serving the previous seed's resolved species until
some unrelated ruleset setter happened to call
`Randomizer_InvalidateWildSlotCache()`. Fixed by adding a signature check
(seed + `RANDOMIZER_VERSION` + encounter-mapping mode) to the cache-line
lookup, so a stale cache is caught on the very next query regardless of how
the save was reached.

**Performance: `GenerateWeighted()` (`src/learnset_gen.c`) was the dominant
cost** — with defaults (learnset size 21, weighted composition), ~21
checkpoints × ~850 eligible moves ≈ 17,850 inner iterations per species, each
recomputing move potency from ROM (5 accessor calls + divides) and paying a
variable-divisor modulo. Landed in two commits:

- **Output-identical (no `RANDOMIZER_VERSION` bump):** potency/STAB
  precomputed once per pool build instead of per checkpoint-visit
  (`sPotency[]`/`sDmgType[]`, indexed the same way `PoolMoveRaw()` is); a
  per-checkpoint 256-entry timing lookup table; the learnset LRU cache
  widened 8→32 slots (a 6-mon enemy party plus the player's party routinely
  exceeded the old cap); `Crc32B` moved from a bit-serial loop to a table
  lookup (host-verified bit-identical over every 1-byte input and 2M random
  24-byte buffers — the `RunRng_Seed` shape); generated abilities memoized
  per species (`AbilityGen_Get` used to re-derive up to 3 seeded streams on
  every call, including every AI party scan); the selector's
  `POOL_STRICT_ORDINARY` scan (wild/starter/gift/ordinary static/most trainer
  slots) now walks a precomputed dense, ascending-order species list
  (`PowerScore_OrdinaryList()`) instead of re-testing all ~1573 species per
  ladder rung. Also added a read-only debug-menu entry ("Encounters… >
  Randomizer pool size") reporting the live rung-0/rung-1 candidate counts for
  the lead party species, so "feels samey" can be measured before anyone
  tunes `sBaseWindow[]`.
- **`RANDOMIZER_VERSION` 5→6 (separate commit):** `GenerateWeighted`'s
  per-checkpoint selection moved from single-pass weighted reservoir sampling
  (one RNG draw *and* one variable-divisor modulo per pool element) to a
  two-pass draw (sum weights, one modulo per checkpoint, walk the prefix sum).
  Same `w_k/totalWeight` distribution (host chi-squared verified against a
  synthetic weight array over 500k trials) but a different RNG draw count/
  order for a given seed, so **this resets in-progress runs** the same way the
  Phase 11D/13A version bumps did — the existing lazy-reinit already handles
  it (default preset, fresh seed) on next load of a stale save.

**User-confirmed design invariant that shaped this work:** a species'
generated ability/learnset is a pure function of `(species, run seed,
settings)` and identical for every instance of that species in the world
(only a player's own in-battle move replacement is per-mon). This is what
makes per-species caching always correct rather than a shortcut.

**Verification done this session:** `make -j$(sysctl -n hw.ncpu)` clean after
every stage (only the pre-existing RWX linker warning); zero new
warnings from the touched translation units. Memory after all changes: EWRAM
248684 B (94.87%, up from 245016 B/93.47% pre-session — the learnset cache
widen, dense ordinary-species list, and ability cache account for the
increase; ~13.4 KB EWRAM headroom remains), IWRAM unchanged (86.74%), ROM
26762800 B (79.76%, negligible increase). Host-side equivalence/chi-squared
harnesses are one-off scripts in this session's scratchpad, not committed to
the repo.

**Not yet done — mGBA playtesting required, cannot be accepted by
inspection:**
1. Confirm two different run seeds produce different Route 101 encounters,
   *and* that loading a save without rebooting still shows that save's own
   seed's encounters (the regression case that failed on `main` before the
   cache-signature fix).
2. Time the pause on a fresh grass encounter, entering Roxanne's gym battle,
   and entering a 6-mon Elite Four battle; compare against a pre-change build.
3. Confirm generated learnsets/abilities are still identical across every
   instance of a species (catch two of the same species and compare).
4. With only the output-identical commit checked out, confirm a pre-change
   save's world is byte-for-byte unchanged (same route species, TM moves,
   starter trio).
5. Read the new debug pool-count entry on a few early-route species and
   decide, with real numbers, whether `sBaseWindow[]` needs widening — not
   done speculatively this session.

## Pointers

- [SPEC.md](SPEC.md) — authoritative product-behavior specification.
- [PLAYER_GUIDE.md](PLAYER_GUIDE.md) — the finished, player-facing feature
  set (start here for "what does the game actually do").
- [PHASES.md](PHASES.md) — Phase 0-12B historical record and current
  high-level roadmap.
- [`../PROMPTS.md`](../PROMPTS.md) — authoritative runbook; only Phase 13B
  remains open, with full scope and acceptance criteria for it.
- [POST_CAPTURE_DIAGNOSTICS.md](POST_CAPTURE_DIAGNOSTICS.md) — an
  unresolved, undiagnosed input-soft-lock investigation from around
  Phase 9, never mentioned again since. See "Phase 13A" above — this needs
  an explicit decision before release, not a silent drop.
