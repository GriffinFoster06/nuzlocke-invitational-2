#include "global.h"
#include "pokemon.h"
#include "ruleset.h"
#include "nuzlocke.h"
#include "test/test.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------
// Phase 3 - Nuzlocke engine: evolutionary-family Dupes Clause closure.
// ---------------------------------------------------------------------------

static void ResetFamilyState(bool32 countForms)
{
    memset(gSaveBlock3Ptr->nuzlocke.familyOwned, 0, sizeof(gSaveBlock3Ptr->nuzlocke.familyOwned));
    SetRulesetSetting(SETTING_DUPES_CLAUSE, 1);
    SetRulesetSetting(SETTING_DUPES_COUNT_FORMS, countForms ? 1 : 0);
}

TEST("Dupes Clause: catching a base stage locks the whole line")
{
    ResetFamilyState(FALSE);
    Nuzlocke_MarkFamilyOwned(SPECIES_CHARMANDER);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CHARMANDER));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CHARMELEON));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CHARIZARD));
    EXPECT(!Nuzlocke_IsFamilyOwned(SPECIES_SQUIRTLE));
}

TEST("Dupes Clause: catching a final stage locks its pre-evolutions")
{
    ResetFamilyState(FALSE);
    Nuzlocke_MarkFamilyOwned(SPECIES_CHARIZARD);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CHARMANDER));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CHARMELEON));
}

TEST("Dupes Clause: split evolutions are one family (Eevee)")
{
    ResetFamilyState(FALSE);
    Nuzlocke_MarkFamilyOwned(SPECIES_VAPOREON);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_EEVEE));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_JOLTEON));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_FLAREON));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_SYLVEON));
}

TEST("Dupes Clause: split evolutions are one family (Wurmple)")
{
    ResetFamilyState(FALSE);
    Nuzlocke_MarkFamilyOwned(SPECIES_DUSTOX);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_WURMPLE));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_SILCOON));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_BEAUTIFLY));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_CASCOON));
}

TEST("Dupes Clause: forms off keeps regional variants separate")
{
    ResetFamilyState(FALSE);
    Nuzlocke_MarkFamilyOwned(SPECIES_VULPIX);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_NINETALES));
    EXPECT(!Nuzlocke_IsFamilyOwned(SPECIES_VULPIX_ALOLA));
    EXPECT(!Nuzlocke_IsFamilyOwned(SPECIES_NINETALES_ALOLA));
}

TEST("Dupes Clause: forms on links regional variants into the family")
{
    ResetFamilyState(TRUE);
    Nuzlocke_MarkFamilyOwned(SPECIES_VULPIX);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_VULPIX_ALOLA));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_NINETALES_ALOLA));
}

TEST("Dupes Clause: forms on links a cross-regional evolution (Meowth -> Perrserker)")
{
    ResetFamilyState(TRUE);
    Nuzlocke_MarkFamilyOwned(SPECIES_MEOWTH);

    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_PERSIAN));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_MEOWTH_GALAR));
    EXPECT(Nuzlocke_IsFamilyOwned(SPECIES_PERRSERKER));
}

TEST("Dupes Clause: an unrelated family stays free")
{
    ResetFamilyState(TRUE);
    Nuzlocke_MarkFamilyOwned(SPECIES_MEOWTH);

    EXPECT(!Nuzlocke_IsFamilyOwned(SPECIES_ZIGZAGOON));
    EXPECT(!Nuzlocke_IsFamilyOwned(SPECIES_PIKACHU));
}
