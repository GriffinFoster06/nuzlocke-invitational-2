#ifndef GUARD_POWER_SCORE_H
#define GUARD_POWER_SCORE_H

// ============================================================================
// Intrinsic species power score (docs/SPEC.md "Species power matching").
//
// One scalar per species in BST-like units (~170-800), combining BST with
// offensive efficiency, defensive efficiency, a speed premium, special-species
// classification, an "artificially restrained by a crippling ability" flag
// (inert unless ability randomization is off), and a blend toward the strongest
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

// Species-pool predicates, evaluated against the current ruleset toggles.
bool32 IsSpeciesPowerEligible(enum Species species);   // enabled, not a hard-excluded form, form-category gates
bool32 IsSpeciesCategoryBanned(enum Species species);  // legendary/mythical/sub/UB/paradox vs SETTING_ALLOW_*
bool32 IsSpeciesPremiumTier(enum Species species);     // curated premium pool membership (blended power >= floor)

#endif // GUARD_POWER_SCORE_H
