// ============================================================================
// Phase 11E - Terminal Run Reports (docs/SPEC.md "Terminal Run Reports").
//
// Owns gSaveBlock3Ptr->runReport (live counters + bounded death history) and
// the flash-resident report bank at SECTOR_ID_RUN_REPORT in full - nothing
// else in the tree touches either directly. See include/run_report.h for the
// save-format contract and tools/export_run.py for the host-side reader.
//
// RunReport_Finalize() is the only entry point that writes to flash. It is
// idempotent (gated on RUN_REPORT_STATE_FINALIZED) and is called from exactly
// two call sites: src/post_battle_event_funcs.c (Victory) and
// src/nuzlocke.c / src/field_poison.c (Wipe, via RunReport_WipeConditionMet()).
// ============================================================================

#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "caps.h"
#include "data.h"
#include "event_data.h"
#include "malloc.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "randomizer.h"
#include "ruleset.h"
#include "save.h"
#include "string_util.h"
#include "nuzlocke.h"
#include "run_report.h"
#include "constants/battle.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/game_stat.h"
#include "constants/pokemon.h"
#include "constants/project_version.h"
#include "constants/ruleset.h"

// TryWrite/ReadSpecialSaveSector() treat the whole sector (minus the 4-byte
// sentinel they own) as one opaque payload - see src/save.c and the comment
// at src/ereader_helpers.c "Save data using TryWriteSpecialSaveSector is
// allowed to exceed SECTOR_DATA_SIZE (up to the counter field)".
#define RUN_REPORT_SECTOR_PAYLOAD SECTOR_COUNTER_OFFSET

STATIC_ASSERT(sizeof(struct RunReportBank) <= RUN_REPORT_SECTOR_PAYLOAD, RunReportBankFitsSector);
STATIC_ASSERT(NUM_SETTINGS <= RUN_REPORT_MAX_SETTINGS, RunReportSettingsArrayBigEnough);

// Exact byte-layout regression guards: tools/export_run.py hardcodes these
// same sizes in its struct.Struct format strings. If one of these ever
// fires, the exporter's format strings must be updated to match (and
// RUN_REPORT_SCHEMA_VERSION bumped).
STATIC_ASSERT(sizeof(struct RunReportMon) == 79, RunReportMonSize);
STATIC_ASSERT(sizeof(struct RunDeathRecord) == 12, RunDeathRecordSize);
STATIC_ASSERT(sizeof(struct RunWipeContext) == 12, RunWipeContextSize);
STATIC_ASSERT(sizeof(struct RunStatsSnapshot) == 38, RunStatsSnapshotSize);
STATIC_ASSERT(sizeof(struct RunReportRecord) == 999, RunReportRecordSize);
STATIC_ASSERT(sizeof(struct RunReportBank) == 2006, RunReportBankSize);

// ---------------------------------------------------------------------------
// Live counters
// ---------------------------------------------------------------------------

static struct RunStatsCounters *Counters(void)
{
    return &gSaveBlock3Ptr->runReport.stats;
}

static void BumpU16(u16 *counter)
{
    if (*counter < 0xFFFF)
        (*counter)++;
}

static u16 ClampU16(u32 value)
{
    return (value > 0xFFFF) ? 0xFFFF : value;
}

static u8 CountBadges(void)
{
    u32 i;
    u8 count = 0;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (FlagGet(gBadgeFlags[i]))
            count++;
    }
    return count;
}

bool32 RunReport_IsFinalized(void)
{
    return (gSaveBlock3Ptr->runReport.stateFlags & RUN_REPORT_STATE_FINALIZED) != 0;
}

bool32 RunReport_WipeConditionMet(void)
{
    if (RunReport_IsFinalized())
        return FALSE;
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_PermadeathOn())
        return FALSE;
    return !Nuzlocke_AnyUsableMonRemains();
}

// Every live counter is gated here on "the run is active and hasn't already
// finalized" so callers never have to duplicate that check.
static bool32 ShouldTrackStats(void)
{
    return Nuzlocke_RunIsActive() && !RunReport_IsFinalized();
}

void RunReport_NoteEncounterStart(bool32 shiny)
{
    if (!ShouldTrackStats())
        return;
    BumpU16(&Counters()->encounters);
    if (shiny)
        BumpU16(&Counters()->shinyEncounters);
}

void RunReport_NoteDupeRerolled(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->dupesRerolled);
}

void RunReport_NoteBallThrown(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->ballsThrown);
}

void RunReport_NoteCatchFailed(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->catchFailures);
}

void RunReport_NoteShinyCatch(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->shinyCatches);
}

void RunReport_NoteEncounterKilled(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->encountersKilled);
}

void RunReport_NoteEncounterFled(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->encountersFled);
}

void RunReport_NoteEncounterRanFrom(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->encountersRanFrom);
}

void RunReport_NoteLevelToCapUsed(void)
{
    if (ShouldTrackStats())
        BumpU16(&Counters()->levelToCapUses);
}

void RunReport_NoteTrainerDefeated(u16 trainerId)
{
    if (!ShouldTrackStats())
        return;
    if (!TrainerClassIsBoss(GetTrainerClassFromId(trainerId)))
        return;
    BumpU16(&Counters()->bossesDefeated);
    gSaveBlock3Ptr->runReport.lastBossTrainerId = trainerId;
}

void RunReport_NoteDeath(struct Pokemon *mon, u8 cause, u16 opponentId, bool32 opponentIsTrainer)
{
    struct RunReportState *rr = &gSaveBlock3Ptr->runReport;
    struct RunDeathRecordLive *rec;
    u32 minutes;

    if (rr->deathRecords >= RUN_REPORT_MAX_DEATHS)
    {
        rr->stateFlags |= RUN_REPORT_STATE_DEATHS_TRUNCATED;
        return;
    }

    rec = &rr->deaths[rr->deathRecords++];
    rec->species = GetMonData(mon, MON_DATA_SPECIES);
    rec->level = GetMonData(mon, MON_DATA_LEVEL);
    rec->cause = cause;
    rec->mapSec = Nuzlocke_CurrentLocationTag();
    rec->opponentId = opponentId;
    rec->badges = CountBadges();
    rec->recFlags = 0;
    if (IsMonShiny(mon))
        rec->recFlags |= RUN_REPORT_DEATH_FLAG_SHINY;
    if (opponentIsTrainer)
        rec->recFlags |= RUN_REPORT_DEATH_FLAG_OPPONENT_TRAINER;

    minutes = (u32)gSaveBlock2Ptr->playTimeHours * 60 + gSaveBlock2Ptr->playTimeMinutes;
    rec->playTimeMinutes = ClampU16(minutes);
}

// ---------------------------------------------------------------------------
// Finalization
// ---------------------------------------------------------------------------

static u32 ComputeReportId(u8 result)
{
    u32 words[6];

    words[0] = GetRunSeed();
    words[1] = gSaveBlock2Ptr->newRunCounter;
    words[2] = ((u32)gSaveBlock2Ptr->playTimeHours << 16)
             | ((u32)gSaveBlock2Ptr->playTimeMinutes << 8)
             | gSaveBlock2Ptr->playTimeSeconds;
    words[3] = result;
    words[4] = Nuzlocke_GetDeathCount();
    words[5] = T1_READ_32(gSaveBlock2Ptr->playerTrainerId);
    return Crc32B((const u8 *)words, sizeof(words));
}

static void BuildWipeContext(struct RunWipeContext *wipe, u8 wipeOpponentKindHint)
{
    memset(wipe, 0, sizeof(*wipe));
    wipe->mapSec = Nuzlocke_CurrentLocationTag();
    wipe->mapGroup = (u8)gSaveBlock1Ptr->location.mapGroup;
    wipe->mapNum = (u8)gSaveBlock1Ptr->location.mapNum;
    wipe->lastBossTrainerId = gSaveBlock3Ptr->runReport.lastBossTrainerId;

    if (wipeOpponentKindHint == RUN_WIPE_OPPONENT_FIELD_POISON)
    {
        wipe->opponentKind = RUN_WIPE_OPPONENT_FIELD_POISON;
        return;
    }

    wipe->battleOutcome = gBattleOutcome;
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
    {
        wipe->opponentTrainerId = TRAINER_BATTLE_PARAM.opponentA;
        wipe->opponentKind = TrainerClassIsBoss(GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA))
                            ? RUN_WIPE_OPPONENT_BOSS : RUN_WIPE_OPPONENT_TRAINER;
    }
    else
    {
        // This runs post-battle, after ZeroEnemyPartyMons() - read the wild
        // species the battle recorded (memset only at the next battle start).
        enum Species species = gBattleResults.lastOpponentSpecies;

        if (species != SPECIES_NONE)
        {
            wipe->opponentSpecies = species;
            wipe->opponentKind = RUN_WIPE_OPPONENT_WILD;
        }
    }
}

static void BuildPartyMon(struct RunReportMon *out, struct Pokemon *mon, u8 slot)
{
    u32 i;

    memset(out, 0, sizeof(*out));
    out->species = GetMonData(mon, MON_DATA_SPECIES);
    if (out->species == SPECIES_NONE)
        return;

    out->heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    out->personality = GetMonData(mon, MON_DATA_PERSONALITY);
    out->otId = GetMonData(mon, MON_DATA_OT_ID);
    out->exp = GetMonData(mon, MON_DATA_EXP);
    GetMonData(mon, MON_DATA_NICKNAME, out->nickname);
    out->nickname[POKEMON_NAME_LENGTH] = EOS;
    out->level = GetMonData(mon, MON_DATA_LEVEL);
    out->ball = GetMonData(mon, MON_DATA_POKEBALL);
    out->nature = GetNature(mon);
    out->gender = GetMonGender(mon);
    out->friendship = GetMonData(mon, MON_DATA_FRIENDSHIP);
    out->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);
    out->metLocation = GetMonData(mon, MON_DATA_MET_LOCATION);
    out->metLevel = GetMonData(mon, MON_DATA_MET_LEVEL);
    out->slot = slot;
    out->ability = GetMonAbility(mon);
    out->status = GetMonData(mon, MON_DATA_STATUS);
    out->hp = GetMonData(mon, MON_DATA_HP);
    out->maxHp = GetMonData(mon, MON_DATA_MAX_HP);
    out->stats[0] = GetMonData(mon, MON_DATA_ATK);
    out->stats[1] = GetMonData(mon, MON_DATA_DEF);
    out->stats[2] = GetMonData(mon, MON_DATA_SPEED);
    out->stats[3] = GetMonData(mon, MON_DATA_SPATK);
    out->stats[4] = GetMonData(mon, MON_DATA_SPDEF);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        out->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
        out->pp[i] = GetMonData(mon, MON_DATA_PP1 + i);
    }
    out->ivPacked = GetMonData(mon, MON_DATA_IVS);
    out->evs[STAT_HP] = GetMonData(mon, MON_DATA_HP_EV);
    out->evs[STAT_ATK] = GetMonData(mon, MON_DATA_ATK_EV);
    out->evs[STAT_DEF] = GetMonData(mon, MON_DATA_DEF_EV);
    out->evs[STAT_SPEED] = GetMonData(mon, MON_DATA_SPEED_EV);
    out->evs[STAT_SPATK] = GetMonData(mon, MON_DATA_SPATK_EV);
    out->evs[STAT_SPDEF] = GetMonData(mon, MON_DATA_SPDEF_EV);

    if (IsMonShiny(mon))
        out->monFlags |= RUN_REPORT_MON_FLAG_SHINY;
    if (GetMonData(mon, MON_DATA_IS_DEAD))
        out->monFlags |= RUN_REPORT_MON_FLAG_DEAD;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        out->monFlags |= RUN_REPORT_MON_FLAG_EGG;
    if (Caps_MonIsBattleIneligible(mon))
        out->monFlags |= RUN_REPORT_MON_FLAG_OVER_CAP_INELIGIBLE;
}

static void BuildSettingsSnapshot(struct RunReportRecord *rec)
{
    u32 i, count = min(NUM_SETTINGS, RUN_REPORT_MAX_SETTINGS);

    for (i = 0; i < count; i++)
        rec->settings[i] = GetRulesetSetting(i);
    rec->settingsCount = count;
}

static void BuildStatsSnapshot(struct RunStatsSnapshot *out)
{
    struct RunStatsCounters *live = Counters();

    out->encounters = live->encounters;
    out->shinyEncounters = live->shinyEncounters;
    out->dupesRerolled = live->dupesRerolled;
    out->ballsThrown = live->ballsThrown;
    out->catchFailures = live->catchFailures;
    out->shinyCatches = live->shinyCatches;
    out->encountersKilled = live->encountersKilled;
    out->encountersFled = live->encountersFled;
    out->encountersRanFrom = live->encountersRanFrom;
    out->bossesDefeated = live->bossesDefeated;
    out->levelToCapUses = live->levelToCapUses;
    out->totalBattles = ClampU16(GetGameStat(GAME_STAT_TOTAL_BATTLES));
    out->wildBattles = ClampU16(GetGameStat(GAME_STAT_WILD_BATTLES));
    out->trainerBattles = ClampU16(GetGameStat(GAME_STAT_TRAINER_BATTLES));
    out->captures = ClampU16(GetGameStat(GAME_STAT_POKEMON_CAPTURES));
    out->evolutions = ClampU16(GetGameStat(GAME_STAT_EVOLVED_POKEMON));
    out->fishingEncounters = ClampU16(GetGameStat(GAME_STAT_FISHING_ENCOUNTERS));
    out->locationsCaught = ClampU16(Nuzlocke_CountLocationsCaught());
    out->locationsUsed = ClampU16(Nuzlocke_CountLocationsUsed());
}

static void BuildDeathHistory(struct RunReportRecord *rec)
{
    struct RunReportState *rr = &gSaveBlock3Ptr->runReport;
    u32 i;

    rec->totalDeaths = ClampU16(Nuzlocke_GetDeathCount());
    rec->deathRecords = rr->deathRecords;
    if (rr->stateFlags & RUN_REPORT_STATE_DEATHS_TRUNCATED)
        rec->flags |= RUN_REPORT_FLAG_DEATHS_TRUNCATED;

    for (i = 0; i < rr->deathRecords; i++)
    {
        struct RunDeathRecordLive *src = &rr->deaths[i];
        struct RunDeathRecord *dst = &rec->deaths[i];

        dst->species = src->species;
        dst->level = src->level;
        dst->cause = src->cause;
        dst->mapSec = src->mapSec;
        dst->opponentId = src->opponentId;
        dst->badges = src->badges;
        dst->recFlags = src->recFlags;
        dst->playTimeMinutes = src->playTimeMinutes;
    }
}

static void BuildRunReportRecord(struct RunReportRecord *rec, u8 result, u8 wipeOpponentKindHint)
{
    u32 i;

    memset(rec, 0, sizeof(*rec));
    rec->magic = RUN_REPORT_RECORD_MAGIC;
    rec->schemaVersion = RUN_REPORT_SCHEMA_VERSION;
    rec->recordSize = sizeof(struct RunReportRecord);
    rec->runSeed = GetRunSeed();
    rec->newRunCounter = gSaveBlock2Ptr->newRunCounter;
    rec->projectVersion = PROJECT_VERSION_ID;
    rec->rulesetVersion = GetSavedRulesetVersion();
    rec->randomizerVersion = GetSavedRandomizerVersion();

    for (i = 0; i < 9; i++)
    {
        if (GetRulesetSetting(SETTING_GEN_1_ENABLED + i))
            rec->genMask |= (1 << i);
    }

    rec->result = result;
    rec->preset = GetDisplayedRulesetPreset();
    rec->playerGender = gSaveBlock2Ptr->playerGender;
    rec->flags = 0;
    if (result == RUN_RESULT_WIPE && Nuzlocke_ShouldEndRunOnWhiteout())
        rec->flags |= RUN_REPORT_FLAG_RUN_LOST;

    StringCopy(rec->playerName, gSaveBlock2Ptr->playerName);
    rec->playTimePacked = ((u32)gSaveBlock2Ptr->playTimeHours << 16)
                        | ((u32)gSaveBlock2Ptr->playTimeMinutes << 8)
                        | gSaveBlock2Ptr->playTimeSeconds;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (FlagGet(gBadgeFlags[i]))
            rec->badgeMask |= (1 << i);
    }
    rec->badgeCount = CountBadges();
    rec->finalCap = GetProgressionLevelCap();
    rec->progressionStep = rec->badgeCount + (FlagGet(FLAG_IS_CHAMPION) ? 1 : 0);

    BuildSettingsSnapshot(rec);
    BuildStatsSnapshot(&rec->stats);

    if (result == RUN_RESULT_WIPE)
        BuildWipeContext(&rec->wipe, wipeOpponentKindHint);

    rec->partyCount = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        BuildPartyMon(&rec->party[i], mon, i + 1);
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE)
            rec->partyCount++;
    }

    BuildDeathHistory(rec);

    rec->reportId = ComputeReportId(result);
    rec->crc32 = Crc32B((const u8 *)&rec->reportId,
        sizeof(struct RunReportRecord) - offsetof(struct RunReportRecord, reportId));
}

// Writes `rec` into the flash report bank, keeping whatever the other slot
// already held (docs/SPEC.md "the latest finalized report remains
// recoverable long enough for manual export, including across practical
// run-reset/new-run handling").
static void WriteRecordToBank(const struct RunReportRecord *rec)
{
    u8 *sectorBuf = AllocZeroed(SECTOR_SIZE);
    struct RunReportBank *bank;
    u32 slot;

    if (sectorBuf == NULL)
        return;
    bank = (struct RunReportBank *)sectorBuf;

    if (TryReadSpecialSaveSector(SECTOR_ID_RUN_REPORT, sectorBuf) != SAVE_STATUS_OK
     || bank->magic != RUN_REPORT_BANK_MAGIC
     || bank->bankVersion != RUN_REPORT_BANK_VERSION)
    {
        memset(bank, 0, SECTOR_SIZE);
        bank->magic = RUN_REPORT_BANK_MAGIC;
        bank->bankVersion = RUN_REPORT_BANK_VERSION;
        bank->newestSlot = RUN_REPORT_BANK_SLOTS - 1; // so slot 0 becomes "next"
        bank->slotsUsed = 0;
    }

    slot = (bank->newestSlot + 1) % RUN_REPORT_BANK_SLOTS;
    bank->slot[slot] = *rec;
    bank->newestSlot = slot;
    if (bank->slotsUsed < RUN_REPORT_BANK_SLOTS)
        bank->slotsUsed++;

    TryWriteSpecialSaveSector(SECTOR_ID_RUN_REPORT, sectorBuf);
    Free(sectorBuf);
}

void RunReport_Finalize(u8 result, u8 wipeOpponentKindHint)
{
    struct RunReportRecord *rec;

    if (RunReport_IsFinalized())
        return;

    rec = AllocZeroed(sizeof(struct RunReportRecord));
    if (rec == NULL)
        return; // out of heap: nothing destructive has happened yet, so it is
                // safe to simply not finalize and let the caller proceed.

    BuildRunReportRecord(rec, result, wipeOpponentKindHint);
    WriteRecordToBank(rec);

    gSaveBlock3Ptr->runReport.finalizedReportId = rec->reportId;
    gSaveBlock3Ptr->runReport.stateFlags |= RUN_REPORT_STATE_FINALIZED;

    Free(rec);
}
