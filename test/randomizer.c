#include "global.h"
#include "pokemon.h"
#include "power_score.h"
#include "randomizer.h"
#include "ruleset.h"
#include "test/test.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------
// Phase 2 core randomizer: power score + deterministic power-matched selection.
// ---------------------------------------------------------------------------

static void UseBalancedRandomizerRuleset(void)
{
    SetRunSeed(0x1234ABCD);
    SetRulesetSetting(SETTING_WILD_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_STARTER_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_GIFT_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_STATIC_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_LEGENDARY_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_POWER_MATCHING, PWRMATCH_NORMAL);
    SetRulesetSetting(SETTING_EVO_STAGE_MATCHING, EVOSTAGE_PREFER);
    SetRulesetSetting(SETTING_ENCOUNTER_MAPPING, ENCMAP_SLOT);
    SetRulesetSetting(SETTING_PREMIUM_POOL_MODE, PREMPOOL_CURATED);
    SetRulesetSetting(SETTING_ABILITY_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_ALLOW_LEGENDARY, 0);
    SetRulesetSetting(SETTING_ALLOW_MYTHICAL, 0);
    SetRulesetSetting(SETTING_ALLOW_SUB_LEGENDARY, 0);
    SetRulesetSetting(SETTING_ALLOW_ULTRA_BEAST, 0);
    SetRulesetSetting(SETTING_ALLOW_PARADOX, 0);
    PowerScore_Invalidate();
    PowerScore_EnsureBuilt();
}

TEST("Power score penalises a lopsided defensive spread (Shuckle < BST)")
{
    UseBalancedRandomizerRuleset();
    EXPECT_LT(GetSpeciesRawPowerScore(SPECIES_SHUCKLE), GetSpeciesBaseStatTotal(SPECIES_SHUCKLE));
}

TEST("Power score rewards a strong offensive / speed spread (Mewtwo > BST)")
{
    UseBalancedRandomizerRuleset();
    EXPECT_GT(GetSpeciesRawPowerScore(SPECIES_MEWTWO), GetSpeciesBaseStatTotal(SPECIES_MEWTWO));
}

TEST("Potential blend lifts an unevolved mon toward its final form")
{
    u32 magikarpRaw, magikarpBlend;

    UseBalancedRandomizerRuleset();
    magikarpRaw = GetSpeciesRawPowerScore(SPECIES_MAGIKARP);
    magikarpBlend = GetSpeciesPowerScore(SPECIES_MAGIKARP);

    EXPECT_GT(magikarpBlend, magikarpRaw);
    // ...but still well short of Gyarados itself.
    EXPECT_LT(magikarpBlend, GetSpeciesPowerScore(SPECIES_GYARADOS));
}

TEST("Ability adjustment flips Slaking / Azumarill only when abilities are vanilla")
{
    u32 slakingRand, slakingVanilla, azuRand, azuVanilla;

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ABILITY_RANDOMIZATION, 1);
    PowerScore_Invalidate();
    slakingRand = GetSpeciesRawPowerScore(SPECIES_SLAKING);
    azuRand = GetSpeciesRawPowerScore(SPECIES_AZUMARILL);

    SetRulesetSetting(SETTING_ABILITY_RANDOMIZATION, 0);
    PowerScore_Invalidate();
    slakingVanilla = GetSpeciesRawPowerScore(SPECIES_SLAKING);
    azuVanilla = GetSpeciesRawPowerScore(SPECIES_AZUMARILL);

    EXPECT_LT(slakingVanilla, slakingRand);   // Truant drags Slaking down
    EXPECT_GT(azuVanilla, azuRand);           // Huge Power lifts Azumarill up
}

TEST("Evolutionary-stage buckets")
{
    UseBalancedRandomizerRuleset();
    EXPECT_EQ(GetSpeciesEvoStageBucket(SPECIES_WURMPLE), EVO_BUCKET_UNEVOLVED);
    EXPECT_EQ(GetSpeciesEvoStageBucket(SPECIES_SILCOON), EVO_BUCKET_MIDDLE);
    EXPECT_EQ(GetSpeciesEvoStageBucket(SPECIES_BEAUTIFLY), EVO_BUCKET_FINAL);
    EXPECT_EQ(GetSpeciesEvoStageBucket(SPECIES_TAUROS), EVO_BUCKET_FINAL);   // single-stage -> FINAL
}

TEST("Restricted legendaries are category-banned by default, ordinary mons are not")
{
    UseBalancedRandomizerRuleset();
    EXPECT((IsSpeciesCategoryBanned(SPECIES_MEWTWO)));
    EXPECT((IsSpeciesCategoryBanned(SPECIES_RAYQUAZA)));
    EXPECT(!(IsSpeciesCategoryBanned(SPECIES_ZIGZAGOON)));
    EXPECT(!(IsSpeciesCategoryBanned(SPECIES_SALAMENCE)));   // pseudo-legend stays in the ordinary pool
}

TEST("Wild slot replacement is deterministic for a fixed (seed, slot)")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_ZIGZAGOON } };
    struct WildPokemonInfo info = { 20, slots, 0xC0FFEEu, 0xBEEF01u };
    enum Species first, i;

    UseBalancedRandomizerRuleset();
    first = Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON);

    for (i = 0; i < 64; i++)
        EXPECT_EQ(Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON), first);
}

TEST("Wild replacement stays in the ordinary pool and near the original's power")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_ZIGZAGOON } };
    struct WildPokemonInfo info = { 20, slots, 0x111u, 0x222u };
    enum Species pick;
    u32 target, got, dist;

    UseBalancedRandomizerRuleset();
    pick = Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON);

    EXPECT((IsSpeciesPowerEligible(pick)));
    EXPECT(!(IsSpeciesCategoryBanned(pick)));

    target = GetSpeciesMatchMetric(SPECIES_ZIGZAGOON, POWER_MATCH_NORMAL);
    got = GetSpeciesMatchMetric(pick, POWER_MATCH_NORMAL);
    dist = (got > target) ? got - target : target - got;
    // NORMAL window is 60; the widest PREFER fallback rung is 3x that.
    EXPECT_LE(dist, 180);
}

TEST("Different encounter slots of the same species can diverge")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_ZIGZAGOON } };
    struct WildPokemonInfo info = { 20, slots, 0xAAAAu, 0xBBBBu };
    enum Species base;
    u32 slot, distinct = 0;
    enum Species seen[8];

    UseBalancedRandomizerRuleset();
    base = Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON);
    seen[0] = base;
    distinct = 1;

    for (slot = 1; slot < 8; slot++)
    {
        enum Species s = Randomizer_WildSlotSpecies(&info, slot, SPECIES_ZIGZAGOON);
        u32 j;
        bool32 isNew = TRUE;
        for (j = 0; j < distinct; j++)
            if (seen[j] == s)
                isNew = FALSE;
        if (isNew)
            seen[distinct++] = s;
    }
    EXPECT_GE(distinct, 2);
}

TEST("Re-rolling the run seed reshuffles the encounter map")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_POOCHYENA } };
    struct WildPokemonInfo info = { 20, slots, 0x5EED01u, 0x5EED02u };
    enum Species a, b;

    UseBalancedRandomizerRuleset();
    SetRunSeed(0x11111111);
    PowerScore_EnsureBuilt();
    a = Randomizer_WildSlotSpecies(&info, 3, SPECIES_POOCHYENA);

    SetRunSeed(0x22222222);
    b = Randomizer_WildSlotSpecies(&info, 3, SPECIES_POOCHYENA);

    EXPECT_NE(a, b);
}

TEST("Disabling wild randomization returns the vanilla species untouched")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_ZIGZAGOON } };
    struct WildPokemonInfo info = { 20, slots, 0x1u, 0x2u };

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_WILD_RANDOMIZATION, 0);
    EXPECT_EQ(Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON), SPECIES_ZIGZAGOON);
}

TEST("Starter trio is randomized, distinct, and all fully evolvable")
{
    enum Species a, b, c;

    UseBalancedRandomizerRuleset();
    a = Randomizer_StarterSpecies(0);
    b = Randomizer_StarterSpecies(1);
    c = Randomizer_StarterSpecies(2);

    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
    // Starters force strict same-stage matching against the unevolved anchors.
    EXPECT_EQ(GetSpeciesEvoStageBucket(a), EVO_BUCKET_UNEVOLVED);
    EXPECT_EQ(GetSpeciesEvoStageBucket(b), EVO_BUCKET_UNEVOLVED);
    EXPECT_EQ(GetSpeciesEvoStageBucket(c), EVO_BUCKET_UNEVOLVED);
}

TEST("A legendary static slot draws from the premium pool")
{
    enum Species pick;

    UseBalancedRandomizerRuleset();
    // Rayquaza's map: Sky Pillar. mapGroup/mapNum come from gSaveBlock1Ptr, which
    // the test harness leaves at a stable default - determinism is what matters.
    pick = Randomizer_StaticSpecies(SPECIES_RAYQUAZA, 70, 0);

    EXPECT_NE(pick, SPECIES_NONE);
    EXPECT((IsSpeciesPowerEligible(pick)));
    // Curated premium pool = category-banned OR blended power >= 600.
    EXPECT((IsSpeciesCategoryBanned(pick) || GetSpeciesPowerScore(pick) >= 600));
}

TEST("An ordinary static slot draws from the ordinary pool")
{
    enum Species pick;

    UseBalancedRandomizerRuleset();
    pick = Randomizer_StaticSpecies(SPECIES_VOLTORB, 25, 0);

    EXPECT_NE(pick, SPECIES_NONE);
    EXPECT((IsSpeciesPowerEligible(pick)));
    EXPECT(!(IsSpeciesCategoryBanned(pick)));
}

TEST("Every wild slot resolves to an eligible species under each power-matching mode")
{
    static const struct WildPokemon slots[3] =
    {
        { 3, 3, SPECIES_WURMPLE },
        { 5, 5, SPECIES_ZIGZAGOON },
        { 40, 40, SPECIES_SALAMENCE },
    };
    struct WildPokemonInfo info = { 20, slots, 0x9001u, 0x9002u };
    u32 mode, slot;

    UseBalancedRandomizerRuleset();
    for (mode = PWRMATCH_STRICT; mode <= PWRMATCH_UNRESTRICTED; mode++)
    {
        SetRulesetSetting(SETTING_POWER_MATCHING, mode);
        for (slot = 0; slot < 3; slot++)
        {
            enum Species pick = Randomizer_WildSlotSpecies(&info, slot, slots[slot].species);
            EXPECT_NE(pick, SPECIES_NONE);
            EXPECT((IsSpeciesPowerEligible(pick)));
            EXPECT(!(IsSpeciesCategoryBanned(pick)));
        }
    }
}
