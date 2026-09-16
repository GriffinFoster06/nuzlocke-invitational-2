# ai_forced_replacement

**Purpose:** compare how each AI difficulty tier picks a forced replacement
after its active mon faints.

**Starting state:** Recommended preset, 4 badges. Player has one mon in the
field-continue location (Littleroot Town):

- Blaziken Lv55, Jolly, Choice Scarf — Flamethrower / Sky Uppercut / Stone
  Edge / Protect

**Setup:** load, then debug menu (hold R + START) → Test Fixtures → AI
Forced Replacement. This starts a battle against
`FIXTURE_TRAINER_AI_FORCED` (`src/data/fixture_trainers.party`):

- lead: Exeggcute (Grass/Psychic) — 4x weak to Blaziken's Flamethrower, slow
- alternates: Wailmer (Water, bulky/stally), Geodude (Rock/Ground, resists
  Fire), Swellow (Flying, fast) — clearly different matchup quality against
  Blaziken

**What to observe:** Blaziken should knock out Exeggcute in one or two hits,
putting the AI at the forced-replacement decision almost immediately.
Compare which alternate each AI Difficulty tier sends out.

**Difficulty:** AI flags come from RULES > AI Difficulty at battle start
(not hardcoded), so re-run the same fixture after changing that setting to
compare Standard/Improved/Expert/Pro Fair.

**Savestate:** none by default. Capture one right after the KO (the forced-
replacement decision point) as `state.ss1` to skip the setup on repeat runs.
