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
#include "pokemon.h"
#include "random.h"
#include "run_rng.h"
#include "ruleset.h"
#include "constants/abilities.h"
#include "constants/characters.h"
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

// ---- signature -----------------------------------------------------------

static u32 CurrentSignature(void)
{
    return ((u32)GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION))
         ^ ((u32)GetRulesetSetting(SETTING_ABILITY_EVO_CONSISTENCY) << 4)
         ^ ((u32)GetRulesetSetting(SETTING_WONDER_GUARD_MODE) << 8)
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

// Forward pass: for every species, record it as the parent of each of its
// evolution targets, then walk each species up to its root.
static void BuildFamilyRoots(void)
{
    enum Species s;
    u32 i, depth;

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

    sSignature = CurrentSignature();
    sBuilt = TRUE;
}

void AbilityGen_Invalidate(void)
{
    sBuilt = FALSE;
}

// ---- queries -------------------------------------------------------------

bool32 AbilityGen_IsActive(void)
{
    return GetRulesetSetting(SETTING_ABILITY_RANDOMIZATION) != 0;
}

enum Ability AbilityGen_Get(enum Species species, u8 slot)
{
    enum Ability chosen[NUM_ABILITY_SLOTS];
    u32 key, s;

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

    key = GetRulesetSetting(SETTING_ABILITY_EVO_CONSISTENCY) ? sFamilyRoot[species] : species;

    // Resolve every slot up to the requested one so a later slot can avoid
    // duplicating an earlier one. At most NUM_ABILITY_SLOTS iterations.
    for (s = 0; s <= slot; s++)
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

    return chosen[slot];
}
