# Phase 10 grouped-arc workflow

`docs/SPEC.md` remains authoritative for gameplay behavior. This document
defines Phase 10's work boundaries and acceptance workflow. The area names
below are the corresponding entries under SPEC's "Map-by-map story cuts".

## Arc membership

### Arc 0 — Littleroot through Roxanne

- Littleroot opening
- First rival battle
- Petalburg/Wally tutorial
- Petalburg Woods
- Rustboro/Roxanne

### Arc 1 — Roxanne through Brawly

- Route 116/Rusturf Tunnel
- Briney ferry
- Dewford

### Arc 2 — Brawly through Wattson

- Slateport
- Route 110/Mauville

### Arc 3 — Wattson through Flannery

- Route 117/Verdanturf
- Routes 111/112/Fiery Path
- Fallarbor/Route 114
- Meteor Falls
- Mt. Chimney
- Jagged Pass/Lavaridge

### Arc 4 — Flannery through Norman

- Petalburg/Norman

### Arc 5 — Norman through Winona

- Routes 118/119
- Weather Institute
- Route 119 rival
- Fortree/Route 120/Devon Scope
- Winona

### Arc 6 — Winona through Tate & Liza / Mossdeep

- Route 121/Lilycove
- Mt. Pyre
- Magma Hideout
- Aqua Hideout
- Routes 124/Mossdeep
- Mossdeep Space Center

### Arc 7 — Tate & Liza through Juan

- Seafloor Cavern
- Kyogre awakening
- Sootopolis crisis
- Sky Pillar
- Return from Sky Pillar
- Post-Rayquaza premium encounters
- Juan/Gym 8

### Arc 8 — Juan through Champion

- Ever Grande
- Victory Road
- Pokémon League

## Required workflow for Arcs 1-8

1. **Planning:** Inspect the actual code first. Produce a separate sub-plan for
   every area in the arc covering scripts, flags and state variables, warps,
   rewards, dependencies, and acceptance checks. Obtain human approval, then
   write the approved plan to `docs/PHASE10_ARC<N>_PLAN.md`.
2. **Execution:** Read that approved plan file and implement only its approved
   scope. Build and inspect the resulting diff.
3. **Review:** Perform a separate, read-only review against both SPEC and the
   approved plan.
4. **Playtest:** The user continuously plays through the entire arc in mGBA.
   Isolated flag-driven checks supplement that traversal; they do not replace
   it.
5. **Completion:** Commit and push only after implementation, build, read-only
   review, and user playtesting all pass.

Any scope change requires renewed human approval and an updated approved plan
file. A large arc may be divided into smaller approved implementation steps
when inspected dependencies justify it. Do not begin a later arc while the
current arc has an unresolved gate.

## Arc 0 existing-state record

Arc 0 predates this workflow. This is an inventory and status record, not a
retroactive plan and not authorization to rebuild or alter its gameplay.

### Starting points

- `de53064ca8`: completed Phase 9.5 checkpoint.
- `af88cb09ad`: committed Littleroot opening and first-rival Phase 10 work.
- `3c9de94ab9`: subsequent committed fixes, preserved as the accepted baseline
  and current `HEAD` when this record was created.
- Uncommitted Arc 0 work: Petalburg city/Gym scripts and city triggers;
  Petalburg Woods script; Rustboro city/Gym scripts and city triggers.

### Source-level inventory and review

- **Littleroot opening:** The committed work skips the moving truck, initializes
  the equivalent story/visibility state for both player genders, enables
  running immediately, preserves clock setting when the skip preference is
  disabled, removes the forced rival-house trip, shortens the Birch rescue
  sequence, and retains starter selection and the rescue battle.
- **First rival battle:** The committed work retains the Route 103 battle,
  shortens its surrounding dialogue/movement, transitions directly to the lab,
  grants the Pokédex, Balls, and Running Shoes in the compressed lab flow, and
  sets terminal rival/Oldale state to prevent replay on re-entry. Starter
  nickname enforcement and the Nuzlocke first-actual-Ball gate are wired into
  this opening. The accepted follow-up commit moves the optional 999-Ball NPC
  into accessible Oldale and makes any actual Ball acquisition latch the gate.
- **Petalburg/Wally tutorial:** The uncommitted work removes the city gym-guide
  escort and Wally tutorial, fast-forwards the tutorial's required state and
  visibility effects on first entry, and preserves Norman's early badge-count
  gate. Roxanne's badge script defensively floors Norman's counter before
  incrementing it.
- **Petalburg Woods:** The uncommitted work removes only the Devon employee's
  look-around filler and shortens the associated dialogue. The encounter area,
  Aqua grunt battle, Great Ball reward, battle-loss handling, and path-specific
  object movements remain.
- **Rustboro/Roxanne:** Roxanne's Gym battle and normal badge/TM rewards remain.
  The uncommitted city work folds the second stop into the post-Gym theft scene,
  restores the Devon employee to the street, and sets the Route 116/Rusturf
  Tunnel state and visibility flags immediately.

No source-level Arc 0 SPEC item was identified as intentionally incomplete in
this baseline review. That does not establish emulator acceptance: map-script
timing, object visibility, loss paths, and re-entry behavior still need the
continuous mGBA traversal below.

### Acceptance status

| Gate | Status |
|---|---|
| Implementation | Present in committed and preserved uncommitted work described above. |
| Build | Passed on 2026-09-09: direct scans of the Rustboro Gym script and aggregate `data/event_scripts.s` exited 0, and `make -j$(sysctl -n hw.ncpu)` exited 0 with the Rustboro diagnostic absent. The linker still emits its unrelated RWX-segment warning. |
| Read-only review | Performed against the five Arc 0 SPEC entries; no additional gameplay edit was authorized or made. |
| Emulator acceptance | Pending the full mGBA checks below. Arc 0 is not complete. |

### Required Arc 0 mGBA playtest

- Start fresh games with both player genders and with both clock-setting
  preferences.
- Verify starter selection and nickname handling, Birch rescue progression,
  first-rival victory and loss, the lab transition, and re-entry without
  repeated scenes.
- Verify Nuzlocke activation before versus after actual Ball acquisition.
- Pass through Petalburg without Wally's tutorial and verify Norman's early
  gate remains intact.
- Enter Petalburg Woods from both approaches and verify battle/loss/retry,
  reward handling, object movements, and encounter access.
- Defeat Roxanne and verify the badge, level-cap and Norman-counter changes,
  Devon employee visibility, and the theft handoff into Route 116/Rusturf
  Tunnel.

Arc 1 is unstarted and must not begin until the remaining Arc 0 acceptance
gates pass.
