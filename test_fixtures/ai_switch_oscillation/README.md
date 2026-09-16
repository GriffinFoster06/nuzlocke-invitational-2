# ai_switch_oscillation

**Purpose:** diagnose voluntary AI switching — does the AI switch out of a
bad matchup into a clearly better one, and does it then stay there (not
immediately switch back into the original bad matchup)?

**Starting state:** Recommended preset, 4 badges, seed fixed at generation
time. Player has one mon in the field-continue location (Littleroot Town):

- Swampert Lv50, Adamant, Leftovers — Surf / Earthquake / Ice Beam / Toxic

**Setup:** load, then open the debug menu (hold R + START) → Test Fixtures →
AI Switch Oscillation. This starts a single battle against a curated
opponent (`FIXTURE_TRAINER_AI_SWITCH`, `src/data/fixture_trainers.party`):

- A: Torkoal (Fire) — bad matchup vs. Swampert
- B: Ludicolo (Water/Grass) — clearly better matchup vs. Swampert
- C: Pelipper (Water/Flying) — another legal option

**What to observe:** with A active and disadvantaged, does the AI switch
A → B? If it does, does B then act normally rather than immediately
switching B → A back into the inferior matchup?

**Difficulty:** the opponent's AI flags are *not* hardcoded — they come from
the current RULES > AI Difficulty setting at battle start
(`GetRulesetAiFlags()`), so the same fixture compares
Standard/Improved/Expert/Pro Fair by changing that setting (RULES menu, or
the "Ruleset Settings…" debug submenu) and re-running the fixture battle.

**Savestate:** none by default. If repeated testing needs the exact moment
right before the AI's decision, capture one by hand right there and save it
as `state.ss1` (see the top-level `test_fixtures/README.md`).
