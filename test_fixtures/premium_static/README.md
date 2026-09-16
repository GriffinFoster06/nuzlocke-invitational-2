# premium_static

**Purpose:** test one representative Premium static encounter with
progression already established, instead of playing to it normally.

**Starting state:** Randomizer preset, all 8 badges, `FLAG_REGI_DOORS_OPENED`
set (the braille-puzzle result that unlocks the Regi chambers). Player has
one mon and is warped inside Desert Ruins (Regirock's map):

- Swampert Lv45, Adamant — Surf / Earthquake / Ice Beam
- Bag: 20x Ultra Ball

**Setup:** load; the player starts inside Desert Ruins.

**Caveat — needs manual verification:** the exact warp-in tile has not been
checked against the map data (map scripts/layouts can't be verified by code
inspection alone — see `AGENTS.md`), so a short walk to Regirock's actual
tile may still be needed. If the warp lands somewhere unexpected, note the
correct coordinates back into `Scenario_PremiumStatic()` in
`src/test_fixtures.c` (`Fixture_SetLocation` call).

**What to observe:** approaching/interacting with the static should start a
battle against whatever the randomizer's Premium pool selected for this
seed (uniform draw over the eligible Premium pool per `PickPremiumStatic`,
`src/randomizer.c`) — confirm it's a legal, seed-consistent pick.

**Adding the other Premium statics:** copy `Scenario_PremiumStatic` with the
right map/flags for that static (e.g. Regice, Registeel, Ho-Oh/Lugia
equivalents), add a row to the scenario table in `src/test_fixtures.c` and
`SCENARIOS` in `tools/generate_test_fixture`.

**Savestate:** none.
