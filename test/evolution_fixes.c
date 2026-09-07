#include "global.h"
#include "caps.h"
#include "evolution_fixes.h"
#include "pokemon.h"
#include "test/test.h"
#include "constants/pokemon.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------
// Phase 7 - evolution overrides (docs/SPEC.md "Trade evolutions",
// "Species-specific evolution fixes", "Evolution system").
// ---------------------------------------------------------------------------

static const struct Evolution *Evos(enum Species species)
{
    return GetSpeciesEvolutions(species);
}

static bool32 HasMethod(const struct Evolution *evos, u32 method)
{
    for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
        if (evos[i].method == method)
            return TRUE;
    return FALSE;
}

static bool32 OnlyMethodIsTrade(const struct Evolution *evos)
{
    bool32 sawTrade = FALSE;
    for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
    {
        if (evos[i].method == EVO_TRADE)
            sawTrade = TRUE;
        else if (evos[i].method != EVO_NONE)
            return FALSE; // a non-trade real evolution exists
    }
    return sawTrade;
}

TEST("GetSpeciesEvolutions returns the Phase 7 override for fixed species")
{
    EXPECT(EvoFix_GetOverride(SPECIES_KARRABLAST) != NULL);
    EXPECT_EQ(Evos(SPECIES_KARRABLAST), EvoFix_GetOverride(SPECIES_KARRABLAST));
    EXPECT_EQ(Evos(SPECIES_KARRABLAST)[0].method, EVO_LEVEL);
    EXPECT_EQ(Evos(SPECIES_KARRABLAST)[0].targetSpecies, SPECIES_ESCAVALIER);
    EXPECT_EQ(Evos(SPECIES_SHELMET)[0].targetSpecies, SPECIES_ACCELGOR);
}

TEST("An unlisted species keeps its stock evolutions")
{
    EXPECT(EvoFix_GetOverride(SPECIES_BULBASAUR) == NULL);
    EXPECT_EQ(Evos(SPECIES_BULBASAUR)[0].targetSpecies, SPECIES_IVYSAUR);
}

TEST("No overridden species is left with a link trade as its only evolution")
{
    static const u16 fixed[] = {
        SPECIES_KARRABLAST, SPECIES_SHELMET, SPECIES_MANTYKE,
        SPECIES_GIMMIGHOUL_CHEST, SPECIES_GIMMIGHOUL_ROAMING,
        SPECIES_YAMASK_GALAR, SPECIES_BISHARP, SPECIES_ZWEILOUS,
        SPECIES_URSARING, SPECIES_FARFETCHD_GALAR,
    };
    for (u32 i = 0; i < ARRAY_COUNT(fixed); i++)
    {
        EXPECT(!OnlyMethodIsTrade(Evos(fixed[i])));
        EXPECT(!HasMethod(Evos(fixed[i]), EVO_SCRIPT_TRIGGER));
    }
}

TEST("No override evolves above the pre-Champion level cap")
{
    for (enum Species s = 1; s < NUM_SPECIES; s++)
    {
        const struct Evolution *evos = EvoFix_GetOverride(s);
        if (evos == NULL)
            continue;
        for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            if (evos[i].method == EVO_LEVEL || evos[i].method == EVO_LEVEL_BATTLE_ONLY)
                EXPECT(evos[i].param <= PRE_CHAMPION_LEVEL_CAP);
        }
    }
}

TEST("Zweilous evolves exactly at the pre-Champion cap, not above it")
{
    EXPECT_EQ(Evos(SPECIES_ZWEILOUS)[0].method, EVO_LEVEL);
    EXPECT_EQ(Evos(SPECIES_ZWEILOUS)[0].param, PRE_CHAMPION_LEVEL_CAP);
    EXPECT_EQ(Evos(SPECIES_ZWEILOUS)[0].targetSpecies, SPECIES_HYDREIGON);
}

TEST("Ursaring keeps an item evolution and its offspring marker")
{
    EXPECT(HasMethod(Evos(SPECIES_URSARING), EVO_ITEM));
    EXPECT(HasMethod(Evos(SPECIES_URSARING), EVO_NONE)); // URSALUNA_BLOODMOON link
}
