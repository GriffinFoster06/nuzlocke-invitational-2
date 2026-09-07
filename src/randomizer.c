// ============================================================================
// Phase 2 core randomizer wiring. See include/randomizer.h for the overview.
// Scoring / eligibility / stage buckets live in src/power_score.c.
// ============================================================================

#include "global.h"
#include "pokemon.h"
#include "power_score.h"
#include "random.h"
#include "randomizer.h"
#include "run_rng.h"
#include "ruleset.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// Per-category salts and the seed helper now live in include/run_rng.h so the
// Phase 4 learnset generator shares one implementation.

#define STARTER_DEDUP_ATTEMPTS 24

enum PoolKind { POOL_ORDINARY, POOL_PREMIUM };

// ---- enable checks --------------------------------------------------------

bool32 Randomizer_WildEnabled(void)      { return GetRulesetSetting(SETTING_WILD_RANDOMIZATION) != 0; }
bool32 Randomizer_StarterEnabled(void)   { return GetRulesetSetting(SETTING_STARTER_RANDOMIZATION) != 0; }
bool32 Randomizer_GiftEnabled(void)      { return GetRulesetSetting(SETTING_GIFT_RANDOMIZATION) != 0; }
bool32 Randomizer_StaticEnabled(void)    { return GetRulesetSetting(SETTING_STATIC_RANDOMIZATION) != 0; }
bool32 Randomizer_LegendaryEnabled(void) { return GetRulesetSetting(SETTING_LEGENDARY_RANDOMIZATION) != 0; }

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

    switch (GetRulesetSetting(SETTING_PREMIUM_POOL_MODE))
    {
    case PREMPOOL_SAME_AS_NORMAL:
        return !IsSpeciesCategoryBanned(s);
    case PREMPOOL_ALL_LEGENDARY:
        return IsSpeciesCategoryBanned(s);
    case PREMPOOL_CURATED:
    default:
        return IsSpeciesCategoryBanned(s) || IsSpeciesPremiumTier(s);
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

static enum Species PickReplacementCore(rng_value_t *st, enum Species vanilla, enum PoolKind kind,
                                        enum PowerMatchMode mode, u32 evoMode)
{
    u32 target = GetSpeciesMatchMetric(vanilla, mode);
    enum EvoStageBucket tgtStage = GetSpeciesEvoStageBucket(vanilla);
    struct LadderRung ladder[6];
    u32 rungs = BuildLadder(ladder, mode, evoMode);
    u32 i;

    for (i = 0; i < rungs; i++)
    {
        u32 count = 0, pick;
        enum Species s;

        for (s = 1; s < NUM_SPECIES; s++)
        {
            if (!InPool(s, kind))
                continue;
            if (RungAccepts(&ladder[i], s, mode, target, tgtStage))
                count++;
        }
        if (count == 0)
            continue;

        pick = LocalRandom32(st) % count;
        for (s = 1; s < NUM_SPECIES; s++)
        {
            if (!InPool(s, kind))
                continue;
            if (!RungAccepts(&ladder[i], s, mode, target, tgtStage))
                continue;
            if (pick == 0)
                return s;
            pick--;
        }
    }
    return vanilla;
}

static enum Species PickReplacement(rng_value_t *st, enum Species vanilla, enum PoolKind kind)
{
    return PickReplacementCore(st, vanilla, kind,
                               GetRulesetSetting(SETTING_POWER_MATCHING),
                               GetRulesetSetting(SETTING_EVO_STAGE_MATCHING));
}

// ---- wild encounters --------------------------------------------------------

enum Species Randomizer_WildSlotSpecies(const struct WildPokemonInfo *info, u32 slot, enum Species vanilla)
{
    rng_value_t st;

    if (info == NULL || !Randomizer_WildEnabled() || !IsReplaceableTarget(vanilla))
        return vanilla;

    PowerScore_EnsureBuilt();

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
    return PickReplacement(&st, vanilla, POOL_ORDINARY);
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
        u32 attempt;

        out[i] = vanilla;
        for (attempt = 0; attempt < STARTER_DEDUP_ATTEMPTS; attempt++)
        {
            rng_value_t st = SeedFor(SALT_STARTER, i, attempt, 0);
            enum Species pick;
            bool32 collide = FALSE;
            u32 j;

            // Starters always evolve twice: force same-stage matching here
            // regardless of the global evo-stage setting.
            pick = PickReplacementCore(&st, vanilla, POOL_ORDINARY,
                                       GetRulesetSetting(SETTING_POWER_MATCHING), EVOSTAGE_STRICT);

            for (j = 0; j < i; j++)
            {
                if (out[j] == pick)
                    collide = TRUE;
            }
            out[i] = pick;
            if (!collide)
                break;
        }
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

    for (i = 0; i < NUM_STATS; i++)
        order[i] = i;
    for (i = NUM_STATS - 1; i > 0; i--)
    {
        u32 k = LocalRandom32(st) % (i + 1);
        u8 tmp = order[i]; order[i] = order[k]; order[k] = tmp;
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
            t->ivs[i] = floor;   // guarantees >= floor (loses the "random above floor")
        break;
    case GIFTIV_NATURAL:
        break;   // leave whatever the script asked for (USE_RANDOM_IVS by default)
    case GIFTIV_3_PERFECT:
    default:
        for (i = 0; i < 3; i++)
            t->ivs[order[i]] = MAX_PER_STAT_IVS;
        break;
    }
}

void Randomizer_ApplyGiftTemplate(struct PokemonTemplate *monTemplate)
{
    u32 mg, mn;
    rng_value_t st;

    if (monTemplate == NULL || monTemplate->isEgg || !Randomizer_GiftEnabled())
        return;
    if (!IsReplaceableTarget(monTemplate->species))
        return;

    PowerScore_EnsureBuilt();

    mg = gSaveBlock1Ptr->location.mapGroup;
    mn = gSaveBlock1Ptr->location.mapNum;
    st = SeedFor(SALT_GIFT, (mg << 8) | mn, monTemplate->species, monTemplate->level);

    monTemplate->species = PickReplacement(&st, monTemplate->species, POOL_ORDINARY);
    ApplyGiftIvMode(monTemplate, &st);
}

// ---- static / event / roamer -----------------------------------------------

static bool32 VanillaIsPremiumTier(enum Species vanilla)
{
    return IsSpeciesCategoryBanned(vanilla) || IsSpeciesPremiumTier(vanilla);
}

enum Species Randomizer_StaticSpecies(enum Species vanilla, u8 level, u8 idx)
{
    u32 mg, mn;
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

    mg = gSaveBlock1Ptr->location.mapGroup;
    mn = gSaveBlock1Ptr->location.mapNum;
    st = SeedFor(SALT_STATIC, (mg << 8) | mn, ((u32)vanilla << 8) | level, idx);

    return PickReplacement(&st, vanilla, premium ? POOL_PREMIUM : POOL_ORDINARY);
}

enum Species Randomizer_RoamerSpecies(enum Species vanilla, u8 level)
{
    bool32 premium;
    rng_value_t st;

    if (!IsReplaceableTarget(vanilla))
        return vanilla;

    PowerScore_EnsureBuilt();
    premium = VanillaIsPremiumTier(vanilla);

    if (premium ? !Randomizer_LegendaryEnabled() : !Randomizer_StaticEnabled())
        return vanilla;

    st = SeedFor(SALT_ROAMER, ((u32)vanilla << 8) | level, 0, 0);
    return PickReplacement(&st, vanilla, premium ? POOL_PREMIUM : POOL_ORDINARY);
}
