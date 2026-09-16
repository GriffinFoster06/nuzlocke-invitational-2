// ============================================================================
// Phase 2 - randomized abilities. See include/ability_gen.h.
//
// Two EWRAM layers, both rebuilt when a relevant ruleset toggle or the run seed
// changes (mirrors src/power_score.c and src/learnset_gen.c):
//   * sFamilyRoot[] - each species' evolution-family root, built by one forward
//     pass over every species' GetSpeciesEvolutions() (the same technique
//     power_score.c uses for its temporary hasPreBits) and then flattened.
//     This avoids GetSpeciesPreEvolution()'s O(N*evos) reverse scan, which is
//     far too slow for an accessor this hot.
//   * sAbilityPool[] - the eligible-ability list, so a pick is one modulo.
// Selection is deterministic from RunRng_Seed(SALT_ABILITY, key, slot, 0).
// ============================================================================

#include "global.h"
#include "ability_gen.h"
#include "malloc.h"
#include "pokemon.h"
#include "random.h"
#include "run_rng.h"
#include "ruleset.h"
#include "constants/abilities.h"
#include "constants/characters.h"
#include "constants/pokedex.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"

#define AG_POOL_CAP     ABILITIES_COUNT
#define AG_MAX_ATTEMPTS 8   // slot-collision re-rolls; bounded, deterministic
#define AG_ROOT_DEPTH   8   // evolution chains are <= 3 deep; guards a bad cycle

static EWRAM_DATA u16 sFamilyRoot[NUM_SPECIES] = {0};
static EWRAM_DATA u16 sAbilityPool[AG_POOL_CAP] = {0};
static EWRAM_DATA u16 sPoolCount = 0;
static EWRAM_DATA bool8 sBuilt = FALSE;  // gates the first build; sSignature is only trusted once set
static EWRAM_DATA u32 sSignature = 0;
static EWRAM_DATA bool8 sBuilding = FALSE; // re-entrancy guard: the build walks the
                                           // evolution table, which must never end up
                                           // back inside GetSpeciesAbility().

// Perf: GetSpeciesAbility() (src/pokemon.c) - which is called on every
// switch-in, every AI party scan and every mon construction - resolves
// through here, and each resolution runs up to NUM_ABILITY_SLOTS
// RunRng_Seed()+reroll passes. All three slots are a pure function of species
// (all consumers of a species see the same result, per docs/SPEC.md), so a
// small direct-mapped cache turns repeat lookups into one array read. Cleared
// alongside the family-root/pool rebuild in AbilityGen_EnsureBuilt(), which
// already re-runs on every signature-affecting change.
#define AG_CACHE_SLOTS 64   // direct-mapped; a collision just recomputes, never wrong
static EWRAM_DATA u16 sAbCacheKey[AG_CACHE_SLOTS] = {0}; // species + 1; 0 == empty
static EWRAM_DATA u16 sAbCacheAbility[AG_CACHE_SLOTS][NUM_ABILITY_SLOTS] = {0}; // ABILITY_NONE == 0; ABILITIES_COUNT exceeds u8

// ---- signature -----------------------------------------------------------

static u32 CurrentSignature(void)
{
    // Phase 11A.6: BuildFamilyRoots() walks GetSpeciesEvolutions(), which is
    // now generation-mask-filtered, so a mask change must invalidate this
    // cache exactly like a ruleset-toggle change does.
    u32 genMask = 0;
    u32 gen;

    for (gen = 0; gen < 9; gen++)
    {
        if (GetRulesetSetting(SETTING_GEN_1_ENABLED + gen))
            genMask |= (1u << gen);
    }

    return ((u32)GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION))
         ^ ((u32)GetRulesetSetting(SETTING_ABILITY_EVO_CONSISTENCY) << 4)
         ^ ((u32)GetRulesetSetting(SETTING_WONDER_GUARD_MODE) << 8)
         ^ (genMask << 12)
         ^ GetRunSeed();
}

// ---- eligibility ---------------------------------------------------------

static bool32 AbilityEligible(enum Ability ability)
{
    const u8 *name = gAbilitiesInfo[ability].name;

    if (ability == ABILITY_NONE)
        return FALSE;
    // Placeholder entries (ABILITY_314, ABILITY_317, ...) are named "-------".
    if (name[0] == EOS || name[0] == CHAR_HYPHEN)
        return FALSE;
    // docs/SPEC.md "Shedinja": Wonder Guard stays Shedinja's alone unless the
    // player explicitly opts into it appearing on other species.
    if (ability == ABILITY_WONDER_GUARD
     && GetRulesetSetting(SETTING_WONDER_GUARD_MODE) != WGUARD_RANDOMIZED)
        return FALSE;

    return TRUE;
}

// ---- build ---------------------------------------------------------------

// Repoint every species currently rooted at `from` to `to` (a small,
// bounded-count union - see the National-Dex-linking pass below).
static void UnionFamilyRoots(u16 from, u16 to)
{
    enum Species u;

    if (from == to)
        return;
    for (u = 1; u < NUM_SPECIES; u++)
    {
        if (IsSpeciesEnabled(u) && sFamilyRoot[u] == from)
            sFamilyRoot[u] = to;
    }
}

// Forward pass: for every species, record it as the parent of each of its
// evolution targets, then walk each species up to its root.
static void BuildFamilyRoots(void)
{
    enum Species s;
    u32 i, depth;
    u16 *dexRoot;

    for (s = 0; s < NUM_SPECIES; s++)
        sFamilyRoot[s] = s;   // a species with no pre-evolution is its own root

    for (s = 1; s < NUM_SPECIES; s++)
    {
        const struct Evolution *evos;

        if (!IsSpeciesEnabled(s))
            continue;

        evos = GetSpeciesEvolutions(s);
        for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
        {
            enum Species t = evos[i].targetSpecies;
            // Guard before touching the table: a disabled ID has no data.
            if (evos[i].method != EVO_NONE && t != SPECIES_NONE && t < NUM_SPECIES
             && IsSpeciesEnabled(t) && t != s)
                sFamilyRoot[t] = s;   // parent, flattened to a root just below
        }
    }

    // Flatten parent chains to roots. Ascending order means a species' parent
    // has usually been flattened already, but branch orders vary, so walk.
    for (s = 1; s < NUM_SPECIES; s++)
    {
        u32 root = sFamilyRoot[s];

        for (depth = 0; depth < AG_ROOT_DEPTH && sFamilyRoot[root] != root; depth++)
            root = sFamilyRoot[root];

        sFamilyRoot[s] = root;
    }

    // Phase 11A.6: union any two evolution families that share a National
    // Dex number (regional forms and other alternate forms sharing a base
    // species' dex entry) into one family. This is what lets
    // AbilityGen_FamilyRoot() also answer Nuzlocke's Dupes Clause "same
    // species" question without nuzlocke.c re-deriving it per encounter.
    // One O(NUM_SPECIES) scan with an O(NATIONAL_DEX_COUNT) scratch table to
    // find same-dex pairs, plus a small, bounded number of O(NUM_SPECIES)
    // unions (regional/alternate forms are a few dozen species, not
    // thousands) - a one-time run-start cost, never repeated per encounter.
    dexRoot = AllocZeroed(sizeof(u16) * (NATIONAL_DEX_COUNT + 1));
    if (dexRoot != NULL)
    {
        for (s = 1; s < NUM_SPECIES; s++)
        {
            enum NationalDexOrder dex;

            if (!IsSpeciesEnabled(s))
                continue;
            dex = SpeciesToNationalPokedexNum(s);
            if (dex == NATIONAL_DEX_NONE || dex > NATIONAL_DEX_COUNT)
                continue;
            if (dexRoot[dex] == 0)
                dexRoot[dex] = sFamilyRoot[s] + 1; // +1: 0 means "unseen"
            else if (sFamilyRoot[s] != dexRoot[dex] - 1)
                UnionFamilyRoots(sFamilyRoot[s], dexRoot[dex] - 1);
        }
        Free(dexRoot);
    }
}

static void BuildAbilityPool(void)
{
    enum Ability a;

    sPoolCount = 0;
    for (a = ABILITY_NONE + 1; a < ABILITIES_COUNT; a++)
    {
        if (!AbilityEligible(a))
            continue;
        if (sPoolCount >= AG_POOL_CAP) // cannot happen; guard anyway
            break;
        sAbilityPool[sPoolCount++] = a;
    }
}

void AbilityGen_EnsureBuilt(void)
{
    if ((sBuilt && sSignature == CurrentSignature()) || sBuilding)
        return;

    sBuilding = TRUE;
    BuildFamilyRoots();
    BuildAbilityPool();
    sBuilding = FALSE;

    memset(sAbCacheKey, 0, sizeof(sAbCacheKey));

    sSignature = CurrentSignature();
    sBuilt = TRUE;
}

void AbilityGen_Invalidate(void)
{
    sBuilt = FALSE;
}

// Phase 11A.6: exposes the family-root map (see include/ability_gen.h) for
// callers other than this module's own ability selection - currently
// src/nuzlocke.c's Dupes Clause. Safe regardless of SETTING_ABILITY_RANDOMIZATION;
// triggers the build itself.
enum Species AbilityGen_FamilyRoot(enum Species species)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return species;
    AbilityGen_EnsureBuilt();
    if (!sBuilt)
        return species; // build failed (out of heap); no family linking this call
    return sFamilyRoot[species];
}

// ---- queries -------------------------------------------------------------

bool32 AbilityGen_IsActive(void)
{
    return GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION) != 0;
}

// Runs the full seeded resolution for every slot of `species`. Split out of
// AbilityGen_Get() so the cache below has one place to fill on a miss.
static void ComputeAbilities(enum Species species, enum Ability chosen[NUM_ABILITY_SLOTS])
{
    u32 key = GetRulesetSetting(SETTING_ABILITY_EVO_CONSISTENCY) ? sFamilyRoot[species] : species;
    u32 s;

    for (s = 0; s < NUM_ABILITY_SLOTS; s++)
    {
        rng_value_t st;
        u32 attempt;

        // An empty vanilla slot stays empty: a one-ability species must not
        // silently gain a second and a hidden ability.
        if (gSpeciesInfo[species].abilities[s] == ABILITY_NONE)
        {
            chosen[s] = ABILITY_NONE;
            continue;
        }

        st = RunRng_Seed(SALT_ABILITY, key, s, 0);
        for (attempt = 0; attempt < AG_MAX_ATTEMPTS; attempt++)
        {
            enum Ability pick = sAbilityPool[LocalRandom32(&st) % sPoolCount];
            bool32 dup = FALSE;
            u32 e;

            for (e = 0; e < s; e++)
            {
                if (chosen[e] == pick)
                    dup = TRUE;
            }

            chosen[s] = pick;
            if (!dup)
                break;
        }
    }
}

enum Ability AbilityGen_Get(enum Species species, u8 slot)
{
    u32 line;

    if (slot >= NUM_ABILITY_SLOTS || species == SPECIES_NONE || species >= NUM_SPECIES)
        return ABILITY_NONE;
    if (!IsSpeciesEnabled(species))
        return ABILITY_NONE;
    // docs/SPEC.md "Shedinja": its Wonder Guard / 1 HP pairing is the species,
    // so it is never randomized regardless of the Wonder Guard setting.
    if (species == SPECIES_SHEDINJA)
        return ABILITY_NONE;

    AbilityGen_EnsureBuilt();
    if (!sBuilt || sPoolCount == 0)
        return ABILITY_NONE;

    line = species % AG_CACHE_SLOTS;
    if (sAbCacheKey[line] != (u16)(species + 1))
    {
        enum Ability chosen[NUM_ABILITY_SLOTS];
        u32 s;

        ComputeAbilities(species, chosen);
        for (s = 0; s < NUM_ABILITY_SLOTS; s++)
            sAbCacheAbility[line][s] = chosen[s];
        sAbCacheKey[line] = (u16)(species + 1);
    }

    return sAbCacheAbility[line][slot];
}
