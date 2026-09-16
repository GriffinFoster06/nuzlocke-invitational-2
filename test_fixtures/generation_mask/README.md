# generation_mask

**Purpose:** quick boundary testing of the generation filter (wild/trainer/
evolution eligibility) with an extreme, easy-to-eyeball mask.

**Starting state:** Randomizer preset with the generation filter set to:

- Gen 1: **ON**
- Gen 2-8: **OFF**
- Gen 9: **ON**

Run seed fixed to `1`. Player has a low-level Treecko (unaffected by the
mask itself — it's the player's existing mon) and starts on Route 101's
grass.

**Setup:** load; the player starts standing in Route 101 grass, ready to
walk into an encounter immediately.

**What to observe:**

- Wild encounters on Route 101 should only ever resolve to Gen 1 or Gen 9
  species (`src/species_generation.c` classification feeding
  `src/power_score.c`'s eligibility cache).
- Any trainer or evolution result reachable from here should honor the same
  mask (no Gen 2-8 species appearing as a wild mon, trainer party member, or
  evolution target).

**Difficulty/settings:** the mask itself is the point of the fixture; RULES
> AI Difficulty doesn't matter here.

**Savestate:** none.
