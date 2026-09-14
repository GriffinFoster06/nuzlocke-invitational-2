#ifndef GUARD_CONSTANTS_RULESET_H
#define GUARD_CONSTANTS_RULESET_H

// ============================================================================
// Nuzlocke-Randomizer ruleset: data-model constants.
//
// This header defines the stable indices and values used by the persistent
// settings store, menu, and gameplay consumers. See docs/SPEC.md ("Settings
// behavior") and docs/PHASES.md.
// ============================================================================

// Ruleset format version, stored with the save so a seed stays reproducible
// across future ROM updates (docs/SPEC.md "Run seed"). Bump ONLY when the
// meaning of an existing stored value changes. Value 0 is reserved to mean
// "settings never initialized" - see RulesetSettings_EnsureInitialized().
//   v2: SETTING_EVOLUTION_ASSISTANCE removed mid-enum (Phase 9.5), shifting the
//       saved byte index of every later setting - old saves must re-init.
//   v3: Phase 11A finalizes presets, seed/version persistence and species bans.
//   v4: Phase 11B removes SETTING_DUPES_COUNT_FORMS and SETTING_DUPES_COUNT_DEAD
//       mid-enum (both were dead - evolutionary-family Dupes linking is now
//       unconditional and Dupes history is permanent), shifting the saved byte
//       index of every later setting - old saves must re-init. Also: any
//       RANDOMIZER_VERSION mismatch (not just RULESET_VERSION) now forces the
//       same full reinit, so a run's generated world can never straddle two
//       randomizer versions.
//   v5: Phase 11A.6 appends the nine SETTING_GEN_N_ENABLED rows (Gen 1-9
//       filter, all default ON) after NUM_SETTINGS's previous end - an
//       append, so this alone would not need a version bump, but
//       RANDOMIZER_VERSION's bump (below) already forces the same reinit.
#define RULESET_VERSION 5
// Phase 11A.6: PickReplacementCoreExcluding (src/randomizer.c) and
// GenerateWeighted (src/learnset_gen.c) moved from a two-pass count+pick
// selection to single-pass reservoir sampling - same selection
// distribution, different RNG draw sequence for a given seed. The new
// generation mask and evolution-mask filtering also change what a given
// seed generates. A save from RANDOMIZER_VERSION 2 must not mix old and new
// generation logic, so this bump forces the same full reinit as a
// RULESET_VERSION change.
#define RANDOMIZER_VERSION 3

// ----------------------------------------------------------------------------
// Presets
// ----------------------------------------------------------------------------
enum RulesetPreset
{
    RULESET_PRESET_RECOMMENDED,
    RULESET_PRESET_MODERN_EMERALD,
    RULESET_PRESET_RANDOMIZER,
    RULESET_PRESET_CUSTOM,
    RULESET_PRESET_COUNT,
};

// Presets that own an override table (everything except CUSTOM).
#define RULESET_NAMED_PRESET_COUNT RULESET_PRESET_CUSTOM

// The overall default preset.
#define RULESET_DEFAULT_PRESET RULESET_PRESET_RECOMMENDED

// ----------------------------------------------------------------------------
// Menu categories (one settings page each)
// ----------------------------------------------------------------------------
enum SettingCategory
{
    SETTING_CAT_PRESET_SEED,
    SETTING_CAT_GENERATIONS,
    SETTING_CAT_SPECIES_POOL,
    SETTING_CAT_WILD,
    SETTING_CAT_STARTERS,
    SETTING_CAT_TRAINERS,
    SETTING_CAT_LEARNSETS,
    SETTING_CAT_TMS,
    SETTING_CAT_ABILITIES,
    SETTING_CAT_ITEMS,
    SETTING_CAT_CAPS,
    SETTING_CAT_NUZLOCKE,
    SETTING_CAT_BATTLE_AI,
    SETTING_CAT_TRAVERSAL,
    SETTING_CAT_DISPLAY,
    SETTING_CAT_COUNT,
};

// ----------------------------------------------------------------------------
// Lock classes (docs/SPEC.md "Settings behavior")
// ----------------------------------------------------------------------------
enum SettingLock
{
    SETTING_LOCK_NONE,        // always editable (QoL / display)
    SETTING_LOCK_GENERATION,  // locked once the run has started
    SETTING_LOCK_RULES,       // locked while a strict run is active
};

// ----------------------------------------------------------------------------
// Value kinds
// ----------------------------------------------------------------------------
enum SettingType
{
    SETTING_TYPE_BOOL,    // 0 / 1, labelled Off / On
    SETTING_TYPE_ENUM,    // 0 .. maxValue, labelled from optionLabels[]
    SETTING_TYPE_NUMBER,  // raw minValue .. maxValue
};

// ----------------------------------------------------------------------------
// Descriptor flags
// ----------------------------------------------------------------------------
#define SETTING_FLAG_NONE        0
#define SETTING_FLAG_NOT_RULESET (1 << 0)  // ignored when matching the config to a preset
#define SETTING_FLAG_READ_ONLY   (1 << 1)  // displayed but never editable
#define SETTING_FLAG_HIDDEN      (1 << 2)  // retained storage id, omitted from player UI

// ----------------------------------------------------------------------------
// ENUM value constants (grouped; prefixes keep them collision-free)
// ----------------------------------------------------------------------------
enum { SEEDMODE_RANDOM, SEEDMODE_MANUAL };

enum { PREMPOOL_CURATED, PREMPOOL_ALL_LEGENDARY, PREMPOOL_SAME_AS_NORMAL };

enum { PWRMATCH_STRICT, PWRMATCH_NORMAL, PWRMATCH_LOOSE, PWRMATCH_BST_ONLY, PWRMATCH_UNRESTRICTED };
enum { EVOSTAGE_OFF, EVOSTAGE_PREFER, EVOSTAGE_STRICT };
enum { ENCMAP_SLOT, ENCMAP_ROUTE_SPECIES, ENCMAP_GLOBAL };
enum { ENCLVL_PROGRESSION, ENCLVL_VANILLA };

enum { IVMODE_5_PLUS_1, IVMODE_3_PERFECT, IVMODE_NATURAL, IVMODE_ALL_31, IVMODE_CUSTOM_FLOOR };
enum { GIFTIV_3_PERFECT, GIFTIV_NATURAL, GIFTIV_ALL_31, GIFTIV_CUSTOM_FLOOR };

enum { TRLEVEL_CAP_SCALED, TRLEVEL_VANILLA, TRLEVEL_FLAT_OFFSET };
enum { BOSSMATCH_SAME_AS_ROUTE, BOSSMATCH_STRICTER };

enum { LRNCOMP_777, LRNCOMP_FULLY_RANDOM, LRNCOMP_WEIGHTED };
enum { MVORDER_WEIGHTED_LATE, MVORDER_FULLY_RANDOM };
enum { MVREMIND_DISABLED, MVREMIND_NORMAL, MVREMIND_FREE, MVREMIND_LEARNED_ONLY };

enum { TMCOMPAT_UNIVERSAL, TMCOMPAT_VANILLA };

enum { WGUARD_SHEDINJA_ONLY, WGUARD_RANDOMIZED };

enum { CATCHRATE_VANILLA, CATCHRATE_MODERATE, CATCHRATE_LARGE, CATCHRATE_GUARANTEED };

enum { CAPMODE_HARD, CAPMODE_SOFT, CAPMODE_WARNING, CAPMODE_OFF };
enum { BREAKPT_OFF, BREAKPT_NEXT_MOVE, BREAKPT_NEXT_EVO, BREAKPT_CAP };

enum { ENCCONSUMED_STRICT, ENCCONSUMED_ON_CATCH_ONLY };
enum { NICK_OPTIONAL, NICK_MANDATORY, NICK_STRICT };
enum { WHITEOUT_RUN_OVER, WHITEOUT_VANILLA };
enum { SHINYODDS_NORMAL, SHINYODDS_BOOSTED, SHINYODDS_DISABLED };

enum { AIDIFF_VANILLA, AIDIFF_IMPROVED, AIDIFF_EXPERT, AIDIFF_PRO_FAIR };
enum { BATSPEED_NORMAL, BATSPEED_FAST, BATSPEED_INSTANT };

// ----------------------------------------------------------------------------
// Setting IDs. Order defines the index into RulesetSettings.values[] AND the
// order the descriptor table must be written in. Never renumber a shipped ID
// (it would silently reinterpret existing saves) - only append before
// NUM_SETTINGS.
// ----------------------------------------------------------------------------
enum SettingId
{
    // -- Preset & Seed --
    SETTING_PRESET,
    SETTING_SEED_MODE,
    SETTING_RUN_SEED,              // display/reroll only; the u32 lives in RulesetSettings.runSeed
    SETTING_RETRY_SAME_SEED,

    // -- Species Pool -- (generation-locked)
    SETTING_ALLOW_LEGENDARY,
    SETTING_ALLOW_MYTHICAL,
    SETTING_ALLOW_SUB_LEGENDARY,
    SETTING_ALLOW_ULTRA_BEAST,
    SETTING_ALLOW_PARADOX,
    SETTING_ALLOW_REGIONAL_FORMS,
    SETTING_ALLOW_OTHER_FORMS,
    SETTING_PREMIUM_POOL_MODE,

    // -- Wild & Encounters -- (generation-locked)
    SETTING_WILD_RANDOMIZATION,
    SETTING_POWER_MATCHING,
    SETTING_EVO_STAGE_MATCHING,
    SETTING_ENCOUNTER_MAPPING,
    SETTING_ENCOUNTER_RATE_RANDOMIZATION,
    SETTING_ENCOUNTER_LEVEL_MODE,

    // -- Starters, Gifts & Statics -- (generation-locked)
    SETTING_STARTER_RANDOMIZATION,
    SETTING_STARTER_IV_MODE,
    SETTING_STARTER_IV_FLOOR,
    SETTING_GIFT_RANDOMIZATION,
    SETTING_GIFT_IV_MODE,
    SETTING_GIFT_IV_FLOOR,
    SETTING_STATIC_RANDOMIZATION,
    SETTING_LEGENDARY_RANDOMIZATION,
    SETTING_ALLOW_DUPLICATE_PREMIUM,

    // -- Trainers -- (generation-locked)
    SETTING_TRAINER_RANDOMIZATION,
    SETTING_TRAINER_PARTY_SIZE_RANDOMIZATION,
    SETTING_BOSS_POWER_MATCHING,
    SETTING_TRAINER_LEVEL_MODE,

    // -- Learnsets & Moves -- (generation-locked, except move reminder)
    SETTING_MOVE_RANDOMIZATION,
    SETTING_LEARNSET_SIZE,
    SETTING_LEARNSET_COMPOSITION,
    SETTING_MOVE_POWER_PROGRESSION,
    SETTING_BAN_OHKO_MOVES,
    SETTING_BAN_EVASION_MOVES,
    SETTING_BAN_SLEEP_MOVES,
    SETTING_BAN_SELF_KO_MOVES,
    SETTING_MOVE_REMINDER_MODE,

    // -- TMs & Tutors -- (generation-locked)
    SETTING_TM_RANDOMIZATION,
    SETTING_TM_COMPATIBILITY,
    SETTING_TUTOR_RANDOMIZATION,
    SETTING_TUTOR_COMPATIBILITY,
    SETTING_ALLOW_DUPLICATE_TMS,

    // -- Abilities -- (generation-locked)
    SETTING_ABILITY_RANDOMIZATION,
    SETTING_ABILITY_EVO_CONSISTENCY,
    SETTING_WONDER_GUARD_MODE,

    // -- Items & Economy --
    SETTING_FIELD_ITEM_RANDOMIZATION,      // generation
    SETTING_HIDDEN_ITEM_RANDOMIZATION,     // generation
    SETTING_GIFT_ITEM_RANDOMIZATION,       // generation
    SETTING_SHOP_RANDOMIZATION,            // generation
    SETTING_UNLIMITED_MONEY,
    SETTING_BALL_NPC_999,
    SETTING_CATCH_RATE,                    // rules
    SETTING_R_BUTTON_BALL_SHORTCUT,

    // -- Level Caps -- (rules-locked)
    SETTING_CAP_MODE,
    SETTING_OVER_CAP_INELIGIBLE,
    SETTING_LEVEL_TO_CAP,
    SETTING_LEVEL_TO_BREAKPOINT,
    SETTING_POST_CHAMPION_CAP,

    // -- Nuzlocke Rules -- (rules-locked)
    SETTING_PERMADEATH,
    SETTING_GRAVEYARD_BOX,
    SETTING_ONE_ENCOUNTER_PER_LOCATION,
    SETTING_ENCOUNTER_CONSUMED_MODE,
    SETTING_DUPES_CLAUSE,
    SETTING_SHINY_CLAUSE,
    SETTING_SHINY_ODDS,
    SETTING_NICKNAME_MODE,
    SETTING_NO_BATTLE_ITEMS,
    SETTING_WHITEOUT_BEHAVIOR,
    SETTING_NEW_SEED_ON_RETRY,
    SETTING_FORCE_SET_BATTLE_STYLE,

    // -- Battle & AI --
    SETTING_AI_DIFFICULTY,                 // rules
    SETTING_ALLOW_MEGA,                    // generation
    SETTING_ALLOW_PRIMAL,                  // generation
    SETTING_ALLOW_Z_MOVES,                 // generation
    SETTING_ALLOW_DYNAMAX,                 // generation
    SETTING_ALLOW_TERASTAL,                // generation
    SETTING_BATTLE_SPEED,

    // -- Traversal & QoL --
    SETTING_HM_FREE_TRAVERSAL,
    SETTING_QUICK_TRAVEL,
    SETTING_PORTABLE_HEAL,
    SETTING_INFINITE_REPEL,
    SETTING_EVOLVE_COMMAND,
    SETTING_SKIP_CLOCK_SET,
    SETTING_OLDALE_MONEY_NPC,

    // -- Display & Records --
    SETTING_SHOW_IVS,
    SETTING_SHOW_EVS,
    SETTING_SHOW_NATURE_EFFECT,
    SETTING_SHOW_CAP_LEGALITY,
    SETTING_SHOW_DEAD_MARKER,
    SETTING_HOF_SPECIES_EXCLUSION,         // generation
    SETTING_FINAL_TEAM_LOCK,               // rules

    // Phase 11A virtual action rows. Existing stored ids above remain stable.
    SETTING_SPECIES_BANS,
    SETTING_RESTORE_ALL,

    // -- Generations -- (Phase 11A.6, generation-locked; append-only, ids
    // above remain stable)
    SETTING_GEN_1_ENABLED,
    SETTING_GEN_2_ENABLED,
    SETTING_GEN_3_ENABLED,
    SETTING_GEN_4_ENABLED,
    SETTING_GEN_5_ENABLED,
    SETTING_GEN_6_ENABLED,
    SETTING_GEN_7_ENABLED,
    SETTING_GEN_8_ENABLED,
    SETTING_GEN_9_ENABLED,

    NUM_SETTINGS,
};

#endif // GUARD_CONSTANTS_RULESET_H
