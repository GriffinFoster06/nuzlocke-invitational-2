// ============================================================================
// Phase 2 core randomizer wiring. See include/randomizer.h for the overview.
// Scoring / eligibility / stage buckets live in src/power_score.c.
// ============================================================================

#include "global.h"
#include "data.h"
#include "event_data.h"
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

#define STARTER_DEDUP_ATTEMPTS 24

enum PoolKind { POOL_ORDINARY, POOL_PREMIUM };

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
            {
                // Backstop: InPool() already excludes anything not power-eligible
                // (and 1435 is never eligible), but keep the guarantee local to
                // the one return point in case the score cache is ever stale.
                return IsSpeciesEnabled(s) ? s : vanilla;
            }
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

void Randomizer_ApplyTrainerMon(struct TrainerMon *entry, u16 trainerId, u32 monIndex, u8 trainerClass)
{
    enum Species vanilla, pick;
    bool32 premium;
    rng_value_t st;
    u32 i;

    if (entry == NULL || trainerId == TRAINER_NONE || !Randomizer_TrainerEnabled())
        return;

    vanilla = entry->species;
    if (!IsReplaceableTarget(vanilla))
        return;

    PowerScore_EnsureBuilt();

    // Steven's / Wallace's aces and any other premium-tier authored mon draw from
    // the premium pool, exactly as static encounters do, and obey the legendary
    // toggle rather than the trainer one.
    premium = VanillaIsPremiumTier(vanilla);
    if (premium && !Randomizer_LegendaryEnabled())
        return;

    st = SeedFor(SALT_TRAINER, trainerId, monIndex, vanilla);
    pick = PickReplacementCore(&st, vanilla, premium ? POOL_PREMIUM : POOL_ORDINARY,
                               TrainerMatchMode(trainerClass),
                               GetRulesetSetting(SETTING_EVO_STAGE_MATCHING));
    if (pick == vanilla)
        return;

    entry->species = pick;

    // The authored ability and moveset belonged to the vanilla species. Keeping
    // either would be illegal for the replacement: SetCorrectAbilityNum() would
    // fail on an ability the new species does not have, and the moves would be
    // off-species. Clearing moves makes CustomTrainerPartyAssignMoves() fall back
    // to GiveMonInitialMoveset(), which reads the Phase 4 *generated* learnset -
    // the right answer in a randomized world.
    entry->ability = ABILITY_NONE;
    for (i = 0; i < MAX_MON_MOVES; i++)
        entry->moves[i] = MOVE_NONE;
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
