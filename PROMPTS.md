# Post-Phase-10 Runbook

This is the authoritative operational runbook for all remaining development,
from the current post-Phase-11B state through PRODUCT COMPLETE. It picks up
where `docs/PHASE10_PROMPTS.md` and the historical portion of `docs/PHASES.md`
leave off.

**`docs/SPEC.md` remains the authoritative product-behavior specification.**
This document is not a second spec — it does not restate gameplay rules in
full, only the specific evidence and scope each phase needs. When anything
here appears to conflict with SPEC.md, SPEC.md controls; report the conflict
rather than resolving it unilaterally.

`docs/PHASES.md` retains the completed Phase 0-10 record as history.
`docs/CLAUDE_HANDOFF.md` carries the current, volatile state (branch, HEAD,
what's actually done, known open items) — read it first, before this
document, in every fresh session.

**Naming note:** older text (including a sentence in `docs/SPEC.md`) refers
to the Run Report save-architecture phase as "Phase 11C2". That is the old
name for what this document calls **Phase 11E**. There is no other meaning
to "11C2".

## Phase taxonomy

| Phase | Status |
|---|---|
| 1 – 10 | Complete (see `docs/PHASES.md`, `docs/PHASE10_PROMPTS.md`) |
| 11A | Complete |
| 11B | Complete |
| **11C** | **Next** — Generation architecture, performance, seed quality, pre-run configuration |
| 11D | World / Premium statics / missing core QoL |
| 11E | Victory + Wipe Run Reports / save architecture / JSON export / optional mGBA integration |
| 11F | Final Phase-11 correctness gate |
| 12A | Fun / accessibility / recommended defaults / settings simplification / UX / game feel |
| 12B | Final technical & presentation polish |
| 13A | Final documentation / repository cleanup |
| 13B | Final release gate |
| — | **PRODUCT COMPLETE** |

There is no Phase 14 and no planned feature phase after 13B.

## Standing rules for every phase below

Every copy/paste prompt in this document already tells a fresh session to do
these things, but they are stated once here so nothing has to repeat them at
length:

1. Read `CLAUDE.md` → `AGENTS.md` → `docs/SPEC.md` (relevant sections) →
   `docs/PHASES.md` → `docs/CLAUDE_HANDOFF.md` before touching anything.
2. Inspect the actual current implementation. Do not infer implementation
   status from any document's wording, including this one — verify with
   `grep`/`Read` against the real source first.
3. Preserve unrelated completed behavior and any pre-existing uncommitted
   work in the worktree.
4. For an architectural phase (marked "plan first" below), produce and get
   explicit human approval for a plan before editing code.
5. Build after every non-trivial change:
   `make -j$(sysctl -n hw.ncpu)`. A change that does not compile is not done.
6. Inspect the full resulting diff (`git status`, `git diff`) before calling
   anything finished.
7. Run `git diff --check` before considering a phase closeable.
8. Stay inside the phase's exact scope. Stop at the phase boundary. Do not
   begin the next phase in the same session, even if it looks trivial.
9. Do not commit or push without the user reviewing the diff first.

---

## Phase 11C — Generation Architecture, Performance, Seed Quality & Pre-Run Configuration

### Purpose

Make the run's randomized universe fully and only a function of
(project/randomizer version, canonical seed, locked generation-affecting
settings) — reproducible, safe under all filter combinations, and not
expensive at battle-start time. Add the Gen 1-9 generation-filter system and
formalize which settings are pre-run/generation-locked versus runtime-safe.

### Prerequisites

Phase 11B closed (it is — `61eca28f18`). No unresolved 11B blocker in
`docs/CLAUDE_HANDOFF.md`.

### Exact scope

**1. Performance — measure, then optimize the deterministic replacement path.**

Known concrete finding to start from: `PickReplacementCoreExcluding` in
`src/randomizer.c` (~line 155) does two full `1..NUM_SPECIES` linear scans
per ladder rung (count pass, then pick pass), and the ladder itself has up to
4-5 rungs (`BuildLadder`). This is called from wild-slot resolution
(`Randomizer_WildSlotSpecies`), the starter trio
(`Randomizer_StarterSpecies`/`ResolveStarterTrio`), static/roamer species, and
trainer party replacement — all on the encounter/battle-start hot path.
`src/pokemon.c:1335` and `src/scrcmd.c:2467` show how static/event species are
currently sourced.

- Profile or otherwise measure the actual pause (frame count / instruction
  count estimate is acceptable if a real profiler isn't available on this
  toolchain — state which method was used).
- Redesign the replacement architecture to avoid repeated full-dex scans on
  this path: valid approaches include a compact precomputed mapping (built
  once per run, sized to the enabled species/pool), fast stateless derivation
  from (seed, subsystem, stable content id), a small compute-once cache, or a
  justified hybrid. Do not allocate large permanent EWRAM/IWRAM structures to
  solve this — RAM is already at approximately 91.5% EWRAM / 86.7% IWRAM
  (re-measure current numbers from a fresh build's `.map` file before relying
  on this baseline; do not trust this document's numbers as current).
- Preserve determinism: the same (version, seed, settings) must still
  produce the same result after the optimization.
- Also address the trainer-creation replacement-scan and generated-learnset
  cache-miss cost named in the brief; inspect actual trainer party generation
  code before assuming its shape matches the wild path.

**2. Seed quality audit.**

Current: `GetRunSeed()`/`SetRunSeed()`/`RerollRunSeed()` in `src/ruleset.c`
(~line 146, 158, 521) source the automatic seed from a bare `Random32()` at
save-init time. Audit whether this is practically non-repeating for normal
human play (e.g., is `Random32()`'s underlying state itself well-seeded at
that point, or could two saves started close together collide?).

- Do not describe any result as hardware true randomness.
- Do not rely on a single poorly-seeded PRNG state alone.
- Mix multiple genuinely varying sources with a proper mixing/hash operation
  — evaluate RTC (if usable), a free-running hardware timer/counter, VBlank
  count, player input timing, and a persistent new-run counter in save data.
- Target at least 32 meaningful bits of entropy in the automatic path.
- Prefer a 64-bit canonical seed if it can be introduced without
  disproportionate save/UI/code complexity; a strong 32-bit seed remains
  acceptable if that is the safer/cleaner architecture — decide and justify,
  don't do both halfway.
- Manual-seed guarantee must hold: same version + same canonical seed + same
  locked generation settings ⇒ same generated run, always.
- Replace the unstable identity key in `src/scrcmd.c:2467`
  (`(u32)ctx->scriptPtr - ROM_START`) with a stable semantic key in the style
  already used by `CreateEnemyEventMon` in `src/pokemon.c:1335`
  (mapGroup/mapNum/lastTalked-derived). Audit for any other ROM-address-keyed
  randomization site.

**3. Generation filters (new).**

Add nine independent pre-run toggles, Gen 1 through Gen 9, default ALL ON.
No existing generation-mask concept exists anywhere in `include/ruleset.h` /
`include/constants/ruleset.h` today — this is new settings-table and
data-table work, following the existing `SettingDescriptor` pattern in
`src/data/ruleset.h`.

- Classification is by the generation that introduced *that specific*
  species/form, never the family's oldest member (Eevee = Gen 1, Sylveon =
  Gen 6; a regional form = the generation that introduced the form; Paradox
  species = Gen 9).
- Applies to: ordinary wilds, trainers, starters, gifts, ordinary statics,
  premium statics, and evolution eligibility.
- Premium statics require both an enabled generation AND Premium-pool
  membership — both conditions, not either.
- Evolution never bypasses the mask: if an evolution's generation is
  disabled, the latest enabled legal stage in that line becomes that path's
  run-long endpoint. This interacts with the existing `GetSpeciesEvolutions`
  override table (`src/randomizer.c` / Phase 7 evolution overhaul, see
  `docs/PHASES.md` Phase 7 and `docs/CLAUDE_HANDOFF.md`); inspect it before
  changing it.
- Invalid/empty candidate pools (e.g. every generation disabled, or a
  disabled-heavy combination that empties a pool `PickReplacementCoreExcluding`
  needs) must fail safely: no hang, no infinite loop, no silent
  re-enabling of a disabled generation. Decide and document the fallback
  (e.g. return vanilla species) and apply it consistently.

**4. Pre-run vs runtime setting classification.**

The infrastructure already exists: `enum SettingLock` in
`include/constants/ruleset.h` (`SETTING_LOCK_NONE` / `_GENERATION` /
`_RULES`), enforced in `src/ruleset.c` (~line 474) via
`IsRulesetSettingEditable`. This is an audit-and-reclassify task, not new
infrastructure, plus adding the new generation-mask settings with the correct
lock class from the start.

- Go through every row in `src/data/ruleset.h`'s descriptor table and confirm
  its `lockClass` actually matches "does this affect deterministic generated
  content." The brief's working list — species pools, generation mask,
  species bans, wild/trainer/learnset/ability/TM/tutor/item randomization
  toggles, premium-generation rules, matching modes — is a starting point,
  not a substitute for checking the real table.
- Pure presentation/QoL settings may remain `SETTING_LOCK_NONE`.
- Target New Game flow (write this into the settings-menu design): New Game
  → Recommended preset by default → optional generation/advanced
  configuration → Gen 1-9 selection → Roll Random Seed or Enter Seed →
  confirmation summary → generation settings lock → deterministic run
  initialization → gameplay. A player choosing Recommended should reach
  gameplay quickly without configuring anything.
- The reproducible run identity is: project/randomizer version + canonical
  seed + locked generation-affecting settings + generation mask. Future Run
  Reports (Phase 11E) must preserve exactly this identity — do not diverge
  from it here.

### Non-goals

- Do not implement Run Reports (Phase 11E).
- Do not touch Premium static slot enumeration/correctness (Phase 11D) beyond
  what the generation mask requires of the static-species path.
- Do not redesign Nuzlocke, AI, or battle-gimmick systems — Phase 11B closed
  that work; re-open only if this phase's own audit finds a genuine defect,
  and say so explicitly rather than silently re-touching it.
- Do not do Phase 12 UX/default work here.

### Acceptance criteria

- Same version + seed + locked settings + generation mask reproduces an
  identical generated run, verified by direct comparison (e.g. same starter
  trio, same first N wild-slot species) across two independent runs from the
  same seed.
- Measured (not assumed) improvement in the profiled hot path, with the
  measurement method and before/after numbers recorded.
- No settings-table row has an obviously wrong lock class after the audit.
- Every generation-mask combination that can produce a valid run does so; the
  ones that legitimately cannot fail safely and are documented as such.
- RAM usage does not regress beyond the current baseline by any material
  amount; no new large permanent EWRAM/IWRAM structure was added merely to
  solve performance.

### Required static/build checks

- `make -j$(sysctl -n hw.ncpu)` clean exit.
- Compare `.map` EWRAM/IWRAM high-water marks before and after (see
  `docs/CLAUDE_HANDOFF.md` for the last known method/baseline).
- `make check` cannot run on this machine (`tools/patchelf` /
  `tools/mgba-rom-test-hydra` fail against this host's `ld`/`clang`) — compile
  individual test TUs instead, e.g.
  `make -j build/emerald-test/test/<name>.o build/emerald-test/src/<name>.o TEST=1`.
- `git diff --check`.

### Runtime/playtest checks

Code inspection cannot establish these — list them explicitly and actually
perform them in mGBA:

- New Game flow end-to-end with the new Gen 1-9 UI, both Recommended-default
  and manual-configuration paths.
- A soft reset mid-run does not change any already-generated result.
- At least one deliberately-restrictive generation combination (e.g. only
  Gen 1 and Gen 9 enabled) actually produces a playable run with no stall.
- Perceptible reduction (or at least no regression) in wild-encounter/
  trainer-battle start latency versus the pre-11C baseline.

### Commit gate

Plan approved → implementation → build passes → diff inspected → runtime
checks above performed and passing → user reviews final diff → commit.

### Copy/paste Claude Code prompt

```
This is Phase 11C from PROMPTS.md: Generation Architecture, Performance,
Seed Quality & Pre-Run Configuration. Read CLAUDE.md, AGENTS.md, docs/SPEC.md
in full, docs/PHASES.md, docs/CLAUDE_HANDOFF.md, and the "Phase 11C" section
of PROMPTS.md before doing anything else.

This is an architectural phase — plan first and get my explicit approval
before writing code. Inspect the actual current implementation yourself
(don't trust any document's description of it, including PROMPTS.md's) for:
- src/randomizer.c's PickReplacementCoreExcluding and every caller on the
  encounter/trainer-creation hot path,
- src/ruleset.c's GetRunSeed/SetRunSeed/RerollRunSeed and how the automatic
  seed is currently produced,
- src/scrcmd.c's ScrCmd_setwildbattle (the ROM-script-address sourceKey) and
  src/pokemon.c's CreateEnemyEventMon (the stable-key pattern to generalize),
- include/constants/ruleset.h's SettingLock enum and src/ruleset.c's
  IsRulesetSettingEditable, and every row of the descriptor table in
  src/data/ruleset.h,
- the current EWRAM/IWRAM headroom from a fresh build's .map file.

Then produce a plan covering: (1) how you will profile/measure the
encounter/battle-start pause and what architecture will fix it without large
permanent RAM allocations, while preserving determinism; (2) the seed-quality
mixing design (sources, target bit width, 32- vs 64-bit canonical seed
decision) and the replacement for the ROM-script-address identity key; (3)
the Gen 1-9 generation-filter data model, its per-species/per-form
classification (never family-rooted), where it must apply, the premium-AND
condition, the evolution-mask-never-bypassed rule, and the safe-failure
behavior for empty/invalid pools; (4) the settings audit reclassifying every
row's lock class plus the new generation-mask rows, and the target New Game
flow. Wait for my approval before implementing.

After I approve: implement, build with make -j$(sysctl -n hw.ncpu), inspect
the full diff, run git diff --check, and tell me exactly what manual mGBA
checks I need to run (new-game flow, seed reproducibility, at least one
restrictive generation combination, perceived encounter-start latency)
before this phase can be committed. Stop there — do not begin Phase 11D.
```

---

## Phase 11D — World / Premium Statics / Missing Core QoL

### Purpose

Finish world/progression correctness and the already-approved-but-not-yet-
implemented gameplay features: full Premium static contract, both bikes,
type icons, restricted Move Reminder, and the remaining Quick Travel/story
streamlining work.

### Prerequisites

Phase 11C closed: generation mask exists and applies to static encounters
(11D's Premium-static work depends on the mask being in place, per SPEC's
"both enabled generation AND Premium membership" rule).

### Exact scope

**1. Premium static contract — full audit, not spot-check.**

"Premium" = Legendary, Mythical, Sub-Legendary-where-classified-Premium,
Ultra Beast, Paradox, and equivalent designated species (`docs/SPEC.md`
"Premium species category"). Current code: `Randomizer_StaticSpecies` and
`Randomizer_RoamerSpecies` in `src/randomizer.c` (~line 524, 550) already
route Premium-tier vanilla species through `POOL_PREMIUM` and
`Randomizer_LegendaryEnabled()` — this exists. What's unverified is
**coverage**: enumerate every actual canonical Premium static slot the ROM
exposes (`CreateScriptedWildMon`/`CreateScriptedDoubleWildMon` callers in
`src/script_pokemon_util.c`, `CreateEnemyEventMon` in `src/pokemon.c`,
roamer setup in `src/roamer.c`, and any map script that sets up a static
encounter) and confirm each of Rayquaza, Groudon, Kyogre, the Regis,
Latios/Latias, Mew, Deoxys, Ho-Oh, Lugia, and any other exposed event/premium
static slot actually routes through this path with a correct, stable
sourceKey (see Phase 11C's sourceKey work — do this after 11C, not before).

- Premium → Premium only; original species remains eligible; slots randomize
  independently; duplicate Premium species across different slots allowed;
  deterministic from seed/settings; original event/location/progression
  behavior preserved.
- Resolve any leftover naming/behavior inconsistency or obsolete unused
  Premium setting found during the audit.

**2. Both bikes.**

Player permanently has Mach and Acro Bike; menu-switchable outside battle;
no repeated Rydel exchange trip. Inspect `src/bike.c` and the field menu
system (`src/ruleset_field.c` already hosts other field-menu additions —
follow its pattern) before designing the switch UI.

**3. Type icons.**

Currently exist only in `src/dexnav.c` and the player battle controller
(`src/battle_controller_player.c:2131`, `LoadTypeIcons`). SPEC requires them
in Party, PC, Summary, starter selection, player battle HUD, and enemy
battle HUD. `LoadTypeIcons` and the dexnav palette/tile setup are the
existing implementations to reuse/generalize rather than reinventing icon
rendering per-screen.

**4. Move Reminder — previously-learned-only mode.**

`SETTING_MOVE_REMINDER_MODE` exists in `include/constants/ruleset.h` and
`src/data/ruleset.h` (~line 304, 520) but has no consumer anywhere in
`src/`. Implement the setting's actual behavior per SPEC ("Move Reminder":
default disabled; Normal Emerald reminder; Free reminder; Previously learned
moves only — restricted to moves the Pokémon actually, legitimately learned
via its randomized level-up learnset, not any move it could theoretically
learn).

**5. Quick Travel / story-streamlining completion.**

`SETTING_QUICK_TRAVEL` currently has exactly one consumer,
`Ruleset_QuickTravelAvailable()` in `src/ruleset_field.c:66`, feeding
`FieldMove_PokeRiderEnabled` in `src/field_move.c:267`. SPEC's "Quick Travel"
section describes a visited-towns quick-travel map, not just Fly-permission
gating — inspect what actually exists (map-select UI, if any) versus what
SPEC describes, and close the gap. Prefer an explicit "Travel there now?"
prompt over an invisible forced teleport where the story would otherwise
force backtracking. Audit `docs/SPEC.md`'s "Map-by-map story cuts" against
actual per-area script state for any other SPEC item not yet implemented.

### Non-goals

- Do not re-touch Nuzlocke/AI/battle-gimmick correctness (11B closed it).
- Do not implement Run Reports (11E).
- Do not do Phase 12 default/preset simplification here.

### Acceptance criteria

- Every enumerated canonical Premium static slot verified Premium→Premium
  with the audit's slot list recorded in the commit/plan.
- Both bikes available and switchable with no Rydel trip required.
- Type icons render correctly (no clipping/palette corruption) in all six
  named surfaces.
- Move Reminder's "previously learned only" mode only offers moves the
  Pokémon's own learn history actually contains.
- Quick Travel matches SPEC's visited-towns description, not just Fly-gating.

### Required static/build checks

- `make -j$(sysctl -n hw.ncpu)` clean exit.
- `git diff --check`.
- Compile-check any touched test TUs per the `make check` limitation above.

### Runtime/playtest checks

- Visit each of the six type-icon surfaces in mGBA and confirm correct,
  non-clipped rendering for a multi-type and single-type Pokémon.
- Trigger at least one enumerated Premium static encounter per major
  category (a Regi, a weather trio member, an event-island legendary) and
  confirm the result is a Premium species.
- Switch bikes from the menu without visiting Rydel.
- Use Move Reminder in "previously learned only" mode on a Pokémon that has
  forgotten a level-up move and confirm only legitimately-learned moves
  appear.
- Use Quick Travel from at least two different unlocked towns.

### Commit gate

Implementation → build → diff inspected → runtime checks above performed →
user review → commit.

### Copy/paste Claude Code prompt

```
This is Phase 11D from PROMPTS.md: World / Premium Statics / Missing Core
QoL. Read CLAUDE.md, AGENTS.md, docs/SPEC.md in full, docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 11D" section of PROMPTS.md first.
Confirm Phase 11C is actually closed (check docs/CLAUDE_HANDOFF.md) before
starting.

Inspect the actual current implementation before planning anything:
src/randomizer.c's Randomizer_StaticSpecies/Randomizer_RoamerSpecies and
every caller (src/script_pokemon_util.c, src/pokemon.c's
CreateEnemyEventMon, src/roamer.c, and any map-script static-encounter
setup) to build the complete enumerated list of canonical Premium static
slots; src/bike.c and src/ruleset_field.c for the bike/field-menu pattern to
extend; src/dexnav.c's LoadTypeIcons and src/battle_controller_player.c's
usage of it as the type-icon implementation to reuse; confirm
SETTING_MOVE_REMINDER_MODE (include/constants/ruleset.h,
src/data/ruleset.h) really has no consumer; and compare
src/ruleset_field.c's Ruleset_QuickTravelAvailable / src/field_move.c's
FieldMove_PokeRiderEnabled against docs/SPEC.md's "Quick Travel" section to
find the actual gap.

This phase does not require a plan-first gate for most of it, but DO show me
your enumerated Premium-static-slot list and your Quick-Travel-gap findings
before implementing those two specifically, since both depend on you having
found the real current state correctly. The bike, type-icon, and Move
Reminder work can be implemented directly following existing patterns.

Preserve all Phase 1-11C behavior. Build after non-trivial changes
(make -j$(sysctl -n hw.ncpu)), inspect the full diff, run git diff --check.
Tell me exactly what to check in mGBA (each Premium slot category, bike
switching, all six type-icon surfaces, Move Reminder previously-learned-only
mode, Quick Travel from two towns) before this can be committed. Stop there
— do not begin Phase 11E.
```

---

## Phase 11E — Victory + Wipe Run Reports / Save Architecture / JSON Export / Optional mGBA Integration

### Purpose

Every strict run ends with a persistent, informational-only Run Report at
Victory (Champion/Hall of Fame) or Wipe (no usable Pokémon remain). Design
and implement the save-side architecture, the report content, and the
external export tooling.

### Prerequisites

Phase 11D closed. The reproducible run identity from Phase 11C (version +
seed + locked settings + generation mask) exists and must be exactly what
gets stored in the report.

### Exact scope

This is a plan-first architectural phase — read `docs/SPEC.md`'s "Terminal
Run Reports" section in full (it is long and detailed; do not summarize it
away) before designing anything.

- **Run identity**: report/schema version, project/ROM version,
  randomizer/ruleset version, canonical seed, preset/ruleset, locked
  generation settings, Gen 1-9 mask, player name where useful, result, play
  time, progression state, unique report ID. This must match Phase 11C's
  reproducible-run-identity definition exactly.
- **Victory team**: complete Hall-of-Fame team with the full per-Pokémon
  attribute list in SPEC (slot, species/form, nickname, gender, shiny,
  level, EXP, types, Nature, ability, held item, moves+PP, IVs, EVs,
  calculated stats, current HP/status, friendship, Ball, met/caught
  level/location, Nuzlocke encounter metadata, alive/dead/legal state).
- **Wipe team/context**: equivalent per-Pokémon detail, snapshotted **before**
  run-loss cleanup destroys relevant state, plus map/location, badges,
  active cap, opponent type/identity/team where available, final KO/cause,
  last major checkpoint, play time.
- **Run statistics**: the SPEC list of inexpensive, robust counters
  (encounters, catches, fails/kills/flees/runs, Dupes skipped, shiny
  encounters/catches, deaths + ordered history, trainer/wild battles, bosses
  defeated, capture attempts, evolutions, Level to Cap uses, badges, final
  cap, play time). Do not add expensive counters merely because they're
  possible.
- **Memory architecture**: EWRAM is already constrained (~91.5% per the
  Phase 11C baseline — re-measure current state before designing this).
  Store compact IDs/counters in save data; do not represent the final JSON
  document as a large in-memory GBA structure. Human-readable names belong
  in the host-side exporter, not duplicated in the GBA save.
- **Export**: versioned human-readable JSON,
  `run_<seed>_victory.json`/`run_<seed>_wipe.json`. Standalone
  `python tools/export_run.py <save-file>` reading `.sav` without modifying
  it — this is new tooling under `tools/`, follow the existing Python tool
  conventions there (see `tools/` directory for precedent, e.g. any existing
  `.py` scripts and their argument/IO conventions).
- **Optional mGBA companion**: automatic export exactly once when a report
  finalizes, with duplicate protection; core ROM must never depend on mGBA;
  the ROM must never claim an external JSON file exists until host-side
  tooling confirms it. If mGBA scripting alone can't safely do the
  conversion, a lightweight host companion process is acceptable.
- Run Report state must never influence future randomization — verify this
  is actually true of whatever storage design is chosen (a shared save
  region reused carelessly could accidentally leak state forward).

### Non-goals

- Do not change randomization systems themselves (only read from them for
  report content).
- Do not implement Phase 12 UX changes to how a report is *presented* beyond
  what's needed to view/export it.

### Acceptance criteria

- A completed Victory run and a completed Wipe run each produce a correctly
  populated, schema-versioned report recoverable after a reset.
- `python tools/export_run.py <save>` produces correct, complete JSON from a
  real `.sav` without modifying that file (verify with a checksum/diff of
  the `.sav` before and after).
- No previous run's report data measurably influences a fresh run's
  randomization (verify by comparing generation results with and without a
  prior finalized report present in save data, same seed).
- No large new permanent EWRAM/IWRAM structure was added for this.

### Required static/build checks

- `make -j$(sysctl -n hw.ncpu)` clean exit.
- `git diff --check`.
- `tools/export_run.py` runs standalone (no ROM/build dependency) against a
  real exported `.sav` and produces valid JSON — verify with Python's own
  `json` module or `jsonschema` if a schema is defined.
- `.sav` byte-identical before/after running the exporter.

### Runtime/playtest checks

- Play to an actual Champion victory and confirm the Victory report
  finalizes and exports correctly.
- Deliberately wipe a run (all Pokémon fainted) and confirm the Wipe report
  captures the pre-cleanup party correctly.
- Reset/reopen the ROM after each terminal outcome and confirm the report
  persists and remains exportable.
- If the optional mGBA companion is implemented: confirm it exports exactly
  once and never falsely claims success before the host confirms the file
  exists.

### Commit gate

Plan approved → implementation → build passes → diff inspected → runtime
checks above performed (both Victory and Wipe paths) → user review → commit.

### Copy/paste Claude Code prompt

```
This is Phase 11E from PROMPTS.md: Victory + Wipe Run Reports / Save
Architecture / JSON Export / Optional mGBA Integration. Read CLAUDE.md,
AGENTS.md, docs/SPEC.md's full "Terminal Run Reports" section (do not
summarize it away — it is long and detailed), docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 11E" section of PROMPTS.md. Confirm
Phase 11D is actually closed first.

This is an architectural phase — plan first and get my explicit approval.
Before planning, inspect: the current save-block structure (gSaveBlock1Ptr /
gSaveBlock2Ptr / gSaveBlock3Ptr and their existing size/versioning
conventions, e.g. how src/ruleset.c versions and migrates
RulesetSettings) as the pattern to follow for a new report save region; the
current EWRAM/IWRAM headroom from a fresh build's .map file; the existing
tools/ directory's Python tooling conventions (if any); and how the run
identity fields (version, seed, locked settings, generation mask) are
currently exposed, since the report's run-identity section must match that
exactly.

Produce a plan covering: the save-side report schema and its size/versioning
strategy (compact IDs and counters only — no full name strings, no
GBA-side giant JSON-shaped structure); exactly when/how Victory and Wipe
snapshots are captured (Wipe specifically must snapshot before run-loss
cleanup runs); the run-statistics counters to track and where they're
incremented without adding expensive per-frame or per-battle overhead;
the tools/export_run.py design (read-only .sav parsing, JSON schema,
filename convention); and, if you judge the optional mGBA companion export
is in scope for this pass, its exactly-once/duplicate-protection design and
the explicit rule that core ROM never depends on it and never falsely claims
export success. Wait for my approval before implementing.

After I approve: implement, build with make -j$(sysctl -n hw.ncpu), inspect
the full diff, run git diff --check, verify tools/export_run.py leaves a
real .sav byte-identical (checksum before/after), and tell me exactly what
to do in mGBA — reach an actual Champion victory, and separately trigger an
actual wipe — to accept this phase. Stop there — do not begin Phase 11F.
```

---

## Phase 11F — Final Phase-11 Correctness Gate

### Purpose

Complete integration audit of everything Phase 11 (11A-11E) was supposed to
deliver. This is a verification pass, not a redesign.

### Prerequisites

Phases 11A-11E all closed per `docs/CLAUDE_HANDOFF.md`.

### Exact scope

Verify, at minimum, each of the following against the actual code and an
actual playthrough — not by re-reading prior session summaries:

- Deterministic run reproduction (version + seed + locked settings +
  generation mask).
- Automatic seed quality (Phase 11C's mixing design actually behaves as
  designed).
- Gen 1-9 filters applied everywhere SPEC/11C required them.
- Disabled-generation evolution blocking (latest enabled stage is the
  endpoint, mask never bypassed).
- Pre-run vs runtime setting classification is correct and consistent.
- No randomizer-caused material encounter/trainer stall under any
  generation-mask combination.
- Ordinary wilds never roll Premium species.
- Every canonical Premium static slot (the Phase 11D enumeration) follows
  Premium→Premium.
- Original Premium species remains eligible at its own slot; duplicate
  Premium statics across slots allowed.
- Nuzlocke/Dupes/Shiny/permadeath semantics (11B's closed work — confirm no
  regression, do not re-litigate).
- Fair-information AI (11B's closed work — confirm no regression).
- No banned battle gimmicks reachable.
- Both bikes work as specified.
- Type icons render in all six required surfaces.
- Move Reminder's modes behave as specified.
- Streamlined world flow (story cuts + Quick Travel) matches SPEC.
- Victory and Wipe Run Reports finalize, persist, and export correctly.
- Manual JSON export (`tools/export_run.py`) works and doesn't mutate the
  save.
- Optional automatic export, if implemented, behaves correctly (exactly
  once, no false claims).
- Memory remains safe (no EWRAM/IWRAM regression beyond what 11C/11E
  measured and accepted).

### Non-goals

- Do not redesign anything found working correctly.
- Fix only real, verified defects. A finding that turns out to already work
  correctly gets recorded as verified, not "fixed."

### Acceptance criteria

Every item in the scope list above has an explicit verified/fixed/
not-applicable determination recorded, with evidence (file/line for static
findings, exact mGBA repro steps for runtime findings).

### Required static/build checks

- `make -j$(sysctl -n hw.ncpu)` clean exit after any fix.
- `git diff --check`.
- Re-run the compile-checked test TUs relevant to anything touched.

### Runtime/playtest checks

A full continuous playthrough exercising: New Game with generation
configuration, at least one wild encounter of each randomized category,
at least one Premium static encounter, a deliberate Nuzlocke death, a
trainer battle against Pro Fair AI, Quick Travel usage, and both a Victory
and (in a separate run) a Wipe ending with report export.

### Commit gate

Audit findings reviewed with the user → any real fix implemented, built, and
diffed → runtime checks above passed → user review → commit. This commit
closes Phase 11 entirely.

### Copy/paste Claude Code prompt

```
This is Phase 11F from PROMPTS.md: Final Phase-11 Correctness Gate. Read
CLAUDE.md, AGENTS.md, docs/SPEC.md in full, docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 11F" section of PROMPTS.md. Confirm
Phases 11A-11E are all actually closed per docs/CLAUDE_HANDOFF.md before
starting.

Do not redesign anything. Go through the full verification checklist in the
"Phase 11F" section of PROMPTS.md item by item, inspecting the actual current
code (and, for anything code inspection can't settle, telling me exactly
what to check in mGBA) for each one. Produce a report with an explicit
verified / fixed / not-applicable determination and evidence for every item
before fixing anything.

Show me that report. For any item you found genuinely broken, get my
approval before fixing it. Fix only what's actually broken — record
everything else as verified working. Build after any fix
(make -j$(sysctl -n hw.ncpu)), inspect the full diff, run git diff --check.
Tell me the exact continuous mGBA playthrough needed to accept this gate.
Stop there — do not begin Phase 12A. This commit closes Phase 11 entirely.
```

---

## Phase 12A — Fun / Accessibility / Recommended Defaults / Settings Simplification / UX / Game Feel

### Purpose

Evaluate the game as a solo replayable product, not a checklist of features,
and tune it for that.

> "The default configuration should be the version you would recommend to
> someone who clicks New Game without reading anything. Settings exist to
> support preferences, not to make the player design the game before they
> can play it."

### Prerequisites

Phase 11F closed — Phase 11 is fully correct before tuning its feel.

### Exact scope

Primary preset/UI should roughly emphasize: Recommended / Modern Emerald /
Randomizer / Custom-Advanced (matches `docs/SPEC.md`'s "Core player-facing
presets" section already).

Permitted: change defaults; simplify settings; hide advanced controls;
remove redundant controls; consolidate settings; improve labels/help text;
improve progressive disclosure; improve pacing/feedback/game feel.

Not permitted: silently violating a core SPEC rule (e.g. don't make
Nuzlocke permadeath optional-by-default under the guise of "fun" without
this being a deliberate, disclosed SPEC-level decision).

Named review questions to actually answer, not skip:

- Is Pro Fair AI, or a slightly less prediction-heavy Smart/Expert Fair
  mode, more fun as the Recommended default?
- Are the settings exposed to ordinary players actually meaningful, or is
  something exposed that nobody meaningfully varies?
- Should IV/EV/detail displays use progressive disclosure (SPEC's default is
  ON for all — is that still right for Recommended, or should it be
  simplified with an "advanced details" toggle)?
- Is New Game configuration concise and comprehensible after Phase 11C's
  flow was implemented?
- Does the Quick Travel/story flow (Phase 11D) feel natural in actual play?
- Are randomization failures (Phase 11C's safe-failure paths) clearly
  communicated to the player, or silent?
- Do repeated runs feel fast to start?

### Non-goals

- Do not touch save architecture or Run Report content (11E is closed).
- Do not do broad technical refactors (12B).

### Acceptance criteria

Each named review question above has an explicit decision recorded (change
made, or explicitly decided to leave as-is and why). Any default change is
consistent with SPEC's "Default preset" description or SPEC is updated to
match a deliberate, disclosed change (get user approval before changing SPEC
here — this phase tunes the game, not the spec, except where the user
explicitly signs off on a SPEC-level default change).

### Required static/build checks

- `make -j$(sysctl -n hw.ncpu)` clean exit.
- `git diff --check`.

### Runtime/playtest checks

- A player unfamiliar with the settings can complete New Game to first
  gameplay in a small number of screens/decisions.
- Subjective game-feel pass: does a full early-game session (through the
  first badge) feel fast, clear, and fun compared to the pre-12A build?

### Commit gate

Findings + proposed default/UX changes reviewed with the user → implemented
→ build passes → diff inspected → runtime playtest → user review → commit.

### Copy/paste Claude Code prompt

```
This is Phase 12A from PROMPTS.md: Fun / Accessibility / Recommended
Defaults / Settings Simplification / UX / Game Feel. Read CLAUDE.md,
AGENTS.md, docs/SPEC.md in full (especially "Default preset" and "Core
player-facing presets"), docs/PHASES.md, docs/CLAUDE_HANDOFF.md, and the
"Phase 12A" section of PROMPTS.md. Confirm Phase 11F is closed first.

This phase evaluates the game as a solo replayable product, not a feature
checklist. Central principle: the default configuration should be the
version you'd recommend to someone who clicks New Game without reading
anything.

Go through each named review question in the "Phase 12A" section of
PROMPTS.md (Pro Fair vs a lighter AI mode as Recommended; whether exposed
settings are actually meaningful; IV/EV progressive disclosure; New Game
concision after Phase 11C; Quick Travel feel; randomization-failure
communication; restart speed) and give me your findings and proposed
changes for each before implementing anything. You may change defaults,
simplify/hide/consolidate settings, and improve labels/pacing/feedback, but
do not silently violate a core SPEC rule — if a change would do that, flag
it and get my explicit sign-off before treating it as approved.

After I approve your findings: implement, build with
make -j$(sysctl -n hw.ncpu), inspect the full diff, run git diff --check,
and play through New Game to the first badge yourself if you can assess it
statically, otherwise tell me exactly what to check. Stop there — do not
begin Phase 12B.
```

---

## Phase 12B — Final Technical & Presentation Polish

### Purpose

Focused cleanup once gameplay/UX is settled.

### Prerequisites

Phase 12A closed.

### Exact scope

- Compiler warnings.
- Dead code.
- Obsolete settings (anything Phase 12A's simplification left orphaned).
- Redundant caches (including anything Phase 11C's optimization work left
  behind that turned out unnecessary).
- RAM headroom re-check against the running baseline.
- Save compatibility across the version bumps made in 11C/11D/11E.
- Malformed text/UI, clipping, transition issues.
- Performance regressions.
- Remaining linker/build concerns where practically actionable — the known
  ELF RWX LOAD-segment warning (present since at least the Phase 10 baseline
  build, per `docs/PHASE10_PROMPTS.md`'s Arc 0 build record) is named here as
  the one to actually investigate, not just re-note.
- Clean full build; diff/static checks.

### Non-goals

Do not perform broad speculative refactors. This is cleanup, not redesign.

### Acceptance criteria

Clean `make clean && make -j$(sysctl -n hw.ncpu)` build with no new
warnings introduced by this phase's own changes (pre-existing warnings may
be investigated and either fixed or explicitly left with a recorded reason).
The RWX LOAD-segment warning has an explicit disposition (fixed, or
determined benign with the reason recorded).

### Required static/build checks

- `make clean && make -j$(sysctl -n hw.ncpu)` clean exit.
- `git diff --check`.
- `.map` EWRAM/IWRAM comparison against the last recorded baseline.

### Runtime/playtest checks

Spot-check any UI surface touched for clipping/text issues; confirm a save
from before this phase still loads correctly (save-compatibility check).

### Commit gate

Implementation → clean full build → diff inspected → save-compatibility spot
check → user review → commit.

### Copy/paste Claude Code prompt

```
This is Phase 12B from PROMPTS.md: Final Technical & Presentation Polish.
Read CLAUDE.md, AGENTS.md, docs/SPEC.md, docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 12B" section of PROMPTS.md. Confirm
Phase 12A is closed first.

This is cleanup, not redesign — do not perform broad speculative refactors.
Do a full make clean && make -j$(sysctl -n hw.ncpu) build and inspect every
warning. Address: dead code, obsolete settings left over from Phase 12A's
simplification, redundant caches left over from Phase 11C's optimization
work if any turned out unnecessary, RAM headroom versus the last recorded
.map baseline, save compatibility across this project's version-bump
history, and any malformed text/UI/clipping/transition issue you find.
Specifically investigate the known ELF "LOAD segment with RWX permissions"
linker warning and give it an explicit disposition — fixed, or determined
benign with a recorded reason.

Build clean, inspect the full diff, run git diff --check. Confirm a
pre-existing save still loads correctly after your changes. Tell me exactly
what to spot-check in mGBA if anything UI-facing changed. Stop there — do
not begin Phase 13A.
```

---

## Phase 13A — Final Documentation / Repository Cleanup

### Purpose

Make repository state accurately describe the finished product.

### Prerequisites

Phase 12B closed.

### Exact scope

- Update `docs/SPEC.md`, `docs/PHASES.md`, `PROMPTS.md`,
  `docs/CLAUDE_HANDOFF.md`, `CLAUDE.md`/`AGENTS.md` where appropriate to
  reflect the now-actually-finished product.
- Update build/run/export instructions (`INSTALL.md` and any tool docs under
  `tools/`) for anything Phase 11E's export tooling added.
- Update user-facing documentation where relevant.
- Remove or explicitly mark stale development handoffs. In particular,
  `AI_HANDOFF.md` is known historical/stale (it is untracked/gitignored —
  see `docs/CLAUDE_HANDOFF.md`) and must never be presented as current
  state; confirm it's still ignored and still doesn't claim current status,
  or delete it if the user agrees it no longer has archival value.
- Ensure implementation and documentation agree — this is the point to
  actually re-verify every SPEC section against real code once more, not
  just trust the running phase-by-phase record.

### Non-goals

No new features. No gameplay code changes except a documentation-accuracy
fix (e.g. correcting a stale SPEC sentence) — anything that changes actual
behavior belongs to an earlier phase and should not be smuggled in here.

### Acceptance criteria

Every SPEC.md section has been checked against real code at least once
since the last feature phase touched it, with no known contradiction left
unrecorded. `docs/PHASES.md` and `PROMPTS.md` accurately reflect that Phases
1-12B are complete and 13A/13B are the only remaining work.

### Required static/build checks

- `git diff --check`.
- No gameplay build required unless a source file was touched for a
  documentation-driven correctness fix, in which case
  `make -j$(sysctl -n hw.ncpu)` must still pass.

### Runtime/playtest checks

None required unless a code correctness fix was made alongside the
documentation update, in which case that specific fix needs the normal
mGBA verification for whatever it touched.

### Commit gate

Documentation diff reviewed with the user → commit.

### Copy/paste Claude Code prompt

```
This is Phase 13A from PROMPTS.md: Final Documentation / Repository
Cleanup. Read CLAUDE.md, AGENTS.md, docs/SPEC.md in full, docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 13A" section of PROMPTS.md. Confirm
Phase 12B is closed first.

This is documentation-only unless you find a genuine documentation/code
contradiction that requires a small correctness fix — flag any such case to
me before changing source. Go through docs/SPEC.md section by section and
verify each one against the actual current implementation (not against
prior session summaries). Update docs/SPEC.md, docs/PHASES.md, PROMPTS.md,
docs/CLAUDE_HANDOFF.md, CLAUDE.md/AGENTS.md, INSTALL.md, and any tools/
documentation to accurately describe the now-complete product. Confirm
AI_HANDOFF.md is still gitignored/untracked and doesn't masquerade as
current state; if you and I agree it no longer has archival value, delete
it — otherwise leave it alone with the existing warning in
docs/CLAUDE_HANDOFF.md.

Show me the full documentation diff, run git diff --check, and report any
contradiction you found between SPEC and actual code that you did not
resolve unilaterally. Stop there — do not begin Phase 13B.
```

---

## Phase 13B — Final Release Gate

### Purpose

Final release audit. No new features.

### Prerequisites

Phase 13A closed.

### Exact scope

Verify:

- Clean build.
- Clean static checks.
- No unresolved critical TODOs.
- Safe RAM usage.
- Save compatibility/migration behavior.
- Reproducibility (Phase 11C's identity guarantee, re-confirmed).
- All generation-filter combinations fail safely.
- Premium restrictions/statics (Phase 11D's enumeration, re-confirmed).
- Nuzlocke correctness (11B, re-confirmed no regression).
- Fair AI (11B, re-confirmed no regression).
- Report persistence/export (11E, re-confirmed).
- Defaults/UX (12A, re-confirmed).
- Story completion.
- Hall of Fame victory.
- Wipe flow.
- Replay/new-run independence — no previous-run species/team state
  influences later randomization.

Runtime/manual tests that cannot be proven statically must be explicitly
listed here and actually performed, not deferred again.

### Non-goals

No new features. Fix only real blockers found; this is not a redesign pass.

### Acceptance criteria

Every item in the scope list above has a recorded pass/fail with evidence.
When all blockers are closed: **PRODUCT COMPLETE**. There is no Phase 14.

### Required static/build checks

- `make clean && make -j$(sysctl -n hw.ncpu)` clean exit.
- `git diff --check`.
- Full `grep`-level TODO/FIXME sweep for anything critical left unresolved.

### Runtime/playtest checks

A complete, continuous mGBA playthrough from New Game through either
Champion victory or a deliberate wipe, exercising every item in the scope
list above that runtime alone can prove, plus starting a second fresh run
afterward to confirm the first run's completion left no trace in the new
run's randomization.

### Commit gate

Audit findings reviewed with the user → any real blocker fixed, built, and
diffed → full playthrough passed → user review → commit/tag as release. This
is the terminal phase.

### Copy/paste Claude Code prompt

```
This is Phase 13B from PROMPTS.md: Final Release Gate — the terminal phase.
Read CLAUDE.md, AGENTS.md, docs/SPEC.md in full, docs/PHASES.md,
docs/CLAUDE_HANDOFF.md, and the "Phase 13B" section of PROMPTS.md. Confirm
Phase 13A is closed first.

No new features. This is a final audit. Go through every item in the "Phase
13B" scope list in PROMPTS.md and give each an explicit pass/fail with
evidence — static inspection where possible, and an exact list of what
needs manual mGBA verification where it isn't. Fix only genuine blockers
you find; do not redesign anything.

Show me the full audit report before fixing anything. For any real blocker,
get my approval, then fix, build with make -j$(sysctl -n hw.ncpu), and
re-verify. Once every item passes and I've completed the full mGBA
playthrough you specify (New Game through either Champion victory or a
deliberate wipe, then a second fresh run confirming no cross-run
contamination), tell me explicitly that the product meets PRODUCT COMPLETE
criteria. Do not commit or tag anything without my review.
```
