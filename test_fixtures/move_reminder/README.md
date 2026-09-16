# move_reminder

**Purpose:** verify Move Reminder behavior — a mon whose generated level-up
learnset includes moves it has already passed the level for, but doesn't
know yet.

**Starting state:** Recommended preset, 6 badges. Player has one mon in
Fallarbor Town, near the move relearner:

- Gardevoir Lv30, Modest — only Confusion / Growl explicitly taught (the
  rest of its generated level-up learnset up to Lv30 is left unlearned)
- Bag: 10x Heart Scale

**Setup:** load; the player starts in Fallarbor Town already near the move
relearner NPC. Open the party menu / talk to the relearner (or use the
debug menu's "Move Relearner" action under Party…) to see which level-up
moves are offered.

**What to observe:** Gardevoir should have several relearnable moves listed
(already-reached levels not currently known). Check this list against the
actual generated learnset (`src/learnset_gen.c` output for this seed) to
confirm accuracy, and check behavior across each RULES > Move Reminder Mode
value:

- **Disabled** — no relearning available at all (Fallarbor NPC included)
- **Normal** — Fallarbor Heart Scale NPC only
- **Free** / **Previously Learned** — also unlocks the free summary-screen
  relearner

**Savestate:** none.
