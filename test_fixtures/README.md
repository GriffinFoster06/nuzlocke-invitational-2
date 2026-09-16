# Developer test-scenario fixtures

Named, reproducible starting points for testing specific gameplay
situations without playing through hours of setup. See each scenario's own
`README.md` for what it sets up and what to check.

## Quick start

```sh
tools/test_scenario --list                    # generated fixtures
tools/generate_test_fixture --list             # all known scenarios
tools/generate_test_fixture --all              # (re)build every fixture
tools/generate_test_fixture <name>             # (re)build one
tools/test_scenario <name>                     # load one into mGBA
tools/test_scenario <name> --fresh             # discard prior play, reload the fixture
tools/test_scenario <name> --state             # also load test_fixtures/<name>/state.ss1
```

`tools/generate_test_fixture` builds `pokeemerald-fixtures.gba` (`make
fixtures`, a normal ROM build with `-DTEST_FIXTURES=1`) if it's missing or
stale, runs it headlessly per scenario under `tools/mgba/mgba-rom-test-mac`,
and writes the resulting ordinary `.sav` to `test_fixtures/<name>/save.sav`.
`tools/test_scenario` copies that `.sav` into an isolated
`build/test_runs/<name>/` directory next to a symlink to your normal
`pokeemerald.gba`, then launches mGBA on that pair — your personal
`pokeemerald.sav` and the canonical fixture file are never opened for
writing.

To go back to normal play, just relaunch `pokeemerald.gba` from the repo
root as usual.

## Layout

```
test_fixtures/<name>/
    save.sav      # generated, gitignored — regenerate with tools/generate_test_fixture
    state.ss1     # optional, gitignored — captured by hand in the mGBA GUI
    README.md     # tracked — purpose, setup, what to check
```

Only the scenario definitions (`src/test_fixtures.c`,
`src/data/fixture_trainers.party`) and these `README.md` files are tracked;
`.sav`/`.ss*` outputs are disposable and gitignored.

## Adding a scenario

Add a builder function + one row in `src/test_fixtures.c`'s scenario table
and the matching name in `tools/generate_test_fixture`'s `SCENARIOS` list
(order must match), then `tools/generate_test_fixture <name>` and write its
`README.md`. No other tool or Makefile changes are needed. See
`AGENTS.md`/`docs/CLAUDE_HANDOFF.md` for when adding one is warranted.

## Savestates

Optional and per-scenario. mGBA 0.10.5 has no CLI scripting to capture one
headlessly, so capture by hand: load the fixture normally
(`tools/test_scenario <name>`), play to the moment worth freezing, then in
mGBA use File > Save State File... and save it as
`test_fixtures/<name>/state.ss1` (or any arbitrary path — pass it to
`--state PATH`). Savestates are more ROM-version-sensitive than `.sav`
fixtures; recapture one whenever a change to the ROM could make it invalid
(e.g. a battle/AI/save-format change), even if the `.sav` fixture itself
still regenerates fine.

## Regeneration

Fixtures are deterministic: the fixture ROM forces a fixed RNG seed and run
seed at boot (see `src/test_fixtures.c`), so regenerating a scenario against
an unchanged ROM produces a byte-identical `.sav`. Regenerate a scenario
after any change that could affect its setup (species/move/item data,
ruleset defaults, save format, `RULESET_VERSION`/`RANDOMIZER_VERSION`).
`tools/generate_test_fixture` rebuilds `pokeemerald-fixtures.gba`
automatically whenever it's older than `src/`/`include/`.
