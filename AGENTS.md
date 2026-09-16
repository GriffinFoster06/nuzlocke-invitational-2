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
  deterministic project mappings for wild, trainer, starter, gift, static,
  TM, Tutor, and item results. `src/ability_gen.c` owns generated abilities,
  consumed through `GetSpeciesAbility` in `src/pokemon.c`; `src/learnset_gen.c`
  owns generated learnsets. These systems use upstream generation helpers
  rather than recreating the unpublished stale Randolocke randomizer.
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
- `src/run_report.c` and `include/run_report.h` implement Victory/Wipe Run
  Reports (a two-slot flash bank plus live EWRAM counters); `tools/export_run.py`
  and `tools/watch_run_export.py` are the read-only host-side JSON exporters.
- `src/species_generation.c` classifies every species/form by the generation
  that introduced it, feeding the Gen 1-9 filter mask consumed through
  `src/power_score.c`'s eligibility cache.
- `src/ruleset_field.c` / `src/ruleset_qol.c` implement the in-run RULES field
  menu, Portable Heal, Infinite Repel, Quick Travel, both-bikes switching, and
  the Run Information overlay.

These systems were implemented and remediated across Phases 1-12B, which are
now all complete. Inspect their current code before changing them; do not
treat old historical prompts as evidence that a subsystem is absent, and do
not assume a setting shown in an old prompt is still live — several
zero-consumer settings were verified and hidden during Phase 13A (see
`docs/CLAUDE_HANDOFF.md`).

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
- `docs/PHASE10_PROMPTS.md` is authoritative for the Phase 10 grouped-arc
  workflow and exact membership; Phase 10 (Arcs 0-8) is now complete, so
  treat that document as a historical workflow/membership record rather than
  active instructions. For Arcs 1-8, a planning/review Codex session
  inspects the actual code and writes an approved `docs/PHASE10_ARC<N>_PLAN.md`;
  a fresh execution Codex session implements only that plan; then a fresh
  planning/review Codex session performs the read-only SPEC-and-plan review.
  Work only within the current approved arc. Do not begin a later arc without
  the required plan, execution, review, build, and user mGBA acceptance.
- `PROMPTS.md` (repo root) is authoritative for every phase after Phase 10
  (11C onward through PRODUCT COMPLETE). Phases 11C-13A are complete; only
  Phase 13B (final release gate) remains open, and this document becomes
  fully historical once it closes — there is no Phase 14.
  `docs/CLAUDE_HANDOFF.md` carries current volatile state (branch, HEAD,
  completed phases, known open items). `docs/PLAYER_GUIDE.md` documents the
  finished player-facing feature set. Read all three before starting new
  work. The root-level `AI_HANDOFF.md` is untracked, gitignored, historical
  scratch state and must never be treated as current status.
- Map scripts and events cannot be accepted by code inspection alone. State
  exactly what still needs emulator playtesting and which path or flags to use.
- When the spec lists factors without an exact formula, propose the formula
  explicitly and obtain user approval before implementing it.
- Preserve unrelated user changes in a dirty worktree.

## Codex session handoff

`AGENTS.md`, `docs/SPEC.md`, `PROMPTS.md`, and `docs/CLAUDE_HANDOFF.md` are
the shared source of truth (plus the current approved Phase 10 arc plan for
any residual Phase 10 review). A fresh Codex session must inspect
`git status`, both staged and unstaged diffs, and relevant recent commits
before editing. Preserve all unrelated uncommitted work and leave
implementation, static verification, build verification, and emulator
acceptance as separately recorded states.
