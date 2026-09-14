# Claude session handoff

Concise current-state snapshot. This is deliberately volatile and short —
detailed phase scope, acceptance criteria, and copy/paste prompts live in
[`../PROMPTS.md`](../PROMPTS.md). Historical Phase 0-10 record lives in
[PHASES.md](PHASES.md). Product-behavior authority is
[SPEC.md](SPEC.md). Read all three before starting work; this file is not a
substitute for any of them.

## Repository state

- Branch: `main`.
- HEAD: `61eca28f18` ("Phase 11B: enforce Nuzlocke rules and fair AI").
- Working tree: clean at last check.

A fresh session must re-run `git status`, `git log --oneline -10`, and
`git diff` (staged and unstaged) rather than trusting this snapshot — it is
a point-in-time record, not a live query.

## Completed phases

Phases 1-10, 11A, 11B are complete and committed. See
[PHASES.md](PHASES.md) for the Phase 0-10 historical record and
[`../PROMPTS.md`](../PROMPTS.md)'s "Phase taxonomy" table for full current
status.

## Exact next phase

**Phase 11C — Generation Architecture, Performance, Seed Quality & Pre-Run
Configuration.** Full scope, acceptance criteria, and copy/paste prompt are
in [`../PROMPTS.md`](../PROMPTS.md).

## Last known memory baseline (re-measure before relying on it)

From a build at or near `61eca28f18`, using the highest-address symbol in
`pokeemerald.map` within each RAM region:

- EWRAM ≈ 234.2 KB / 256 KB (~91.5%)
- IWRAM ≈ 27.7 KB / 32 KB (~86.7%)
- ROM ≈ 79.7% (per prior project record; re-derive from a fresh
  `pokeemerald.gba` size against the 32 MB ROM limit if precision matters)

Phase 11C is expected to measure this freshly rather than trust these
numbers, since its own optimization work changes them.

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
- Phase 11B's wild-dupe-selector latency fix — this was a local,
  non-redesigning fix, not the full hot-path optimization; the full
  optimization is Phase 11C's job, and the local fix's real-world latency
  impact hasn't been measured on hardware/emulator, only reasoned about
  statically.
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
