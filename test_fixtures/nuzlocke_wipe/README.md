# nuzlocke_wipe

**Purpose:** reach a terminal wipe almost immediately, for testing wipe
handling and Wipe Run Reports without a real run's worth of setup.

**Starting state:** Randomizer preset with Permadeath forced on, run reset
and active (`Nuzlocke_BeginRun()`). Player has a single low-level mon in the
field-continue location (Littleroot Town):

- Zigzagoon Lv8, Hardy — Tackle / Growl (no held item)

**Setup:** load, then debug menu (hold R + START) → Test Fixtures →
Nuzlocke Wipe Test. This starts a battle against
`FIXTURE_TRAINER_NUZLOCKE_WIPE` (`src/data/fixture_trainers.party`): a Lv25
Linoone (Belly Drum into Extreme Speed) that outclasses the Lv8 Zigzagoon
badly enough to win in one or two turns.

**What to observe:** losing the battle with Permadeath on and no other party
mons should trigger the terminal wipe flow (whiteout handling, Wipe Run
Report). Check `src/run_report.c` / `tools/export_run.py` output afterward
if testing Run Report persistence/export.

**Difficulty:** not AI-tier sensitive; any RULES > AI Difficulty setting
works, since the opponent only needs to out-damage a Lv8 mon.

**Savestate:** none.
