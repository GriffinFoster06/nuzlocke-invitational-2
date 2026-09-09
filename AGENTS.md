# Project: Custom Nuzlocke-Randomizer Emerald Hack

These project-wide instructions apply to any coding agent working in this
repository.

## What this is

A custom GBA ROM hack built on current upstream `pokeemerald-expansion`. It is
inspired by Pokémon Randolocke and the "Nuzlocke Invitational 2" ruleset, but
is its own design. The full, authoritative gameplay ruleset is in
`docs/SPEC.md`; read it before implementing anything. Where the spec is
ambiguous, use its stated priority order: (1) Invitational 2 behavior where
known, (2) Randolocke fallback behavior, (3) a project-specific design choice.

## Why upstream master, not a Randolocke-lineage fork

Randolocke's own source was never published. The credited base
(`Zetraphes/pokeemerald-expansion@tertu-randomizer`) is substantially stale
relative to current upstream. Upstream's `src/random_mon_generation.c` provides
the general species/item generation primitives and category filters that this
project builds on, while the project's deterministic randomizer and mappings
live in `src/randomizer.c` and `include/randomizer.h`. Extend those systems;
do not replace them or recreate the old fork's bespoke randomizer.

## Existing project infrastructure

- `src/ruleset.c`, `src/data/ruleset.h`, and `include/constants/ruleset.h`
  define settings and presets. `src/ruleset_menu.c` provides the settings UI,
  with player access through the start-menu Run Info flow in
  `src/ruleset_field.c` / `src/start_menu.c`.
- `src/randomizer.c` and `include/randomizer.h` implement the seeded,
  deterministic project randomizer for wild, trainer, starter, gift, static,
  ability, TM, Tutor, and item results. They consume the upstream helpers in
  `src/random_mon_generation.c`.
- `src/nuzlocke.c` and `include/nuzlocke.h` implement the Nuzlocke subsystem,
  including the first-Ball activation gate, encounter/location tracking,
  Dupes and Shiny clauses, permadeath, nickname enforcement, battle-item
  restrictions, and whiteout handling.
- `src/field_move.c` and `include/field_move.h` implement HM-free traversal,
  including badge/progression unlocks and synthetic field-move users.
- `src/caps.c` and `include/caps.h` implement the runtime cap modes, progression
  table, acquisition-level clamping, over-cap ineligibility, and cap queries.
  `include/config/caps.h` keeps the upstream cap call sites compiled in; runtime
  settings select hard, soft, warning, or off behavior.
- `include/config/ai.h` contains the fair-AI tuning switches and prediction
  percentages. Pro Fair must not enable omniscient hidden-information flags.

These systems were implemented and remediated during Phases 1-9.5. Inspect
their current code before changing them; do not treat old historical prompts
as evidence that a subsystem is absent.

## Build

```sh
make -j$(sysctl -n hw.ncpu)
make clean
make debug
```

This project builds entirely from source. No baserom is required and none
should be added. `pokeemerald.gba` in the project root is build output, not an
input. The stock-decomp SHA1 only checks whether an unmodified build reproduces
vanilla Emerald byte-for-byte; this hack will not match it, so a missing or
different output hash is not a build-failure diagnosis. Debug the actual tool
or compiler diagnostic.

## Working conventions

- Build after every non-trivial change. A change that does not compile is not
  done.
- `docs/PHASES.md` retains the completed Phase 0-9.5 prompts as project history.
  Do not rerun them or infer current implementation status from their wording.
- Phase 10 uses the grouped-arc workflow and exact membership in
  `docs/PHASE10_PROMPTS.md`. Work only within the current approved arc. Do not
  begin a later arc without completing the required plan, execution, review,
  build, and user mGBA acceptance for the current one.
- Map scripts and events cannot be accepted by code inspection alone. State
  exactly what still needs emulator playtesting and which path or flags to use.
- When the spec lists factors without an exact formula, propose the formula
  explicitly and obtain user approval before implementing it.
- Preserve unrelated user changes in a dirty worktree.

## Cross-agent handoff

`AGENTS.md` is the canonical shared project-instruction file.

`AI_HANDOFF.md` is the local, ignored shared-state file for the current
unfinished task. During substantial unfinished work, update it whenever the
objective, implementation decision, test result, unresolved error, or exact
next step materially changes.

Before ending unfinished work, record:

- current objective and state;
- files changed;
- tests or commands run and their results;
- important findings and decisions;
- approaches already attempted or ruled out;
- exact next step.

When taking over unfinished work:

1. Read `AGENTS.md` and `AI_HANDOFF.md`.
2. Inspect `git status` and both staged and unstaged diffs.
3. Continue from the documented next step and existing investigation results.
4. Preserve all existing uncommitted work.
