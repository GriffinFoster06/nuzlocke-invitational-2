# Build Phases

Work through these roughly in order. Each is meant to be its own Claude Code
session (or a few). Don't let Claude Code jump ahead into later phases on its
own — keep sessions scoped.

## Working setup (defaults for the whole project)

- **Launch every session with:** `claude --model opusplan`
- **Effort:** leave on default (`high`). Only bump with `/effort xhigh` for
  one hard moment (noted below where relevant), then run `/effort high`
  to drop back.
- **Plan mode:** toggle with `Shift+Tab` (cycles through modes) or type
  `/plan`. Each phase below says whether to turn it on before pasting the
  prompt. When it's on, Claude Code drafts an approach and won't touch files
  until you approve it — press `Shift+Tab` again (or approve when asked) to
  let it proceed with the actual edits.

---

## Phase 0 — Environment sanity check
**Before pasting:** nothing extra — stay in normal mode.

**Prompt:**
> Build this project as-is with no modifications, using the baserom already
> in the project root. Confirm the resulting ROM's size and that gbafix
> completed without errors. Don't change anything yet — this is just to
> confirm the environment is set up correctly.

---

## Phase 1 — Settings/config scaffolding
**Before pasting:** turn on plan mode (`Shift+Tab`).

**Prompt:**
> Read docs/SPEC.md and docs/PHASES.md. We're starting Phase 1 only:
> settings/config scaffolding. Design a settings struct that covers every
> category listed under "Settings behavior" in the spec, implement the
> preset list (Invitational 2 Solo / Tournament / Randolocke / Modern
> Emerald / Randomizer / Custom) and the "changing one setting reverts
> display to Custom" logic, and add a minimal debug-menu-accessible screen
> to view/edit the values. Don't implement any of the actual gameplay
> behaviors yet — this phase is just the data model and menu skeleton. Show
> me your plan before writing anything.

**After you approve the plan:** exit plan mode (`Shift+Tab`) and reply:
> Looks good, go ahead. Build and confirm it compiles when you're done.

---

## Phase 2 — Core randomizer wiring
**Before pasting:** turn on plan mode. Optionally run `/effort xhigh` first
since this phase includes the power-score formula design — run
`/effort high` again once that specific decision is made.

**Prompt:**
> Read docs/SPEC.md's sections on wild Pokémon randomization, species power
> matching, evolutionary-stage matching, fixed encounter-slot mapping, and
> starter/gift/static Pokémon. This is Phase 2 from docs/PHASES.md, building
> on `src/random_mon_generation.c`. Before implementing anything, tell me
> your proposed weighting formula for combining BST, evolutionary stage,
> stat distribution, offensive efficiency, defensive efficiency, speed, and
> the "artificially restrained by ability" flag into one power score, and
> wait for me to confirm it. Also show your plan for the category bans,
> the persistent per-encounter-slot mapping, and the seed generation/storage
> approach before writing code.

**After you approve:** exit plan mode and reply:
> Confirmed, proceed with implementation. Build and confirm it compiles.

---

## Phase 3 — Nuzlocke ruleset engine
**Before pasting:** turn on plan mode.

**Prompt:**
> Read the Nuzlocke-related sections of docs/SPEC.md (permadeath, one
> encounter per location, Dupes Clause, Shiny Clause, nicknames, no battle
> items, whiteout). This is Phase 3 from docs/PHASES.md — build this as its
> own self-contained subsystem, separate from the randomizer. Show me your
> plan for how location-tag tracking and the evolutionary-family Dupes
> Clause will work before writing any code.

**After you approve:** exit plan mode and reply:
> Confirmed, proceed. Build and confirm it compiles, and tell me how to
> manually test permadeath and the Dupes Clause once it's in.

---

## Phase 4 — Level caps, Level-to-Cap, learnsets
**Before pasting:** turn on plan mode for the Level-to-Cap logic specifically.

**Prompt:**
> Read the level cap, Level to Cap, learnset size, and 7/7/7 composition
> sections of docs/SPEC.md. This is Phase 4 from docs/PHASES.md. Wire the
> per-badge cap table from the spec into the existing `include/config/
> caps.h` system. Then show me your plan for the Level-to-Cap sequential
> move-learning behavior specifically — it needs to present every skipped
> level-up move in chronological order, not just skip straight to the cap —
> before you implement that part. The 21-move 7/7/7 learnset generator with
> move-power progression can be planned and implemented in the same pass.

**After you approve the Level-to-Cap plan:** exit plan mode and reply:
> Confirmed, proceed with all of it. Build and confirm it compiles.

---

## Phase 5 — AI tuning
**Before pasting:** stay in normal mode (no plan mode needed).

**Prompt:**
> Read the "Maximum-strength fair AI" section of docs/SPEC.md. This is
> Phase 5 from docs/PHASES.md. Tune the existing percentages in
> `include/config/ai.h` to implement the "Pro Fair" behavior described in
> the spec, and confirm no omniscient-information AI flag is enabled
> anywhere. Build and confirm it compiles, and summarize exactly which
> values you changed and why.

---

## Phase 6 — Display/UI
**Before pasting:** stay in normal mode.

**Prompt:**
> Read the Pokémon Summary improvements, IV/EV/Nature display, and Run
> Information sections of docs/SPEC.md. This is Phase 6 from
> docs/PHASES.md. Add the described fields to the existing summary screen
> (legality-under-cap, dead marker, IVs, EVs, nature with boosted/reduced
> indicator) and show the seed and ruleset in Run Information. Build and
> confirm it compiles.

---

## Phase 7 — Evolution overhaul
**Before pasting:** turn on plan mode.

**Prompt:**
> Read the Evolution system, Lilycove evolution-item sellers,
> Move-dependent evolution anti-softlock, Trade evolutions, and
> Species-specific evolution fixes sections of docs/SPEC.md. This is Phase
> 7 from docs/PHASES.md. Show me your plan for: converting every trade
> evolution to single-player, the specific fixes for Shelmet, Karrablast,
> Mantyke, Gimmighoul, Galarian Yamask, and Bisharp, the Lilycove
> evolution-item sellers, the Evolution Assistance function, and lowering
> any evolution level above the final pre-Champion cap (63, unless we've
> updated that table). I want to check nothing is missed before you touch
> any scripts.

**After you approve:** exit plan mode and reply:
> Confirmed, proceed. Build and confirm it compiles, and list every species
> you touched so I can spot-check them.

---

## Phase 8 — Remaining QoL
**Before pasting:** turn on plan mode just for the HM-free traversal system
(mention this explicitly in the prompt, as below).

**Prompt:**
> Read the Catch rates, R-button Ball shortcut, Unlimited money, Poké Ball
> availability, 999 Poké Ball NPC, Expanded Bag, and HM-free traversal
> sections of docs/SPEC.md. This is Phase 8 from docs/PHASES.md. Most of
> this is straightforward config/value changes you can implement directly.
> The HM-free field-move permission system is new — no existing badge-gated
> field-move system exists in this codebase — so show me your plan for that
> one specifically before implementing it; the rest you can just build.

**After you approve the HM-free plan:** exit plan mode and reply:
> Confirmed, proceed with everything. Build and confirm it compiles.

---

## Phase 9 — Map content additions
**Before pasting:** stay in normal mode.

**Prompt:**
> Read the "Map additions" and "Late-game unlocks" sections of docs/SPEC.md.
> This is Phase 9 from docs/PHASES.md. Implement the Route 103 Old Rod move,
> Littleroot water encounter, Oldale grass encounter, Scorched Slab
> population, and the post-Rayquaza unlocks (Regi caves, Weather Institute
> events, Slateport legendary-map NPC), gated on the Sootopolis/Rayquaza
> progression flag as described. Build and confirm it compiles.

---

## Phase 9.5 — Pre-story checkpoint (audit + playable build)
**Before pasting:** stay in normal mode for the audit itself. Consider
`/effort xhigh` for this one session since it's reviewing across all nine
prior phases at once, not a single focused task — run `/effort high` again
once it's done. If it finds real bugs, it should stop and show you a plan
before fixing anything that touches an already-built system — don't let it
silently patch across phases.

This is the point to stop and confirm everything built so far (Phases 1-9:
settings/presets, the randomizer, Nuzlocke rules, level caps, AI, display,
evolution fixes, remaining QoL, map additions) actually holds together
before starting the biggest and riskiest phase. Story linearization touches
scripts throughout the whole game — you want a solid, verified foundation
under it, not nine phases of work you're hoping integrated correctly.

**Prompt:**
> Before we start Phase 10, I want a full checkpoint. Re-read docs/SPEC.md
> in full and go category by category — settings/presets, the randomizer
> (species/trainer/item/ability, power score, persistent slot mapping),
> Nuzlocke rules (permadeath, one-per-location, Dupes Clause, Shiny Clause,
> nicknames, no battle items, whiteout), level caps and Level to Cap,
> learnset generation, AI tuning, display/UI additions, the evolution
> overhaul, and the remaining QoL items (catch rate, repel, bag, ball
> shortcut, money, HM-free traversal, map additions). For each one, confirm
> what was actually implemented matches what the spec says, and specifically
> check for integration problems between phases that were built in separate
> sessions — for example, does the settings menu actually control the
> randomizer's behavior, does the Nuzlocke over-cap marking correctly block
> battle entry, does Level to Cap respect the current preset's rules, does
> the evolution overhaul interact correctly with the randomized learnsets.
> Don't fix anything yet — first give me a clear report: what's confirmed
> working, what's missing or incomplete, and what looks like it might not
> be wired together correctly and needs a closer look.

**After reviewing its report,** for anything it flagged as broken or
missing, ask for a plan before letting it fix things:
> Show me your plan for fixing [the specific issues], one at a time, before
> you touch anything.

**Once everything's confirmed or fixed, get a clean playable build:**
> Now do a full clean build — run `make clean` first, then build from
> scratch, and confirm it compiles with no errors. Once it's built, copy
> the resulting pokeemerald.gba to checkpoint-after-phase9.gba in the
> project root so I have a preserved, working snapshot before story
> linearization starts.

**Then playtest it yourself before starting Phase 10.** Load
`checkpoint-after-phase9.gba` in an emulator and actually check:
- Start a new game — does the settings/preset menu appear and work?
- Catch a few wild Pokémon — are they randomized, power-matched, and does
  the same encounter slot stay consistent if you soft-reset and re-catch it?
- Faint a Pokémon on purpose — does permadeath actually trigger (marked
  dead, can't be used)?
- Try to re-catch on an already-resolved encounter location — does the Dupes
  Clause / one-per-location rule actually block it correctly?
- Try to name a caught Pokémon nothing (skip the nickname) — is it actually
  enforced as mandatory?
- Level a Pokémon up near the first badge's cap — does the hard cap actually
  stop it, and does Level to Cap correctly walk through skipped moves?
- Open a Pokémon's summary — are IVs, EVs, nature, and legality status all
  showing correctly?
- Fight a trainer — does the AI feel noticeably smarter than vanilla, and
  does it seem to only react to information it should actually know?
- Try a couple of the QoL items — infinite Repel toggle, R-button Ball
  throw, unlimited money at a shop with progression-gated Ball types.

If anything on that list doesn't work as expected, that's exactly what you
want to catch now — bring it back to Claude Code as its own focused fix
before starting Phase 10, rather than discovering it buried under a story
linearization pass later.

---

## Phase 10 — Story linearization (do one area per session)
**Before pasting:** turn on plan mode — always, every area, no exceptions.
Replace `[AREA NAME]` with the specific area from the "Map-by-map story
cuts" list in docs/SPEC.md (e.g. "Littleroot opening", "Petalburg Woods",
"Mt. Chimney"). If a session feels like it's losing track of a large area's
scripts, try `/model fable` for that one session instead of opusplan.

**Prompt:**
> Read the "[AREA NAME]" entry under "Map-by-map story cuts" in
> docs/SPEC.md, plus the "Story streamlining — overall rule" and "General
> story QoL" sections above it for the general philosophy. This is Phase 10
> from docs/PHASES.md, this area only — do not touch any other area's
> scripts this session. Show me your plan for exactly which scripts, flags,
> and warps you'll change before touching anything, and flag any place
> where you're not fully certain a flag or warp dependency is safe to
> remove.

**After you approve:** exit plan mode and reply:
> Confirmed, proceed. Build when done, and give me a precise list of what
> to actually test when I load this in an emulator — which flags to set,
> which path to walk, what should and shouldn't happen.

*(Then go actually test it before starting the next area's session.)*

---

## Phase 11 — Tournament persistence & polish
**Before pasting:** turn on plan mode for the persistent-SRAM design.

**Prompt:**
> Read the Run seed, Hall of Fame, and Hall-of-Fame species exclusion
> sections of docs/SPEC.md. This is Phase 11 from docs/PHASES.md. Show me
> your plan for a persistent SRAM region that stores the tournament species
> exclusion list and survives "New Game" without corrupting or being
> corrupted by normal save data, before implementing it. Seed-reproducibility
> guarantees and the Hall of Fame / Run Information export can be planned
> and implemented in the same pass.

**After you approve:** exit plan mode and reply:
> Confirmed, proceed. Build and confirm it compiles.

---

## General pattern for any phase not listed exactly as above

> Read [the relevant SPEC.md section(s)] and docs/PHASES.md. This is Phase
> [N] from docs/PHASES.md — stay inside this phase's scope only. [State
> what to plan vs. what's fine to build directly, per this phase's "Mode"
> note above.] Build and confirm it compiles when you're done.
