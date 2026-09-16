#ifndef GUARD_POWER_SCORE_H
#define GUARD_POWER_SCORE_H

// ============================================================================
// Intrinsic species power score (docs/SPEC.md "Species power matching").
//
// One scalar per species in BST-like units (~170-800), combining BST with
// offensive efficiency, defensive efficiency, a speed premium, special-species
// classification, canonical ability restraints (independent of randomized
// ability settings), and a blend toward the strongest
// reachable final evolution. Also exposes the evolutionary-stage bucket and the
// species-pool eligibility / category-ban predicates the selector needs.
//
// The score table is built once at runtime into EWRAM from gSpeciesInfo (base
// stats in the species headers are config-dependent expressions, so a build-
// time parse would be wrong) and rebuilt when a species-pool ruleset toggle
// changes. Phase 2.
// ============================================================================

// Mirrors PWRMATCH_* in include/constants/ruleset.h.
enum PowerMatchMode
{
    POWER_MATCH_STRICT,
    POWER_MATCH_NORMAL,
    POWER_MATCH_LOOSE,
    POWER_MATCH_BST_ONLY,
    POWER_MATCH_UNRESTRICTED,
};

enum EvoStageBucket
{
    EVO_BUCKET_UNEVOLVED,   // no pre-evolution, has an evolution
    EVO_BUCKET_MIDDLE,      // has both
    EVO_BUCKET_FINAL,       // has a pre-evolution or none, no evolution (single-stage -> FINAL)
};

// Build / invalidate the EWRAM score cache. EnsureBuilt is cheap once warm and
// rebuilds itself if a relevant ruleset toggle changed since the last build.
void PowerScore_EnsureBuilt(void);
void PowerScore_Invalidate(void);

// Blended power (potential-aware). Integer BST-like units.
u32 GetSpeciesPowerScore(enum Species species);
// Pre-blend power of this exact species (no evolution look-ahead).
u32 GetSpeciesRawPowerScore(enum Species species);
// The metric the selector compares under a given match mode (BST for BST_ONLY,
// blended power otherwise).
u32 GetSpeciesMatchMetric(enum Species species, enum PowerMatchMode mode);

enum EvoStageBucket GetSpeciesEvoStageBucket(enum Species species);

// Ascending-order dense list of every species currently passing
// InPool(s, POOL_STRICT_ORDINARY) in src/randomizer.c (power-eligible, not
// category-banned, not Premium) - built alongside the score cache so the
// selector's hot loop can walk this instead of re-testing every one of
// NUM_SPECIES candidates. Returns the count and points *out at the list
// (stable until the next EnsureBuilt rebuild); 0/unchanged *out on
// out-of-heap build failure.
u32 PowerScore_OrdinaryList(const u16 **out);

// Species-pool predicates, evaluated against the current ruleset toggles.
bool32 IsSpeciesPowerEligible(enum Species species);   // enabled, not a hard-excluded form, form-category gates
bool32 IsSpeciesCategoryBanned(enum Species species);  // legendary/mythical/sub/UB/paradox vs SETTING_ALLOW_*
bool32 IsSpeciesPremium(enum Species species);         // immutable upstream traits + project designations
bool32 IsSpeciesPremiumTier(enum Species species);     // curated premium membership (Premium && power >= floor)

// Phase 11A.6 (docs/SPEC.md "Generation filters"): is `species`' introducing
// generation currently enabled? Exposed (beyond ComputeEligible's internal
// use) for GetSpeciesEvolutions' evolution-mask filter in src/pokemon.c.
bool32 IsSpeciesGenerationEnabled(enum Species species);
// True once any of the nine generation toggles is off - lets a caller (the
// evolution-mask filter) skip its own work entirely in the common
// all-enabled case without needing to know the SETTING_GEN_* ids itself.
bool32 PowerScore_AnyGenerationDisabled(void);

#endif // GUARD_POWER_SCORE_H
