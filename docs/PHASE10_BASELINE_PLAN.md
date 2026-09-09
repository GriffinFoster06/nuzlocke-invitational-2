# Minimal baseline remediation and Arc 0 status

Preserve Phases 1–9.5 and all existing Arc 0 work. Make one comment correction and seven documentation updates. Arc 1 remains unstarted.

## 1. Exact files and changes

| File | Planned change |
|---|---|
| [data/maps/RustboroCity_Gym/scripts.inc](/Users/griffinfoster/Projects/pokeemerald-expansion/data/maps/RustboroCity_Gym/scripts.inc:22) | Replace only `Norman's badge counter;` with `Norman's badge counter,`. Preserve all executable script commands and dialogue bytes. |
| `docs/PHASE10_PROMPTS.md` **— new** | Add the nine arcs and exact area membership supplied in your request. Add the workflow below, document existing Arc 0 work, and distinguish implementation status from build/review/playtest acceptance. |
| [AGENTS.md](/Users/griffinfoster/Projects/pokeemerald-expansion/AGENTS.md) | Describe the existing randomizer (`src/randomizer.c`), Nuzlocke subsystem, HM-free traversal, settings UI, and runtime cap system. Remove obsolete “not implemented” claims and instructions implying the project randomizer must still be built. Preserve the upstream-base rationale, source-only build instructions, and SPEC’s gameplay authority. Establish PHASE10_PROMPTS as the Phase 10 workflow and AI_HANDOFF as unfinished-task state. Use agent-neutral wording. |
| [CLAUDE.md](/Users/griffinfoster/Projects/pokeemerald-expansion/CLAUDE.md) | Replace duplicated shared instructions with exactly `@AGENTS.md` on the first line. The current file contains no unique Claude-specific instructions worth retaining, so add nothing below it. |
| [docs/PHASES.md](/Users/griffinfoster/Projects/pokeemerald-expansion/docs/PHASES.md) | Correct Phase 0 to build entirely from source. Preserve completed-phase historical prompts, including Phase 9.5, under an explicit historical-status notice. Relabel old Claude setup guidance as historical rather than project-wide defaults. Replace Phase 10’s one-area-per-session restriction with the grouped-arc workflow and link to PHASE10_PROMPTS. |
| [docs/SPEC.md](/Users/griffinfoster/Projects/pokeemerald-expansion/docs/SPEC.md) | Correct only the identified stale baseline descriptions: Evolution Assistance was verified unnecessary and removed during Phase 9.5; the Evolve command remains. Acquisition levels are clamped when caps are enabled; soft/warning modes can still produce over-cap Pokémon; the enabled ineligibility restriction applies until progression catches up, with the existing no-cap-legal-battler exception. Preserve current gameplay and settings behavior. |
| [docs/PHASE9_REMEDIATION_PROMPTS.md](/Users/griffinfoster/Projects/pokeemerald-expansion/docs/PHASE9_REMEDIATION_PROMPTS.md) | Add a short notice identifying the document as completed historical remediation. Leave its existing historical content intact. |
| [AI_HANDOFF.md](/Users/griffinfoster/Projects/pokeemerald-expansion/AI_HANDOFF.md) | Replace placeholder entries with the actual objective, committed/uncommitted Arc 0 inventory, remediation changes, verification results, remaining playtests, and exact next step. Retain its existing locally ignored status. |

The existing `.gitignore` change and all other map/script changes will be preserved without further edits. No additional plan files will be created during this remediation.

## 2. Phase 10 workflow document

Use your area lists verbatim under these titles:

| Arc | Title |
|---|---|
| 0 | Littleroot through Roxanne |
| 1 | Roxanne through Brawly |
| 2 | Brawly through Wattson |
| 3 | Wattson through Flannery |
| 4 | Flannery through Norman |
| 5 | Norman through Winona |
| 6 | Winona through Tate & Liza / Mossdeep |
| 7 | Tate & Liza through Juan |
| 8 | Juan through Champion |

For Arcs 1–8, require:

1. **Planning:** Inspect actual code first. Produce a separate sub-plan for each area covering scripts, flags/state variables, warps, rewards, dependencies, and acceptance checks. Obtain human approval, then write the approved plan to `docs/PHASE10_ARC<N>_PLAN.md`.
2. **Execution:** Read the approved plan file, implement only its approved scope, build, and inspect the resulting diff.
3. **Review:** Perform a read-only review against both SPEC and the approved plan.
4. **Playtest:** The user continuously plays through the entire arc in mGBA; isolated flag-driven checks supplement that traversal.
5. **Completion:** Commit and push only after implementation, build, review, and user playtesting all pass.

Changes to an approved scope require renewed approval and an updated plan file. Large arcs may be divided into smaller approved implementation steps when inspected dependencies justify it.

Arc 0 receives an **existing-state record**, not a retroactive implementation plan or instructions to rebuild it.

## 3. Establish Arc 0’s actual state

Record these starting points:

- `de53064ca8`: completed Phase 9.5 checkpoint.
- `af88cb09ad`: committed opening/first-rival Phase 10 changes.
- `3c9de94ab9`: subsequent committed fixes, preserved as accepted baseline.
- Uncommitted Arc 0: Petalburg city/Gym scripts and city triggers; Petalburg Woods script; Rustboro city/Gym scripts and city triggers.

Inspect those changes against Arc 0’s SPEC entries and document what is implemented, incomplete, or unverified. Findings do not authorize additional gameplay edits.

Keep separate statuses for **implementation**, **build**, **read-only review**, and **emulator acceptance**. Do not mark Arc 0 complete solely because compilation succeeds.

## 4. Verification

After approval and the planned edits:

- Run direct dependency scanning on `data/maps/RustboroCity_Gym/scripts.inc` and the aggregate `data/event_scripts.s`, without bypassing scanning.
- Run `make -j$(sysctl -n hw.ncpu)`; require successful completion and absence of the Rustboro diagnostic.
- Confirm the Rustboro edit changes only the specified comment and preserves executable script/dialogue content.
- Run `git diff --check`; inspect staged and unstaged diffs against the pre-remediation snapshot to confirm all existing work is preserved.
- Check document links, exact CLAUDE first line, all nine arc memberships, approval requirements, and completed/historical status labels.
- Record results in AI_HANDOFF. If verification exposes an unrelated problem, record it without expanding into gameplay remediation.

Document these pending mGBA checks for Arc 0:

- Fresh games with both player genders and clock-setting preferences.
- Starter selection/nickname, rescue progression, first rival victory/loss, lab transition, and re-entry without repeated scenes.
- Nuzlocke activation before versus after actual Ball acquisition.
- Petalburg passage without Wally’s tutorial; Norman’s early gate remains intact.
- Both Woods approaches, battle/reward handling, and encounter access.
- Roxanne’s badge/cap/counter changes, Devon employee visibility, and theft handoff into Route 116/Rusturf.

## 5. Gameplay impact and expected state

**No proposed remediation changes gameplay, dialogue, public APIs, save formats, or settings values.** Existing Arc 0 gameplay changes remain preserved.

The expected result is a warning-free build of the current Arc 0 implementation, consistent shared documentation, the restored nine-arc workflow, and an accurate handoff with explicit remaining playtests.

The working tree remains uncommitted during this pass. Arc 0 completion stays pending wherever review or user playtest evidence is missing; Arc 1 remains unstarted. Implementation awaits your approval.
