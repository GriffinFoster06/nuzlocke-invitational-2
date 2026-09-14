// ============================================================================
// Phase 11A.6 - generation classification. See include/species_generation.h.
//
// Two ROM-only layers, checked in order:
//  1. A small hand-curated exceptions list, for the documented cases where a
//     form/variant was added to an OLDER base species well after that base
//     species' own generation (event Pikachu forms, Spiky-eared Pichu,
//     Zygarde's Sun/Moon-added formes, the Let's Go partner forms). These
//     inherit their base species' natDexNum, so the dex-bucket rule below
//     would otherwise misclassify them.
//  2. Upstream's per-species category flags (regional/mega/gigantamax/tera/
//     primal/totem), then a National-Dex-number range bucket for everything
//     else - base species and same-generation alternate forms (Deoxys
//     formes, Rotom appliances, Arceus plates, Unown letters, Vivillon
//     patterns, etc. are all introduced alongside their base and need no
//     entry here).
// ============================================================================

#include "global.h"
#include "pokemon.h"
#include "species_generation.h"
#include "constants/pokedex.h"
#include "constants/species.h"

struct SpeciesGenerationException
{
    enum Species species;
    u8 generation;
};

// Sorted roughly by species id; add to this list only for a form/variant
// that (a) shares its base species' National Dex number and (b) was
// introduced in a later generation than that base species.
static const struct SpeciesGenerationException sGenerationExceptions[] =
{
    // Pikachu event/cap cosmetic forms (Pikachu itself is Gen 1).
    { SPECIES_PIKACHU_COSPLAY,   6 }, // ORAS
    { SPECIES_PIKACHU_ROCK_STAR, 6 },
    { SPECIES_PIKACHU_BELLE,     6 },
    { SPECIES_PIKACHU_POP_STAR,  6 },
    { SPECIES_PIKACHU_PHD,       6 },
    { SPECIES_PIKACHU_LIBRE,     6 },
    { SPECIES_PIKACHU_ORIGINAL,  6 }, // XY cap Pikachu
    { SPECIES_PIKACHU_HOENN,     6 },
    { SPECIES_PIKACHU_SINNOH,    6 },
    { SPECIES_PIKACHU_UNOVA,     6 },
    { SPECIES_PIKACHU_KALOS,     6 },
    { SPECIES_PIKACHU_ALOLA,     7 }, // Sun/Moon cap Pikachu
    { SPECIES_PIKACHU_PARTNER,   7 }, // Let's Go
    { SPECIES_PIKACHU_WORLD,     8 }, // Sword/Shield
    { SPECIES_PIKACHU_STARTER,   7 }, // Let's Go partner form
    { SPECIES_EEVEE_STARTER,     7 }, // Let's Go partner form (Eevee is Gen 1)
    // Spiky-eared Pichu (Pichu is Gen 2) - HeartGold/SoulSilver event, Gen 4.
    { SPECIES_PICHU_SPIKY_EARED, 4 },
    // Zygarde's 10%/Complete Formes (Zygarde is Gen 6) were added in Sun/Moon.
    { SPECIES_ZYGARDE_10_AURA_BREAK,      7 },
    { SPECIES_ZYGARDE_10_POWER_CONSTRUCT, 7 },
    { SPECIES_ZYGARDE_50_POWER_CONSTRUCT, 7 },
    { SPECIES_ZYGARDE_COMPLETE,           7 },
};

// Last National Dex number belonging to each generation.
static const u16 sGenDexEnd[9] =
{
    151,  // Gen 1: Bulbasaur..Mew
    251,  // Gen 2: Chikorita..Celebi
    386,  // Gen 3: Treecko..Deoxys
    493,  // Gen 4: Turtwig..Arceus
    649,  // Gen 5: Victini..Genesect
    721,  // Gen 6: Chespin..Volcanion
    809,  // Gen 7: Rowlet..Melmetal
    905,  // Gen 8: Grookey..Enamorus
    1025, // Gen 9: Sprigatito..Pecharunt
};

static u8 DexNumberToGeneration(enum NationalDexOrder dex)
{
    u32 i;

    if (dex == NATIONAL_DEX_NONE)
        return 0;
    for (i = 0; i < ARRAY_COUNT(sGenDexEnd); i++)
    {
        if (dex <= sGenDexEnd[i])
            return i + 1;
    }
    return 9; // beyond the known table; treat as the newest generation
}

u8 GetSpeciesGeneration(enum Species species)
{
    const struct SpeciesInfo *si;
    u32 i;

    species = SanitizeSpeciesId(species);
    if (species == SPECIES_NONE)
        return 0;

    for (i = 0; i < ARRAY_COUNT(sGenerationExceptions); i++)
    {
        if (sGenerationExceptions[i].species == species)
            return sGenerationExceptions[i].generation;
    }

    si = &gSpeciesInfo[species];
    if (si->isPaldeanForm)
        return 9;
    if (si->isGalarianForm || si->isHisuianForm || si->isGigantamax)
        return 8;
    if (si->isAlolanForm || si->isTotem)
        return 7;
    if (si->isTeraForm)
        return 9;
    if (si->isMegaEvolution)
        return (species >= SPECIES_CLEFABLE_MEGA) ? 9 : 6; // Legends Z-A megas
    if (si->isPrimalReversion)
        return 6;

    return DexNumberToGeneration(si->natDexNum);
}
