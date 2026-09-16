# A Randomized Emerald Nuzlocke

This is a solo, replay-focused Pokémon Emerald ROM hack: a heavily
streamlined campaign built around a seeded, deterministic randomizer and a
hardcore Nuzlocke ruleset, running on
[RHH's `pokeemerald-expansion`](https://github.com/rh-hideout/pokeemerald-expansion)
engine (itself built on [pret's `pokeemerald`](https://github.com/pret/pokeemerald)
decompilation).

Pick **Recommended**, get a run seed, and go: a new species behind every
encounter, an intelligent AI behind every trainer, permadeath on the line,
and a Run Report waiting at the end. There is no Tournament mode and no
cross-run species bans — every fresh run is fully independent, and the
finished project is meant to be replayed, not "solved" once.

**Full player-facing documentation:** [`docs/PLAYER_GUIDE.md`](docs/PLAYER_GUIDE.md).
**Authoritative design spec:** [`docs/SPEC.md`](docs/SPEC.md).

## What's actually in the game

- **Deterministic seeded randomization** — an automatic or manually-entered
  run seed drives every generated system; the same ROM version, seed, and
  locked-in settings always regenerate the identical world.
- **Gen 1–9 inclusion** — nine independent toggles (all default ON) decide
  which generations' species/forms can appear anywhere, set once per run
  through the New Game settings wizard.
- **Gen 9 battle engine** — the modern physical/special split, Fairy type,
  current type chart, move/item/ability effects through Gen 9, and Gen 9-era
  damage/crit/weather/terrain/priority mechanics. Mega Evolution, Z-Moves,
  Dynamax, Gigantamax, and Terastallization are supported by the underlying
  engine but are **off by default and absent from every Recommended-style
  run** — see [`FEATURES.md`](FEATURES.md) for the engine's full baseline
  capability list versus this hack's own defaults.
- **Power- and stage-matched species replacement** — wild, trainer, starter,
  gift, and static encounters draw power-appropriate, evolutionary-stage-
  appropriate replacements, with **no artificial quality floor**: a
  spectacularly bad or spectacularly good roll is an intended, legal outcome.
- **A separate Premium category** (Legendaries, Mythicals, Ultra Beasts,
  Paradox Pokémon, and equivalent restricted species) — excluded from
  ordinary encounters and most trainers, but can appear at legendary/premium
  static slots, on the Elite Four, and always at least once on Champion
  Wallace.
- **Randomized, weighted, unique 21-slot learnsets** — every species gets its
  own generated set of 21 level-up moves, weighted toward STAB and toward
  higher power later, but never quota-locked or quality-floored.
- **Randomized abilities**, consistent through evolution.
- **Randomized TMs, Move Tutors, and field/hidden/gift items**, universally
  compatible across the whole roster.
- **Level caps and Level to Cap** — a per-badge progression cap with an
  instant-level tool (party and PC) that replays every skipped move in order
  and never auto-evolves.
- **Maximum-strength fair AI** — sophisticated switching, prediction, and
  strategic play, built entirely from legitimately visible information; no
  difficulty setting reads hidden player data.
- **Nuzlocke rules** — permadeath, one encounter per location, Dupes and
  Shiny Clauses, no battle items, and a run-ending whiteout, gated so nothing
  activates until the player actually holds a Poké Ball.
- **HM-free traversal, Portable Heal, Infinite Repel, both bikes**, unlimited
  money, an expanded Bag, and 999 starting Poké Balls.
- **Story streamlining and Quick Travel** — busywork, fetch quests, and
  forced backtracking cut throughout; every meaningful encounter and battle
  preserved.
- **Victory/Wipe Run Reports** — a comprehensive, versioned record written to
  the save at either terminal outcome, exportable to JSON without ever
  touching the input save.

See [`docs/PLAYER_GUIDE.md`](docs/PLAYER_GUIDE.md) for exactly how each of
these works, what the Recommended defaults are, and what stays configurable
in Custom/Advanced.

## Building

This project builds entirely from source — **no baserom is required and
none should be added.**

```sh
make -j$(sysctl -n hw.ncpu)   # parallel build
make clean                    # remove build output
make debug                    # debug build
```

`rom.sha1` only checks whether an *unmodified* build reproduces vanilla
Emerald byte-for-byte; this hack intentionally does not match it, so a
missing/different hash here is not a build failure — it's expected. Toolchain
setup instructions (all upstream, still accurate for this hack) are in
[`INSTALL.md`](INSTALL.md).

## Exporting a Run Report

Every finished run (Victory or Wipe) leaves a versioned report in the save
file. Export it from any computer, without touching the save:

```sh
python3 tools/export_run.py <save-file>              # one-shot manual export
python3 tools/watch_run_export.py <save-file>         # optional: auto-export while playing in an emulator
```

See [`docs/PLAYER_GUIDE.md`](docs/PLAYER_GUIDE.md#run-reports) for output
format, flags, and what each field means.

## Documentation map

| Document | What it's for |
|---|---|
| [`docs/PLAYER_GUIDE.md`](docs/PLAYER_GUIDE.md) | Everything a player needs: settings, rules, items, QoL, Run Reports |
| [`docs/SPEC.md`](docs/SPEC.md) | Authoritative design specification (for contributors/agents) |
| [`docs/PHASES.md`](docs/PHASES.md) | Historical development record |
| [`PROMPTS.md`](PROMPTS.md) | Development runbook (historical once Phase 13 closes) |
| [`INSTALL.md`](INSTALL.md) | Toolchain/build setup (upstream, unmodified) |
| [`docs/UPSTREAM_README.md`](docs/UPSTREAM_README.md) | The original RHH `pokeemerald-expansion` README |

## Credits and upstream

Built on **RHH's `pokeemerald-expansion`**:

```
Based off RHH's pokeemerald-expansion 1.17.0 https://github.com/rh-hideout/pokeemerald-expansion/
```

...itself built on [pret's `pokeemerald`](https://github.com/pret/pokeemerald)
decompilation project. See [`CREDITS.md`](CREDITS.md) for the full upstream
contributor list.

This is an independent fan-made ROM hack. It is **not endorsed by or
affiliated with** RHH, pret, Nintendo, Game Freak, Creatures Inc., or The
Pokémon Company.
