// Developer-only test-scenario fixture generator. See include/test_fixtures.h.
//
// Only compiled into the `make fixtures` build (-DTEST_FIXTURES=1). Never
// part of a normal or release ROM.
#include "global.h"
#include "test_fixtures.h"
#if TEST_FIXTURES

#include "caps.h"
#include "event_data.h"
#include "item.h"
#include "load_save.h"
#include "money.h"
#include "new_game.h"
#include "nuzlocke.h"
#include "pokemon.h"
#include "random.h"
#include "ruleset.h"
#include "save.h"
#include "script_pokemon_util.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "constants/moves.h"
#include "constants/pokemon.h" // NATURE_*
#include "constants/ruleset.h"
#include "constants/species.h"
#include "gba/flash_internal.h"
#include "gba/isagbprint.h"

// -----------------------------------------------------------------------
// Host -> ROM request: tools/generate_test_fixture patches the byte right
// after this 8-byte magic in a scratch copy of the fixtures ROM to select
// which scenario builder runs. volatile so the constant-index read below
// can't be folded away at compile time (which would let --gc-sections drop
// the whole array, and the host would never find the magic to patch).
// -----------------------------------------------------------------------
static const volatile u8 sFixtureRequest[9] __attribute__((used)) = "FIXTREQ\0"; // [8] = scenario id, host-patched

// -----------------------------------------------------------------------
// Shared scenario-construction helpers. All state is built with the
// project's normal helpers (ruleset/nuzlocke/save/pokemon), never raw save
// bytes.
// -----------------------------------------------------------------------

// Split in two so a scenario can layer SETTING_* overrides (generation
// mask, permadeath, ...) on top of the preset before NewGameInitData()
// commits them and locks generation-locked settings for the run (it calls
// SetRulesetRunStarted(TRUE) itself).
static void Fixture_BeginRuleset(u32 preset)
{
    RulesetSettings_MarkPreconfigured(); // NewGameInitData: don't stomp what we set below
    ResetRulesetSettings();
    ApplyRulesetPreset(preset);
    // ApplyRulesetPreset's lazy init auto-rolls a run seed folding in the
    // emulated RTC's current time, which is real-wall-clock-dependent and
    // would make every regeneration produce a different (but otherwise
    // valid) .sav. Force a fixed seed so fixtures are reproducible; a
    // scenario can still call SetRunSeed() again afterward to pick its own.
    SetRunSeed(20040919); // Emerald's JP release date, chosen only for stability
}

static void Fixture_CommitNewRun(void)
{
    NewGameInitData();
}

static void Fixture_SetLocation(s8 mapGroup, s8 mapNum, s16 x, s16 y)
{
    gSaveBlock1Ptr->location.mapGroup = mapGroup;
    gSaveBlock1Ptr->location.mapNum = mapNum;
    gSaveBlock1Ptr->location.warpId = -1;
    gSaveBlock1Ptr->location.x = x;
    gSaveBlock1Ptr->location.y = y;
    gSaveBlock1Ptr->continueGameWarp = gSaveBlock1Ptr->location;
    gSaveBlock1Ptr->pos.x = x;
    gSaveBlock1Ptr->pos.y = y;
}

static void Fixture_SetBadges(u32 count) // 0-8, sets FLAG_BADGE01_GET..count
{
    u32 i;
    for (i = 0; i < count && i < NUM_BADGES; i++)
        FlagSet(FLAG_BADGE01_GET + i);
}

static u32 Fixture_GiveMon(enum Species species, u8 level, u8 nature, enum Item item,
                            u16 move0, u16 move1, u16 move2, u16 move3)
{
    struct PokemonTemplate template = {0};
    template.species = species;
    template.level = level;
    template.nature = nature;
    template.heldItem = item;
    template.origin = GIFTMON_ORIGIN; // avoid the UNDEFINED_MON_ORIGIN debug-build assert (resumable crash screen, no input ever comes in a headless run)
    template.ivs[0] = template.ivs[1] = template.ivs[2] = 31;
    template.ivs[3] = template.ivs[4] = template.ivs[5] = 31;
    template.moves[0] = move0;
    template.moves[1] = move1;
    template.moves[2] = move2;
    template.moves[3] = move3;
    template.ball = ITEM_POKE_BALL;
    return ScriptGiveMonParameterized(B_SIDE_PLAYER, PARTY_SIZE, &template);
}

// -----------------------------------------------------------------------
// Scenario builders
// -----------------------------------------------------------------------

// ai_switch_oscillation: player leads a mon disadvantaged against the
// fixture trainer's lead (FIXTURE_TRAINER_AI_SWITCH, src/data/fixture_trainers.party)
// but that trainer's B/C mons are clearly better/worse alternatives.
// Load, R+START -> Test Fixtures -> AI Switch Oscillation to start the battle.
static void Scenario_AiSwitchOscillation(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RECOMMENDED);
    Fixture_CommitNewRun();
    Fixture_SetBadges(4);
    Fixture_GiveMon(SPECIES_SWAMPERT, 50, NATURE_ADAMANT, ITEM_LEFTOVERS,
                     MOVE_SURF, MOVE_EARTHQUAKE, MOVE_ICE_BEAM, MOVE_TOXIC);
    Fixture_SetLocation(MAP_GROUP(MAP_LITTLEROOT_TOWN), MAP_NUM(MAP_LITTLEROOT_TOWN), 8, 8);
}

// ai_forced_replacement: player leads a fast, hard-hitting Fire mon that can
// knock out the fixture trainer's lead (FIXTURE_TRAINER_AI_FORCED) in one or
// two hits, putting the AI at the forced-replacement decision almost
// immediately. Capture a savestate right after that KO to skip the setup on
// repeat runs (test_fixtures/ai_forced_replacement/state.ss1).
static void Scenario_AiForcedReplacement(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RECOMMENDED);
    Fixture_CommitNewRun();
    Fixture_SetBadges(4);
    Fixture_GiveMon(SPECIES_BLAZIKEN, 55, NATURE_JOLLY, ITEM_CHOICE_SCARF,
                     MOVE_FLAMETHROWER, MOVE_SKY_UPPERCUT, MOVE_STONE_EDGE, MOVE_PROTECT);
    Fixture_SetLocation(MAP_GROUP(MAP_LITTLEROOT_TOWN), MAP_NUM(MAP_LITTLEROOT_TOWN), 8, 8);
}

// nuzlocke_wipe: strict Nuzlocke run, active, one low-level party mon in
// front of FIXTURE_TRAINER_NUZLOCKE_WIPE, which outclasses it enough that a
// terminal wipe is reachable in under a minute.
static void Scenario_NuzlockeWipe(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RANDOMIZER);
    SetRulesetSetting(SETTING_PERMADEATH, TRUE);
    Fixture_CommitNewRun(); // also resets Nuzlocke state for the new run
    Nuzlocke_BeginRun();
    Fixture_GiveMon(SPECIES_ZIGZAGOON, 8, NATURE_HARDY, ITEM_NONE,
                     MOVE_TACKLE, MOVE_GROWL, MOVE_NONE, MOVE_NONE);
    Fixture_SetLocation(MAP_GROUP(MAP_LITTLEROOT_TOWN), MAP_NUM(MAP_LITTLEROOT_TOWN), 8, 8);
}

// move_reminder: a mid-level mon with only its first two level-up moves
// explicitly taught, so several later generated level-up moves are
// reachable but not yet learned. Placed at the Fallarbor Town move relearner
// with Heart Scales in the bag. Verify RULES > Move Reminder Mode is not
// Disabled before testing.
static void Scenario_MoveReminder(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RECOMMENDED);
    Fixture_CommitNewRun();
    Fixture_SetBadges(6);
    Fixture_GiveMon(SPECIES_GARDEVOIR, 30, NATURE_MODEST, ITEM_NONE,
                     MOVE_CONFUSION, MOVE_GROWL, MOVE_NONE, MOVE_NONE);
    AddBagItem(ITEM_HEART_SCALE, 10);
    Fixture_SetLocation(MAP_GROUP(MAP_FALLARBOR_TOWN), MAP_NUM(MAP_FALLARBOR_TOWN), 14, 8);
}

// premium_static: progression/flags set for the Regirock static (Desert
// Ruins), player warped inside. FLAG_REGI_DOORS_OPENED only unlocks the
// chamber's inner door via the braille puzzle result; the exact walk-in
// tile has not been verified against the map data, so a short walk inside
// the ruins may still be needed - see test_fixtures/premium_static/README.md.
// Adding another Premium static later is one more builder + fixture_scenarios.h row.
static void Scenario_PremiumStatic(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RANDOMIZER);
    Fixture_CommitNewRun();
    Fixture_SetBadges(8);
    FlagSet(FLAG_REGI_DOORS_OPENED);
    Fixture_GiveMon(SPECIES_SWAMPERT, 45, NATURE_ADAMANT, ITEM_NONE,
                     MOVE_SURF, MOVE_EARTHQUAKE, MOVE_ICE_BEAM, MOVE_NONE);
    AddBagItem(ITEM_ULTRA_BALL, 20);
    Fixture_SetLocation(MAP_GROUP(MAP_DESERT_RUINS), MAP_NUM(MAP_DESERT_RUINS), 5, 5);
}

// generation_mask: deterministic run with Gen 1 + Gen 9 enabled and Gens
// 2-8 disabled, parked on Route 101 grass for immediate wild/trainer/
// evolution filter checks.
static void Scenario_GenerationMask(void)
{
    Fixture_BeginRuleset(RULESET_PRESET_RANDOMIZER);
    SetRulesetSetting(SETTING_GEN_1_ENABLED, TRUE);
    SetRulesetSetting(SETTING_GEN_2_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_3_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_4_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_5_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_6_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_7_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_8_ENABLED, FALSE);
    SetRulesetSetting(SETTING_GEN_9_ENABLED, TRUE);
    SetRunSeed(1);
    Fixture_CommitNewRun();
    Fixture_GiveMon(SPECIES_TREECKO, 10, NATURE_HARDY, ITEM_NONE,
                     MOVE_POUND, MOVE_LEER, MOVE_NONE, MOVE_NONE);
    Fixture_SetLocation(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), 10, 10);
}

typedef void (*FixtureBuilder)(void);

static const FixtureBuilder sFixtureBuilders[] =
{
    [0] = Scenario_AiSwitchOscillation,   // ai_switch_oscillation
    [1] = Scenario_AiForcedReplacement,   // ai_forced_replacement
    [2] = Scenario_NuzlockeWipe,          // nuzlocke_wipe
    [3] = Scenario_MoveReminder,          // move_reminder
    [4] = Scenario_PremiumStatic,         // premium_static
    [5] = Scenario_GenerationMask,        // generation_mask
};
#define NUM_FIXTURE_SCENARIOS (sizeof(sFixtureBuilders) / sizeof(sFixtureBuilders[0]))

// -----------------------------------------------------------------------
// Flash dump: emitted over the mGBA debug-print channel so the headless
// host tool can reassemble it. See tools/generate_test_fixture.
// -----------------------------------------------------------------------

#define FIXTURE_FLASH_SIZE (128 * 1024)
#define FIXTURE_SECTOR_SIZE 0x1000
#define FIXTURE_NUM_SECTORS (FIXTURE_FLASH_SIZE / FIXTURE_SECTOR_SIZE)
#define FIXTURE_BYTES_PER_LINE 64

static const char sHexDigits[] = "0123456789ABCDEF";

static EWRAM_DATA u8 sFixtureSectorBuf[FIXTURE_SECTOR_SIZE] = {0}; // IWRAM is nearly full; keep this out of it

static void DumpFlashOverMgba(void)
{
    u8 *sector = sFixtureSectorBuf;
    char line[FIXTURE_BYTES_PER_LINE * 2 + 1];
    u32 sectorNum, offset, i;

    MgbaPrintf(MGBA_LOG_INFO, "FIXBEGIN:%d", FIXTURE_FLASH_SIZE);
    for (sectorNum = 0; sectorNum < FIXTURE_NUM_SECTORS; sectorNum++)
    {
        ReadFlash(sectorNum, 0, sector, FIXTURE_SECTOR_SIZE);
        for (offset = 0; offset < FIXTURE_SECTOR_SIZE; offset += FIXTURE_BYTES_PER_LINE)
        {
            for (i = 0; i < FIXTURE_BYTES_PER_LINE; i++)
            {
                u8 b = sector[offset + i];
                line[i * 2 + 0] = sHexDigits[b >> 4];
                line[i * 2 + 1] = sHexDigits[b & 0xF];
            }
            line[FIXTURE_BYTES_PER_LINE * 2] = '\0';
            MgbaPrintf(MGBA_LOG_INFO, "FIXDATA:%s", line);
        }
    }
    MgbaPrintf(MGBA_LOG_INFO, "FIXEND");
}

static void ExitFixtureRom(void)
{
    register u32 exitCode asm("r0") = 0;
    asm("swi 0x3" :: "r" (exitCode));
}

void CB2_TestFixtureBoot(void)
{
    static bool8 sRan = FALSE;
    static u16 sDelay = 0;
    u8 scenarioId;

    if (sRan)
        return;
    // Let hardware/RTC settle a couple frames before touching flash/save.
    if (sDelay < 4)
    {
        sDelay++;
        return;
    }
    sRan = TRUE;

    SetSaveBlocksPointers(GetSaveBlocksPointersBaseOffset());
    // Force a fixed RNG seed so fixture generation is reproducible: the
    // normal boot path seeds from the emulated RTC's current time (BUGFIX
    // in src/main.c), which varies run to run and would make every mon's
    // personality/IVs/nature non-deterministic across regenerations.
    SeedRng(20040919); // Emerald's JP release date, chosen only for stability

    scenarioId = sFixtureRequest[8];
    MgbaPrintf(MGBA_LOG_INFO, "FIXTURE: building scenario id %d", scenarioId);

    if (scenarioId < NUM_FIXTURE_SCENARIOS)
    {
        sFixtureBuilders[scenarioId]();
        MgbaPrintf(MGBA_LOG_INFO, "FIXTURE: built, saving");
        TrySavingData(SAVE_NORMAL);
        MgbaPrintf(MGBA_LOG_INFO, "FIXTURE: saved, dumping flash");
        DumpFlashOverMgba();
        MgbaPrintf(MGBA_LOG_INFO, "FIXTURE: done");
    }
    else
    {
        MgbaPrintf(MGBA_LOG_ERROR, "FIXTURE: unknown scenario id %d", scenarioId);
    }

    ExitFixtureRom();
}

#endif // TEST_FIXTURES
