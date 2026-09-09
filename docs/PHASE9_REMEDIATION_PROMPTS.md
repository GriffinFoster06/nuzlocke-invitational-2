# Phase 9.5 Remediation — Every Prompt, In Order

> **Completed historical remediation:** This sequence has already been
> executed. It is retained as project history, not as current work instructions
> or a statement of missing functionality.

This picks up after the audit that found: the disabled-species crash was
fixed, Evolution Assistance and two terrain tables were fixed and
rebuilt into checkpoint-after-phase9.gba, but Pro Fair AI's status is
unconfirmed, a large chunk of Phase 2's randomizer scope was never built,
and several QoL/settings items are missing or non-functional.

Do these in order. Don't skip ahead — several later prompts assume an
earlier one is confirmed first.

---

## 1. AI flag clarification

**Model:** `opusplan`
**Plan mode:** ON — this is re-verifying its own prior claim, keep it
read-only until confirmed.
**Effort:** default (`high`)

**Prompt:**
> Confirm: did you actually apply the Pro Fair AI flags
> (AI_FLAG_SMART_SWITCHING, PREDICT_SWITCH, PREDICT_MOVE,
> ASSUME_STATUS_MOVES, ASSUME_STAB, PP_STALL_PREVENTION) to trainer data
> this session, or was that description cut off before it happened? Check
> the actual trainer data, don't just recall what you intended to do. If
> it's done, tell me exactly how many trainers now have which flags and
> show me a sample. If it's not done yet, don't fix it in this message —
> just confirm the current state.

**If it reports the flags were NOT applied**, exit plan mode and continue
in the same session with:
> Show me your plan for applying these flags across the trainer roster —
> which trainers get which flags, and whether this is uniform or tiered
> by trainer importance (e.g. bosses get fuller AI than route trainers).

Review that plan, then approve it. Build and confirm it compiles.

**If it reports the flags WERE applied**, move straight to step 2.

---

## 2. Your own playtest (not a Claude Code prompt)

Load `checkpoint-after-phase9.gba` (or the freshly rebuilt one from step 1
if it needed a fix) in mGBA and check:
- Evolve something via Evolution Assistance — try Sylveon or Yanma
  specifically, since those were the named broken cases.
- Walk through Littleroot's new pond and Oldale's new grass — confirm
  encounters actually trigger.
- Fight a trainer — does anything about its behavior feel different from
  vanilla (switching, prediction), confirming the AI flags are live.

Don't move to step 3 until you've confirmed these yourself.

---

## 3. CLAUDE.md correction

**Model:** `opusplan`
**Plan mode:** OFF — trivial, single-file doc edit, no risk.
**Effort:** default (`high`)

**Prompt:**
> Fix CLAUDE.md per your own earlier finding: pokeemerald.gba is build
> output, not a baserom, and the project needs no baserom at all — the
> SHA1 f3ae0881... check described in there is misleading for a from-source
> build. Correct that section so it doesn't mislead a future session.

---

## 4. Tier 2 remediation — features reported done but actually absent

**Model:** `opusplan`
**Plan mode:** ON — touches several separate systems (battle entry logic,
settings menu UI, repel/healing systems).
**Effort:** default (`high`)

**Prompt:**
> This is Tier 2 remediation from the Phase 9.5 audit. Fix the following,
> reading the actual current code for each before changing anything —
> show me your plan for all of them before implementing:
> 1. Universal TM/Tutor compatibility — SETTING_TM_COMPATIBILITY and the
>    tutor equivalent exist but nothing consumes them; every Pokémon
>    should be able to learn every randomized TM/Tutor move when this is
>    on, per docs/SPEC.md.
> 2. Infinite Repel — this doesn't exist at all right now: no toggle, no
>    indicator, no encounter suppression. Build it per the "Infinite
>    Repel" section of docs/SPEC.md.
> 3. Portable healing — also entirely absent. Build it per the "Portable
>    healing" section of docs/SPEC.md.
> 4. Over-cap ineligibility — currently just a summary-screen label; it
>    needs to actually block battle entry, per the "Caught Pokémon above
>    the cap" section of docs/SPEC.md.
> 5. Set battle style forced ON by default — new_game.c currently still
>    sets OPTIONS_BATTLE_STYLE_SHIFT; SETTING_FORCE_SET_BATTLE_STYLE is
>    unread.
> 6. A player-reachable settings/Run Information entry point — right now
>    this only exists in the debug menu. Add a real access point (title
>    screen or start menu) so an actual player can reach it.

**After you approve the plan:** exit plan mode and reply:
> Confirmed, proceed with all six. Build and confirm it compiles, and tell
> me exactly how to test each one.

---

## 5. Small cleanup pass

**Model:** `opusplan`
**Plan mode:** OFF — these are small, independent, well-defined fixes,
not systemic changes.
**Effort:** default (`high`)

**Prompt:**
> Fix these smaller items found in the Phase 9.5 audit:
> 1. NICK_STRICT currently behaves identically to NICK_MANDATORY — the
>    naming screen can still be exited leaving the species name. Make
>    strict mode actually prevent that.
> 2. Whiteout has no confirmation prompt before starting a new attempt —
>    add one.
> 3. LRNCOMP_WEIGHTED is currently a stub aliased to 7/7/7 — either
>    implement the actual weighted composition or, if we're deferring
>    that, remove it from the settings menu so it's not a dead option.
> 4. There's a mangled comment in include/config/caps.h — clean it up.
> Build and confirm it compiles after each fix.

---

## 6. The big one — finish Phase 2's deferred randomizer scope

**Model:** `opusplan` at `/effort xhigh` first — this is the right tool for
complex architecture decisions and long agentic sessions. Only switch to
`/model fable` mid-session if it's visibly struggling (shallow plans,
losing track of the five-part scope, giving up on ambiguity) — Fable is
the escalation for when Opus at higher effort falls short, not the
default starting point, and it runs roughly double the cost.
**Plan mode:** ON for the whole session, re-engaged before each major
piece — don't approve all of this in one shot.
**Effort:** `/effort xhigh` for the whole session. Drop to `high` once
this is done and confirmed.

**Prompt:**
> include/run_rng.h:28 admits this outright: "Reserved for later phases:
> trainers, abilities, TMs, tutors, items." Those were never actually
> built despite being core Phase 2 scope in docs/SPEC.md. This is now
> that phase. Read the Trainer Pokémon, Abilities, Ability consistency
> through evolution, TMs, Move Tutors, and Items sections of docs/SPEC.md,
> and the existing (working) wild/starter/gift/static randomizer code as
> your pattern to follow for consistency.
>
> Show me your plan first, broken into pieces, before implementing any of
> it:
> 1. Trainer Pokémon randomization — power-appropriate per trainer,
>    stricter matching for bosses, fixed for the run.
> 2. Ability randomization — remaining fixed per species/slot through the
>    run, with evolution preserving the corresponding ability (this was
>    already built for the working parts of the randomizer — reuse that
>    pattern), and Shedinja's Wonder Guard exemption.
> 3. TM randomization with universal compatibility (should now already
>    work once Tier 2's compatibility fix lands).
> 4. Move Tutor randomization, same compatibility approach.
> 5. Item randomization — field items and gift items, with the item-pool
>    safety rules (key items protected, evolution items guaranteed via
>    Lilycove).
>
> Wait for me to review and confirm each piece's plan before implementing
> it — don't build all five at once even after I approve the overall
> plan.

Work through its five pieces one at a time: review each specific plan,
approve, let it build that piece, confirm it compiles, THEN move to the
next piece's plan. This is the largest remaining gap — treat it with the
same care as the original Phase 2.

---

## 6.5. Global evolution overhaul — level-based except stones

New design decision, added after the Phase 7 audit: every evolution in
the entire dataset is now level-based except item/stone evolutions. See
the rewritten "Evolution system" section in docs/SPEC.md for the full
rule and the specific Eeveelution stone assignments (Espeon→Sun Stone,
Umbreon→Moon Stone, Sylveon→Shiny Stone).

**Model:** `opusplan` at `/effort xhigh` first — this is a full-dex sweep,
comparable in scope to step 6. Escalate to `/model fable` mid-session only
if it's visibly struggling with the scope, same rule as step 6.
**Plan mode:** ON for the whole session, re-engaged per category — this
should NOT be approved as one giant plan. Expect to break it into pieces
by evolution-method category (move-gated, friendship-gated,
time-of-day-gated, location-gated, party-member-gated, stat-comparison-
gated, weather-gated, region-gated, beauty-gated, trade) and review/
approve each category's plan separately.
**Effort:** `/effort xhigh` for the whole session. Drop to `high` once
done and confirmed.

**Prompt:**
> Read the updated "Evolution system" section of docs/SPEC.md — the
> global rule is now: every evolution is level-based except item/stone
> evolutions. Inventory every non-item evolution condition in the entire
> species dataset (through Gen 9, matching whatever this build's roster
> covers) — friendship, time-of-day, specific-move-known, location,
> party-member-present, stat-comparison, weather, region, beauty, trade,
> and any other non-level condition you find. Group them by category and
> show me the count in each category and a sample of affected species
> before proposing anything.
>
> Then, category by category, propose specific level thresholds to
> replace each non-item condition, and show me each category's plan
> before implementing it — don't batch-approve all categories at once.
> Apply the specific Eeveelution stone assignments from docs/SPEC.md
> exactly as specified.
>
> Once the sweep is done, confirm whether Evolution Assistance
> (src/party_menu.c / src/evolve_menu.c, from the earlier Phase 7 fix)
> has any remaining use case now that no evolution requires a specific
> known move. If it's genuinely unused, tell me before removing it rather
> than assuming — I want to confirm there's no edge case you're missing.

Work through each category's plan individually: review, approve, let it
implement that category, confirm it builds, then move to the next
category's plan. Commit after each category or logical group, not just
once at the very end — this is a large enough change that you want
checkpoints within it.

---

## 7. Final checkpoint rebuild (after everything above is done)

**Model:** `opusplan`
**Plan mode:** OFF — just building and packaging, not editing.
**Effort:** default (`high`)

**Prompt:**
> All Phase 9.5 remediation is complete. Do a full clean build —
> make clean, then rebuild from scratch — and confirm no errors. Overwrite
> checkpoint-after-phase9.gba with this new build, and give me a complete
> updated status table across all of Phases 1-9 so I have an accurate
> record of what's actually in this build before Phase 10 starts.

**Then playtest thoroughly yourself** — same checklist as the original
Phase 9.5 prompt in docs/PHASES.md, plus specifically testing every item
fixed in steps 1-6 above. Only move to Phase 10 once this genuinely holds
up under real, sustained play.
