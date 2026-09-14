# Claude session handoff

Concise current-state snapshot. This is deliberately volatile and short —
detailed phase scope, acceptance criteria, and copy/paste prompts live in
[`../PROMPTS.md`](../PROMPTS.md). Historical Phase 0-10 record lives in
[PHASES.md](PHASES.md). Product-behavior authority is
[SPEC.md](SPEC.md). Read all three before starting work; this file is not a
substitute for any of them.

## Repository state

- Branch: `main`.
- HEAD (at last check): `61eca28f18` ("Phase 11B: enforce Nuzlocke rules and
  fair AI"); Phase 11A.6/11C's changes are implemented and build-verified on
  top of it but not yet committed as of this update.
- Working tree: not clean — Phase 11A.6/11C implementation is in place; see
  "Exact next phase" below.

A fresh session must re-run `git status`, `git log --oneline -10`, and
`git diff` (staged and unstaged) rather than trusting this snapshot — it is
a point-in-time record, not a live query.

## Completed phases

Phases 1-10, 11A, 11B are complete and committed. Phase 11A.6 (== "Phase
11C" in [`../PROMPTS.md`](../PROMPTS.md)'s taxonomy) — Generation
Architecture, Seed Quality, Pre-Run Settings, and Performance — is
**implemented and build-verified**, but not yet committed and not yet
mGBA-acceptance-tested; see "Unresolved runtime-verification items" below
for exactly what remains. See [PHASES.md](PHASES.md) for the Phase 0-10
historical record and [`../PROMPTS.md`](../PROMPTS.md)'s "Phase taxonomy"
table for full current status.

## Exact next phase

Phase 11A.6/11C's code is done; the immediate next step for any session
picking this up is: (1) mGBA acceptance-test the items listed under
"Unresolved runtime-verification items", (2) commit the change, (3) proceed
to Phase 11D (Premium statics) / 11E (Run Reports) per
[`../PROMPTS.md`](../PROMPTS.md).

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
- Terminal Run Reports and the full Premium-static contract (Mew/Deoxys/
  Ho-Oh/Lugia event-island slots specifically) are specified in
  [SPEC.md](SPEC.md) but **not yet implemented** — this is exactly Phase
  11D (Premium statics) and Phase 11E (Run Reports)'s job, not a regression.

## Pointers

- [SPEC.md](SPEC.md) — authoritative product-behavior specification.
- [PHASES.md](PHASES.md) — Phase 0-10 historical record and current
  high-level roadmap.
- [`../PROMPTS.md`](../PROMPTS.md) — authoritative runbook for all
  remaining phases (11C through 13B), with full scope, acceptance criteria,
  and copy/paste prompts per phase.
