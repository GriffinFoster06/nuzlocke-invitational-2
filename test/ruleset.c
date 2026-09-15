#include "global.h"
#include "config_changes.h"
#include "nuzlocke.h"
#include "ruleset.h"
#include "test/test.h"
#include "constants/battle_ai.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// Declared directly (rather than including battle_ai_main.h) since that header's other
// prototypes pull in battle-internal struct definitions this file has no other need for.
extern u64 GetRulesetAiFlags(void); // docs/SPEC.md "AI difficulty settings"

static u8 ExpectedPresetValue(u32 preset, u32 settingId)
{
    u8 value = GetSettingDescriptor(settingId)->defaultValue;

    if (settingId == SETTING_PRESET)
        return preset;

    if (preset == RULESET_PRESET_MODERN_EMERALD)
    {
        switch (settingId)
        {
        case SETTING_WILD_RANDOMIZATION:
        case SETTING_STARTER_RANDOMIZATION:
        case SETTING_GIFT_RANDOMIZATION:
        case SETTING_STATIC_RANDOMIZATION:
        case SETTING_LEGENDARY_RANDOMIZATION:
        case SETTING_TRAINER_RANDOMIZATION:
        case SETTING_MOVE_RANDOMIZATION:
        case SETTING_TM_RANDOMIZATION:
        case SETTING_TUTOR_RANDOMIZATION:
        case SETTING_ABILITY_RANDOMIZATION:
        case SETTING_FIELD_ITEM_RANDOMIZATION:
        case SETTING_HIDDEN_ITEM_RANDOMIZATION:
        case SETTING_GIFT_ITEM_RANDOMIZATION:
        case SETTING_UNLIMITED_MONEY:
        case SETTING_BALL_NPC_999:
        case SETTING_OVER_CAP_INELIGIBLE:
        case SETTING_LEVEL_TO_CAP:
        case SETTING_PERMADEATH:
        case SETTING_GRAVEYARD_BOX:
        case SETTING_ONE_ENCOUNTER_PER_LOCATION:
        case SETTING_DUPES_CLAUSE:
        case SETTING_SHINY_CLAUSE:
        case SETTING_NO_BATTLE_ITEMS:
        case SETTING_FORCE_SET_BATTLE_STYLE:
            return 0;
        case SETTING_STARTER_IV_MODE:       return IVMODE_NATURAL;
        case SETTING_GIFT_IV_MODE:          return GIFTIV_NATURAL;
        case SETTING_TRAINER_LEVEL_MODE:    return TRLEVEL_VANILLA;
        case SETTING_MOVE_REMINDER_MODE:    return MVREMIND_NORMAL;
        case SETTING_CATCH_RATE:            return CATCHRATE_VANILLA;
        case SETTING_CAP_MODE:              return CAPMODE_OFF;
        case SETTING_WHITEOUT_BEHAVIOR:     return WHITEOUT_VANILLA;
        default:                            return value;
        }
    }

    if (preset == RULESET_PRESET_RANDOMIZER)
    {
        switch (settingId)
        {
        case SETTING_OVER_CAP_INELIGIBLE:
        case SETTING_LEVEL_TO_CAP:
        case SETTING_PERMADEATH:
        case SETTING_GRAVEYARD_BOX:
        case SETTING_ONE_ENCOUNTER_PER_LOCATION:
        case SETTING_DUPES_CLAUSE:
        case SETTING_NO_BATTLE_ITEMS:
        case SETTING_FORCE_SET_BATTLE_STYLE:
            return 0;
        case SETTING_CATCH_RATE:             return CATCHRATE_LARGE;
        case SETTING_CAP_MODE:               return CAPMODE_OFF;
        case SETTING_ENCOUNTER_CONSUMED_MODE:return ENCCONSUMED_ON_CATCH_ONLY;
        case SETTING_WHITEOUT_BEHAVIOR:      return WHITEOUT_VANILLA;
        default:                             return value;
        }
    }

    return value;
}

TEST("Each named ruleset preset expands to its exact setting table")
{
    EXPECT_EQ(RULESET_NAMED_PRESET_COUNT, 3);
    EXPECT((RulesetTablesAreValid()));

    for (u32 preset = 0; preset < RULESET_NAMED_PRESET_COUNT; preset++)
    {
        EXPECT((ApplyRulesetPreset(preset)));
        EXPECT_EQ(GetDisplayedRulesetPreset(), preset);
        for (u32 settingId = 0; settingId < NUM_SETTINGS; settingId++)
            EXPECT_EQ(GetRulesetSetting(settingId), ExpectedPresetValue(preset, settingId));
    }
}

TEST("Recommended is the versioned default and uses weighted learnsets")
{
    ResetRulesetSettings();

    EXPECT_EQ(GetDisplayedRulesetPreset(), RULESET_PRESET_RECOMMENDED);
    EXPECT_EQ(GetSavedRulesetVersion(), RULESET_VERSION);
    EXPECT_EQ(GetSavedRandomizerVersion(), RANDOMIZER_VERSION);
    EXPECT((gSaveBlock3Ptr->ruleset.seedInitialized));
    EXPECT_EQ(GetRulesetSetting(SETTING_LEARNSET_COMPOSITION), LRNCOMP_WEIGHTED);
    EXPECT_EQ(GetRulesetSetting(SETTING_TRAINER_LEVEL_MODE), TRLEVEL_CAP_SCALED);
}

TEST("Individual and category edits stay Custom until an explicit full restore")
{
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_MODERN_EMERALD)));
    EXPECT((SetRulesetSetting(SETTING_WILD_RANDOMIZATION, TRUE)));
    EXPECT_EQ(GetDisplayedRulesetPreset(), RULESET_PRESET_CUSTOM);

    // Returning the edited byte to its named value does not silently relabel.
    EXPECT((SetRulesetSetting(SETTING_WILD_RANDOMIZATION, FALSE)));
    EXPECT_EQ(GetDisplayedRulesetPreset(), RULESET_PRESET_CUSTOM);
    EXPECT((RestoreRulesetCategory(SETTING_CAT_WILD)));
    EXPECT_EQ(GetDisplayedRulesetPreset(), RULESET_PRESET_CUSTOM);

    EXPECT((RestoreRulesetAll()));
    EXPECT_EQ(GetDisplayedRulesetPreset(), RULESET_PRESET_MODERN_EMERALD);
}

TEST("Generation locks cover direct, preset, reset, seed, and ban mutations")
{
    u32 seed;

    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    EXPECT((Ruleset_SetSpeciesBanned(SPECIES_ZIGZAGOON, TRUE)));
    seed = GetRunSeed();
    SetRulesetRunStarted(TRUE);

    EXPECT(!(SetRulesetSetting(SETTING_WILD_RANDOMIZATION, FALSE)));
    EXPECT(!(ApplyRulesetPreset(RULESET_PRESET_MODERN_EMERALD)));
    EXPECT(!(RestoreRulesetCategory(SETTING_CAT_WILD)));
    EXPECT(!(RestoreRulesetAll()));
    EXPECT(!(SetRunSeed(seed ^ 1)));
    EXPECT(!(RerollRunSeed()));
    EXPECT(!(Ruleset_SetSpeciesBanned(SPECIES_ZIGZAGOON, FALSE)));
    EXPECT(!(Ruleset_ClearSpeciesBans()));
    EXPECT_EQ(GetRunSeed(), seed);
    EXPECT((Ruleset_IsSpeciesBanned(SPECIES_ZIGZAGOON)));

    // Unprotected QoL/display rows remain individually editable.
    EXPECT((SetRulesetSetting(SETTING_QUICK_TRAVEL, FALSE)));
    EXPECT((SetRulesetSetting(SETTING_SHOW_IVS, FALSE)));
    EXPECT((RestoreRulesetCategory(SETTING_CAT_TRAVERSAL)));
}

TEST("Strict lifecycle only locks rules for a core challenge configuration")
{
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_MODERN_EMERALD)));
    Nuzlocke_BeginRun();
    EXPECT(!(Nuzlocke_RunIsActive()));
    EXPECT((SetRulesetSetting(SETTING_CATCH_RATE, CATCHRATE_MODERATE)));

    ResetRulesetSettings();
    Nuzlocke_BeginRun();
    EXPECT((Nuzlocke_RunIsActive()));
    EXPECT(!(SetRulesetSetting(SETTING_PERMADEATH, FALSE)));
    EXPECT(!(RestoreRulesetCategory(SETTING_CAT_NUZLOCKE)));
    EXPECT(!(ApplyRulesetPreset(RULESET_PRESET_RANDOMIZER)));
    EXPECT((SetRulesetSetting(SETTING_SHOW_EVS, FALSE)));
}

TEST("Manual seed zero survives a ruleset-store round trip")
{
    struct RulesetSettings saved;

    EXPECT((SetRulesetSetting(SETTING_SEED_MODE, SEEDMODE_MANUAL)));
    EXPECT((SetRunSeed(0)));
    saved = gSaveBlock3Ptr->ruleset;
    memset(&gSaveBlock3Ptr->ruleset, 0, sizeof(gSaveBlock3Ptr->ruleset));
    gSaveBlock3Ptr->ruleset = saved;

    EXPECT_EQ(GetRunSeed(), 0);
    EXPECT((gSaveBlock3Ptr->ruleset.seedInitialized));
    EXPECT_EQ(GetRulesetSetting(SETTING_SEED_MODE), SEEDMODE_MANUAL);
    EXPECT_EQ(GetSavedRulesetVersion(), RULESET_VERSION);
    EXPECT_EQ(GetSavedRandomizerVersion(), RANDOMIZER_VERSION);
}

TEST("Deprecated Phase 11A controls retain IDs but are hidden")
{
    EXPECT((GetSettingDescriptor(SETTING_RETRY_SAME_SEED)->flags & SETTING_FLAG_HIDDEN));
    EXPECT((GetSettingDescriptor(SETTING_ENCOUNTER_LEVEL_MODE)->flags & SETTING_FLAG_HIDDEN));
    EXPECT((GetSettingDescriptor(SETTING_ALLOW_DUPLICATE_PREMIUM)->flags & SETTING_FLAG_HIDDEN));
    EXPECT((GetSettingDescriptor(SETTING_HOF_SPECIES_EXCLUSION)->flags & SETTING_FLAG_HIDDEN));
    EXPECT((GetSettingDescriptor(SETTING_FINAL_TEAM_LOCK)->flags & SETTING_FLAG_HIDDEN));
    EXPECT_EQ(GetSettingDescriptor(SETTING_TRAINER_LEVEL_MODE)->maxValue, TRLEVEL_VANILLA);
}

// docs/SPEC.md "AI difficulty settings" / "Maximum-strength fair AI". Standard (AIDIFF_VANILLA
// internally, no save/API churn from the player-facing rename) must add nothing on top of a
// trainer's authored AI flags, so it reproduces original Emerald trainer AI behavior; every
// higher tier must never author a hidden-information flag itself.
TEST("AI Difficulty Standard adds no flags; no tier authors hidden-information flags")
{
    const u64 hiddenInfoFlags = AI_FLAG_OMNISCIENT | AI_FLAG_ABILITY_OMNISCIENCE
                               | AI_FLAG_ITEM_OMNISCIENCE | AI_FLAG_MOVE_OMNISCIENCE
                               | AI_FLAG_KNOW_OPPONENT_PARTY;

    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_VANILLA)));
    EXPECT(GetRulesetAiFlags() == 0);

    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_IMPROVED)));
    EXPECT((GetRulesetAiFlags() & hiddenInfoFlags) == 0);

    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_EXPERT)));
    EXPECT((GetRulesetAiFlags() & hiddenInfoFlags) == 0);

    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_PRO_FAIR)));
    EXPECT((GetRulesetAiFlags() & hiddenInfoFlags) == 0);
}

// Expert keeps AI_FLAG_RANDOMIZE_SWITCHIN for variety; Pro Fair (the "strongest fair-information
// reasoning available" tier, spec) drops it so switch-in selection deterministically picks the
// best qualifying candidate GetBestMonIntegrated tracked, rather than a random one from the tier.
TEST("AI Difficulty Pro Fair keeps Expert's smart switching but drops switch-in randomization")
{
    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_EXPERT)));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_SMART_SWITCHING));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_SMART_MON_CHOICES));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_RANDOMIZE_SWITCHIN));

    EXPECT((SetRulesetSetting(SETTING_AI_DIFFICULTY, AIDIFF_PRO_FAIR)));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_SMART_SWITCHING));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_SMART_MON_CHOICES));
    EXPECT((GetRulesetAiFlags() & AI_FLAG_PREDICT_MOVE));
    EXPECT(!(GetRulesetAiFlags() & AI_FLAG_RANDOMIZE_SWITCHIN));
}

// Regression coverage for the A -> B -> A voluntary-switch oscillation: with this OFF,
// ShouldSwitchIfAllScoresBad (src/battle_ai_switch.c) could fire with no party mon having
// cleared canSwitchinWin1v1, falling back to a blind last-in-party-order switch every time
// all of the active mon's move scores are bad - producing exactly that oscillation. ON keeps
// it consistent with every other voluntary-switch trigger, which already require a candidate
// that cleared canSwitchinWin1v1 (mostSuitableMonId != PARTY_SIZE) before giving up the turn.
TEST("AI all-scores-bad switch requires a real switch-in by project default")
{
    EXPECT((GetConfig(ALL_SCORES_BAD_NEEDS_GOOD_SWITCHIN)));
}
