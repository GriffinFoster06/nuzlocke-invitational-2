# Project: Custom Nuzlocke-Randomizer Emerald Hack

## What this is
A custom GBA ROM hack built on `pokeemerald-expansion` (current upstream
master, NOT the older `tertu-randomizer` fork — see rationale below). It's
inspired by Pokémon Randolocke and the "Nuzlocke Invitational 2" ruleset, but
is its own design. The full, authoritative ruleset is in `docs/SPEC.md` — read
it before implementing anything. Where the spec is ambiguous, its own stated
priority order applies: (1) Invitational 2 behavior where known, (2) Randolocke
fallback behavior, (3) our own design choice.

## Why upstream master, not a Randolocke-lineage fork
Randolocke's own source was never published. The credited base
(`Zetraphes/pokeemerald-expansion@tertu-randomizer`) is ~16 months stale vs.
current master (500+ diverged files in `src/`, 370+ in `include/`) and its
bespoke randomizer has effectively been superseded by a cleaner, more general
system that's since landed upstream: `src/random_mon_generation.c`. That file
already supports `banLegendary` / `banMythical` / `banSubLegendary` /
`banUltraBeast` / `banParadox` / `randomizeForms` options and has an unused
BST-range filter (`IsInBstRangeFilterFunc`) ready to wire in. Its option
tables (`src/data/random_mon_generator.h`) are currently EMPTY — that's our
extension point, not a system to fight against.

## Key existing infrastructure (confirmed present, don't rebuild)
- `include/config/caps.h` — level cap system already supports hard/soft caps,
  flag-list-driven per-gym progression (`LEVEL_CAP_FLAG_LIST`), and a
  rare-candy-cap toggle. The spec's per-badge cap table plugs into this.
- `include/config/ai.h` — extensive tunable AI switch/prediction percentages
  already exist (Wonder Guard, Encore, Natural Cure, prediction chance, etc.).
  "Pro Fair" AI is primarily about tuning these + NOT using any omniscient
  info flag, not writing new battle AI.
- `src/random_mon_generation.c` / `random_mon_generation.h` — the species/item
  random-generation engine described above.
- No existing Nuzlocke ruleset (permadeath, dupes clause, location-tag
  tracking, mandatory nicknames, no-battle-items) — this is genuinely new.
- No existing HM-free field-move permission system (badge-gated field moves
  instead of HM-known-by-a-party-member) — genuinely new.
- No existing settings/preset menu UI — genuinely new.

## Build
```
make -j$(sysctl -n hw.ncpu)          # build
make clean                            # clean
make debug                            # build with debug symbols
```
Requires `pokeemerald.gba` (the vanilla baserom) in the project root, exact
SHA1 `f3ae088181bf583e55daf962a92bb46f4f1d07b7` — verify with `shasum
pokeemerald.gba` before ever reporting a build issue as a code problem.

## Working conventions
- Build after every non-trivial change. A change that doesn't compile is not done.
- This is a large multi-phase project — see `docs/PHASES.md` for the agreed
  order. Stay inside the current phase's scope; don't drift into later phases
  unprompted.
- Anything touching map scripts/events (story linearization, phase 10) cannot
  be verified by reading code alone — flag clearly when something needs
  actual emulator playtesting, and say what to test for.
- When the spec lists factors for something without giving an exact formula
  (e.g. the multi-factor power score weighting), propose the formula
  explicitly and ask before committing to it — don't silently invent and
  bury a design decision.
