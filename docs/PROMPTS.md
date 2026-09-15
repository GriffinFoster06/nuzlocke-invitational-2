# PROMPTS.md — Claude Code Development Runbook

> Current project: `pokeemerald-expansion`
>
> This file is the operational runbook for the remaining development work.
> `docs/SPEC.md` is the authoritative product-behavior specification.
> `docs/CLAUDE_HANDOFF.md` is the authoritative current-state handoff.
>
> The old Phase-10 prompt bank is historical. Do not use it to infer current work.

---

# 0. Current roadmap

As of the handoff that produced this file:

```text
Phases 1–10   COMPLETE
Phase 11A     COMPLETE
Phase 11B     COMPLETE

Phase 11C     IMPLEMENTED + BUILD-VERIFIED
              RUNTIME ACCEPTANCE + COMMIT STILL REQUIRED

Targeted AI switching remediation
              AFTER 11C COMMIT, BEFORE 11D

Phase 11D     NEXT FEATURE PHASE
Phase 11E     Run Reports / save / JSON / optional mGBA integration
Phase 11F     Final Phase-11 correctness gate

Phase 12      Final player experience + technical/presentation polish
Phase 13      Final documentation + repository cleanup + release gate

PRODUCT COMPLETE
```

There is no Phase 14.

Phase letters such as `11E-A` / `11E-B`, `12A` / `12B`, and `13A` / `13B` are intentionally removed from the roadmap. Each numbered phase is one phase. Claude may split a phase into implementation units or fresh sessions only when that materially improves correctness or context management.

---

# 1. Claude Code operating rules

These rules apply to every prompt below unless the phase explicitly overrides them.

## 1.1 Source of truth

Use this authority order:

1. actual current repository state;
2. `docs/SPEC.md` for accepted product behavior;
3. `docs/CLAUDE_HANDOFF.md` for current implementation state;
4. this `PROMPTS.md` for execution scope/order;
5. `docs/PHASES.md` for roadmap/history.

Historical prompt files and stale local handoffs are not current authority.

Do not use `AI_HANDOFF.md` as current-state evidence.

## 1.2 Minimal startup context

At the start of a fresh phase/session:

```bash
git status --short
git log -3 --oneline
```

Then read:

- `CLAUDE.md`;
- `docs/CLAUDE_HANDOFF.md`;
- only the relevant sections of `docs/SPEC.md`;
- only the files required for the current task.

Do not automatically read the entire repository, the entire Git history, every historical prompt file, or every completed phase.

If the relevant SPEC section is not obvious, inspect headings first and then read the matching section.

## 1.3 Implementation-phase discipline

Implementation phases are not whole-project audits.

Assume completed and committed earlier phases are correct unless:

- the current task directly depends on a specific earlier behavior and its interface must be understood; or
- there is concrete new runtime/code evidence of a defect.

Do not systematically re-check historical acceptance criteria.

If an already-required capability exists, inspect only enough to reuse it safely and move on.

If an unrelated possible defect is discovered incidentally:

- record it briefly;
- continue the current phase;
- fix it only if it blocks the current work or is an immediate correctness/save-safety issue.

Broad cross-system checking is intentionally reserved for Phase 11F and Phase 13.

## 1.4 Investigation budget

Prefer the shortest repository path that can establish what is needed.

Good:

- targeted `rg`;
- opening a known implementation file;
- following one call chain;
- inspecting one relevant script family;
- focused tests.

Avoid unless the phase is an explicit audit:

- repo-wide speculative searches;
- re-reading unrelated subsystems;
- enumerating every passing behavior;
- proving the same fact multiple ways;
- reviewing code that the current change cannot affect.

When choosing an approach, make a reasonable decision and proceed. Revisit it only if new evidence directly contradicts it.

## 1.5 Subagents

Default: work directly.

Use subagents only when tasks:

- are genuinely independent;
- can run in parallel;
- benefit from isolated context;
- do not need shared mutable state.

Do not use subagents for:

- simple exploration;
- single-file/small multi-file work;
- sequential call-chain tracing;
- implementation that must share context;
- checks a direct `rg`/read can answer quickly.

Unless a phase says otherwise, use at most two subagents.

The main agent owns architectural decisions, edits, integration, diff review, and the final result.

## 1.6 Plan mode

Use Plan mode when architecture or scope choices materially affect multiple systems.

A useful plan is concise and implementation-oriented:

- relevant current architecture;
- proposed changes;
- files/functions;
- dependencies/risks;
- focused verification.

Do not restate the SPEC at length.
Do not narrate repository exploration.
Do not produce a second giant audit before implementation.

Once a plan is approved, implement in the same Claude Code session unless Claude has a concrete context-management reason to split.

## 1.7 Phase splitting

Treat each numbered phase as one roadmap phase.

Claude may choose:

- one implementation unit; or
- multiple ordered implementation units.

Split only if it materially improves correctness, architectural safety, or context management.

Do not create artificial `A/B` roadmap phases.

If multiple units are useful:

- give each a descriptive unit name;
- state its dependency;
- keep the overall phase acceptance criteria unified;
- use a fresh session between units only when context isolation is genuinely useful.

Do not mark the phase complete until all units are complete.

## 1.8 Avoid over-engineering

Make the smallest robust change that satisfies the accepted design.

Do not:

- add speculative future flexibility;
- refactor unrelated code;
- invent new settings;
- create helper abstractions for one-time work;
- add defensive checks for impossible internal states;
- rewrite working systems for style.

Tests verify the implementation; do not hard-code behavior merely to satisfy tests.

Temporary scripts/files used for investigation must be removed before the phase finishes unless they are useful permanent project tools.

## 1.9 Verification discipline

During implementation, run focused checks for what changed.

After non-trivial code changes:

```bash
make -j$(sysctl -n hw.ncpu)
```

At a phase completion boundary, unless the phase says otherwise:

```bash
git diff --check
git status --short
git diff
```

Use `make clean && make -j$(sysctl -n hw.ncpu)` only at major integration/release gates or when incremental-build correctness is in doubt.

Do not repeatedly rebuild after every tiny edit.

Do not add generic “double-check everything” loops. The dedicated audit phases exist for broad verification.

`make check` is known not to work on this machine unless that limitation has since been fixed. Do not claim those tests passed when they did not run. Compile-check targeted test translation units or use the available test path when appropriate.

## 1.10 Runtime/manual verification

Static analysis is not a substitute for emulator behavior.

When a changed behavior needs mGBA verification:

- give a short checklist containing only behaviors changed in this phase;
- stop before commit if the runtime check is a real acceptance gate;
- do not turn the checklist into a replay of the whole game.

## 1.11 RAM discipline

RAM remains constrained.

Recent known Phase-11C build:

```text
EWRAM 241088 B / 256 KB  (91.97%)
IWRAM  28412 B /  32 KB  (86.71%)
ROM   26748560 B /  32 MB (79.72%)
```

Re-measure after relevant changes.

Prefer:

- ROM/static metadata;
- compact IDs/bitsets;
- small bounded caches;
- save/flash storage;
- host-side expansion for reporting.

Avoid giant permanent EWRAM/IWRAM structures.

## 1.12 Commit behavior

Do not commit automatically before any explicit manual/runtime gate for the phase has passed.

When the gate has passed, use the shared commit prompt near the end of this file.

---

# 2. Immediate task — finish Phase 11C

Phase 11C implementation is already complete and build-verified.

Do not rerun its architecture review or reimplement it.

The remaining acceptance checks are:

- New Game wizard works end-to-end with the Recommended/default path;
- manual configuration path works;
- zero-enabled-generations cannot be confirmed;
- a restrictive generation mask such as Gen 1 + Gen 9 produces a playable run without disabled-generation leaks;
- soft reset does not change already-determined generated results;
- wild/trainer battle startup feels materially faster than before;
- the Nuzlocke retry path still reuses its intended settings and is not forced through the fresh-New-Game wizard.

After those pass, paste this into the existing Claude session:

```text
The Phase 11C mGBA runtime acceptance checks passed.

Finalize Phase 11C only.

Run:
- git status --short
- git diff --check
- inspect the complete current diff
- make -j$(sysctl -n hw.ncpu)

If there is no unresolved Phase 11C defect, commit and push the intended Phase 11C changes with:

git add -A
git commit -m "Phase 11C: optimize generation and pre-run configuration"
git push

Then run:
git status --short
git log -1 --oneline

Do not begin the AI remediation or Phase 11D.

Report only:
- commit hash;
- push result;
- build result;
- EWRAM/IWRAM/ROM usage;
- final working-tree state.
```

After a successful commit, start a fresh session for the targeted AI task below.

---

# 3. Targeted AI switching remediation

This is a narrow bug investigation prompted by runtime behavior.

It is not a Phase 11B re-audit and is not a new numbered roadmap phase.

## Suggested Claude Code setup

```text
Model: Claude Opus 5
Effort: medium
Plan mode: ON initially
Subagents: none by default
```

Medium effort is intentional: this is a bounded diagnosis, not a whole-repository architecture task.

## Prompt

```text
Perform a narrow diagnose-then-fix pass on trainer AI difficulty and switching behavior.

<context>
Phase 11B's fair-information AI work is complete and should not be broadly re-audited.

A new runtime observation needs investigation:

The AI sometimes appears to switch from Pokémon A into a better-positioned Pokémon B, then on the next turn switch straight back into the worse Pokémon A without an obvious board-state reason. Repeated switching can cause it to spend turns doing nothing and become easy to beat.

This may be a real logic defect or may be an unrepresentative battle. Determine which from the relevant code. Do not change behavior just because of the anecdote.

The project also needs a player-facing AI difficulty named Standard whose semantics are original Pokémon Emerald / vanilla trainer AI behavior.
</context>

<scope>
Read only what is needed to understand:
- the AI difficulty setting/descriptor;
- difficulty-to-AI-flags mapping;
- voluntary switch decision logic;
- switch-in candidate scoring/selection;
- forced replacement selection after a faint;
- directly relevant battle-history/state;
- the AI difficulty/switching section of docs/SPEC.md.

Do not audit Nuzlocke, Gen 9 mechanics, trainer randomization, doubles generally, hidden-information fairness generally, or unrelated battle systems.

Do not launch broad subagents.
</scope>

<requirements>
1. Standard difficulty
- Determine what the current player-facing Vanilla mode actually does.
- If it already maps to true vanilla Emerald behavior, preserve the implementation and rename only the player-facing label/documentation to Standard.
- If it does not, make Standard use the original Emerald AI behavior as faithfully as the current engine supports.
- Internal enum names may remain VANILLA if renaming would create needless save/API churn.
- Standard must not inherit custom smart-switching/prediction enhancements intended for harder modes.

2. Difficulty progression
Preserve/reuse existing behavior where it already works.

Conceptually:
- Standard: vanilla Emerald behavior.
- Improved: modestly better decisions; sensible switching is allowed.
- Expert: meaningfully stronger matchup/positioning/switch-in reasoning.
- Pro Fair: strongest fair-information reasoning; sophisticated switching/pivoting is allowed.

Do not rewrite working AI merely to make these tiers look different on paper.

3. Voluntary switching
Trace how the AI chooses between staying in and voluntarily switching, and how it selects the replacement.

Look specifically for a real mechanism that could cause unjustified A -> B -> A oscillation, such as:
- inconsistent current-mon and candidate scoring;
- a switch-in selector whose choice the next-turn switch-out evaluator immediately rejects;
- negligible score differences causing switches;
- no cost for giving up the turn;
- stale target/prediction state;
- one criterion choosing B and a different incompatible criterion immediately preferring A.

Do not add an arbitrary cooldown unless the existing architecture makes that the cleanest correct solution.

Desired behavior:
A voluntary switch should require a meaningful expected positional gain after accounting for the lost turn.

After intentionally choosing B because B is better, the AI should ordinarily be willing to use B instead of immediately undoing its own decision.

Immediate switching back remains valid when something materially changed: player switch, newly revealed information, status/stat changes, trapping, Yawn/Perish effects, hazards, legitimate tactical pivoting, or another real board-state change.

4. Switch-in choice
Higher difficulties should intentionally select among legal switch-ins for both:
- voluntary switches;
- forced replacement after a faint.

Reuse existing scoring infrastructure where possible.

Relevant factors may include type matchup, legitimately known/revealed moves, immunities/resistances, expected damage, speed, ability interactions, status, HP, hazards, immediate KO risk, offensive pressure, and preservation value.

Standard retains vanilla Emerald replacement behavior.

Do not improve the randomized team itself; improve only how the AI plays what it rolled.

5. Fairness
Do not regress Phase 11B fair-information restrictions.

Harder fair modes still may not directly read unrevealed moves/items/abilities, IVs/EVs/Nature, unseen reserves when not legitimately known, current player input, future RNG, or hidden randomizer data.

6. Focused regression coverage
If a real switching defect exists, add focused tests when practical, particularly:
- A -> B for a meaningful reason does not become immediate B -> A with unchanged state;
- a material state change can legitimately cause a switch back;
- higher difficulty chooses a materially better legal replacement;
- Standard retains vanilla behavior.

Do not create tests for promises the architecture does not make.
</requirements>

<plan_output>
Keep the Plan-mode response concise.

Report only:
1. current behavior of each AI difficulty relevant to this task;
2. whether current Vanilla really equals vanilla Emerald behavior;
3. whether the reported switch oscillation has a real code-level cause;
4. exact minimal changes needed, if any;
5. focused tests/runtime checks.

If no behavioral defect exists, say so and do not invent one.

If a fix is needed, wait for approval and then implement it in this same session.
</plan_output>

<implementation_rules>
After approval:
- implement only the needed correction;
- preserve unrelated Phase 11B AI work;
- build once after the substantive edits;
- run focused AI tests that are actually available;
- run git diff --check;
- inspect only this task's diff.

Give a short mGBA checklist for Standard behavior and switching.

Do not begin Phase 11D.
</implementation_rules>
```

Suggested commit message after runtime checks pass:

```text
Fix AI switching and standard difficulty behavior
```

---

# 4. Phase 11D — World, Premium statics, and remaining core QoL

## Goal

Implement the remaining known world/content/QoL requirements without re-auditing the whole campaign.

This phase is implementation-first.

## Suggested Claude Code setup

```text
Model: Claude Opus 5 for brief plan, then Sonnet 5 for implementation if desired
Effort: medium
Plan mode: ON
Subagents: default 0; at most 2 if Premium-static enumeration and UI/QoL work are genuinely independent
```

## Prompt

```text
Perform Phase 11D — World, Premium Statics, and Remaining Core QoL.

<context>
Phases 1-11C are complete. The targeted AI-switching remediation has also been resolved.

This is an implementation phase, not a campaign-wide audit.

Use docs/SPEC.md as product authority and docs/CLAUDE_HANDOFF.md as current-state authority.
</context>

<startup>
Read:
- CLAUDE.md;
- docs/CLAUDE_HANDOFF.md;
- only the SPEC sections for Premium/static encounters, post-Rayquaza progression, bikes, type icons, Move Reminder, Quick Travel/story streamlining;
- directly relevant implementation files/scripts.

Do not trace every Emerald story arc unless a specific change below requires it.
Do not re-check completed Nuzlocke, AI, seed, generation-filter, trainer, or item systems.
</startup>

<scope>
Implement/finalize these known remaining areas:

1. Premium static encounters
Enumerate the canonical Premium/event static encounter slots actually exposed by this ROM and make every one obey the final contract.

For each actual Premium static slot:
- it randomizes;
- replacement is Premium-only;
- generation mask is respected;
- original species remains eligible;
- different slots randomize independently;
- duplicate Premium species across separate slots are allowed;
- result is deterministic from run identity;
- original event/location/progression behavior remains intact.

Premium includes the project's configured Legendary/Mythical/Sub-Legendary/Ultra Beast/Paradox/equivalent restricted categories.

Known examples to locate where present include Rayquaza, Groudon, Kyogre, the Regis, Latios/Latias Premium encounters, Mew, Deoxys, Ho-Oh, Lugia, and other exposed Premium/event statics.

The enumeration is necessary because it defines implementation coverage. Do not turn it into a general campaign audit.

Ordinary wilds and forbidden trainer classes must remain unable to roll Premium species. Elite Four may roll them naturally. Wallace retains the accepted guarantee.

Remove or resolve obsolete/misleading Premium settings if they remain, especially any setting that claims to control duplicate Premium statics when duplicates are unconditionally allowed by the accepted design.

2. Post-Rayquaza Premium progression
Implement/finish only the relevant progression hooks required by SPEC:
- exactly one guaranteed Master Ball immediately before the intended post-Rayquaza Premium opportunities;
- Master Ball is never sold;
- intended Regi access;
- intended Weather Institute Groudon/Kyogre content;
- intended Slateport legendary-map functionality;
- intended supported Premium/event-island access.

Inspect the relevant scripts directly. Do not trace unrelated story arcs.

3. Both bikes
Give permanent access to both Mach and Acro Bike behavior without repeated Rydel swapping.
Provide a sensible outside-battle menu switch.
Preserve map mechanics requiring either bike.

4. Type icons
Finish type icons beside Pokémon names on the required surfaces:
- Party;
- PC;
- Summary;
- starter selection;
- player battle HUD;
- enemy battle HUD.

Reuse existing assets/infrastructure. Avoid duplicate assets and unnecessary UI rewrites.

5. Move Reminder
Free reminder access must expose only randomized level-up moves the Pokémon legitimately reached previously.
It must not expose future unreached generated moves.

6. Quick Travel / remaining story QoL
Finish the actual accepted Quick Travel behavior rather than treating early Fly access alone as complete.

Implement only the missing behavior needed to:
- reduce already-cleared backtracking;
- preserve first traversal of meaningful encounter-bearing routes;
- avoid progression skips;
- provide optional direct travel prompts where SPEC calls for them.

Do not broadly rewrite story scripting.

7. Hall of Fame independence
Ensure no remaining Tournament/cross-run species exclusion/final-team-lock behavior affects later runs.
If this is already absent, a targeted search is enough; do not build another audit around it.
</scope>

<planning>
Produce a concise plan containing:
- missing implementations found;
- exact files/scripts/functions;
- Premium-static slot list;
- implementation order;
- any real save/RAM/UI risk;
- focused runtime checks.

If one listed feature is already correctly implemented, say "already implemented" and move on. Do not spend context proving it repeatedly.

Claude may split Phase 11D into implementation units if that materially helps, but keep them under Phase 11D.
</planning>

<implementation>
After approval, implement the plan.

Use focused builds while coding. At completion:
- make -j$(sysctl -n hw.ncpu)
- git diff --check
- inspect the Phase 11D diff
- report current EWRAM/IWRAM/ROM if affected materially.

Give only runtime checks for behavior changed in this phase.

Do not begin Phase 11E.
</implementation>
```

Suggested phase commit message:

```text
Phase 11D: finalize world premium encounters and core QoL
```

---

# 5. Phase 11E — Terminal Run Reports, save persistence, JSON export, and optional mGBA integration

## Goal

Implement the complete Run Report system as one numbered phase.

Claude decides whether the implementation is best done in one unit or multiple ordered units.

## Suggested Claude Code setup

```text
Model: Claude Opus 5
Effort: high for architecture
Plan mode: ON
Subagents: default 0; at most 2 for independent save-side vs host-tool investigation
```

High effort is justified here because save layout, terminal timing, schema design, and host tooling interact.

## Prompt

```text
Perform Phase 11E — Terminal Run Reports, Save Persistence, JSON Export, and Optional mGBA Integration.

<context>
Phases through 11D are complete.

This is a new system. Do not re-audit unrelated gameplay.

docs/SPEC.md's Terminal Run Reports section is authoritative.
Current RAM is tight, so the GBA-side representation must be compact.
</context>

<startup>
Read:
- CLAUDE.md;
- docs/CLAUDE_HANDOFF.md;
- only the SPEC Run Report/export sections;
- save structures/versioning/checksum code;
- terminal Victory/Hall-of-Fame path;
- terminal strict-wipe path;
- existing Nuzlocke counters/death data that can be reused;
- repository tooling relevant to .sav parsing;
- mGBA integration docs/code only if needed for automatic export.

Do not read the rest of the battle/world/randomizer code unless a report field requires a specific hook.
</startup>

<required_behavior>
1. Terminal outcomes
Every strict run finalizes one report on:
- VICTORY: Champion/Hall of Fame;
- WIPE: no usable Pokémon remain.

Wipe snapshotting must occur before destructive cleanup loses relevant state.
Victory snapshotting must represent the final Hall-of-Fame team.
Finalization must be idempotent.

2. Run identity
Persist compact values sufficient to report:
- report schema version;
- project/ROM version identifier;
- randomizer/ruleset version;
- canonical seed;
- preset/ruleset;
- locked generation settings;
- Gen 1-9 mask;
- player name where useful;
- terminal result;
- play time;
- progression state;
- unique report ID.

Report state must never affect randomization.

3. Final Pokémon snapshot
Persist the useful non-reconstructable player-relevant data required by SPEC for the Victory team or final Wipe party.

Prefer compact IDs and source values over duplicated strings or a giant JSON-shaped runtime object.

Include where meaningful/available:
- slot/species/form/nickname/gender/shiny;
- level/EXP;
- Nature/ability/held item;
- moves/PP/PP bonuses;
- IVs/EVs;
- enough data for final stats;
- HP/status/friendship where meaningful;
- Poké Ball;
- met/caught and Nuzlocke encounter metadata;
- alive/dead/legal state.

Do not persist values that can be reliably derived later if save space is better used elsewhere.

4. Wipe context
Capture reliable, inexpensive context such as:
- map/location;
- badges/cap/progression checkpoint;
- last major boss;
- opponent category/trainer ID;
- opponent team or final cause only where reliably available;
- play time.

Omit unreliable fields rather than inventing them.

5. Run statistics
Track the smallest robust set of useful comparison stats that must be collected live, including the accepted categories in SPEC such as encounters/catches/failures/flees/runs/dupes/shinies/deaths/battles/bosses/evolutions/Level-to-Cap uses/balls or capture attempts where cheap/badges/play time.

Reuse existing counters/events where possible.
Do not add meaningless counters because they are easy.

6. Death history
Use a bounded compact representation if save architecture safely permits it.
Do not risk save integrity for unlimited history.

7. Persistence
The finalized report must survive normal save, reset, and emulator close.

Prefer retaining the latest finalized report long enough for manual export even when another attempt begins, if actual save architecture permits this safely.

Make the behavior explicit rather than relying on accidental New Game clearing semantics.

8. Unique report ID
Create an ID suitable for duplicate-export protection.
It must distinguish separate terminal reports even if the same manual seed is replayed.
It must not influence generation.

9. Manual JSON exporter — required
Implement a repository tool with a simple interface such as:

python tools/export_run.py <save-file>

It must:
- read an ordinary .sav;
- never modify the source save;
- validate enough save/report structure to fail clearly;
- emit versioned human-readable JSON;
- expand stable IDs into names where useful;
- avoid silent overwrite.

Preferred filenames:
run_<seed>_victory.json
run_<seed>_wipe.json

Use both ID and name where helpful for machine readability + human readability.

10. JSON structure
Use an explicit stable schema organized roughly around:
- schema/game identity;
- run identity/settings;
- outcome;
- progression;
- statistics;
- final_party or hall_of_fame_team;
- death_history;
- wipe_context where applicable.

Do not expose only opaque engine numbers.

11. Automatic mGBA export
Attempt the simplest robust supported mGBA/host integration that can export a newly finalized report exactly once.

Core ROM gameplay must never depend on mGBA.
Manual .sav export is mandatory regardless.

Prefer host-side duplicate tracking by report ID rather than mutating gameplay state solely to acknowledge export.

If mGBA scripting alone is insufficient, a minimal companion process is acceptable.

Do not make the ROM claim external JSON export succeeded unless host-side integration can genuinely confirm it.
"Run Report saved" is safe because the canonical report exists in save data.
</required_behavior>

<architecture_rules>
EWRAM is already highly constrained.

Prefer:
live cheap counters
+ compact finalized save records
+ host-side JSON expansion.

Avoid:
- persistent strings;
- duplicate full Pokémon objects;
- large EWRAM report buffers;
- a GBA-side JSON document.

Inspect actual save capacity/versioning before choosing structures.

Use existing save migration patterns where possible.
</architecture_rules>

<phase_splitting>
This is one phase.

During Plan mode, decide whether implementation should remain one unit or be split into ordered implementation units.

Split only if the actual architecture makes separation materially safer or clearer.

A natural split might be ROM/save work versus host exporter/integration, but do not force that split if the implementation is better kept together.

If multiple fresh sessions are recommended, define exactly what the first unit must commit/document so the next session can continue from git + docs/CLAUDE_HANDOFF.md without conversational context.
</phase_splitting>

<plan_output>
Produce a concise architecture plan covering:
- exact save space/layout impact;
- new structures and sizes;
- terminal hooks;
- statistics/death instrumentation points;
- report ID/finalization behavior;
- save migration/persistence behavior;
- exporter parser/schema;
- automatic-export architecture;
- implementation-unit split only if useful;
- focused static/runtime tests.

Do not audit unrelated systems.
Do not restate the entire SPEC.
</plan_output>

<implementation_and_tests>
After approval, implement the selected unit(s).

Required focused verification includes:
- Victory finalization;
- Wipe finalization before cleanup;
- save/reload and emulator restart persistence;
- report content correctness;
- replay/new-run independence;
- manual exporter;
- source .sav unchanged byte-for-byte by manual export;
- no-report/malformed/unsupported-version errors;
- collision/no-overwrite behavior;
- automatic export and duplicate prevention if shipped.

At the phase boundary run:
make clean
make -j$(sysctl -n hw.ncpu)
git diff --check

Inspect the Phase 11E diff and report EWRAM/IWRAM/ROM.

Do not begin Phase 11F.
</implementation_and_tests>
```

Suggested phase-level commit message:

```text
Phase 11E: add terminal run reports and export
```

If Claude chooses multiple coherent internal commits, keep them descriptive but do not rename them as new phases.

---

# 6. Phase 11F — Final Phase-11 correctness and integration gate

## Goal

This is the dedicated broad Phase-11 audit.

Unlike implementation phases, broad cross-system verification belongs here.

The output should be blockers, not a giant report of everything that passes.

## Suggested Claude Code setup

```text
Model: Claude Opus 5
Effort: high
Plan mode: ON / read-only audit first
Subagents: up to 3 only for independent audit tracks
```

## Prompt

```text
Perform Phase 11F — Final Phase-11 Correctness and Integration Gate.

<context>
Phases 11A through 11E are intended complete.

This is the dedicated cross-system audit. It is allowed to be broad, but optimize for finding genuine blockers rather than documenting every passing behavior.

Do not add discretionary features.
Do not do Phase-12 UX polish.
</context>

<startup>
Read:
- CLAUDE.md;
- docs/CLAUDE_HANDOFF.md;
- relevant SPEC acceptance sections;
- recent Phase-11 git history.

Use actual code selectively based on the audit tracks below.
</startup>

<subagents>
You may use at most three read-only subagents if useful:

1. deterministic generation / settings / Nuzlocke / AI;
2. world / Premium / core QoL;
3. Run Reports / save / exporter.

Tell each subagent to report only defects, contradictions, or unverified critical assumptions.
Do not have them narrate passing code.
The main agent reconciles duplicates and decides what is actually blocking.
</subagents>

<audit_targets>
Check for genuine Phase-11 defects in these contracts:

A. Run identity and deterministic generation
- same version + seed + locked generation settings reproduces generated content;
- automatic seed architecture is coherent;
- generation settings lock correctly;
- Gen 1-9 filtering reaches required generation paths and evolution;
- invalid pools fail safely;
- no material randomizer-caused encounter/trainer stall;
- RAM remains safe.

B. Randomizer behavior
- species/trainer/starter/gift/static/ability/learnset/TM/Tutor/item integration;
- intended 21 generated level-up opportunities and no duplicate generated move within a species;
- weighting remains weighting, not a quality floor;
- trainer level/cap rules;
- no accidental high/low-roll normalization.

C. Premium rules
- ordinary wild/forbidden trainer Premium exclusion;
- Elite Four allowance;
- Wallace guarantee;
- every exposed Premium static obeys Premium-only replacement, generation mask, original-species eligibility, independent deterministic slot behavior, and cross-slot duplicates allowed.

D. Nuzlocke
- permadeath;
- no revival bypass;
- dead-mon no-benefit rule;
- location consumption;
- Dupes history after all valid encounter outcomes;
- safe duplicate handling;
- Shiny Clause;
- wild doubles;
- battle-item rule;
- wipe terminal path.

E. AI / battle
- Standard = vanilla Emerald behavior;
- harder difficulty progression works as intended;
- no pathological switch oscillation regression;
- fair modes do not read hidden information;
- banned battle gimmicks remain unavailable.

F. Core QoL/world
- Level to Cap;
- Move Reminder;
- evolution;
- Portable Heal;
- Infinite Repel;
- HM-free traversal;
- both bikes;
- type icons;
- Quick Travel;
- Master Ball timing;
- campaign remains finishable.

G. Run Reports/export
- Victory and Wipe terminal hooks;
- Wipe snapshot timing;
- persistence;
- report identity/settings;
- final Pokémon/statistics/death data as designed;
- no effect on future generation;
- manual JSON exporter;
- input save unchanged;
- schema/version/error behavior;
- automatic export/duplicate prevention if shipped.
</audit_targets>

<output>
Report ONLY genuine blockers or items that cannot be established statically and require a specific runtime test.

For every blocker include:
- exact behavior;
- exact file/function or subsystem;
- smallest correct fix;
- focused verification.

Do not list passing requirements one by one.

If there are no code blockers, provide a short runtime acceptance checklist and stop.

If code blockers exist, present a concise correction plan and wait for approval before editing.
</output>

<fix_and_close>
If fixes are approved:
- implement only blockers;
- run focused tests;
- run make clean && make -j$(sysctl -n hw.ncpu);
- run git diff --check;
- inspect the Phase-11F diff.

When no known required Phase-11 defect remains, end exactly:

PHASE 11 COMPLETE

Do not begin Phase 12.
</fix_and_close>
```

Suggested commit message if fixes/docs are required:

```text
Phase 11: complete final correctness review
```

If the audit produces no changes, do not create an empty commit.

---

# 7. Phase 12 — Final player experience, accessibility, defaults, UX, and technical/presentation polish

## Goal

Treat the functionally complete game as a product.

This is one phase. Claude may split internal implementation units if useful.

This is not another Phase-11 correctness audit.

## Suggested Claude Code setup

```text
Model: Claude Opus 5 for concise product plan; Sonnet 5 for implementation if desired
Effort: medium
Plan mode: ON
Subagents: default 0; at most 2 for genuinely independent UX vs technical polish work
```

## Prompt

```text
Perform Phase 12 — Final Player Experience, Accessibility, Defaults, UX, and Technical/Presentation Polish.

<context>
Phase 11 is complete and functionally correct.

The product target is a highly replayable streamlined Emerald randomized Nuzlocke where each seed defines a deterministic world, players adapt to random Pokémon/moves/abilities/items, grind and busywork are minimized, and opponents play intelligently without cheating.

Randomization is intentionally allowed to produce spectacular, terrible, easy, hard, and awkward outcomes within its eligibility logic. Do not normalize those outcomes away.

This phase improves the player experience. It does not re-audit all Phase-11 implementation.
</context>

<startup>
Read:
- CLAUDE.md;
- docs/CLAUDE_HANDOFF.md;
- only the SPEC sections for presets/settings/defaults, AI difficulty presentation, QoL/UI, story streamlining, and Run Report presentation;
- the concrete UI/settings/menu files implicated by those sections.

Do not inspect unrelated low-level systems unless a UX change directly requires them.
</startup>

<priorities>
1. Recommended/default experience
A new player selecting Recommended and changing nothing should get the intended experience quickly.

Review only the actual default/pre-run surfaces needed to answer:
- is Recommended already selected and understandable?
- are all Gen 1-9 enabled by default?
- is automatic seed the normal path?
- are Advanced controls out of the way?
- are irreversible generation choices clearly distinguished from runtime-safe settings?

Do not reopen settled randomizer formulas unless the UI/default itself is wrong.

2. AI difficulty presentation
Player-facing difficulty should include:
- Standard — vanilla Pokémon Emerald AI behavior;
- Improved;
- Expert;
- Pro Fair.

Keep internal names if changing them would create needless churn.

Descriptions should communicate increasing decision quality without implying hidden-information cheating.

Do not redo the AI switching remediation unless a current runtime defect remains.

3. Settings overload
Classify visible settings as:
- primary;
- advanced;
- internal/hidden;
- obsolete/remove.

Remove or hide dead/redundant controls.
Do not expose implementation switches simply because they exist.

Primary preset surface should remain approximately:
- Recommended;
- Modern Emerald;
- Randomizer;
- Custom/Advanced.

4. Information presentation
Improve clarity where currently needed on:
- New Game/pre-run confirmation;
- Summary;
- Party;
- PC;
- battle HUD;
- starter selection;
- Run Information;
- terminal Run Report summary.

Use progressive disclosure for dense details such as IV/EV/nature/randomizer information rather than crowding primary screens.

5. Replay friction
Focus on actual remaining friction in:
- New Game/restart;
- seed entry/reroll;
- starter selection;
- Level to Cap;
- Move Reminder/Evolve;
- healing;
- Ball selection/throw shortcut;
- bike switching;
- Quick Travel;
- wipe/restart;
- Victory/Hall of Fame;
- Run Report save/export discoverability.

A failed run should be able to restart quickly.

6. Run Report UX
Make Victory/Wipe feel like meaningful run conclusions.
Clearly distinguish:
- report saved internally;
- external automatic export succeeded, if host can actually confirm it;
- manual export availability.

Never claim external JSON success without confirmation.

7. Accessibility/readability
Fix actual issues in:
- terminology;
- text density;
- clipping;
- color-only state communication where avoidable;
- irreversible-choice messaging;
- control discoverability;
- stale labels.

8. Technical/presentation polish
Perform a targeted final polish pass for issues likely to affect the shipped experience:
- project-caused compiler warnings;
- temporary instrumentation;
- stale Tournament/7-7-7 text;
- obvious transition/menu glitches near changed systems;
- save/init edge cases introduced by final features;
- generation-filter empty-pool messaging;
- Run Report/export error presentation;
- obvious RAM regression;
- the known ELF RWX warning only if it is project-actionable and safe to fix.

Do not broad-refactor working systems.
</priorities>

<planning>
Produce a concise prioritized shipping plan.

Only include changes genuinely worth shipping.

For each proposed change give:
- user-visible problem;
- minimal fix;
- files/surfaces;
- runtime check.

If something is already good enough, omit it from the plan instead of proving it at length.

Claude may split Phase 12 internally if UX changes should land before a final technical polish unit, but keep one overall phase.
</planning>

<implementation>
After approval, implement only the approved Phase-12 changes.

Use focused builds.

At final phase boundary:
make clean
make -j$(sysctl -n hw.ncpu)
git diff --check

Inspect the Phase-12 diff.
Provide a short runtime checklist for changed player-facing behavior.

Do not begin Phase 13.
</implementation>
```

Suggested phase commit message:

```text
Phase 12: finalize player experience and polish
```

---

# 8. Phase 13 — Final documentation, repository cleanup, and release gate

## Goal

Make the repository describe the finished product, then perform the final blocker-only release gate.

This is one phase.

Claude may split it into documentation/cleanup and release-audit implementation units if that is cleaner, but they remain Phase 13.

## Suggested Claude Code setup

```text
Model: Claude Opus 5
Effort: high
Plan mode: ON
Subagents: up to 3 for independent final audits, only after docs/cleanup state is established
```

## Prompt

```text
Perform Phase 13 — Final Documentation, Repository Cleanup, and Product Release Gate.

<context>
Phases 1-12 are complete.
Gameplay features are frozen.

There is no Phase 14.

This phase may fix genuine release blockers but must not invent optional features, speculative refactors, or new roadmap work.
</context>

<phase_splitting>
Treat this as one phase.

If useful, split internally into:
- final documentation/repository cleanup;
- final release audit + blocker fixes.

Do not create new numbered/sub-lettered roadmap phases.
</phase_splitting>

<documentation_and_cleanup>
First make active repository documentation accurately describe the finished implementation.

Inspect actual implementation only where needed to resolve a documentation claim.

Update the relevant current docs, including as appropriate:
- README.md;
- docs/SPEC.md;
- docs/PHASES.md;
- PROMPTS.md;
- docs/CLAUDE_HANDOFF.md;
- CLAUDE.md / AGENTS.md only when their active instructions are stale.

Document the actual shipped product, including:
- deterministic run identity and seed behavior;
- Gen 1-9 pre-run filtering;
- Recommended/Advanced flow;
- Standard/Improved/Expert/Pro Fair AI presentation;
- randomization philosophy without hidden quality floors;
- Nuzlocke/Dupes/Shiny/whiteout rules;
- Premium restrictions and Premium-static behavior;
- level caps / Level to Cap / Move Reminder / Evolve;
- items/Balls/Master Ball behavior;
- both bikes/type icons/Portable Heal/Infinite Repel/HM-free/Quick Travel;
- Victory and Wipe Run Reports;
- manual JSON exporter;
- automatic mGBA integration if actually shipped;
- build/export instructions.

Do not document unimplemented features.

Remove or clearly mark stale active workflow material so future agents cannot mistake it for current state.

Historical development docs may remain if clearly historical.

Resolve stale `AI_HANDOFF.md` so it cannot masquerade as current authority.

Remove accidental:
- debug/profiling output;
- temporary files;
- obsolete generated artifacts;
- required-work TODO/FIXME that should have been completed.

Verify credits/licenses/upstream attribution without implying endorsement.
</documentation_and_cleanup>

<release_audit>
After documentation/cleanup reaches a coherent state, perform the final blocker-only release gate.

Use up to three read-only subagents only if useful:
1. randomizer/Nuzlocke/battle/AI;
2. world/Premium/QoL/UX;
3. Run Reports/save/export/docs/build.

Each subagent must report only genuine release blockers, not passing requirements or optional improvements.

Check for blockers in:

A. Build/save stability
- clean documented build;
- no crash/save corruption;
- safe RAM;
- coherent save migration/versioning;
- no required TODO/debug instrumentation.

B. Run generation
- pre-run flow;
- automatic/manual seed;
- reproducibility;
- locked settings;
- Gen 1-9 filtering/evolution;
- invalid-pool behavior;
- no material generation stall.

C. Randomizer/trainer correctness
- required randomization systems;
- learnset uniqueness/weighting;
- trainer levels/caps;
- no accidental team/moveset quality floor.

D. Premium
- ordinary wild/trainer exclusions;
- Elite Four/Wallace rules;
- every exposed Premium static is Premium-only, deterministic, generation-filtered, original species eligible, duplicates across slots allowed.

E. Nuzlocke
- permadeath/no revival;
- dead-mon restrictions;
- location/Dupes/Shiny;
- battle-item rule;
- wipe flow.

F. AI/battle
- Standard vanilla Emerald semantics;
- harder difficulty behavior;
- no pathological switching regression;
- fair-information compliance;
- banned battle gimmicks unavailable.

G. QoL/world
- both bikes/type icons/Move Reminder/Level to Cap/Evolve/Portable Heal/Infinite Repel/HM-free/Quick Travel;
- campaign finishability;
- Ball/Master Ball progression.

H. Run Reports/export
- Victory/Wipe hooks;
- Wipe snapshot timing;
- persistence;
- report contents;
- future-run independence;
- JSON/manual export;
- source save unchanged;
- automatic export duplicate protection if shipped.

I. Documentation
- README/SPEC/build/export docs match implementation;
- no stale active workflow misrepresents current state.
</release_audit>

<release_output>
Output ONLY genuine blockers.

For each blocker:
- exact behavior;
- exact location/subsystem;
- required correction;
- focused verification.

Do not output nice-to-haves.

If zero blockers remain:
- do not create an empty diff;
- run the final commands below;
- end exactly with the release-ready marker.

If blockers exist:
- present a concise correction plan;
- wait for approval;
- implement only blockers;
- repeat only the checks relevant to those blockers plus the final build.
</release_output>

<final_commands>
Run:

make clean
make -j$(sysctl -n hw.ncpu)
git diff --check
git status --short

If the tree contains intended Phase-13 documentation/cleanup/fixes, inspect the complete Phase-13 diff before commit.

If no known release blocker remains, end exactly:

PRODUCT COMPLETE — PHASE 13 COMPLETE — NO FURTHER DEVELOPMENT PHASES REQUIRED
</final_commands>
```

Suggested phase commit message:

```text
Phase 13: finalize product release
```

---

# 9. Shared runtime-gate commit prompt

Use this after the phase's required manual/runtime checks have actually passed:

```text
The runtime/manual acceptance gate for this phase has passed.

Finalize this phase only.

Run:
git status --short
git diff --check
git diff

Inspect the complete intended phase diff.

If there are no unresolved in-scope defects:
- run the appropriate final build for this phase;
- commit all intended changes with the approved phase commit message;
- push.

Then run:
git status --short
git log -1 --oneline

Do not begin the next phase.

Report only:
- commit hash;
- push result;
- build result;
- relevant memory usage if code changed materially;
- final working-tree state.
```

---

# 10. Fresh-session continuation prompt

Use this only when Claude itself recommended splitting a numbered phase across fresh sessions.

```text
Continue the CURRENT numbered phase from the repository state.

Do not restart the phase audit or re-plan completed units.

First run:
git status --short
git log -5 --oneline

Read:
- CLAUDE.md;
- docs/CLAUDE_HANDOFF.md;
- the current phase section of PROMPTS.md;
- only the files needed for the remaining implementation unit.

Use git history and docs/CLAUDE_HANDOFF.md to determine what the prior unit completed.

Continue directly with the next remaining unit.

Do not re-verify earlier units unless the remaining implementation depends on a specific interface or new evidence indicates a defect.

Do not begin the next numbered phase.
```

---

# 11. Final expected state

```text
Phases 1–10      COMPLETE
Phase 11A        COMPLETE
Phase 11B        COMPLETE
Phase 11C        COMPLETE
AI switching remediation COMPLETE
Phase 11D        COMPLETE
Phase 11E        COMPLETE
Phase 11F        COMPLETE
Phase 12         COMPLETE
Phase 13         COMPLETE

Seed / deterministic generation:
COMPLETE

Gen 1–9 pre-run filtering:
COMPLETE

Encounter/trainer startup performance:
COMPLETE

AI difficulty:
STANDARD / IMPROVED / EXPERT / PRO FAIR
with Standard = vanilla Emerald behavior

AI switching:
intelligent at higher difficulties without pathological oscillation

Premium static slots:
PREMIUM -> PREMIUM ONLY

Victory Run Reports:
COMPLETE

Wipe Run Reports:
COMPLETE

Manual JSON export:
COMPLETE

Automatic supported-mGBA export:
COMPLETE if technically supported and shipped

No Tournament mode
No cross-run species/team exclusions
No Phase 14
No known required unfinished work

PRODUCT COMPLETE
```
