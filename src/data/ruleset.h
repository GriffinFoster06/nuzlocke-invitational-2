// ============================================================================
// Nuzlocke-Randomizer ruleset data tables.
//
// Included once, by src/ruleset.c. Phase 1 scaffolding: this is the data model
// only - no gameplay code consumes these values yet.
//
//  * sSettingDescriptors  - one row per SettingId, in enum order. defaultValue
//                           is the Invitational 2 Solo value.
//  * sPresetOverrides_*    - sparse "differs from Invitational 2 Solo" lists for
//                           each other named preset.
// ============================================================================

// ---------------------------------------------------------------------------
// Option-label tables (indexed by the ENUM value constants in
// include/constants/ruleset.h)
// ---------------------------------------------------------------------------
static const u8 *const sLbl_OffOn[] = { COMPOUND_STRING("Off"), COMPOUND_STRING("On") };

static const u8 *const sLbl_Preset[] =
{
    COMPOUND_STRING("Inv. 2 Solo"),
    COMPOUND_STRING("Inv. 2 Tourney"),
    COMPOUND_STRING("Randolocke"),
    COMPOUND_STRING("Modern Emerald"),
    COMPOUND_STRING("Randomizer"),
    COMPOUND_STRING("Custom"),
};

static const u8 *const sLbl_SeedMode[]   = { COMPOUND_STRING("Random"), COMPOUND_STRING("Manual") };
static const u8 *const sLbl_PremiumPool[] =
{
    COMPOUND_STRING("Curated high-power"),
    COMPOUND_STRING("All legendaries"),
    COMPOUND_STRING("Same as normal"),
};
static const u8 *const sLbl_PowerMatch[] =
{
    COMPOUND_STRING("Strict balanced"),
    COMPOUND_STRING("Normal balanced"),
    COMPOUND_STRING("Loose balanced"),
    COMPOUND_STRING("BST only"),
    COMPOUND_STRING("Unrestricted chaos"),
};
static const u8 *const sLbl_EvoStage[] =
{
    COMPOUND_STRING("Off"),
    COMPOUND_STRING("Prefer same stage"),
    COMPOUND_STRING("Strict same stage"),
};
static const u8 *const sLbl_EncMap[] =
{
    COMPOUND_STRING("Encounter slot"),
    COMPOUND_STRING("Route species"),
    COMPOUND_STRING("Global species"),
};
static const u8 *const sLbl_EncLvl[]  = { COMPOUND_STRING("Progression"), COMPOUND_STRING("Vanilla") };
static const u8 *const sLbl_IvMode[]  =
{
    COMPOUND_STRING("5 perfect + 1 random"),
    COMPOUND_STRING("3 perfect"),
    COMPOUND_STRING("Natural"),
    COMPOUND_STRING("All 31"),
    COMPOUND_STRING("Custom floor"),
};
static const u8 *const sLbl_GiftIv[] =
{
    COMPOUND_STRING("3 perfect"),
    COMPOUND_STRING("Natural"),
    COMPOUND_STRING("All 31"),
    COMPOUND_STRING("Custom floor"),
};
static const u8 *const sLbl_TrLevel[] =
{
    COMPOUND_STRING("Cap-scaled"),
    COMPOUND_STRING("Vanilla"),
    COMPOUND_STRING("Flat offset"),
};
static const u8 *const sLbl_BossMatch[] = { COMPOUND_STRING("Same as route"), COMPOUND_STRING("Stricter") };
static const u8 *const sLbl_LrnComp[] =
{
    COMPOUND_STRING("7 / 7 / 7"),
    COMPOUND_STRING("Fully random"),
    // LRNCOMP_WEIGHTED is deferred - see include/constants/ruleset.h. No label
    // because the descriptor's maxValue keeps it out of the menu.
};
static const u8 *const sLbl_MoveOrder[] = { COMPOUND_STRING("Higher power later"), COMPOUND_STRING("Fully random") };
static const u8 *const sLbl_MoveReminder[] =
{
    COMPOUND_STRING("Disabled"),
    COMPOUND_STRING("Normal Emerald"),
    COMPOUND_STRING("Free"),
    COMPOUND_STRING("Learned moves only"),
};
static const u8 *const sLbl_TmCompat[] = { COMPOUND_STRING("Universal"), COMPOUND_STRING("Vanilla") };
static const u8 *const sLbl_WonderGuard[] = { COMPOUND_STRING("Shedinja only"), COMPOUND_STRING("Randomized") };
static const u8 *const sLbl_CatchRate[] =
{
    COMPOUND_STRING("Vanilla"),
    COMPOUND_STRING("Moderate boost"),
    COMPOUND_STRING("Large boost"),
    COMPOUND_STRING("Guaranteed"),
};
static const u8 *const sLbl_CapMode[] =
{
    COMPOUND_STRING("Hard"),
    COMPOUND_STRING("Soft"),
    COMPOUND_STRING("Warning only"),
    COMPOUND_STRING("Off"),
};
static const u8 *const sLbl_Breakpoint[] =
{
    COMPOUND_STRING("Off"),
    COMPOUND_STRING("Next move"),
    COMPOUND_STRING("Next evolution"),
    COMPOUND_STRING("Current cap"),
};
static const u8 *const sLbl_EncConsumed[] = { COMPOUND_STRING("Strict"), COMPOUND_STRING("On catch only") };
static const u8 *const sLbl_Nickname[] =
{
    COMPOUND_STRING("Optional"),
    COMPOUND_STRING("Mandatory"),
    COMPOUND_STRING("Strict"),
};
static const u8 *const sLbl_Whiteout[] = { COMPOUND_STRING("Run over"), COMPOUND_STRING("Vanilla") };
static const u8 *const sLbl_ShinyOdds[] =
{
    COMPOUND_STRING("Normal"),
    COMPOUND_STRING("Boosted"),
    COMPOUND_STRING("Disabled"),
};
static const u8 *const sLbl_AiDifficulty[] =
{
    COMPOUND_STRING("Vanilla"),
    COMPOUND_STRING("Improved"),
    COMPOUND_STRING("Expert"),
    COMPOUND_STRING("Pro Fair"),
};
static const u8 *const sLbl_BattleSpeed[] =
{
    COMPOUND_STRING("Normal"),
    COMPOUND_STRING("Fast"),
    COMPOUND_STRING("Instant"),
};

// ---------------------------------------------------------------------------
// Category names (indexed by enum SettingCategory)
// ---------------------------------------------------------------------------
static const u8 *const sSettingCategoryNames[SETTING_CAT_COUNT] =
{
    [SETTING_CAT_PRESET_SEED]  = COMPOUND_STRING("Preset & Seed"),
    [SETTING_CAT_SPECIES_POOL] = COMPOUND_STRING("Species Pool"),
    [SETTING_CAT_WILD]         = COMPOUND_STRING("Wild & Encounters"),
    [SETTING_CAT_STARTERS]     = COMPOUND_STRING("Starters, Gifts & Statics"),
    [SETTING_CAT_TRAINERS]     = COMPOUND_STRING("Trainers"),
    [SETTING_CAT_LEARNSETS]    = COMPOUND_STRING("Learnsets & Moves"),
    [SETTING_CAT_TMS]          = COMPOUND_STRING("TMs & Tutors"),
    [SETTING_CAT_ABILITIES]    = COMPOUND_STRING("Abilities"),
    [SETTING_CAT_ITEMS]        = COMPOUND_STRING("Items & Economy"),
    [SETTING_CAT_CAPS]         = COMPOUND_STRING("Level Caps"),
    [SETTING_CAT_NUZLOCKE]     = COMPOUND_STRING("Nuzlocke Rules"),
    [SETTING_CAT_BATTLE_AI]    = COMPOUND_STRING("Battle & AI"),
    [SETTING_CAT_TRAVERSAL]    = COMPOUND_STRING("Traversal & QoL"),
    [SETTING_CAT_DISPLAY]      = COMPOUND_STRING("Display & Records"),
};

// ---------------------------------------------------------------------------
// Descriptor table
// ---------------------------------------------------------------------------
#define DESC_BOOL(cat, lock, def, flags, name, desc) \
    { COMPOUND_STRING(name), COMPOUND_STRING(desc), sLbl_OffOn, (cat), (lock), SETTING_TYPE_BOOL, 0, 1, (def), (flags) }
#define DESC_ENUM(cat, lock, def, flags, labels, maxv, name, desc) \
    { COMPOUND_STRING(name), COMPOUND_STRING(desc), (labels), (cat), (lock), SETTING_TYPE_ENUM, 0, (maxv), (def), (flags) }
#define DESC_NUM(cat, lock, minv, maxv, def, flags, name, desc) \
    { COMPOUND_STRING(name), COMPOUND_STRING(desc), NULL, (cat), (lock), SETTING_TYPE_NUMBER, (minv), (maxv), (def), (flags) }

#define GEN  SETTING_LOCK_GENERATION
#define RUL  SETTING_LOCK_RULES
#define FREE SETTING_LOCK_NONE

static const struct SettingDescriptor sSettingDescriptors[NUM_SETTINGS] =
{
    // -- Preset & Seed --
    [SETTING_PRESET] = DESC_ENUM(SETTING_CAT_PRESET_SEED, FREE, RULESET_DEFAULT_PRESET, SETTING_FLAG_NOT_RULESET,
        sLbl_Preset, RULESET_PRESET_COUNT - 1, "Ruleset Preset",
        "Load a full ruleset. Editing any setting shows Custom."),
    [SETTING_SEED_MODE] = DESC_ENUM(SETTING_CAT_PRESET_SEED, GEN, SEEDMODE_RANDOM, SETTING_FLAG_NONE,
        sLbl_SeedMode, 1, "Seed Source",
        "Random: new seed each run. Manual: enter your own."),
    [SETTING_RUN_SEED] = DESC_NUM(SETTING_CAT_PRESET_SEED, GEN, 0, 0, 0, SETTING_FLAG_NOT_RULESET | SETTING_FLAG_READ_ONLY,
        "Run Seed",
        "This run's seed. Press A to reroll (before the run)."),
    [SETTING_RETRY_SAME_SEED] = DESC_BOOL(SETTING_CAT_PRESET_SEED, GEN, 0, SETTING_FLAG_NONE,
        "Retry Same Seed",
        "Reuse this seed on the next attempt instead of rerolling."),

    // -- Species Pool --
    [SETTING_ALLOW_LEGENDARY] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 0, SETTING_FLAG_NONE,
        "Allow Legendary",
        "Let Legendary species replace ordinary Pokemon."),
    [SETTING_ALLOW_MYTHICAL] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 0, SETTING_FLAG_NONE,
        "Allow Mythical",
        "Let Mythical species replace ordinary Pokemon."),
    [SETTING_ALLOW_SUB_LEGENDARY] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 0, SETTING_FLAG_NONE,
        "Allow Sub-Legendary",
        "Let Sub-Legendary species into the ordinary pool."),
    [SETTING_ALLOW_ULTRA_BEAST] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 0, SETTING_FLAG_NONE,
        "Allow Ultra Beast",
        "Let Ultra Beasts into the ordinary pool."),
    [SETTING_ALLOW_PARADOX] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 0, SETTING_FLAG_NONE,
        "Allow Paradox",
        "Let Paradox Pokemon into the ordinary pool."),
    [SETTING_ALLOW_REGIONAL_FORMS] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 1, SETTING_FLAG_NONE,
        "Allow Regional Forms",
        "Include Alolan/Galarian/Hisuian/Paldean forms."),
    [SETTING_ALLOW_OTHER_FORMS] = DESC_BOOL(SETTING_CAT_SPECIES_POOL, GEN, 1, SETTING_FLAG_NONE,
        "Allow Other Forms",
        "Include misc alternate forms with working battle data."),
    [SETTING_PREMIUM_POOL_MODE] = DESC_ENUM(SETTING_CAT_SPECIES_POOL, GEN, PREMPOOL_CURATED, SETTING_FLAG_NONE,
        sLbl_PremiumPool, 2, "Premium Pool",
        "Species used for legendary/static premium slots."),

    // -- Wild & Encounters --
    [SETTING_WILD_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_WILD, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Wild",
        "Give every wild encounter slot a randomized species."),
    [SETTING_POWER_MATCHING] = DESC_ENUM(SETTING_CAT_WILD, GEN, PWRMATCH_NORMAL, SETTING_FLAG_NONE,
        sLbl_PowerMatch, 4, "Power Matching",
        "How closely a replacement's power must match the original."),
    [SETTING_EVO_STAGE_MATCHING] = DESC_ENUM(SETTING_CAT_WILD, GEN, EVOSTAGE_PREFER, SETTING_FLAG_NONE,
        sLbl_EvoStage, 2, "Evo-Stage Matching",
        "Match unevolved/middle/final evolutionary stage."),
    [SETTING_ENCOUNTER_MAPPING] = DESC_ENUM(SETTING_CAT_WILD, GEN, ENCMAP_SLOT, SETTING_FLAG_NONE,
        sLbl_EncMap, 2, "Encounter Mapping",
        "Granularity of the persistent encounter replacement map."),
    [SETTING_ENCOUNTER_RATE_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_WILD, GEN, 0, SETTING_FLAG_NONE,
        "Randomize Enc. Rates",
        "Also randomize each slot's encounter rate."),
    [SETTING_ENCOUNTER_LEVEL_MODE] = DESC_ENUM(SETTING_CAT_WILD, GEN, ENCLVL_PROGRESSION, SETTING_FLAG_NONE,
        sLbl_EncLvl, 1, "Encounter Levels",
        "Progression-appropriate levels, or the vanilla levels."),

    // -- Starters, Gifts & Statics --
    [SETTING_STARTER_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_STARTERS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Starters",
        "Randomize the three starter choices."),
    [SETTING_STARTER_IV_MODE] = DESC_ENUM(SETTING_CAT_STARTERS, GEN, IVMODE_5_PLUS_1, SETTING_FLAG_NONE,
        sLbl_IvMode, 4, "Starter IVs",
        "IV spread guaranteed on the chosen starter."),
    [SETTING_STARTER_IV_FLOOR] = DESC_NUM(SETTING_CAT_STARTERS, GEN, 0, 31, 31, SETTING_FLAG_NONE,
        "Starter IV Floor",
        "Minimum IV per stat when Starter IVs is Custom floor."),
    [SETTING_GIFT_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_STARTERS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Gifts",
        "Randomize gift Pokemon into power-appropriate species."),
    [SETTING_GIFT_IV_MODE] = DESC_ENUM(SETTING_CAT_STARTERS, GEN, GIFTIV_3_PERFECT, SETTING_FLAG_NONE,
        sLbl_GiftIv, 3, "Gift IVs",
        "IV spread guaranteed on ordinary gift Pokemon."),
    [SETTING_GIFT_IV_FLOOR] = DESC_NUM(SETTING_CAT_STARTERS, GEN, 0, 31, 31, SETTING_FLAG_NONE,
        "Gift IV Floor",
        "Minimum IV per stat when Gift IVs is Custom floor."),
    [SETTING_STATIC_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_STARTERS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Statics",
        "Randomize static encounters (fixed once per run)."),
    [SETTING_LEGENDARY_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_STARTERS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Legendaries",
        "Randomize legendary static slots from the premium pool."),
    [SETTING_ALLOW_DUPLICATE_PREMIUM] = DESC_BOOL(SETTING_CAT_STARTERS, GEN, 1, SETTING_FLAG_NONE,
        "Dup. Premium Allowed",
        "Allow the same premium species at more than one slot."),

    // -- Trainers --
    [SETTING_TRAINER_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_TRAINERS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Trainers",
        "Randomize every trainer's party (fixed for the run)."),
    [SETTING_TRAINER_PARTY_SIZE_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_TRAINERS, GEN, 0, SETTING_FLAG_NONE,
        "Randomize Party Size",
        "Vary party sizes instead of keeping the authored count."),
    [SETTING_BOSS_POWER_MATCHING] = DESC_ENUM(SETTING_CAT_TRAINERS, GEN, BOSSMATCH_STRICTER, SETTING_FLAG_NONE,
        sLbl_BossMatch, 1, "Boss Power Matching",
        "Use stricter power matching for Gym Leaders/E4/bosses."),
    [SETTING_TRAINER_LEVEL_MODE] = DESC_ENUM(SETTING_CAT_TRAINERS, GEN, TRLEVEL_CAP_SCALED, SETTING_FLAG_NONE,
        sLbl_TrLevel, 2, "Trainer Levels",
        "Scale trainer levels around the level-cap progression."),

    // -- Learnsets & Moves --
    [SETTING_MOVE_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_LEARNSETS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Learnsets",
        "Replace canonical learnsets with generated ones."),
    [SETTING_LEARNSET_SIZE] = DESC_NUM(SETTING_CAT_LEARNSETS, GEN, 4, 25, 21, SETTING_FLAG_NONE,
        "Learnset Size",
        "Randomized level-up moves per species."),
    [SETTING_LEARNSET_COMPOSITION] = DESC_ENUM(SETTING_CAT_LEARNSETS, GEN, LRNCOMP_777, SETTING_FLAG_NONE,
        sLbl_LrnComp, 1, "Learnset Mix",
        "7/7/7 STAB / coverage / status, or fully random."),
    [SETTING_MOVE_POWER_PROGRESSION] = DESC_ENUM(SETTING_CAT_LEARNSETS, GEN, MVORDER_WEIGHTED_LATE, SETTING_FLAG_NONE,
        sLbl_MoveOrder, 1, "Move Order",
        "Bias stronger attacks toward later levels, or full random."),
    [SETTING_BAN_OHKO_MOVES] = DESC_BOOL(SETTING_CAT_LEARNSETS, GEN, 1, SETTING_FLAG_NONE,
        "Ban OHKO Moves",
        "Exclude one-hit-KO moves from generated learnsets."),
    [SETTING_BAN_EVASION_MOVES] = DESC_BOOL(SETTING_CAT_LEARNSETS, GEN, 1, SETTING_FLAG_NONE,
        "Ban Evasion Moves",
        "Exclude evasion-raising moves from generated learnsets."),
    [SETTING_BAN_SLEEP_MOVES] = DESC_BOOL(SETTING_CAT_LEARNSETS, GEN, 1, SETTING_FLAG_NONE,
        "Ban Sleep Moves",
        "Exclude sleep-inducing moves from generated learnsets."),
    [SETTING_BAN_SELF_KO_MOVES] = DESC_BOOL(SETTING_CAT_LEARNSETS, GEN, 0, SETTING_FLAG_NONE,
        "Ban Self-KO Moves",
        "Exclude self-KO moves (Explosion, etc.) from learnsets."),
    [SETTING_MOVE_REMINDER_MODE] = DESC_ENUM(SETTING_CAT_LEARNSETS, RUL, MVREMIND_DISABLED, SETTING_FLAG_NONE,
        sLbl_MoveReminder, 3, "Move Reminder",
        "Availability of re-teaching forgotten level-up moves."),

    // -- TMs & Tutors --
    [SETTING_TM_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_TMS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize TMs",
        "Randomize TM move assignments at New Game."),
    [SETTING_TM_COMPATIBILITY] = DESC_ENUM(SETTING_CAT_TMS, GEN, TMCOMPAT_UNIVERSAL, SETTING_FLAG_NONE,
        sLbl_TmCompat, 1, "TM Compatibility",
        "Universal: every Pokemon can learn every TM."),
    [SETTING_TUTOR_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_TMS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Tutors",
        "Randomize every standard move tutor's move."),
    [SETTING_TUTOR_COMPATIBILITY] = DESC_ENUM(SETTING_CAT_TMS, GEN, TMCOMPAT_UNIVERSAL, SETTING_FLAG_NONE,
        sLbl_TmCompat, 1, "Tutor Compatibility",
        "Universal: every Pokemon can learn every tutor move."),
    [SETTING_ALLOW_DUPLICATE_TMS] = DESC_BOOL(SETTING_CAT_TMS, GEN, 1, SETTING_FLAG_NONE,
        "Duplicate TMs Allowed",
        "Allow the same move on more than one TM."),

    // -- Abilities --
    [SETTING_ABILITY_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_ABILITIES, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Abilities",
        "Randomize abilities; strong combos are allowed."),
    [SETTING_ABILITY_EVO_CONSISTENCY] = DESC_BOOL(SETTING_CAT_ABILITIES, GEN, 1, SETTING_FLAG_NONE,
        "Ability Evo Consistency",
        "Keep the family's randomized ability slot on evolution."),
    [SETTING_WONDER_GUARD_MODE] = DESC_ENUM(SETTING_CAT_ABILITIES, GEN, WGUARD_SHEDINJA_ONLY, SETTING_FLAG_NONE,
        sLbl_WonderGuard, 1, "Wonder Guard",
        "Keep Wonder Guard on Shedinja only, or fully randomize."),

    // -- Items & Economy --
    [SETTING_FIELD_ITEM_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_ITEMS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Field Items",
        "Randomize visible field item balls (fixed for the run)."),
    [SETTING_HIDDEN_ITEM_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_ITEMS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Hidden Items",
        "Randomize hidden field items too."),
    [SETTING_GIFT_ITEM_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_ITEMS, GEN, 1, SETTING_FLAG_NONE,
        "Randomize Gift Items",
        "Randomize given items (key items stay protected)."),
    [SETTING_SHOP_RANDOMIZATION] = DESC_BOOL(SETTING_CAT_ITEMS, GEN, 0, SETTING_FLAG_NONE,
        "Randomize Shops",
        "Randomize shop inventories."),
    [SETTING_UNLIMITED_MONEY] = DESC_BOOL(SETTING_CAT_ITEMS, FREE, 1, SETTING_FLAG_NONE,
        "Unlimited Money",
        "Purchases never meaningfully deplete your money."),
    [SETTING_BALL_NPC_999] = DESC_BOOL(SETTING_CAT_ITEMS, FREE, 1, SETTING_FLAG_NONE,
        "999 Poke Ball NPC",
        "An NPC that hands over 999 ordinary Poke Balls."),
    [SETTING_CATCH_RATE] = DESC_ENUM(SETTING_CAT_ITEMS, RUL, CATCHRATE_MODERATE, SETTING_FLAG_NONE,
        sLbl_CatchRate, 3, "Catch Rate",
        "Global catch-rate multiplier."),
    [SETTING_R_BUTTON_BALL_SHORTCUT] = DESC_BOOL(SETTING_CAT_ITEMS, FREE, 1, SETTING_FLAG_NONE,
        "R = Throw Ball",
        "Press R in a wild battle to throw a Ball you own."),

    // -- Level Caps --
    [SETTING_CAP_MODE] = DESC_ENUM(SETTING_CAT_CAPS, RUL, CAPMODE_HARD, SETTING_FLAG_NONE,
        sLbl_CapMode, 3, "Level Cap Mode",
        "How the per-badge level cap is enforced."),
    [SETTING_OVER_CAP_INELIGIBLE] = DESC_BOOL(SETTING_CAT_CAPS, RUL, 1, SETTING_FLAG_NONE,
        "Over-Cap Ineligible",
        "Pokemon above the cap can't battle until it rises."),
    [SETTING_LEVEL_TO_CAP] = DESC_BOOL(SETTING_CAT_CAPS, RUL, 1, SETTING_FLAG_NONE,
        "Level to Cap",
        "Party-menu command that jumps a Pokemon to the cap."),
    [SETTING_LEVEL_TO_BREAKPOINT] = DESC_ENUM(SETTING_CAT_CAPS, FREE, BREAKPT_OFF, SETTING_FLAG_NONE,
        sLbl_Breakpoint, 3, "Level to Breakpoint",
        "Optional: level only to the next move/evo/cap."),
    [SETTING_POST_CHAMPION_CAP] = DESC_NUM(SETTING_CAT_CAPS, RUL, 63, 100, 100, SETTING_FLAG_NONE,
        "Post-Champion Cap",
        "Level cap once the Elite Four/Champion is beaten."),

    // -- Nuzlocke Rules --
    [SETTING_PERMADEATH] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Permadeath",
        "A fainted Pokemon is permanently dead."),
    [SETTING_GRAVEYARD_BOX] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Graveyard Box",
        "Auto-move dead Pokemon to a dedicated PC box."),
    [SETTING_ONE_ENCOUNTER_PER_LOCATION] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "One Per Location",
        "Only the first valid encounter per location tag counts."),
    [SETTING_ENCOUNTER_CONSUMED_MODE] = DESC_ENUM(SETTING_CAT_NUZLOCKE, RUL, ENCCONSUMED_STRICT, SETTING_FLAG_NONE,
        sLbl_EncConsumed, 1, "Encounter Consumed",
        "Strict: killing/fleeing/failing also uses the location."),
    [SETTING_DUPES_CLAUSE] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Dupes Clause",
        "Reroll if the encounter's family is already caught."),
    [SETTING_DUPES_COUNT_FORMS] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Dupes: Forms Count",
        "Regional/other forms count as the same family."),
    [SETTING_DUPES_COUNT_DEAD] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Dupes: Dead Count",
        "Dead Pokemon still count as previously owned."),
    [SETTING_SHINY_CLAUSE] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Shiny Clause",
        "A shiny can always be caught, extra to the location."),
    [SETTING_SHINY_ODDS] = DESC_ENUM(SETTING_CAT_NUZLOCKE, RUL, SHINYODDS_NORMAL, SETTING_FLAG_NONE,
        sLbl_ShinyOdds, 2, "Shiny Odds",
        "Encounter shiny rate."),
    [SETTING_NICKNAME_MODE] = DESC_ENUM(SETTING_CAT_NUZLOCKE, RUL, NICK_OPTIONAL, SETTING_FLAG_NONE,
        sLbl_Nickname, 2, "Nicknames",
        "Optional / Mandatory / Strict: force naming every obtained Pokemon."),
    [SETTING_NO_BATTLE_ITEMS] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "No Battle Items",
        "Disable Bag combat items in trainer battles."),
    [SETTING_WHITEOUT_BEHAVIOR] = DESC_ENUM(SETTING_CAT_NUZLOCKE, RUL, WHITEOUT_RUN_OVER, SETTING_FLAG_NONE,
        sLbl_Whiteout, 1, "Whiteout",
        "Run over: a whiteout ends the attempt."),
    [SETTING_NEW_SEED_ON_RETRY] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "New Seed on Retry",
        "A fresh attempt generates a new world seed."),
    [SETTING_FORCE_SET_BATTLE_STYLE] = DESC_BOOL(SETTING_CAT_NUZLOCKE, RUL, 1, SETTING_FLAG_NONE,
        "Force Set Style",
        "No free switch after defeating an opposing Pokemon."),

    // -- Battle & AI --
    [SETTING_AI_DIFFICULTY] = DESC_ENUM(SETTING_CAT_BATTLE_AI, RUL, AIDIFF_PRO_FAIR, SETTING_FLAG_NONE,
        sLbl_AiDifficulty, 3, "AI Difficulty",
        "Trainer AI strength. No setting grants omniscience."),
    [SETTING_ALLOW_MEGA] = DESC_BOOL(SETTING_CAT_BATTLE_AI, GEN, 0, SETTING_FLAG_NONE,
        "Mega Evolution",
        "Enable Mega Evolution as a battle mechanic."),
    [SETTING_ALLOW_PRIMAL] = DESC_BOOL(SETTING_CAT_BATTLE_AI, GEN, 0, SETTING_FLAG_NONE,
        "Primal Reversion",
        "Enable Primal Reversion as a battle mechanic."),
    [SETTING_ALLOW_Z_MOVES] = DESC_BOOL(SETTING_CAT_BATTLE_AI, GEN, 0, SETTING_FLAG_NONE,
        "Z-Moves",
        "Enable Z-Moves as a battle mechanic."),
    [SETTING_ALLOW_DYNAMAX] = DESC_BOOL(SETTING_CAT_BATTLE_AI, GEN, 0, SETTING_FLAG_NONE,
        "Dynamax",
        "Enable Dynamax/Gigantamax as a battle mechanic."),
    [SETTING_ALLOW_TERASTAL] = DESC_BOOL(SETTING_CAT_BATTLE_AI, GEN, 0, SETTING_FLAG_NONE,
        "Terastallization",
        "Enable Terastallization as a battle mechanic."),
    [SETTING_BATTLE_SPEED] = DESC_ENUM(SETTING_CAT_BATTLE_AI, FREE, BATSPEED_FAST, SETTING_FLAG_NOT_RULESET,
        sLbl_BattleSpeed, 2, "Battle Speed",
        "Pacing of battle intros, HP bars and animations."),

    // -- Traversal & QoL --
    [SETTING_HM_FREE_TRAVERSAL] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "HM-Free Traversal",
        "Field moves work by badge/story progress, not HMs."),
    [SETTING_QUICK_TRAVEL] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Quick Travel",
        "Fast-travel to visited towns to skip backtracking."),
    [SETTING_PORTABLE_HEAL] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Portable Heal",
        "Menu command that heals HP/PP/status (not the dead)."),
    [SETTING_INFINITE_REPEL] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Infinite Repel",
        "Toggle random encounters off from the menu."),
    [SETTING_EVOLVE_COMMAND] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Evolve Command",
        "Player-controlled evolve option in the Pokemon menu."),
    [SETTING_SKIP_CLOCK_SET] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Skip Clock Setting",
        "Skip the Littleroot wall-clock setting sequence."),
    [SETTING_OLDALE_MONEY_NPC] = DESC_BOOL(SETTING_CAT_TRAVERSAL, FREE, 1, SETTING_FLAG_NONE,
        "Oldale Money NPC",
        "Keep the redundant Oldale money-refill NPC."),

    // -- Display & Records --
    [SETTING_SHOW_IVS] = DESC_BOOL(SETTING_CAT_DISPLAY, FREE, 1, SETTING_FLAG_NOT_RULESET,
        "Show IVs",
        "Show exact per-stat IVs in the summary screen."),
    [SETTING_SHOW_EVS] = DESC_BOOL(SETTING_CAT_DISPLAY, FREE, 1, SETTING_FLAG_NOT_RULESET,
        "Show EVs",
        "Show exact per-stat EVs in the summary screen."),
    [SETTING_SHOW_NATURE_EFFECT] = DESC_BOOL(SETTING_CAT_DISPLAY, FREE, 1, SETTING_FLAG_NOT_RULESET,
        "Show Nature Effect",
        "Mark the nature's boosted/reduced stat."),
    [SETTING_SHOW_CAP_LEGALITY] = DESC_BOOL(SETTING_CAT_DISPLAY, FREE, 1, SETTING_FLAG_NOT_RULESET,
        "Show Cap Legality",
        "Mark Pokemon that are over the current legal cap."),
    [SETTING_SHOW_DEAD_MARKER] = DESC_BOOL(SETTING_CAT_DISPLAY, FREE, 1, SETTING_FLAG_NOT_RULESET,
        "Show Dead Marker",
        "Clearly mark dead Pokemon in menus."),
    [SETTING_HOF_SPECIES_EXCLUSION] = DESC_BOOL(SETTING_CAT_DISPLAY, GEN, 0, SETTING_FLAG_NONE,
        "HoF Species Exclusion",
        "Tournament: winning species are banned from later runs."),
    [SETTING_FINAL_TEAM_LOCK] = DESC_BOOL(SETTING_CAT_DISPLAY, RUL, 0, SETTING_FLAG_NONE,
        "Final Team Lock",
        "Tournament: lock/export the final six for records."),
};

#undef DESC_BOOL
#undef DESC_ENUM
#undef DESC_NUM
#undef GEN
#undef RUL
#undef FREE

// ---------------------------------------------------------------------------
// Preset override tables. Each lists only what differs from the Invitational 2
// Solo defaults above. Keep entries grouped by category for readability.
// ---------------------------------------------------------------------------

// Invitational 2 Solo == the descriptor defaults, so it has no overrides.
static const struct RulesetPresetOverride sPresetOverrides_InvitationalSolo[] = { {0} };

// Invitational 2 Tournament: same rules + persistent Hall-of-Fame exclusions.
static const struct RulesetPresetOverride sPresetOverrides_InvitationalTournament[] =
{
    { SETTING_HOF_SPECIES_EXCLUSION, 1 },
    { SETTING_FINAL_TEAM_LOCK,       1 },
};

// Randolocke: its documented defaults, but keep our fair AI.
static const struct RulesetPresetOverride sPresetOverrides_Randolocke[] =
{
    { SETTING_PREMIUM_POOL_MODE,          PREMPOOL_SAME_AS_NORMAL },
    { SETTING_BOSS_POWER_MATCHING,        BOSSMATCH_SAME_AS_ROUTE },
    { SETTING_BAN_OHKO_MOVES,             0 },
    { SETTING_BAN_EVASION_MOVES,          0 },
    { SETTING_BAN_SLEEP_MOVES,            0 },
    { SETTING_MOVE_REMINDER_MODE,         MVREMIND_NORMAL },
    { SETTING_CATCH_RATE,                 CATCHRATE_LARGE },
    { SETTING_ENCOUNTER_CONSUMED_MODE,    ENCCONSUMED_ON_CATCH_ONLY },
    { SETTING_NICKNAME_MODE,              NICK_MANDATORY },
    { SETTING_NO_BATTLE_ITEMS,            0 },
    { SETTING_FORCE_SET_BATTLE_STYLE,     0 },
};

// Modern Emerald: vanilla species/trainers, modern engine + QoL kept.
static const struct RulesetPresetOverride sPresetOverrides_ModernEmerald[] =
{
    { SETTING_WILD_RANDOMIZATION,            0 },
    { SETTING_STARTER_RANDOMIZATION,         0 },
    { SETTING_STARTER_IV_MODE,               IVMODE_NATURAL },
    { SETTING_GIFT_RANDOMIZATION,            0 },
    { SETTING_GIFT_IV_MODE,                  GIFTIV_NATURAL },
    { SETTING_STATIC_RANDOMIZATION,          0 },
    { SETTING_LEGENDARY_RANDOMIZATION,       0 },
    { SETTING_TRAINER_RANDOMIZATION,         0 },
    { SETTING_MOVE_RANDOMIZATION,            0 },
    { SETTING_MOVE_REMINDER_MODE,            MVREMIND_NORMAL },
    { SETTING_TM_RANDOMIZATION,              0 },
    { SETTING_TUTOR_RANDOMIZATION,           0 },
    { SETTING_ABILITY_RANDOMIZATION,         0 },
    { SETTING_FIELD_ITEM_RANDOMIZATION,      0 },
    { SETTING_HIDDEN_ITEM_RANDOMIZATION,     0 },
    { SETTING_GIFT_ITEM_RANDOMIZATION,       0 },
    { SETTING_UNLIMITED_MONEY,               0 },
    { SETTING_BALL_NPC_999,                  0 },
    { SETTING_CATCH_RATE,                    CATCHRATE_VANILLA },
    { SETTING_CAP_MODE,                      CAPMODE_OFF },
    { SETTING_OVER_CAP_INELIGIBLE,           0 },
    { SETTING_LEVEL_TO_CAP,                  0 },
    { SETTING_PERMADEATH,                    0 },
    { SETTING_GRAVEYARD_BOX,                 0 },
    { SETTING_ONE_ENCOUNTER_PER_LOCATION,    0 },
    { SETTING_DUPES_CLAUSE,                  0 },
    { SETTING_SHINY_CLAUSE,                  0 },
    { SETTING_NO_BATTLE_ITEMS,               0 },
    { SETTING_WHITEOUT_BEHAVIOR,             WHITEOUT_VANILLA },
    { SETTING_FORCE_SET_BATTLE_STYLE,        0 },
};

// Randomizer: full randomization + QoL, no mandatory Nuzlocke restrictions.
static const struct RulesetPresetOverride sPresetOverrides_Randomizer[] =
{
    { SETTING_CATCH_RATE,                 CATCHRATE_LARGE },
    { SETTING_CAP_MODE,                   CAPMODE_OFF },
    { SETTING_OVER_CAP_INELIGIBLE,        0 },
    { SETTING_LEVEL_TO_CAP,               0 },
    { SETTING_PERMADEATH,                 0 },
    { SETTING_GRAVEYARD_BOX,              0 },
    { SETTING_ONE_ENCOUNTER_PER_LOCATION, 0 },
    { SETTING_ENCOUNTER_CONSUMED_MODE,    ENCCONSUMED_ON_CATCH_ONLY },
    { SETTING_DUPES_CLAUSE,               0 },
    { SETTING_NO_BATTLE_ITEMS,            0 },
    { SETTING_WHITEOUT_BEHAVIOR,          WHITEOUT_VANILLA },
    { SETTING_FORCE_SET_BATTLE_STYLE,     0 },
};

// Indexed by enum RulesetPreset (only the named presets; CUSTOM has none).
static const struct RulesetPresetOverride *const sPresetOverrideTables[RULESET_NAMED_PRESET_COUNT] =
{
    [RULESET_PRESET_INVITATIONAL_SOLO]       = sPresetOverrides_InvitationalSolo,
    [RULESET_PRESET_INVITATIONAL_TOURNAMENT] = sPresetOverrides_InvitationalTournament,
    [RULESET_PRESET_RANDOLOCKE]              = sPresetOverrides_Randolocke,
    [RULESET_PRESET_MODERN_EMERALD]          = sPresetOverrides_ModernEmerald,
    [RULESET_PRESET_RANDOMIZER]              = sPresetOverrides_Randomizer,
};

static const u8 sPresetOverrideCounts[RULESET_NAMED_PRESET_COUNT] =
{
    [RULESET_PRESET_INVITATIONAL_SOLO]       = 0,  // the {0} entry is a placeholder, not a real override
    [RULESET_PRESET_INVITATIONAL_TOURNAMENT] = ARRAY_COUNT(sPresetOverrides_InvitationalTournament),
    [RULESET_PRESET_RANDOLOCKE]              = ARRAY_COUNT(sPresetOverrides_Randolocke),
    [RULESET_PRESET_MODERN_EMERALD]          = ARRAY_COUNT(sPresetOverrides_ModernEmerald),
    [RULESET_PRESET_RANDOMIZER]              = ARRAY_COUNT(sPresetOverrides_Randomizer),
};
