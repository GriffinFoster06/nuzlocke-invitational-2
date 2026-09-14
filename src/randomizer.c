// ============================================================================
// Phase 2 core randomizer wiring. See include/randomizer.h for the overview.
// Scoring / eligibility / stage buckets live in src/power_score.c.
// ============================================================================

#include "global.h"
#include "caps.h"
#include "data.h"
#include "event_data.h"
#include "fishing.h"
#include "item.h"
#include "move.h"
#include "learnset_gen.h"
#include "pokemon.h"
#include "power_score.h"
#include "random.h"
#include "randomizer.h"
#include "run_rng.h"
#include "string_util.h"
#include "ruleset.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"
#include "constants/trainers.h"

// Per-category salts and the seed helper now live in include/run_rng.h so the
// Phase 4 learnset generator shares one implementation.

enum PoolKind { POOL_ORDINARY, POOL_STRICT_ORDINARY, POOL_PREMIUM, POOL_ORDINARY_OR_PREMIUM };

// ---- enable checks --------------------------------------------------------

bool32 Randomizer_WildEnabled(void)      { return GetRulesetSetting(SETTING_WILD_RANDOMIZATION) != 0; }
bool32 Randomizer_StarterEnabled(void)   { return GetRulesetSetting(SETTING_STARTER_RANDOMIZATION) != 0; }
bool32 Randomizer_GiftEnabled(void)      { return GetRulesetSetting(SETTING_GIFT_RANDOMIZATION) != 0; }
bool32 Randomizer_StaticEnabled(void)    { return GetRulesetSetting(SETTING_STATIC_RANDOMIZATION) != 0; }
bool32 Randomizer_LegendaryEnabled(void) { return GetRulesetSetting(SETTING_LEGENDARY_RANDOMIZATION) != 0; }
bool32 Randomizer_TrainerEnabled(void)   { return GetRulesetSetting(SETTING_TRAINER_RANDOMIZATION) != 0; }

// ---- seeding ------------------------------------------------------------------

#define SeedFor(salt, k0, k1, k2) RunRng_Seed((salt), (k0), (k1), (k2))

static bool32 IsReplaceableTarget(enum Species species)
{
    return species > SPECIES_NONE
        && species < NUM_SPECIES
        && species != SPECIES_EGG
        && IsSpeciesEnabled(species);
}

// ---- pool membership --------------------------------------------------------

static bool32 InPool(enum Species s, enum PoolKind kind)
{
    if (!IsSpeciesPowerEligible(s))
        return FALSE;

    if (kind == POOL_ORDINARY)
        return !IsSpeciesCategoryBanned(s);

    if (kind == POOL_STRICT_ORDINARY)
        return !IsSpeciesCategoryBanned(s) && !IsSpeciesPremium(s);

    if (kind == POOL_ORDINARY_OR_PREMIUM)
        return InPool(s, POOL_STRICT_ORDINARY) || InPool(s, POOL_PREMIUM);

    switch (GetRulesetSetting(SETTING_PREMIUM_POOL_MODE))
    {
    case PREMPOOL_SAME_AS_NORMAL:
    case PREMPOOL_ALL_LEGENDARY:
        return IsSpeciesPremium(s);
    case PREMPOOL_CURATED:
    default:
        return IsSpeciesPremiumTier(s);
    }
}

// ---- the count-and-pick selector -----------------------------------------------

struct LadderRung
{
    u32 window;       // absolute power distance; ignored when `unbounded`
    bool8 exactStage;
    bool8 unbounded;
};

static u32 BuildLadder(struct LadderRung *out, enum PowerMatchMode mode, u32 evoMode)
{
    static const u16 sBaseWindow[] =
    {
        [POWER_MATCH_STRICT]       = 30,
        [POWER_MATCH_NORMAL]       = 60,
        [POWER_MATCH_LOOSE]        = 110,
        [POWER_MATCH_BST_ONLY]     = 60,
        [POWER_MATCH_UNRESTRICTED] = 0,
    };
    bool32 unbounded = (mode == POWER_MATCH_UNRESTRICTED);
    u32 w = sBaseWindow[mode];
    u32 n = 0;

    if (evoMode == EVOSTAGE_STRICT)
    {
        out[n++] = (struct LadderRung){ w,        TRUE,  unbounded };
        out[n++] = (struct LadderRung){ w * 3/2,  TRUE,  unbounded };
        out[n++] = (struct LadderRung){ w * 3,    TRUE,  unbounded };
        out[n++] = (struct LadderRung){ 0,        TRUE,  TRUE      };
    }
    else if (evoMode == EVOSTAGE_OFF)
    {
        out[n++] = (struct LadderRung){ w,        FALSE, unbounded };
        out[n++] = (struct LadderRung){ w * 3/2,  FALSE, unbounded };
        out[n++] = (struct LadderRung){ w * 3,    FALSE, unbounded };
        out[n++] = (struct LadderRung){ 0,        FALSE, TRUE      };
    }
    else // EVOSTAGE_PREFER
    {
        out[n++] = (struct LadderRung){ w,        TRUE,  unbounded };
        out[n++] = (struct LadderRung){ w,        FALSE, unbounded };
        out[n++] = (struct LadderRung){ w * 3/2,  FALSE, unbounded };
        out[n++] = (struct LadderRung){ w * 3,    FALSE, unbounded };
        out[n++] = (struct LadderRung){ 0,        FALSE, TRUE      };
    }
    return n;
}

static bool32 RungAccepts(const struct LadderRung *rung, enum Species s,
                          enum PowerMatchMode mode, u32 target, enum EvoStageBucket tgtStage)
{
    if (rung->exactStage && GetSpeciesEvoStageBucket(s) != tgtStage)
        return FALSE;
    if (!rung->unbounded)
    {
        u32 m = GetSpeciesMatchMetric(s, mode);
        u32 d = (m > target) ? m - target : target - m;
        if (d > rung->window)
            return FALSE;
    }
    return TRUE;
}

static bool32 SpeciesIsExcluded(enum Species species, const enum Species *excluded, u32 excludedCount)
{
    u32 i;
    for (i = 0; i < excludedCount; i++)
        if (excluded[i] == species)
            return TRUE;
    return FALSE;
}

static enum Species PickReplacementCoreExcluding(rng_value_t *st, enum Species vanilla, enum PoolKind kind,
                                                  enum PowerMatchMode mode, u32 evoMode,
                                                  const enum Species *excluded, u32 excludedCount)
{
    u32 target = GetSpeciesMatchMetric(vanilla, mode);
    enum EvoStageBucket tgtStage = GetSpeciesEvoStageBucket(vanilla);
    struct LadderRung ladder[6];
    u32 rungs = BuildLadder(ladder, mode, evoMode);
    u32 i;

    for (i = 0; i < rungs; i++)
    {
        // Phase 11A.6: reservoir sampling (Algorithm R) - a single pass over
        // NUM_SPECIES instead of a count pass followed by a pick pass. Each
        // accepted candidate replaces the running choice with probability
        // 1/count-so-far, which is exactly uniform over every candidate this
        // rung accepts. This changes the RNG draw sequence for a given seed
        // relative to the old two-pass selector (RANDOMIZER_VERSION bumped).
        u32 count = 0;
        enum Species chosen = SPECIES_NONE;
        enum Species s;

        for (s = 1; s < NUM_SPECIES; s++)
        {
            if (!InPool(s, kind) || SpeciesIsExcluded(s, excluded, excludedCount))
                continue;
            if (!RungAccepts(&ladder[i], s, mode, target, tgtStage))
                continue;
            count++;
            if (LocalRandom32(st) % count == 0)
                chosen = s;
        }
        if (count == 0)
            continue;

        // Backstop: InPool() already excludes anything not power-eligible
        // (and 1435 is never eligible), but keep the guarantee local to the
        // one return point in case the score cache is ever stale.
        return IsSpeciesEnabled(chosen) ? chosen : vanilla;
    }
    return vanilla;
}

static enum Species PickReplacementCore(rng_value_t *st, enum Species vanilla, enum PoolKind kind,
                                        enum PowerMatchMode mode, u32 evoMode)
{
    return PickReplacementCoreExcluding(st, vanilla, kind, mode, evoMode, NULL, 0);
}

static enum Species PickReplacement(rng_value_t *st, enum Species vanilla, enum PoolKind kind)
{
    return PickReplacementCore(st, vanilla, kind,
                               GetRulesetSetting(SETTING_POWER_MATCHING),
                               GetRulesetSetting(SETTING_EVO_STAGE_MATCHING));
}

// ---- wild encounters --------------------------------------------------------

// Phase 11A.6: Randomizer_WildSlotSpecies(info, slot, vanilla) is a pure
// function of (info->slotSeed, slot, run seed, locked settings) for the
// whole run - the Deterministic World Principle guarantees a table's slot
// mapping can never change once generation settings are locked. Without a
// cache, NuzlockeChooseNonDupeSlot (src/wild_encounter.c) and the Pokedex
// area-search screen both re-run the full O(NUM_SPECIES) selector for the
// same (table, slot) every single time the player revisits a route - the
// single largest source of the pre-battle pause. Cache a handful of
// recently-touched tables (LRU), each holding up to
// NUM_LAND_MONS_ENCOUNTER_SLOTS resolved slots, resolved lazily on first
// query. Small and bounded: 6 lines x 12 slots x u16 plus tags/LRU, well
// under 300 bytes EWRAM.
#define WILD_SLOT_CACHE_LINES 6
#define WILD_SLOT_CACHE_SLOTS NUM_LAND_MONS_ENCOUNTER_SLOTS // 12; covers every area/rod range

static EWRAM_DATA u32 sWildSlotCacheTag[WILD_SLOT_CACHE_LINES] = {0}; // slotSeed + 1; 0 == empty line
static EWRAM_DATA u16 sWildSlotCacheSpecies[WILD_SLOT_CACHE_LINES][WILD_SLOT_CACHE_SLOTS];
static EWRAM_DATA u16 sWildSlotCacheValidBits[WILD_SLOT_CACHE_LINES] = {0}; // bit per resolved slot
static EWRAM_DATA u32 sWildSlotCacheLru[WILD_SLOT_CACHE_LINES] = {0};
static EWRAM_DATA u32 sWildSlotCacheClock = 0;

void Randomizer_InvalidateWildSlotCache(void)
{
    u32 i;
    for (i = 0; i < WILD_SLOT_CACHE_LINES; i++)
        sWildSlotCacheTag[i] = 0;
}

// Returns the cache line for `tag` (a table's slotSeed+1), creating/evicting
// one if this table isn't already resident.
static u32 FindOrClaimWildSlotCacheLine(u32 tag)
{
    u32 i, victim;

    for (i = 0; i < WILD_SLOT_CACHE_LINES; i++)
    {
        if (sWildSlotCacheTag[i] == tag)
        {
            sWildSlotCacheLru[i] = ++sWildSlotCacheClock;
            return i;
        }
    }

    victim = 0;
    for (i = 1; i < WILD_SLOT_CACHE_LINES; i++)
    {
        if (sWildSlotCacheLru[i] < sWildSlotCacheLru[victim])
            victim = i;
    }
    sWildSlotCacheTag[victim] = tag;
    sWildSlotCacheValidBits[victim] = 0;
    sWildSlotCacheLru[victim] = ++sWildSlotCacheClock;
    return victim;
}

enum Species Randomizer_WildSlotSpecies(const struct WildPokemonInfo *info, u32 slot, enum Species vanilla)
{
    rng_value_t st;
    u32 line = WILD_SLOT_CACHE_LINES; // sentinel: "don't cache this call"
    enum Species result;

    if (info == NULL || !Randomizer_WildEnabled() || !IsReplaceableTarget(vanilla))
        return vanilla;

    PowerScore_EnsureBuilt();

    if (slot < WILD_SLOT_CACHE_SLOTS)
    {
        line = FindOrClaimWildSlotCacheLine(info->slotSeed + 1);
        if (sWildSlotCacheValidBits[line] & (1u << slot))
            return sWildSlotCacheSpecies[line][slot];
    }

    switch (GetRulesetSetting(SETTING_ENCOUNTER_MAPPING))
    {
    case ENCMAP_ROUTE_SPECIES:
        st = SeedFor(SALT_WILD_ROUTE, info->routeSeed, vanilla, 0);
        break;
    case ENCMAP_GLOBAL:
        st = SeedFor(SALT_WILD_GLOBAL, vanilla, 0, 0);
        break;
    case ENCMAP_SLOT:
    default:
        st = SeedFor(SALT_WILD_SLOT, info->slotSeed, slot, 0);
        break;
    }
    // Phase 11B: docs/SPEC.md "Premium encounter balancing" - ordinary wild
    // encounter slots never generate Premium species. POOL_ORDINARY only
    // excludes category-banned species, not Premium ones; POOL_STRICT_ORDINARY
    // excludes both, so enabling a species-pool category toggle (e.g. Legendary)
    // can no longer leak that category into ordinary wild slots.
    result = PickReplacement(&st, vanilla, POOL_STRICT_ORDINARY);

    if (line < WILD_SLOT_CACHE_LINES)
    {
        sWildSlotCacheSpecies[line][slot] = result;
        sWildSlotCacheValidBits[line] |= (1u << slot);
    }
    return result;
}

u32 Randomizer_WildRateSlot(const struct WildPokemonInfo *info, enum WildPokemonArea area,
                            u8 rod, u32 slot)
{
    u8 permutation[NUM_LAND_MONS_ENCOUNTER_SLOTS];
    u32 start = 0, count, i;
    rng_value_t st;

    if (info == NULL || !GetRulesetSetting(SETTING_ENCOUNTER_RATE_RANDOMIZATION))
        return slot;
    switch (area)
    {
    case WILD_AREA_LAND:  count = NUM_LAND_MONS_ENCOUNTER_SLOTS; break;
    case WILD_AREA_WATER:
    case WILD_AREA_ROCKS: count = NUM_WATER_MONS_ENCOUNTER_SLOTS; break;
    case WILD_AREA_FISHING:
        if (rod == OLD_ROD)       { start = 0; count = 2; }
        else if (rod == GOOD_ROD) { start = 2; count = 3; }
        else                      { start = 5; count = 5; }
        break;
    default:
        return slot;
    }
    if (slot < start || slot >= start + count)
        return slot;

    for (i = 0; i < count; i++)
        permutation[i] = start + i;
    st = SeedFor(SALT_WILD_RATE, info->slotSeed, area, rod);
    for (i = count - 1; i > 0; i--)
    {
        u32 k = LocalRandom32(&st) % (i + 1);
        u8 tmp = permutation[i]; permutation[i] = permutation[k]; permutation[k] = tmp;
    }
    return permutation[slot - start];
}

enum Species Randomizer_SpecialWildSpecies(enum Species vanilla, u32 sourceKey, u32 routeKey)
{
    rng_value_t st;

    if (!Randomizer_WildEnabled() || !IsReplaceableTarget(vanilla))
        return vanilla;
    PowerScore_EnsureBuilt();
    switch (GetRulesetSetting(SETTING_ENCOUNTER_MAPPING))
    {
    case ENCMAP_ROUTE_SPECIES: st = SeedFor(SALT_WILD_SPECIAL, routeKey, vanilla, 0); break;
    case ENCMAP_GLOBAL:        st = SeedFor(SALT_WILD_SPECIAL, vanilla, 0, 0); break;
    case ENCMAP_SLOT:
    default:                   st = SeedFor(SALT_WILD_SPECIAL, sourceKey, 0, 0); break;
    }
    // Phase 11B: same ordinary-wild-slot Premium exclusion as
    // Randomizer_WildSlotSpecies (this covers Feebas/mass-outbreak special slots).
    return PickReplacement(&st, vanilla, POOL_STRICT_ORDINARY);
}

// ---- starters --------------------------------------------------------------

// Emerald build. (This is an Emerald hack; the FRLG starter trio is not handled.)
static const enum Species sVanillaStarters[3] =
{
    SPECIES_TREECKO,
    SPECIES_TORCHIC,
    SPECIES_MUDKIP,
};

static void ResolveStarterTrio(enum Species out[3])
{
    u32 i;

    for (i = 0; i < 3; i++)
    {
        enum Species vanilla = sVanillaStarters[i];
        rng_value_t st = SeedFor(SALT_STARTER, i, 0, 0);

        // Count-and-pick excludes prior choices directly, guaranteeing a
        // unique trio whenever the eligible rung contains three candidates.
        out[i] = PickReplacementCoreExcluding(&st, vanilla, POOL_ORDINARY,
                                               GetRulesetSetting(SETTING_POWER_MATCHING),
                                               EVOSTAGE_STRICT, out, i);
    }
}

enum Species Randomizer_StarterSpecies(u32 index)
{
    enum Species trio[3];

    if (index >= 3)
        return SPECIES_TREECKO;
    if (!Randomizer_StarterEnabled())
        return sVanillaStarters[index];

    PowerScore_EnsureBuilt();
    ResolveStarterTrio(trio);
    return trio[index];
}

void Randomizer_ApplyStarterIVs(struct Pokemon *mon)
{
    u32 mode = GetRulesetSetting(SETTING_STARTER_IV_MODE);
    rng_value_t st = SeedFor(SALT_STARTER_IV, 0, 0, 0);
    u8 order[NUM_STATS];
    u32 i, floor;

    if (mon == NULL || GetMonData(mon, MON_DATA_SPECIES_OR_EGG) == SPECIES_NONE)
        return;
    if (mode == IVMODE_NATURAL)
        return;

    for (i = 0; i < NUM_STATS; i++)
        order[i] = i;
    // Fisher-Yates over the local stream, so "which stat" choices are seeded.
    for (i = NUM_STATS - 1; i > 0; i--)
    {
        u32 k = LocalRandom32(&st) % (i + 1);
        u8 tmp = order[i]; order[i] = order[k]; order[k] = tmp;
    }

    switch (mode)
    {
    case IVMODE_ALL_31:
        for (i = 0; i < NUM_STATS; i++)
        {
            u8 v = MAX_PER_STAT_IVS;
            SetMonData(mon, MON_DATA_HP_IV + i, &v);
        }
        break;
    case IVMODE_3_PERFECT:
        for (i = 0; i < 3; i++)
        {
            u8 v = MAX_PER_STAT_IVS;
            SetMonData(mon, MON_DATA_HP_IV + order[i], &v);
        }
        break;
    case IVMODE_CUSTOM_FLOOR:
        floor = GetRulesetSetting(SETTING_STARTER_IV_FLOOR);
        for (i = 0; i < NUM_STATS; i++)
        {
            u8 cur = GetMonData(mon, MON_DATA_HP_IV + i);
            if (cur < floor)
            {
                u8 v = floor;
                SetMonData(mon, MON_DATA_HP_IV + i, &v);
            }
        }
        break;
    case IVMODE_5_PLUS_1:
    default:
        // Five perfect IVs; one randomly-chosen stat keeps a random roll.
        for (i = 0; i < NUM_STATS - 1; i++)
        {
            u8 v = MAX_PER_STAT_IVS;
            SetMonData(mon, MON_DATA_HP_IV + order[i], &v);
        }
        {
            u8 v = LocalRandom32(&st) % (MAX_PER_STAT_IVS + 1);
            SetMonData(mon, MON_DATA_HP_IV + order[NUM_STATS - 1], &v);
        }
        break;
    }

    CalculateMonStats(mon);
}

void Randomizer_MarkRunStarted(void)
{
    SetRulesetRunStarted(TRUE);
}

// ---- script gifts --------------------------------------------------------

static void ApplyGiftIvMode(struct PokemonTemplate *t, rng_value_t *st)
{
    u32 mode = GetRulesetSetting(SETTING_GIFT_IV_MODE);
    u8 order[NUM_STATS];
    u32 i, floor;

    // Natural mode deliberately leaves USE_RANDOM_IVS markers in place so the
    // normal template resolver can preserve fixed IVs and species-specific
    // perfect-IV counts.
    if (mode == GIFTIV_NATURAL)
        return;

    for (i = 0; i < NUM_STATS; i++)
        order[i] = i;
    for (i = NUM_STATS - 1; i > 0; i--)
    {
        u32 k = LocalRandom32(st) % (i + 1);
        u8 tmp = order[i]; order[i] = order[k]; order[k] = tmp;
    }

    // Resolve natural/template-random IVs from the gift-IV stream first. This
    // makes every IV mode reproducible without coupling it to gift species.
    for (i = 0; i < NUM_STATS; i++)
    {
        if (t->ivs[i] == USE_RANDOM_IVS)
            t->ivs[i] = LocalRandom32(st) % (MAX_PER_STAT_IVS + 1);
    }

    switch (mode)
    {
    case GIFTIV_ALL_31:
        for (i = 0; i < NUM_STATS; i++)
            t->ivs[i] = MAX_PER_STAT_IVS;
        break;
    case GIFTIV_CUSTOM_FLOOR:
        floor = GetRulesetSetting(SETTING_GIFT_IV_FLOOR);
        for (i = 0; i < NUM_STATS; i++)
            if (t->ivs[i] < floor)
                t->ivs[i] = floor;
        break;
    case GIFTIV_3_PERFECT:
    default:
        for (i = 0; i < 3; i++)
            t->ivs[order[i]] = MAX_PER_STAT_IVS;
        break;
    }
}

enum Species Randomizer_GiftSpecies(enum Species vanilla, u8 level, u32 sourceKey)
{
    rng_value_t st;

    if (!Randomizer_GiftEnabled() || !IsReplaceableTarget(vanilla))
        return vanilla;
    PowerScore_EnsureBuilt();
    st = SeedFor(SALT_GIFT, sourceKey, vanilla, level);
    return PickReplacement(&st, vanilla, POOL_ORDINARY);
}

void Randomizer_ApplyGiftMonIVs(struct Pokemon *mon, enum Species vanilla, u32 sourceKey)
{
    struct PokemonTemplate t = {0};
    rng_value_t st;
    u32 i;

    if (mon == NULL)
        return;
    if (GetRulesetSetting(SETTING_GIFT_IV_MODE) == GIFTIV_NATURAL)
        return;
    // Standalone gift constructors (including eggs) have already consumed the
    // global RNG by the time they reach this hook.  Treat those IVs as the
    // template's natural/random request so the saved run seed and source key,
    // rather than call timing, determine the final result.
    for (i = 0; i < NUM_STATS; i++)
        t.ivs[i] = USE_RANDOM_IVS;
    st = SeedFor(SALT_GIFT_IV, sourceKey, vanilla, GetMonData(mon, MON_DATA_LEVEL));
    ApplyGiftIvMode(&t, &st);
    for (i = 0; i < NUM_STATS; i++)
    {
        u8 iv = t.ivs[i];
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);
    }
    CalculateMonStats(mon);
}

void Randomizer_ApplyGiftTemplate(struct PokemonTemplate *monTemplate, u32 sourceKey)
{
    enum Species vanilla;
    rng_value_t ivRng;

    if (monTemplate == NULL)
        return;
    if (!IsReplaceableTarget(monTemplate->species))
        return;

    vanilla = monTemplate->species;
    PowerScore_EnsureBuilt();
    ivRng = SeedFor(SALT_GIFT_IV, sourceKey, vanilla, monTemplate->level);
    monTemplate->species = Randomizer_GiftSpecies(vanilla, monTemplate->level, sourceKey);
    ApplyGiftIvMode(monTemplate, &ivRng);
}

// ---- static / event / roamer -----------------------------------------------

static bool32 VanillaIsPremiumTier(enum Species vanilla)
{
    return IsSpeciesPremium(vanilla);
}

enum Species Randomizer_StaticSpecies(enum Species vanilla, u8 level, u32 sourceKey)
{
    bool32 premium;
    rng_value_t st;

    if (!IsReplaceableTarget(vanilla))
        return vanilla;

    PowerScore_EnsureBuilt();
    premium = VanillaIsPremiumTier(vanilla);

    if (premium)
    {
        if (!Randomizer_LegendaryEnabled())
            return vanilla;
    }
    else if (!Randomizer_StaticEnabled())
    {
        return vanilla;
    }

    st = SeedFor(SALT_STATIC, sourceKey, ((u32)vanilla << 8) | level, 0);

    return PickReplacement(&st, vanilla, premium ? POOL_PREMIUM : POOL_ORDINARY);
}

enum Species Randomizer_RoamerSpecies(enum Species vanilla, u8 level, u32 roamerId)
{
    bool32 premium;
    rng_value_t st;

    if (!IsReplaceableTarget(vanilla))
        return vanilla;

    PowerScore_EnsureBuilt();
    premium = VanillaIsPremiumTier(vanilla);

    if (premium ? !Randomizer_LegendaryEnabled() : !Randomizer_StaticEnabled())
        return vanilla;

    st = SeedFor(SALT_ROAMER, roamerId, ((u32)vanilla << 8) | level, 0);
    return PickReplacement(&st, vanilla, premium ? POOL_PREMIUM : POOL_ORDINARY);
}

// ---- trainer parties -------------------------------------------------------

// docs/SPEC.md "Trainer Pokemon": "Bosses use stricter power matching than
// ordinary route trainers." One step tighter, never past STRICT. BST-only and
// unrestricted are deliberate global choices, so they are left alone.
static bool32 TrainerClassIsBoss(u8 trainerClass)
{
    switch (trainerClass)
    {
    case TRAINER_CLASS_LEADER:
    case TRAINER_CLASS_ELITE_FOUR:
    case TRAINER_CLASS_CHAMPION:
    case TRAINER_CLASS_RIVAL:
    case TRAINER_CLASS_AQUA_LEADER:
    case TRAINER_CLASS_MAGMA_LEADER:
    case TRAINER_CLASS_AQUA_ADMIN:
    case TRAINER_CLASS_MAGMA_ADMIN:
        return TRUE;
    default:
        return FALSE;
    }
}

static u32 TrainerMatchMode(u8 trainerClass)
{
    u32 mode = GetRulesetSetting(SETTING_POWER_MATCHING);

    if (GetRulesetSetting(SETTING_BOSS_POWER_MATCHING) != BOSSMATCH_STRICTER)
        return mode;
    if (!TrainerClassIsBoss(trainerClass))
        return mode;

    if (mode == PWRMATCH_NORMAL)
        return PWRMATCH_STRICT;
    if (mode == PWRMATCH_LOOSE)
        return PWRMATCH_NORMAL;
    return mode;
}

static bool32 TrainerIsEliteFour(u16 trainerId)
{
    return trainerId >= TRAINER_SIDNEY && trainerId <= TRAINER_DRAKE;
}

static bool32 TrainerIsGymRematch(u16 trainerId)
{
    return trainerId >= TRAINER_ROXANNE_2 && trainerId <= TRAINER_JUAN_5;
}

static u8 MainGymLevel(u16 trainerId)
{
    static const u8 sLevels[] = { 14, 21, 24, 29, 36, 43, 47, 50 };

    if (trainerId < TRAINER_ROXANNE_1 || trainerId > TRAINER_JUAN_1)
        return 0;
    return sLevels[trainerId - TRAINER_ROXANNE_1];
}

static u8 UpcomingVanillaAce(void)
{
    static const u8 sAceLevels[] = { 15, 19, 24, 29, 31, 33, 42, 46, 58 };
    static const u16 sBadgeFlags[] =
    {
        FLAG_BADGE01_GET, FLAG_BADGE02_GET, FLAG_BADGE03_GET, FLAG_BADGE04_GET,
        FLAG_BADGE05_GET, FLAG_BADGE06_GET, FLAG_BADGE07_GET, FLAG_BADGE08_GET,
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sBadgeFlags); i++)
        if (!FlagGet(sBadgeFlags[i]))
            return sAceLevels[i];
    return sAceLevels[ARRAY_COUNT(sAceLevels) - 1];
}

static u8 TrainerLevel(u16 trainerId, u8 authoredLevel)
{
    u32 cap, gap;
    u8 gymLevel;

    if (GetRulesetSetting(SETTING_TRAINER_LEVEL_MODE) == TRLEVEL_VANILLA
     || trainerId == TRAINER_NONE
     || FlagGet(FLAG_IS_CHAMPION)
     || TrainerIsGymRematch(trainerId))
        return authoredLevel;

    gymLevel = MainGymLevel(trainerId);
    if (gymLevel != 0)
        return gymLevel;
    if (TrainerIsEliteFour(trainerId) || trainerId == TRAINER_WALLACE)
        return 63;

    cap = GetProgressionLevelCap();
    gap = UpcomingVanillaAce();
    if (authoredLevel < gap)
        gap -= authoredLevel;
    else
        gap = 0;
    if (gap >= cap)
        return 1;
    return cap - gap;
}

u8 Randomizer_GetTrainerPartySize(u16 trainerId, u8 authoredPartySize)
{
    rng_value_t st;

    if (trainerId == TRAINER_NONE || authoredPartySize == 0
     || !GetRulesetSetting(SETTING_TRAINER_PARTY_SIZE_RANDOMIZATION))
        return authoredPartySize;
    st = SeedFor(SALT_TRAINER_SIZE, trainerId, authoredPartySize, 0);
    return 1 + LocalRandom32(&st) % authoredPartySize;
}

static enum Species ResolveTrainerSpecies(enum Species vanilla, u16 trainerId,
                                           u32 sourceIndex, u8 trainerClass)
{
    enum PoolKind kind = POOL_STRICT_ORDINARY;
    rng_value_t st;

    if (TrainerIsEliteFour(trainerId) || trainerId == TRAINER_WALLACE)
        kind = POOL_ORDINARY_OR_PREMIUM;
    st = SeedFor(SALT_TRAINER, trainerId, sourceIndex, vanilla);
    return PickReplacementCore(&st, vanilla, kind, TrainerMatchMode(trainerClass),
                               GetRulesetSetting(SETTING_EVO_STAGE_MATCHING));
}

void Randomizer_ApplyTrainerParty(struct TrainerMon *entries, const u32 *sourceIndices,
                                  u8 count, u16 trainerId, u8 trainerClass)
{
    bool32 hasPremium = FALSE;
    enum Species vanillaSpecies[PARTY_SIZE];
    u32 i;

    if (entries == NULL || trainerId == TRAINER_NONE)
        return;
    // partySize is a 3-bit authored field (max 7) but the party engine and
    // this function's scratch array are both bounded by PARTY_SIZE (6); clamp
    // defensively so an authored 7-mon party can never overflow vanillaSpecies.
    if (count > PARTY_SIZE)
        count = PARTY_SIZE;

    if (Randomizer_TrainerEnabled())
        PowerScore_EnsureBuilt();

    for (i = 0; i < count; i++)
    {
        vanillaSpecies[i] = entries[i].species;
        entries[i].lvl = TrainerLevel(trainerId, entries[i].lvl);
        if (Randomizer_TrainerEnabled() && IsReplaceableTarget(entries[i].species))
        {
            entries[i].species = ResolveTrainerSpecies(entries[i].species, trainerId,
                sourceIndices == NULL ? i : sourceIndices[i], trainerClass);
            entries[i].ability = ABILITY_NONE;
            for (u32 move = 0; move < MAX_MON_MOVES; move++)
                entries[i].moves[move] = MOVE_NONE;
        }
        if (IsSpeciesPremium(entries[i].species))
            hasPremium = TRUE;
    }

    if (Randomizer_TrainerEnabled() && trainerId == TRAINER_WALLACE && count != 0 && !hasPremium)
    {
        rng_value_t choose = SeedFor(SALT_TRAINER_PREMIUM_GUARANTEE, trainerId, count, 0);
        u32 slot = LocalRandom32(&choose) % count;
        enum Species vanilla = vanillaSpecies[slot];
        rng_value_t st = SeedFor(SALT_TRAINER, trainerId,
            (sourceIndices == NULL ? slot : sourceIndices[slot]), vanilla);

        entries[slot].species = PickReplacementCore(&st, vanilla, POOL_PREMIUM,
            TrainerMatchMode(trainerClass), GetRulesetSetting(SETTING_EVO_STAGE_MATCHING));
    }
}

// ============================================================================
// TMs (docs/SPEC.md "TMs", "Universal TM compatibility").
//
// The 50 TM->move assignments are drawn once from the same ban-filtered move
// pool the Phase 4 learnset generator builds, and cached in a small EWRAM
// table: the bag list, the relearner and the Pokedex all scan every TM index
// while rendering, so a per-call reseed would be far too slow. HMs are never
// randomized, per SPEC.
// ============================================================================

#define TM_DEDUP_ATTEMPTS 16

static EWRAM_DATA u16 sTmMove[NUM_TECHNICAL_MACHINES] = {0};
static EWRAM_DATA bool8 sTmBuilt = FALSE;
static EWRAM_DATA u32 sTmSig = 0;

bool32 Randomizer_TmEnabled(void) { return GetRulesetSetting(SETTING_TM_RANDOMIZATION) != 0; }

static u32 TmSignature(void)
{
    return GetRunSeed()
         ^ ((u32)GetRulesetSetting(SETTING_TM_RANDOMIZATION) << 1)
         ^ ((u32)GetRulesetSetting(SETTING_ALLOW_DUPLICATE_TMS) << 2)
         ^ (LearnsetGen_PoolSignature() << 8);
}

static void BuildTmTable(void)
{
    u32 poolCount = LearnsetGen_PoolCount();
    bool32 allowDupes = GetRulesetSetting(SETTING_ALLOW_DUPLICATE_TMS) != 0;
    u32 i, j, attempt;

    for (i = 0; i < NUM_TECHNICAL_MACHINES; i++)
    {
        rng_value_t st;

        // Fall back to the canonical move if the pool is unusable.
        sTmMove[i] = GetTMHMMoveId(i + 1);
        if (poolCount == 0)
            continue;

        st = SeedFor(SALT_TM, i + 1, 0, 0);
        for (attempt = 0; attempt < TM_DEDUP_ATTEMPTS; attempt++)
        {
            enum Move pick = LearnsetGen_PoolMove(LocalRandom32(&st) % poolCount);
            bool32 dup = FALSE;

            if (!allowDupes)
            {
                for (j = 0; j < i; j++)
                {
                    if (sTmMove[j] == pick)
                        dup = TRUE;
                }
            }

            sTmMove[i] = pick;
            if (!dup)
                break;
        }
    }
}

static void EnsureTmTable(void)
{
    u32 sig = TmSignature();

    if (sTmBuilt && sTmSig == sig)
        return;

    BuildTmTable();
    sTmSig = sig;
    sTmBuilt = TRUE;
}

void Randomizer_InvalidateTms(void)
{
    sTmBuilt = FALSE;
}

enum Move Randomizer_TmMoveByIndex(u32 tmhmIndex)
{
    // 1-based; 0 means "not a machine", and anything past the TM block is an HM.
    if (tmhmIndex == 0 || tmhmIndex > NUM_TECHNICAL_MACHINES || !Randomizer_TmEnabled())
        return GetTMHMMoveId(tmhmIndex);

    EnsureTmTable();
    return sTmMove[tmhmIndex - 1];
}

enum Move Randomizer_TmMove(enum Item item)
{
    return Randomizer_TmMoveByIndex(GetItemTMHMIndex(item));
}

enum Item Randomizer_TmItemForMove(enum Move move)
{
    u32 i;

    if (move == MOVE_NONE)
        return ITEM_NONE;

    if (Randomizer_TmEnabled())
    {
        EnsureTmTable();
        for (i = 0; i < NUM_TECHNICAL_MACHINES; i++)
        {
            if (sTmMove[i] == move)
                return GetTMHMItemId(i + 1);
        }
        // Not on a TM; an HM may still teach it, and HMs keep their moves.
        for (i = NUM_TECHNICAL_MACHINES; i < NUM_ALL_MACHINES; i++)
        {
            if (GetTMHMMoveId(i + 1) == move)
                return GetTMHMItemId(i + 1);
        }
        return ITEM_NONE;
    }

    return GetTMHMItemIdFromMoveId(move);
}

// ============================================================================
// Move Tutors (docs/SPEC.md "Move Tutors").
//
// The ten standard Hoenn tutors (data/scripts/move_tutors.inc) each draw one
// move from the same ban-filtered pool as TMs, keyed on the tutor's vanilla
// move so the assignment is stable for the run without needing a table - a
// tutor is talked to, not rendered in a list. Battle Frontier tutors are a
// separate list and stay vanilla, per SPEC's "every standard Tutor".
// ============================================================================

bool32 Randomizer_TutorEnabled(void) { return GetRulesetSetting(SETTING_TUTOR_RANDOMIZATION) != 0; }

enum Move Randomizer_TutorMove(enum Move vanilla)
{
    u32 poolCount;
    rng_value_t st;

    if (vanilla == MOVE_NONE || !Randomizer_TutorEnabled())
        return vanilla;

    poolCount = LearnsetGen_PoolCount();
    if (poolCount == 0)
        return vanilla;

    st = SeedFor(SALT_TUTOR, vanilla, 0, 0);
    return LearnsetGen_PoolMove(LocalRandom32(&st) % poolCount);
}

// special: the move_tutor macro has just put the tutor's canonical move in
// VAR_0x8005. Swap in the randomized one and buffer its name into STR_VAR_1 for
// MoveTutor_Text_GenericWhichMon. Everything downstream (the party menu, the
// PC-box filter, CanTeachMoveBoxMon) already reads VAR_0x8005.
void ApplyTutorMoveRandomization(void)
{
    enum Move move = Randomizer_TutorMove(gSpecialVar_0x8005);

    gSpecialVar_0x8005 = move;
    StringCopy(gStringVar1, GetMoveName(move));
}

// ============================================================================
// Items (docs/SPEC.md "Items", "Item-pool safety").
//
// Three sources - visible field balls, hidden items and script gifts - share
// one pool and one safety filter, each with its own salt so they never
// cross-reshuffle. The pool is walked with the same count-and-pick the species
// picker uses rather than cached: an item is picked once per interaction, never
// in a render loop, so ~2 x ITEMS_COUNT iterations costs nothing and the EWRAM
// budget stays untouched.
//
// Evolution items need no special handling here: the Phase 7 Lilycove clerk is
// a `pokemart` and shops are not randomized by default, so every stone stays
// purchasable no matter how the field rolls ("Delayed is acceptable. Impossible
// is not.").
// ============================================================================

// What may *appear*. Restricted to ordinary consumables, berries and balls, so
// a field item can never become a key item or a machine.
static bool32 ItemIsPoolEligible(enum Item item)
{
    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return FALSE;
    // Undefined item slots are all-zero, which reads as POCKET_ITEMS.
    if (gItemsInfo[item].name == NULL)
        return FALSE;
    if (GetItemImportance(item) != 0)
        return FALSE;
    if (!Ruleset_ItemIsEnabled(item))
        return FALSE;

    switch (GetItemPocket(item))
    {
    case POCKET_ITEMS:
    case POCKET_BERRIES:
    case POCKET_POKE_BALLS:
        return TRUE;
    default:
        return FALSE;
    }
}

// What may be *replaced*. Importance covers every progression item, HM and
// reusable TM without a hand-maintained list; the pocket checks are belt and
// braces, plus TMs, which already carry a randomized move of their own and
// would simply be deleted from the run if they became Potions.
static bool32 ItemIsReplaceableTarget(enum Item item)
{
    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return FALSE;
    if (gItemsInfo[item].name == NULL)
        return FALSE;
    if (GetItemImportance(item) != 0)
        return FALSE;

    switch (GetItemPocket(item))
    {
    case POCKET_KEY_ITEMS:
    case POCKET_TM_HM:
        return FALSE;
    default:
        return TRUE;
    }
}

static enum Item PickPoolItem(rng_value_t *st)
{
    enum Item item;
    u32 count = 0, n;

    for (item = ITEM_NONE + 1; item < ITEMS_COUNT; item++)
    {
        if (ItemIsPoolEligible(item))
            count++;
    }
    if (count == 0)
        return ITEM_NONE;

    n = LocalRandom32(st) % count;
    for (item = ITEM_NONE + 1; item < ITEMS_COUNT; item++)
    {
        if (!ItemIsPoolEligible(item))
            continue;
        if (n == 0)
            return item;
        n--;
    }
    return ITEM_NONE;
}

static u32 CurrentMapKey(void)
{
    return ((u32)gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum;
}

static enum Item ReplaceItem(enum Item vanilla, bool32 enabled, u32 salt, u32 k0, u32 k1)
{
    rng_value_t st;
    enum Item pick;

    if (!enabled || !ItemIsReplaceableTarget(vanilla))
        return vanilla;

    st = SeedFor(salt, k0, k1, vanilla);
    pick = PickPoolItem(&st);
    return (pick == ITEM_NONE) ? vanilla : pick;
}

bool32 Randomizer_FieldItemEnabled(void)  { return GetRulesetSetting(SETTING_FIELD_ITEM_RANDOMIZATION) != 0; }
bool32 Randomizer_HiddenItemEnabled(void) { return GetRulesetSetting(SETTING_HIDDEN_ITEM_RANDOMIZATION) != 0; }
bool32 Randomizer_GiftItemEnabled(void)   { return GetRulesetSetting(SETTING_GIFT_ITEM_RANDOMIZATION) != 0; }

// Visible item balls: keyed on the map plus the ball's local object id, so two
// balls on one map differ and neither moves when an unrelated map changes.
enum Item Randomizer_FieldItem(enum Item vanilla, u32 objectId)
{
    return ReplaceItem(vanilla, Randomizer_FieldItemEnabled(), SALT_ITEM_FIELD,
                       CurrentMapKey(), objectId);
}

// Hidden items: hiddenItemId is already unique per item across the game.
enum Item Randomizer_HiddenItem(enum Item vanilla, u32 hiddenItemId)
{
    return ReplaceItem(vanilla, Randomizer_HiddenItemEnabled(), SALT_ITEM_HIDDEN,
                       hiddenItemId, 0);
}

enum Item Randomizer_GiftItem(enum Item vanilla)
{
    return ReplaceItem(vanilla, Randomizer_GiftItemEnabled(), SALT_ITEM_GIFT,
                       CurrentMapKey(), 0);
}

// special: Std_FindItem has the ball's item in VAR_0x8000 and the ball object in
// VAR_LAST_TALKED. Rewrite the item before any of the downstream buffering runs,
// so the message, pocket and fanfare all follow the new item.
void ApplyFieldItemRandomization(void)
{
    gSpecialVar_0x8000 = Randomizer_FieldItem(gSpecialVar_0x8000, VarGet(VAR_LAST_TALKED));
}

// special: same for Std_ObtainItem (script gifts).
void ApplyGiftItemRandomization(void)
{
    gSpecialVar_0x8000 = Randomizer_GiftItem(gSpecialVar_0x8000);
}
