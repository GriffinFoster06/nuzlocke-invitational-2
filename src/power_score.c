// ============================================================================
// Intrinsic species power score - docs/SPEC.md "Species power matching".
// See include/power_score.h for the model overview. Phase 2.
// ============================================================================

#include "global.h"
#include "malloc.h"
#include "pokemon.h"
#include "power_score.h"
#include "ruleset.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// ---- fixed-point (Q8, 256 == 1.0) tunables ------------------------------------
#define Q8                  256
#define MIX_CREDIT_Q8       64    // 0.25: credit given to the weaker offensive stat
#define OFF_DEN_Q8          160   // 0.625: (1 + MIX_CREDIT) / 2, the OffEff denominator scale
#define K_OFF_Q8            26    // 0.10: weight of the offensive-efficiency correction
#define K_DEF_Q8            64    // 0.25: weight of the defensive-efficiency correction
#define K_SPE_Q8            31    // 0.12: weight of the speed premium
#define SPE_ANCHOR          75
#define SPE_CLAMP           150
#define W_RAW_Q8            166   // 0.65: own strength
#define W_FINAL_Q8          90    // 0.35: strongest reachable final form (W_RAW + W_FINAL == 256)

#define CLASSMUL_RESTRICTED 269   // 1.05  (restricted legendary / mythical)
#define CLASSMUL_SUB        262   // 1.025 (sub-legendary / Ultra Beast / Paradox)
#define CLASSMUL_NONE       256

#define PREMIUM_TIER_FLOOR  600   // blended-power cutoff for the curated premium pool

#define POWER_CACHE_MAX     4095  // 12-bit field

// ---- packed per-species cache ----------------------------------------------
// power:12 | stage:2 | eligible:1 | catBanned:1
#define PS_POWER(v)      ((v) & 0x0FFF)
#define PS_STAGE(v)      (((v) >> 12) & 0x3)
#define PS_ELIGIBLE(v)   (((v) >> 14) & 0x1)
#define PS_CATBANNED(v)  (((v) >> 15) & 0x1)
#define PS_PACK(power, stage, elig, banned) \
    ((u16)((power) | ((stage) << 12) | ((elig) << 14) | ((banned) << 15)))

static EWRAM_DATA u16 sScoreCache[NUM_SPECIES] = {0};
static EWRAM_DATA bool8 sBuilt = FALSE;         // gates the very first build; sSignature is only trusted once set
static EWRAM_DATA u8 sSignature = 0;            // ruleset-toggle snapshot the cache was built for

// Snapshot of the species-pool toggles that affect eligibility / bans, so
// EnsureBuilt can detect a stale cache after a menu change.
static u8 CurrentSignature(void)
{
    return (GetRulesetSetting(SETTING_ALLOW_LEGENDARY)      ? (1 << 0) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_MYTHICAL)       ? (1 << 1) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_SUB_LEGENDARY)  ? (1 << 2) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_ULTRA_BEAST)    ? (1 << 3) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_PARADOX)        ? (1 << 4) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_REGIONAL_FORMS) ? (1 << 5) : 0)
         | (GetRulesetSetting(SETTING_ALLOW_OTHER_FORMS)    ? (1 << 6) : 0)
         | (GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION)? (1 << 7) : 0);
}

// ---- helpers over gSpeciesInfo ---------------------------------------------

static inline u32 Bst(const struct SpeciesInfo *si)
{
    return si->baseHP + si->baseAttack + si->baseDefense
         + si->baseSpeed + si->baseSpAttack + si->baseSpDefense;
}

static u32 OffEffQ8(const struct SpeciesInfo *si)
{
    u32 atk = si->baseAttack, spa = si->baseSpAttack;
    u32 hi = (atk > spa) ? atk : spa;
    u32 lo = (atk > spa) ? spa : atk;
    u32 raw = atk + spa;

    if (raw == 0)
        return Q8;
    // (hi + 0.25*lo) / (0.625 * raw), in Q8
    return ((hi * Q8) + (lo * MIX_CREDIT_Q8)) * Q8 / (raw * OFF_DEN_Q8);
}

static u32 DefEffQ8(const struct SpeciesInfo *si)
{
    u32 hp = si->baseHP, def = si->baseDefense, spd = si->baseSpDefense;
    u32 sum = hp + def + spd;
    u32 bulk;

    if (sum == 0)
        return Q8;
    // bulk = HP * (Def + SpD)/2 ;  ideal = sum/3 ;  return bulk / ideal^2, in Q8
    // = bulk * 256 * 9 / sum^2
    bulk = hp * (def + spd) / 2;
    return bulk * (Q8 * 9) / (sum * sum);
}

static s32 SpeTermQ8(const struct SpeciesInfo *si)
{
    s32 spe = si->baseSpeed;
    if (spe > SPE_CLAMP)
        spe = SPE_CLAMP;
    return (spe - SPE_ANCHOR) * Q8 / SPE_ANCHOR;   // -256 .. +256
}

static u32 ClassMulQ8(const struct SpeciesInfo *si)
{
    if (si->isRestrictedLegendary || si->isMythical)
        return CLASSMUL_RESTRICTED;
    if (si->isSubLegendary || si->isUltraBeast || si->isParadox)
        return CLASSMUL_SUB;
    return CLASSMUL_NONE;
}

// Applied only when ability randomization is OFF: the species keeps its
// canonical, sometimes crippling, ability. Q8.
static u32 AbilityAdjQ8(enum Species species)
{
    if (GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION))
        return Q8;

    switch (species)
    {
    case SPECIES_SLAKING:   return 159;  // 0.62  Truant
    case SPECIES_REGIGIGAS: return 179;  // 0.70  Slow Start
    case SPECIES_ARCHEOPS:  return 200;  // 0.78  Defeatist
    case SPECIES_SHEDINJA:  return 90;   // 0.35  1 HP, Wonder Guard is a coin flip
    case SPECIES_AZUMARILL: return 346;  // 1.35  Huge Power
    case SPECIES_MEDICHAM:  return 346;  // 1.35  Pure Power
    case SPECIES_DIGLETT:
    case SPECIES_DUGTRIO:   return 333;  // 1.30  Arena Trap
    default:                return Q8;
    }
}

static u32 RawScore(enum Species species)
{
    const struct SpeciesInfo *si = &gSpeciesInfo[species];
    s32 acc = (s32)Bst(si) * Q8;   // Q8
    s32 f;

    f = Q8 + (s32)K_OFF_Q8 * ((s32)OffEffQ8(si) - Q8) / Q8;
    acc = acc * f / Q8;
    f = Q8 + (s32)K_DEF_Q8 * ((s32)DefEffQ8(si) - Q8) / Q8;
    acc = acc * f / Q8;
    f = Q8 + (s32)K_SPE_Q8 * SpeTermQ8(si) / Q8;
    acc = acc * f / Q8;
    acc = acc * (s32)ClassMulQ8(si) / Q8;
    acc = acc * (s32)AbilityAdjQ8(species) / Q8;

    acc /= Q8;   // back to integer BST-like units
    if (acc < 0)
        acc = 0;
    return (u32)acc;
}

static bool32 SpeciesHasRealEvolution(enum Species species)
{
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    u32 i;

    if (evos == NULL)
        return FALSE;
    for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
    {
        enum Species t = evos[i].targetSpecies;

        if (evos[i].method == EVO_NONE)
            continue;
        // Test enabled-ness before SanitizeSpeciesId: the latter asserts on a
        // disabled ID (e.g. the dataless SPECIES_LUGIA_SHADOW).
        if (t != SPECIES_NONE && t < NUM_SPECIES && IsSpeciesEnabled(t))
            return TRUE;
    }
    return FALSE;
}

// max RawScore over species reachable through real evolutions, incl. self.
// Memoised in the scratch table; depth-bounded against pathological data.
static u16 BestFinalRaw(enum Species species, u16 *rawTbl, u16 *memo, u32 depth)
{
    const struct Evolution *evos;
    u16 best;
    u32 i;

    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return 0;
    if (memo[species] != 0)
        return memo[species];

    best = rawTbl[species];
    memo[species] = best ? best : 1;   // provisional, breaks cycles

    if (depth < 5)
    {
        evos = GetSpeciesEvolutions(species);
        for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
        {
            enum Species t = evos[i].targetSpecies;
            u16 sub;

            // Filter before any sanitizing accessor: a disabled target (the
            // dataless SPECIES_LUGIA_SHADOW) would assert in GetSpeciesEvolutions.
            if (evos[i].method == EVO_NONE || t == SPECIES_NONE || t == species
             || t >= NUM_SPECIES || !IsSpeciesEnabled(t))
                continue;
            sub = BestFinalRaw(t, rawTbl, memo, depth + 1);
            if (sub > best)
                best = sub;
        }
    }

    memo[species] = best ? best : 1;
    return memo[species];
}

static bool32 IsHardExcludedForm(const struct SpeciesInfo *si)
{
    return si->isMegaEvolution || si->isPrimalReversion || si->isGigantamax
        || si->isTotem || si->isTeraForm || si->isUltraBurst || si->cannotBeTraded;
}

static bool32 ComputeEligible(enum Species species, const struct SpeciesInfo *si)
{
    bool32 isRegional;

    if (!IsSpeciesEnabled(species) || species == SPECIES_EGG)
        return FALSE;
    if (IsHardExcludedForm(si))
        return FALSE;

    isRegional = si->isAlolanForm || si->isGalarianForm || si->isHisuianForm || si->isPaldeanForm;
    if (isRegional)
        return GetRulesetSetting(SETTING_ALLOW_REGIONAL_FORMS) != 0;

    // A non-base alternate form (Rotom-Heat, Deoxys-Attack, ...) that isn't a
    // regional form is gated by the "other forms" toggle. Base species always pass.
    if (species != GET_BASE_SPECIES_ID(species))
        return GetRulesetSetting(SETTING_ALLOW_OTHER_FORMS) != 0;

    return TRUE;
}

static bool32 ComputeCategoryBanned(const struct SpeciesInfo *si)
{
    if (si->isRestrictedLegendary && !GetRulesetSetting(SETTING_ALLOW_LEGENDARY))
        return TRUE;
    if (si->isMythical && !GetRulesetSetting(SETTING_ALLOW_MYTHICAL))
        return TRUE;
    if (si->isSubLegendary && !GetRulesetSetting(SETTING_ALLOW_SUB_LEGENDARY))
        return TRUE;
    if (si->isUltraBeast && !GetRulesetSetting(SETTING_ALLOW_ULTRA_BEAST))
        return TRUE;
    if (si->isParadox && !GetRulesetSetting(SETTING_ALLOW_PARADOX))
        return TRUE;
    return FALSE;
}

// ---- build ----------------------------------------------------------------

void PowerScore_Invalidate(void)
{
    sBuilt = FALSE;
}

void PowerScore_EnsureBuilt(void)
{
    u16 *rawTbl;
    u16 *memo;
    u8 *hasPreBits;
    enum Species s;
    u32 i, raw;

    if (sBuilt && sSignature == CurrentSignature())
        return;

    rawTbl     = AllocZeroed(sizeof(u16) * NUM_SPECIES);
    memo       = AllocZeroed(sizeof(u16) * NUM_SPECIES);
    hasPreBits = AllocZeroed(sizeof(u8) * NUM_SPECIES);
    if (rawTbl == NULL || memo == NULL || hasPreBits == NULL)
    {
        // Out of heap: leave the cache marked unbuilt; callers fall back to the
        // vanilla species. Extremely unlikely at the points this is called.
        if (rawTbl != NULL) Free(rawTbl);
        if (memo != NULL) Free(memo);
        if (hasPreBits != NULL) Free(hasPreBits);
        return;
    }

    // Pass 1: raw score per species + a forward-built "has a pre-evolution" flag
    // (avoids GetSpeciesPreEvolution's O(N*evos) reverse scan per species).
    for (s = 1; s < NUM_SPECIES; s++)
    {
        const struct Evolution *evos;

        if (!IsSpeciesEnabled(s))
            continue;

        raw = RawScore(s);
        rawTbl[s] = (raw > POWER_CACHE_MAX) ? POWER_CACHE_MAX : raw;

        evos = GetSpeciesEvolutions(s);
        for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
        {
            enum Species t = evos[i].targetSpecies;
            // Guard before SanitizeSpeciesId, which asserts on a disabled ID.
            if (evos[i].method != EVO_NONE && t != SPECIES_NONE && t < NUM_SPECIES
             && IsSpeciesEnabled(t))
                hasPreBits[t] = 1;
        }
    }

    // Pass 2: blend toward the strongest reachable final form, bucket the stage,
    // evaluate the pool predicates, and pack.
    for (s = 1; s < NUM_SPECIES; s++)
    {
        const struct SpeciesInfo *si = &gSpeciesInfo[s];
        u32 blended, stage;
        bool32 hasPre, hasEvo, elig, banned;

        if (!IsSpeciesEnabled(s))
        {
            sScoreCache[s] = 0;
            continue;
        }

        blended = ((u32)W_RAW_Q8 * rawTbl[s]
                 + (u32)W_FINAL_Q8 * BestFinalRaw(s, rawTbl, memo, 0)) / Q8;
        if (blended > POWER_CACHE_MAX)
            blended = POWER_CACHE_MAX;

        hasPre = hasPreBits[s] != 0;
        hasEvo = SpeciesHasRealEvolution(s);
        if (!hasPre && hasEvo)
            stage = EVO_BUCKET_UNEVOLVED;
        else if (hasPre && hasEvo)
            stage = EVO_BUCKET_MIDDLE;
        else
            stage = EVO_BUCKET_FINAL;

        elig = ComputeEligible(s, si);
        banned = ComputeCategoryBanned(si);

        sScoreCache[s] = PS_PACK(blended, stage, elig ? 1 : 0, banned ? 1 : 0);
    }

    Free(rawTbl);
    Free(memo);
    Free(hasPreBits);
    sSignature = CurrentSignature();
    sBuilt = TRUE;
}

// ---- accessors ----------------------------------------------------------------

static u16 CacheEntry(enum Species species)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return 0;
    // Cold-build only. Staleness after a ruleset change is handled by
    // PowerScore_Invalidate() (wired into the settings setter), so the
    // per-candidate calls in the selector's hot loop stay a plain array read.
    if (!sBuilt)
        PowerScore_EnsureBuilt();
    return sScoreCache[species];
}

u32 GetSpeciesPowerScore(enum Species species)
{
    return PS_POWER(CacheEntry(species));
}

u32 GetSpeciesRawPowerScore(enum Species species)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return 0;
    return RawScore(species);
}

u32 GetSpeciesMatchMetric(enum Species species, enum PowerMatchMode mode)
{
    if (mode == POWER_MATCH_BST_ONLY)
        return GetSpeciesBaseStatTotal(species);
    return GetSpeciesPowerScore(species);
}

enum EvoStageBucket GetSpeciesEvoStageBucket(enum Species species)
{
    return (enum EvoStageBucket)PS_STAGE(CacheEntry(species));
}

bool32 IsSpeciesPowerEligible(enum Species species)
{
    return PS_ELIGIBLE(CacheEntry(species)) != 0;
}

bool32 IsSpeciesCategoryBanned(enum Species species)
{
    return PS_CATBANNED(CacheEntry(species)) != 0;
}

bool32 IsSpeciesPremiumTier(enum Species species)
{
    return GetSpeciesPowerScore(species) >= PREMIUM_TIER_FLOOR;
}
