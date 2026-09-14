#include "global.h"
#include "data.h"
#include "event_data.h"
#include "fishing.h"
#include "pokemon.h"
#include "power_score.h"
#include "random_mon_generation.h"
#include "randomizer.h"
#include "ruleset.h"
#include "test/test.h"
#include "constants/ruleset.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "constants/trainers.h"

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
    SetRulesetSetting(SETTING_TRAINER_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_TRAINER_LEVEL_MODE, TRLEVEL_CAP_SCALED);
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

TEST("Intrinsic power and species mappings do not depend on ability randomization")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_SLAKING } };
    struct WildPokemonInfo info = { 20, slots, 0xAB11u, 0xAB12u };
    u32 slakingRand, slakingVanilla, azuRand, azuVanilla;
    enum Species pickRand, pickVanilla;

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ABILITY_RANDOMIZATION, 1);
    PowerScore_Invalidate();
    slakingRand = GetSpeciesRawPowerScore(SPECIES_SLAKING);
    azuRand = GetSpeciesRawPowerScore(SPECIES_AZUMARILL);
    pickRand = Randomizer_WildSlotSpecies(&info, 0, SPECIES_SLAKING);

    SetRulesetSetting(SETTING_ABILITY_RANDOMIZATION, 0);
    PowerScore_Invalidate();
    slakingVanilla = GetSpeciesRawPowerScore(SPECIES_SLAKING);
    azuVanilla = GetSpeciesRawPowerScore(SPECIES_AZUMARILL);
    pickVanilla = Randomizer_WildSlotSpecies(&info, 0, SPECIES_SLAKING);

    EXPECT_EQ(slakingVanilla, slakingRand);
    EXPECT_EQ(azuVanilla, azuRand);
    EXPECT_EQ(pickVanilla, pickRand);
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

// Phase 11A.6 Deterministic World Principle: once (version, seed, locked
// settings) match, the whole generated world must match too - starters,
// wild slots, and trainer parties alike, independent of call order or of
// whatever the wild-slot cache (src/randomizer.c) happens to hold from a
// previous run. Re-applying the same settings/seed from scratch (as a real
// New Game would via the wizard) must reproduce every result bit-for-bit.
TEST("Same (version, seed, settings) reproduces starter trio, wild slots, and a trainer party identically")
{
    static const struct WildPokemon slots[3] =
    {
        { 5, 5, SPECIES_ZIGZAGOON },
        { 5, 5, SPECIES_POOCHYENA },
        { 5, 5, SPECIES_WINGULL },
    };
    struct WildPokemonInfo info = { 20, slots, 0xFEEDFACEu, 0x0BADBEEFu };
    struct TrainerMon party[3];
    u32 indices[3] = { 0, 1, 2 };
    enum Species starterA[3], starterB[3];
    enum Species wildA[3], wildB[3];
    enum Species trainerA[3], trainerB[3];
    u32 i, m;

    for (i = 0; i < 3; i++)
    {
        // ---- Run A on the first pass, Run B on the second: identical
        // inputs, rebuilt from scratch each time via the public API only
        // (no direct cache pokes), exactly like two separate New Games.
        UseBalancedRandomizerRuleset();
        SetRunSeed(0x600DF00Du);

        for (m = 0; m < 3; m++)
            (i == 0 ? starterA : starterB)[m] = Randomizer_StarterSpecies(m);
        for (m = 0; m < 3; m++)
            (i == 0 ? wildA : wildB)[m] = Randomizer_WildSlotSpecies(&info, m, slots[m].species);

        for (m = 0; m < 3; m++)
        {
            u32 mv;
            party[m].species = slots[m].species;
            party[m].lvl = 30;
            party[m].ability = ABILITY_NONE;
            for (mv = 0; mv < MAX_MON_MOVES; mv++)
                party[m].moves[mv] = MOVE_NONE;
        }
        FlagClear(FLAG_IS_CHAMPION);
        Randomizer_ApplyTrainerParty(party, indices, 3, TRAINER_ROXANNE_1, TRAINER_CLASS_LEADER);
        for (m = 0; m < 3; m++)
            (i == 0 ? trainerA : trainerB)[m] = party[m].species;
    }

    for (i = 0; i < 3; i++)
    {
        EXPECT_EQ(starterA[i], starterB[i]);
        EXPECT_EQ(wildA[i], wildB[i]);
        EXPECT_EQ(trainerA[i], trainerB[i]);
    }
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

TEST("Starter choices remain unique across a broad seed sample")
{
    UseBalancedRandomizerRuleset();
    // Count-and-pick provides the proof; keep a representative seed sweep
    // small enough for the 60-second GBA test-runner watchdog.
    for (u32 seed = 0; seed < 64; seed++)
    {
        enum Species a, b, c;

        SetRunSeed(seed);
        a = Randomizer_StarterSpecies(0);
        b = Randomizer_StarterSpecies(1);
        c = Randomizer_StarterSpecies(2);
        EXPECT_NE(a, b);
        EXPECT_NE(a, c);
        EXPECT_NE(b, c);
    }
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
    EXPECT((IsSpeciesPremiumTier(pick)));
}

TEST("Zero is a valid deterministic run seed")
{
    static const struct WildPokemon slots[1] = { { 5, 5, SPECIES_ZIGZAGOON } };
    struct WildPokemonInfo info = { 20, slots, 0x010203u, 0x040506u };
    enum Species first;

    UseBalancedRandomizerRuleset();
    EXPECT((SetRunSeed(0)));
    EXPECT_EQ(GetRunSeed(), 0);
    first = Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON);
    EXPECT_EQ(Randomizer_WildSlotSpecies(&info, 0, SPECIES_ZIGZAGOON), first);
}

TEST("Exact species bans exclude only the selected species")
{
    UseBalancedRandomizerRuleset();
    EXPECT((Ruleset_SetSpeciesBanned(SPECIES_ZIGZAGOON, TRUE)));
    EXPECT((Ruleset_IsSpeciesBanned(SPECIES_ZIGZAGOON)));
    EXPECT(!(Ruleset_IsSpeciesBanned(SPECIES_LINOONE)));
    PowerScore_EnsureBuilt();
    EXPECT(!(IsSpeciesPowerEligible(SPECIES_ZIGZAGOON)));
    EXPECT((IsSpeciesPowerEligible(SPECIES_LINOONE)));
}

TEST("Unsafe transient forms never enter the project roster")
{
    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ALLOW_OTHER_FORMS, TRUE);
    PowerScore_EnsureBuilt();

    EXPECT(!(IsRandomSpeciesFormSafe(SPECIES_DARMANITAN_ZEN)));
    EXPECT(!(IsSpeciesPowerEligible(SPECIES_DARMANITAN_ZEN)));
    EXPECT((IsRandomSpeciesFormSafe(SPECIES_VULPIX_ALOLA)));
    EXPECT((IsSpeciesPowerEligible(SPECIES_VULPIX_ALOLA)));
}

TEST("Premium identity is immutable and excludes ordinary high-BST species")
{
    UseBalancedRandomizerRuleset();
    EXPECT((IsSpeciesPremium(SPECIES_RAYQUAZA)));
    EXPECT((IsSpeciesPremium(SPECIES_MEWTWO)));
    EXPECT(!(IsSpeciesPremium(SPECIES_SALAMENCE)));
    EXPECT(!(IsSpeciesPremium(SPECIES_SLAKING)));

    SetRulesetSetting(SETTING_ALLOW_LEGENDARY, TRUE);
    EXPECT((IsSpeciesPremium(SPECIES_RAYQUAZA)));
    SetRulesetSetting(SETTING_ALLOW_LEGENDARY, FALSE);
    EXPECT((IsSpeciesPremium(SPECIES_RAYQUAZA)));
}

TEST("Encounter-rate layouts are deterministic permutations within each table")
{
    static const struct WildPokemon slots[NUM_LAND_MONS_ENCOUNTER_SLOTS] = {0};
    struct WildPokemonInfo info = { 20, slots, 0x13579u, 0x24680u };
    bool8 seen[NUM_LAND_MONS_ENCOUNTER_SLOTS] = {0};

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ENCOUNTER_RATE_RANDOMIZATION, 1);
    for (u32 i = 0; i < NUM_LAND_MONS_ENCOUNTER_SLOTS; i++)
    {
        u32 mapped = Randomizer_WildRateSlot(&info, WILD_AREA_LAND, 0, i);
        EXPECT_LT(mapped, NUM_LAND_MONS_ENCOUNTER_SLOTS);
        EXPECT(!(seen[mapped]));
        seen[mapped] = TRUE;
        EXPECT_EQ(Randomizer_WildRateSlot(&info, WILD_AREA_LAND, 0, i), mapped);
    }
}

TEST("Fishing encounter-rate permutations never cross rod subgroups")
{
    static const struct WildPokemon slots[NUM_FISHING_MONS_ENCOUNTER_SLOTS] = {0};
    struct WildPokemonInfo info = { 20, slots, 0x97531u, 0x86420u };

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ENCOUNTER_RATE_RANDOMIZATION, TRUE);
    for (u32 i = 0; i < 2; i++)
        EXPECT_LT(Randomizer_WildRateSlot(&info, WILD_AREA_FISHING, OLD_ROD, i), 2);
    for (u32 i = 2; i < 5; i++)
    {
        u32 mapped = Randomizer_WildRateSlot(&info, WILD_AREA_FISHING, GOOD_ROD, i);
        EXPECT_GE(mapped, 2);
        EXPECT_LT(mapped, 5);
    }
    for (u32 i = 5; i < 10; i++)
        EXPECT_GE(Randomizer_WildRateSlot(&info, WILD_AREA_FISHING, SUPER_ROD, i), 5);
}

TEST("Special wild mapping honors slot route and global identities")
{
    enum Species a, b;

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_ENCOUNTER_MAPPING, ENCMAP_ROUTE_SPECIES);
    a = Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, 0x10, 0xCAFE);
    b = Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, 0x20, 0xCAFE);
    EXPECT_EQ(a, b);

    SetRulesetSetting(SETTING_ENCOUNTER_MAPPING, ENCMAP_GLOBAL);
    EXPECT_EQ(Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, 0x10, 0xCAFE),
              Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, 0x20, 0xBABE));

    SetRulesetSetting(SETTING_ENCOUNTER_MAPPING, ENCMAP_SLOT);
    a = Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, 0x10, 0xCAFE);
    for (u32 source = 0x11; source < 0x30; source++)
    {
        b = Randomizer_SpecialWildSpecies(SPECIES_FEEBAS, source, 0xCAFE);
        if (b != a)
            break;
    }
    EXPECT_NE(a, b);
}

TEST("Gift IV modes are deterministic and independent of gift species randomization")
{
    struct PokemonTemplate vanilla = { .species = SPECIES_CASTFORM, .level = 25 };
    struct PokemonTemplate randomized = vanilla;

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_GIFT_IV_MODE, GIFTIV_NATURAL);
    for (u32 i = 0; i < NUM_STATS; i++)
    {
        vanilla.ivs[i] = USE_RANDOM_IVS;
        randomized.ivs[i] = USE_RANDOM_IVS;
    }

    SetRulesetSetting(SETTING_GIFT_RANDOMIZATION, FALSE);
    Randomizer_ApplyGiftTemplate(&vanilla, 0x12345678);
    SetRulesetSetting(SETTING_GIFT_RANDOMIZATION, TRUE);
    Randomizer_ApplyGiftTemplate(&randomized, 0x12345678);
    for (u32 i = 0; i < NUM_STATS; i++)
        EXPECT_EQ(vanilla.ivs[i], randomized.ivs[i]);

    randomized = (struct PokemonTemplate){ .species = SPECIES_CASTFORM, .level = 25,
        .ivs = { 0, 5, 10, 15, 20, 31 } };
    SetRulesetSetting(SETTING_GIFT_RANDOMIZATION, FALSE);
    SetRulesetSetting(SETTING_GIFT_IV_MODE, GIFTIV_CUSTOM_FLOOR);
    SetRulesetSetting(SETTING_GIFT_IV_FLOOR, 12);
    Randomizer_ApplyGiftTemplate(&randomized, 0x87654321);
    EXPECT_EQ(randomized.ivs[0], 12);
    EXPECT_EQ(randomized.ivs[1], 12);
    EXPECT_EQ(randomized.ivs[2], 12);
    EXPECT_EQ(randomized.ivs[3], 15);
    EXPECT_EQ(randomized.ivs[4], 20);
    EXPECT_EQ(randomized.ivs[5], 31);
}

TEST("Trainer party roles and level overrides are applied party-wide")
{
    struct TrainerMon ordinary[2] =
    {
        { .species = SPECIES_RAYQUAZA, .lvl = 5, .ability = ABILITY_AIR_LOCK,
          .moves = { MOVE_DRAGON_ASCENT, MOVE_FLY, MOVE_REST, MOVE_EXTREME_SPEED } },
        { .species = SPECIES_ZIGZAGOON, .lvl = 6, .ability = ABILITY_PICKUP,
          .moves = { MOVE_TACKLE, MOVE_GROWL, MOVE_NONE, MOVE_NONE } },
    };
    struct TrainerMon wallace[2] =
    {
        { .species = SPECIES_ZIGZAGOON, .lvl = 40 },
        { .species = SPECIES_POOCHYENA, .lvl = 41 },
    };
    u32 indices[2] = {0, 1};

    UseBalancedRandomizerRuleset();
    FlagClear(FLAG_IS_CHAMPION);
    Randomizer_ApplyTrainerParty(ordinary, indices, 2, TRAINER_ROXANNE_1, TRAINER_CLASS_LEADER);
    for (u32 i = 0; i < 2; i++)
    {
        EXPECT(!(IsSpeciesPremium(ordinary[i].species)));
        EXPECT_EQ(ordinary[i].lvl, 14);
        EXPECT_EQ(ordinary[i].ability, ABILITY_NONE);
        for (u32 move = 0; move < MAX_MON_MOVES; move++)
            EXPECT_EQ(ordinary[i].moves[move], MOVE_NONE);
    }

    Randomizer_ApplyTrainerParty(wallace, indices, 2, TRAINER_WALLACE, TRAINER_CLASS_CHAMPION);
    EXPECT_EQ(wallace[0].lvl, 63);
    EXPECT_EQ(wallace[1].lvl, 63);
    EXPECT((IsSpeciesPremium(wallace[0].species) || IsSpeciesPremium(wallace[1].species)));
}

TEST("Trainer party-size randomization is deterministic and bounded")
{
    bool8 seen[PARTY_SIZE + 1] = {0};

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_TRAINER_PARTY_SIZE_RANDOMIZATION, TRUE);
    for (u32 seed = 0; seed < 128; seed++)
    {
        u8 count;

        SetRunSeed(seed);
        count = Randomizer_GetTrainerPartySize(TRAINER_ROXANNE_1, PARTY_SIZE);
        EXPECT_GE(count, 1);
        EXPECT_LE(count, PARTY_SIZE);
        EXPECT_EQ(count, Randomizer_GetTrainerPartySize(TRAINER_ROXANNE_1, PARTY_SIZE));
        seen[count] = TRUE;
    }
    for (u32 count = 1; count <= PARTY_SIZE; count++)
        EXPECT((seen[count]));
}

TEST("Elite Four slots naturally allow both ordinary and Premium results")
{
    bool32 sawOrdinary = FALSE, sawPremium = FALSE;
    u32 index = 0;

    UseBalancedRandomizerRuleset();
    for (u32 seed = 0; seed < 256 && (!sawOrdinary || !sawPremium); seed++)
    {
        // Drake's high-power ordinary ace is a target for which strict power
        // matching contains both pseudo-legendary and Premium candidates.
        struct TrainerMon mon = { .species = SPECIES_SALAMENCE, .lvl = 50 };

        SetRunSeed(seed);
        Randomizer_ApplyTrainerParty(&mon, &index, 1, TRAINER_SIDNEY, TRAINER_CLASS_ELITE_FOUR);
        if (IsSpeciesPremium(mon.species))
            sawPremium = TRUE;
        else
            sawOrdinary = TRUE;
    }
    EXPECT((sawOrdinary));
    EXPECT((sawPremium));
}

TEST("Cap-relative trainer levels follow badge offsets and preserve postgame levels")
{
    struct TrainerMon mon = { .species = SPECIES_ZIGZAGOON, .lvl = 10 };
    u32 index = 0;

    UseBalancedRandomizerRuleset();
    SetRulesetSetting(SETTING_TRAINER_RANDOMIZATION, FALSE);
    SetRulesetSetting(SETTING_TRAINER_LEVEL_MODE, TRLEVEL_CAP_SCALED);
    for (u32 flag = FLAG_BADGE01_GET; flag <= FLAG_BADGE08_GET; flag++)
        FlagClear(flag);
    FlagClear(FLAG_IS_CHAMPION);

    Randomizer_ApplyTrainerParty(&mon, &index, 1, 1, TRAINER_CLASS_YOUNGSTER);
    EXPECT_EQ(mon.lvl, 9); // cap 14 - (upcoming vanilla ace 15 - authored 10)

    FlagSet(FLAG_BADGE01_GET);
    mon.lvl = 10;
    Randomizer_ApplyTrainerParty(&mon, &index, 1, 1, TRAINER_CLASS_YOUNGSTER);
    EXPECT_EQ(mon.lvl, 12); // cap 21 - (upcoming vanilla ace 19 - authored 10)

    FlagSet(FLAG_IS_CHAMPION);
    mon.lvl = 10;
    Randomizer_ApplyTrainerParty(&mon, &index, 1, 1, TRAINER_CLASS_YOUNGSTER);
    EXPECT_EQ(mon.lvl, 10);

    FlagClear(FLAG_IS_CHAMPION);
    mon.lvl = 44;
    Randomizer_ApplyTrainerParty(&mon, &index, 1, TRAINER_ROXANNE_2, TRAINER_CLASS_LEADER);
    EXPECT_EQ(mon.lvl, 44);
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
