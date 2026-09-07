// ============================================================================
// Phase 4 - generated level-up learnsets. See include/learnset_gen.h.
//
// Two layers, both in EWRAM, both rebuilt when a relevant ruleset toggle or the
// run seed changes (mirrors src/power_score.c):
//   * a move-pool index  - eligible damaging moves grouped by type + a status
//     list, filtered by the four SETTING_BAN_*_MOVES toggles.
//   * a small LRU cache of per-species generated learnsets.
// Selection is deterministic from RunRng_Seed(SALT_LEARNSET, species, ...).
// ============================================================================

#include "global.h"
#include "learnset_gen.h"
#include "move.h"
#include "pokemon.h"
#include "random.h"
#include "run_rng.h"
#include "ruleset.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_set_effect.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"

#define LG_MIN_MOVES     4
#define LG_MAX_MOVES     25   // SETTING_LEARNSET_SIZE max
#define LG_CACHE_SLOTS   8
#define LG_POOL_CAP      MOVES_COUNT
#define LG_LAST_LEVEL    61   // checkpoint for the final move at the default size

// ---- move pool -----------------------------------------------------------
// One array: damaging moves fill from the front ([0, sDmgCount)), status moves
// fill from the back ([sStatusStart, LG_POOL_CAP)). Every eligible move is in
// exactly one region, so LG_POOL_CAP slots is always enough.

static EWRAM_DATA u16 sPool[LG_POOL_CAP] = {0};
static EWRAM_DATA u8  sDmgType[LG_POOL_CAP] = {0}; // parallel, damaging region only
static EWRAM_DATA u16 sDmgCount = 0;
static EWRAM_DATA u16 sStatusStart = 0; // real value set by BuildPool; guarded by sPoolBuilt
static EWRAM_DATA bool8 sPoolBuilt = FALSE;
static EWRAM_DATA u32 sPoolSig = 0;

// ---- per-species cache ----------------------------------------------------

static EWRAM_DATA struct LevelUpMove sCache[LG_CACHE_SLOTS][LG_MAX_MOVES + 1];
static EWRAM_DATA u16 sCacheSpecies[LG_CACHE_SLOTS] = {0}; // stored as species + 1; 0 == empty
static EWRAM_DATA u32 sCacheLru[LG_CACHE_SLOTS] = {0};
static EWRAM_DATA u32 sCacheClock = 0;
static EWRAM_DATA u32 sCacheSig = 0;
static EWRAM_DATA bool8 sCacheInit = FALSE;

// ---- generation scratch (single-threaded, no recursion) ------------------

static EWRAM_DATA u16 sScratch[LG_POOL_CAP] = {0};
static EWRAM_DATA u32 sPickedBits[(LG_POOL_CAP + 31) / 32] = {0};

static void PickedClear(void) { memset(sPickedBits, 0, sizeof(sPickedBits)); }
static bool32 PickedGet(u32 m) { return (sPickedBits[m >> 5] >> (m & 31)) & 1; }
static void PickedSet(u32 m) { sPickedBits[m >> 5] |= (1u << (m & 31)); }

// ---- ban predicates -----------------------------------------------------

static bool32 MoveRaisesEvasion(enum Move m)
{
    u32 i, n;

    if (GetMoveEffect(m) == EFFECT_MINIMIZE)
        return TRUE;

    n = GetMoveAdditionalEffectCount(m);
    for (i = 0; i < n; i++)
    {
        if (GetMoveAdditionalEffectById(m, i)->evasion != 0)
            return TRUE;
    }
    return FALSE;
}

static bool32 MoveInducesSleep(enum Move m)
{
    u32 i, n;

    if (GetMoveNonVolatileStatus(m) == MOVE_EFFECT_SLEEP)
        return TRUE;

    n = GetMoveAdditionalEffectCount(m);
    for (i = 0; i < n; i++)
    {
        if (GetMoveAdditionalEffectById(m, i)->moveEffect == MOVE_EFFECT_SLEEP)
            return TRUE;
    }
    return FALSE;
}

static bool32 MoveIsSelfKO(enum Move m)
{
    if (IsExplosionMove(m))
        return TRUE;

    switch (GetMoveEffect(m))
    {
    case EFFECT_MEMENTO:
    case EFFECT_FINAL_GAMBIT:
    case EFFECT_HEALING_WISH:
    case EFFECT_LUNAR_DANCE:
        return TRUE;
    default:
        return FALSE;
    }
}

// docs/SPEC.md "Move pool": Gen 1-9, signature moves allowed on unrelated
// species unless the move can't function outside its implementation (sketch
// ban is the proxy), plus the configurable blacklist categories.
static bool32 MoveEligible(enum Move m)
{
    if (m == MOVE_NONE || m >= MOVES_COUNT || m == MOVE_STRUGGLE)
        return FALSE;
    if (GetMoveEffect(m) == EFFECT_PLACEHOLDER)
        return FALSE;
    if (IsMoveSketchBanned(m))
        return FALSE;
    if (GetRulesetSetting(SETTING_BAN_OHKO_MOVES) && GetMoveEffect(m) == EFFECT_OHKO)
        return FALSE;
    if (GetRulesetSetting(SETTING_BAN_SELF_KO_MOVES) && MoveIsSelfKO(m))
        return FALSE;
    if (GetRulesetSetting(SETTING_BAN_EVASION_MOVES) && MoveRaisesEvasion(m))
        return FALSE;
    if (GetRulesetSetting(SETTING_BAN_SLEEP_MOVES) && MoveInducesSleep(m))
        return FALSE;
    return TRUE;
}

// ---- pool / cache signatures -------------------------------------------

static u32 PoolSignature(void)
{
    return (GetRulesetSetting(SETTING_BAN_OHKO_MOVES)    ? (1u << 0) : 0)
         | (GetRulesetSetting(SETTING_BAN_EVASION_MOVES) ? (1u << 1) : 0)
         | (GetRulesetSetting(SETTING_BAN_SLEEP_MOVES)   ? (1u << 2) : 0)
         | (GetRulesetSetting(SETTING_BAN_SELF_KO_MOVES) ? (1u << 3) : 0);
}

static u32 CacheSignature(void)
{
    return ((u32)GetRulesetSetting(SETTING_LEARNSET_SIZE))
         ^ ((u32)GetRulesetSetting(SETTING_LEARNSET_COMPOSITION) << 8)
         ^ ((u32)GetRulesetSetting(SETTING_MOVE_POWER_PROGRESSION) << 12)
         ^ (PoolSignature() << 16)
         ^ GetRunSeed();
}

static void BuildPool(void)
{
    enum Move m;

    sDmgCount = 0;
    sStatusStart = LG_POOL_CAP;

    for (m = MOVE_NONE + 1; m < MOVES_COUNT; m++)
    {
        if (!MoveEligible(m))
            continue;

        if (sDmgCount >= sStatusStart) // pool full (cannot happen; guard anyway)
            break;

        if (GetMoveCategory(m) == DAMAGE_CATEGORY_STATUS)
        {
            sPool[--sStatusStart] = m;
        }
        else
        {
            sPool[sDmgCount] = m;
            sDmgType[sDmgCount] = GetMoveType(m);
            sDmgCount++;
        }
    }

    sPoolBuilt = TRUE;
    sPoolSig = PoolSignature();
}

void LearnsetGen_EnsureBuilt(void)
{
    u32 csig = CacheSignature();
    u32 i;

    if (sCacheInit && sCacheSig == csig && sPoolBuilt && sPoolSig == PoolSignature())
        return;

    if (!sPoolBuilt || sPoolSig != PoolSignature())
        BuildPool();

    for (i = 0; i < LG_CACHE_SLOTS; i++)
    {
        sCacheSpecies[i] = 0;
        sCacheLru[i] = 0;
    }
    sCacheClock = 0;
    sCacheSig = csig;
    sCacheInit = TRUE;
}

void LearnsetGen_Invalidate(void)
{
    sPoolBuilt = FALSE;
    sCacheInit = FALSE;
}

bool32 LearnsetGen_IsActive(void)
{
    return GetRulesetSetting(SETTING_MOVE_RANDOMIZATION) != 0;
}

// ---- generation --------------------------------------------------------

// kind: 0 = STAB (damaging, type t0/t1), 1 = any damaging, 2 = status.
// Draws up to k distinct not-yet-picked moves into out[*outCount], marking them.
static void DrawMoves(rng_value_t *st, u32 kind, u8 t0, u8 t1, u32 k,
                      enum Move *out, u32 *outCount)
{
    u32 scratchN = 0;
    u32 i, drawn;

    if (kind == 2)
    {
        for (i = sStatusStart; i < LG_POOL_CAP; i++)
        {
            enum Move m = sPool[i];
            if (!PickedGet(m))
                sScratch[scratchN++] = m;
        }
    }
    else
    {
        for (i = 0; i < sDmgCount; i++)
        {
            enum Move m = sPool[i];
            if (PickedGet(m))
                continue;
            if (kind == 0 && sDmgType[i] != t0 && sDmgType[i] != t1)
                continue;
            sScratch[scratchN++] = m;
        }
    }

    for (drawn = 0; drawn < k && scratchN > 0; drawn++)
    {
        u32 r = LocalRandom32(st) % scratchN;
        enum Move m = sScratch[r];

        sScratch[r] = sScratch[--scratchN]; // partial Fisher-Yates
        out[(*outCount)++] = m;
        PickedSet(m);
    }
}

static bool32 IsStab(enum Move m, u8 t0, u8 t1)
{
    u8 mt = GetMoveType(m);
    return mt == t0 || mt == t1;
}

// A 0..255 "how strong" key, biased later in the learnset, with a triangular
// jitter so an occasional strong move can still roll early.
static u16 PowerKey(enum Move m, rng_value_t *st)
{
    u32 p = GetMovePower(m);
    u32 acc = GetMoveAccuracy(m);
    u32 sc = GetMoveStrikeCount(m);
    s32 eff, j;

    if (p <= 1)   p = 60;   // fixed-damage / variable-power move: nominal
    if (acc == 0) acc = 100; // never-miss
    if (sc < 1)   sc = 1;

    eff = (s32)(p * acc * sc / 100);
    if (eff < 10)  eff = 10;
    if (eff > 150) eff = 150;

    j = eff * 255 / 150;
    j += (s32)(LocalRandom32(st) % 48) + (s32)(LocalRandom32(st) % 48) - 47; // ~ +/-18%
    if (j < 0)   j = 0;
    if (j > 255) j = 255;
    return (u16)j;
}

static void ShuffleMoves(enum Move *arr, u32 n, rng_value_t *st)
{
    u32 i;
    for (i = n; i > 1; i--)
    {
        u32 r = LocalRandom32(st) % i;
        enum Move tmp = arr[i - 1];
        arr[i - 1] = arr[r];
        arr[r] = tmp;
    }
}

static void SortByPowerKey(enum Move *arr, u32 n, rng_value_t *st)
{
    u16 key[LG_MAX_MOVES];
    u32 i;

    for (i = 0; i < n; i++)
        key[i] = PowerKey(arr[i], st);

    for (i = 1; i < n; i++) // insertion sort ascending
    {
        enum Move mv = arr[i];
        u16 kv = key[i];
        s32 j = (s32)i - 1;

        while (j >= 0 && key[j] > kv)
        {
            key[j + 1] = key[j];
            arr[j + 1] = arr[j];
            j--;
        }
        key[j + 1] = kv;
        arr[j + 1] = mv;
    }
}

static u16 CheckpointLevel(u32 idx, u32 n)
{
    if (n < 2)
        return 1;
    return (u16)(1 + (idx * (LG_LAST_LEVEL - 1)) / (n - 1));
}

// Fills out[] (must hold LG_MAX_MOVES + 1). Returns the move count, 0 on
// total failure (empty pool).
static u32 Generate(enum Species species, struct LevelUpMove *out)
{
    u32 n = GetRulesetSetting(SETTING_LEARNSET_SIZE);
    u32 comp = GetRulesetSetting(SETTING_LEARNSET_COMPOSITION);
    u32 order = GetRulesetSetting(SETTING_MOVE_POWER_PROGRESSION);
    u8 t0 = GetSpeciesType(species, 0);
    u8 t1 = GetSpeciesType(species, 1);
    rng_value_t st = RunRng_Seed(SALT_LEARNSET, species, 0, 0);

    enum Move dmg[LG_MAX_MOVES];
    enum Move status[LG_MAX_MOVES];
    u32 nDmg = 0, nStatus = 0;
    u32 wantStab, wantStatus, wantCoverage;
    u32 total, di, si, count, i;
    bool8 isStatusSlot[LG_MAX_MOVES];

    if (n < LG_MIN_MOVES) n = LG_MIN_MOVES;
    if (n > LG_MAX_MOVES) n = LG_MAX_MOVES;

    PickedClear();

    if (comp == LRNCOMP_FULLY_RANDOM)
    {
        wantStab = 0;
        wantStatus = 0;
        wantCoverage = n;
    }
    else // LRNCOMP_777, and LRNCOMP_WEIGHTED (stub -> 7/7/7)
    {
        wantStab = (n + 2) / 3;
        wantStatus = n / 3;
        wantCoverage = n - wantStab - wantStatus;
    }

    if (comp == LRNCOMP_FULLY_RANDOM)
    {
        u32 half = n / 2;

        DrawMoves(&st, 1, 0, 0, half, dmg, &nDmg);
        DrawMoves(&st, 2, 0, 0, n - nDmg, status, &nStatus);
        if (nDmg + nStatus < n)
            DrawMoves(&st, 1, 0, 0, n - nDmg - nStatus, dmg, &nDmg);
        if (nDmg + nStatus < n)
            DrawMoves(&st, 2, 0, 0, n - nDmg - nStatus, status, &nStatus);
    }
    else
    {
        DrawMoves(&st, 0, t0, t1, wantStab, dmg, &nDmg);      // STAB
        if (nDmg < wantStab)                                  // short -> coverage
            DrawMoves(&st, 1, 0, 0, wantStab - nDmg, dmg, &nDmg);
        DrawMoves(&st, 1, 0, 0, wantCoverage, dmg, &nDmg);    // coverage
        DrawMoves(&st, 2, 0, 0, wantStatus, status, &nStatus); // status
        if (nStatus < wantStatus)                             // short -> coverage
            DrawMoves(&st, 1, 0, 0, wantStatus - nStatus, dmg, &nDmg);
    }

    total = nDmg + nStatus;
    if (total == 0)
        return 0;
    if (total > n)
        total = n;

    // Order the damaging moves.
    if (order == MVORDER_FULLY_RANDOM || comp == LRNCOMP_FULLY_RANDOM)
    {
        ShuffleMoves(dmg, nDmg, &st);
    }
    else
    {
        SortByPowerKey(dmg, nDmg, &st);

        // docs/SPEC.md: level 1 is a damaging STAB move (7/7/7 comp only).
        if (nDmg > 1)
        {
            for (i = 0; i < nDmg; i++)
            {
                if (IsStab(dmg[i], t0, t1))
                {
                    if (i != 0)
                    {
                        enum Move mv = dmg[i];
                        while (i > 0) { dmg[i] = dmg[i - 1]; i--; }
                        dmg[0] = mv;
                    }
                    break;
                }
            }
        }
    }

    // Status moves land in seeded-random order.
    ShuffleMoves(status, nStatus, &st);

    // Choose which checkpoints carry a status move - evenly spread, never slot 0.
    for (i = 0; i < LG_MAX_MOVES; i++)
        isStatusSlot[i] = FALSE;

    if (nStatus > 0 && total >= 2)
    {
        for (i = 0; i < nStatus; i++)
        {
            u32 slot = 1 + ((2 * i + 1) * (total - 1)) / (2 * nStatus);
            u32 tries = 0;

            if (slot < 1) slot = 1;
            if (slot > total - 1) slot = total - 1;

            while (isStatusSlot[slot] && tries < total)
            {
                slot++;
                if (slot > total - 1)
                    slot = 1;
                tries++;
            }
            isStatusSlot[slot] = TRUE;
        }
    }

    // Emit.
    di = 0;
    si = 0;
    count = 0;
    for (i = 0; i < total; i++)
    {
        enum Move mv;

        if (isStatusSlot[i] && si < nStatus)
            mv = status[si++];
        else if (di < nDmg)
            mv = dmg[di++];
        else if (si < nStatus)
            mv = status[si++];
        else
            break;

        out[count].move = mv;
        out[count].level = CheckpointLevel(i, n);
        count++;
    }

    out[count].move = LEVEL_UP_MOVE_END;
    out[count].level = 0;
    return count;
}

// ---- public accessor -------------------------------------------------

const struct LevelUpMove *LearnsetGen_GetLearnset(enum Species species)
{
    u32 i, victim;

    species = SanitizeSpeciesId(species);
    LearnsetGen_EnsureBuilt();

    for (i = 0; i < LG_CACHE_SLOTS; i++)
    {
        if (sCacheSpecies[i] == (u16)(species + 1))
        {
            sCacheLru[i] = ++sCacheClock;
            return sCache[i];
        }
    }

    victim = 0;
    for (i = 1; i < LG_CACHE_SLOTS; i++)
    {
        if (sCacheLru[i] < sCacheLru[victim])
            victim = i;
    }

    if (Generate(species, sCache[victim]) == 0)
    {
        sCacheSpecies[victim] = 0;
        return NULL;
    }

    sCacheSpecies[victim] = (u16)(species + 1);
    sCacheLru[victim] = ++sCacheClock;
    return sCache[victim];
}
