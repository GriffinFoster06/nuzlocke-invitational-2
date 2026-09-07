#ifndef GUARD_LEARNSET_GEN_H
#define GUARD_LEARNSET_GEN_H

// ============================================================================
// Phase 4 - generated level-up learnsets (docs/SPEC.md "Randomized level-up
// moves", "Learnset size", "7/7/7 learnset composition", "Move-power
// progression").
//
// When SETTING_MOVE_RANDOMIZATION is on, every species gets a deterministic
// generated learnset of SETTING_LEARNSET_SIZE moves (default 21) placed on an
// even level grid (1, 4, 7, ... 61 for N=21). Default composition is 7 STAB
// damaging / 7 other damaging / 7 status, with stronger attacks weighted
// toward later levels. The learnset for a (run seed, species) pair never
// changes.
//
// Hook point: GetSpeciesLevelUpLearnset() in src/pokemon.c.
// ============================================================================

struct LevelUpMove;

bool32 LearnsetGen_IsActive(void); // SETTING_MOVE_RANDOMIZATION

// Generated learnset for `species`, terminated by LEVEL_UP_MOVE_END. The
// pointer is stable until the next cache eviction, so callers must not hold it
// across an unrelated GetSpeciesLevelUpLearnset() call for a different species
// (the LRU cache has a few slots for exactly that pattern). Returns NULL only
// if the move pool is empty (pathological ban settings) - the caller then
// falls back to the canonical learnset.
const struct LevelUpMove *LearnsetGen_GetLearnset(enum Species species);

void LearnsetGen_EnsureBuilt(void); // rebuild pool / reset cache if settings moved
void LearnsetGen_Invalidate(void);  // force the above on next call

#endif // GUARD_LEARNSET_GEN_H
