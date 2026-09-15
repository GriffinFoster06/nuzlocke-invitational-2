#ifndef GUARD_RUN_REPORT_H
#define GUARD_RUN_REPORT_H

// ============================================================================
// Phase 11E - Terminal Run Reports (docs/SPEC.md "Terminal Run Reports").
//
// Two halves:
//   - struct RunReportState (EWRAM, appended to SaveBlock3, see include/global.h):
//     the small set of live counters and the bounded death history that must
//     be collected while the run is in progress. Zeroed by ClearSav3()
//     (src/load_save.c), which NewGameInitData() (src/new_game.c) calls on
//     every New Game before Nuzlocke_ResetState() runs - the same mechanism
//     that already resets NuzlockeState, so no extra reset code is needed
//     here.
//   - struct RunReportRecord (flash, SECTOR_ID_RUN_REPORT, see include/save.h):
//     the finalized, self-contained report written exactly once per terminal
//     outcome (Victory or Wipe). Never resident in EWRAM as a whole; built on
//     the stack in RunReport_Finalize() and written straight to flash through
//     the existing gSaveDataBuffer staging buffer (src/save.c).
//
// Every multi-byte field in the flash-resident structs is PACKED so their
// layout is a fixed, tool-independent byte format that tools/export_run.py
// parses without depending on compiler padding rules. RunReportState (EWRAM
// counters) is a plain struct - it is never parsed externally, only ever
// copied field-by-field into a RunReportRecord at finalize time.
//
// Report state is one-way: gameplay writes to it, nothing gameplay-visible
// ever reads it back. It must never influence randomization (docs/SPEC.md
// "Terminal Run Reports": "Reports never affect later runs, randomization
// pools, or species eligibility").
// ============================================================================

// Deliberately lightweight: only the plain size/count constants, not the
// full Pokemon API - this header is included from include/global.h (via
// struct SaveBlock3) and must not drag in pokemon.h's much larger closure.
#include "constants/global.h"
#include "constants/pokemon.h"

struct Pokemon;

// ----------------------------------------------------------------------------
// Bounds
// ----------------------------------------------------------------------------
#define RUN_REPORT_MAX_DEATHS   24  // bounded compact death history; deathCount
                                    // in NuzlockeState remains the true total
#define RUN_REPORT_MAX_SETTINGS 128 // NUM_SETTINGS is 105 today; headroom for
                                    // future append-only settings growth
#define RUN_REPORT_BANK_SLOTS   2   // keep the previous attempt's report when
                                    // a new one finalizes (docs/SPEC.md
                                    // "remains recoverable long enough for
                                    // manual export")

#define RUN_REPORT_SCHEMA_VERSION 1

// Little-endian byte-order-explicit four-character codes: on a little-endian
// CPU (ARM/GBA) these constants place 'R' at the lowest address, matching
// what tools/export_run.py reads with struct.unpack_from("<4s", ...).
#define RUN_REPORT_BANK_MAGIC \
    ((u32)'R' | ((u32)'U' << 8) | ((u32)'N' << 16) | ((u32)'R' << 24))
#define RUN_REPORT_RECORD_MAGIC \
    ((u32)'R' | ((u32)'R' << 8) | ((u32)'E' << 16) | ((u32)'C' << 24))

#define RUN_REPORT_BANK_VERSION 1

// ----------------------------------------------------------------------------
// Enums / flag bits
// ----------------------------------------------------------------------------
enum RunReportResult
{
    RUN_RESULT_NONE = 0,
    RUN_RESULT_VICTORY = 1,
    RUN_RESULT_WIPE = 2,
};

enum RunDeathCause
{
    RUN_DEATH_CAUSE_UNKNOWN = 0,
    RUN_DEATH_CAUSE_BATTLE = 1,
    RUN_DEATH_CAUSE_FIELD_POISON = 2,
    RUN_DEATH_CAUSE_DEBUG = 3,
};

enum RunWipeOpponentKind
{
    RUN_WIPE_OPPONENT_UNKNOWN = 0,
    RUN_WIPE_OPPONENT_WILD = 1,
    RUN_WIPE_OPPONENT_TRAINER = 2,
    RUN_WIPE_OPPONENT_BOSS = 3,
    RUN_WIPE_OPPONENT_FIELD_POISON = 4,
};

// struct RunReportRecord.flags
#define RUN_REPORT_FLAG_RUN_LOST         (1 << 0) // Nuzlocke_ShouldEndRunOnWhiteout()
                                                   // was also true (SETTING_WHITEOUT_BEHAVIOR
                                                   // == WHITEOUT_RUN_OVER); the report itself
                                                   // finalizes on "no usable Pokemon remain"
                                                   // regardless of that setting.
#define RUN_REPORT_FLAG_DEATHS_TRUNCATED (1 << 1)

// struct RunReportMon.monFlags. "Nicknamed" and "caught this run" are left
// out: the former is derivable host-side (compare nickname to the species
// name) and the latter is always true (a run's storage is wiped by New Game,
// so anything present at report time was necessarily obtained this run).
#define RUN_REPORT_MON_FLAG_SHINY                (1 << 0)
#define RUN_REPORT_MON_FLAG_DEAD                 (1 << 1)
#define RUN_REPORT_MON_FLAG_EGG                  (1 << 2)
#define RUN_REPORT_MON_FLAG_OVER_CAP_INELIGIBLE  (1 << 3)

// struct RunDeathRecord.recFlags
#define RUN_REPORT_DEATH_FLAG_SHINY              (1 << 0)
#define RUN_REPORT_DEATH_FLAG_OPPONENT_TRAINER   (1 << 1)

// gSaveBlock3Ptr->runReport.stateFlags
#define RUN_REPORT_STATE_FINALIZED         (1 << 0)
#define RUN_REPORT_STATE_DEATHS_TRUNCATED  (1 << 1)

// ----------------------------------------------------------------------------
// Live, EWRAM-resident state (SaveBlock3) - §3/§6 of the Phase 11E plan
// ----------------------------------------------------------------------------

// One bounded death-history entry, live copy. Same shape as the flash
// version (struct RunDeathRecord below); kept as a separate, non-PACKED type
// so the live struct never has to fight the compiler's natural alignment for
// no reason - it is copied field-by-field into the PACKED flash version at
// finalize time.
struct RunDeathRecordLive
{
    u16 species;
    u8 level;
    u8 cause;           // enum RunDeathCause
    u16 mapSec;
    u16 opponentId;      // trainer id, or opposing species when wild/unknown
    u8 badges;
    u8 recFlags;         // RUN_REPORT_DEATH_FLAG_*
    u16 playTimeMinutes; // hours*60 + minutes, saturating at 0xFFFF
};

struct RunStatsCounters
{
    u16 encounters;         // Nuzlocke_NoteWildEncounterStart() calls
    u16 shinyEncounters;    // Shiny Clause-flagged encounters
    u16 dupesRerolled;      // Dupes Clause reroll events
    u16 ballsThrown;
    u16 catchFailures;
    u16 shinyCatches;
    u16 encountersKilled;   // wild mon defeated, not caught
    u16 encountersFled;     // wild mon fled
    u16 encountersRanFrom;  // player ran
    u16 bossesDefeated;     // TrainerClassIsBoss() trainers beaten
    u16 levelToCapUses;
};

struct RunReportState
{
    struct RunStatsCounters stats;
    u32 finalizedReportId;   // 0 == this attempt has not finalized a report
    u16 lastBossTrainerId;
    u8 deathRecords;         // entries actually present, <= RUN_REPORT_MAX_DEATHS
    u8 stateFlags;           // RUN_REPORT_STATE_*
    struct RunDeathRecordLive deaths[RUN_REPORT_MAX_DEATHS];
};

// ----------------------------------------------------------------------------
// Finalized, flash-resident report (SECTOR_ID_RUN_REPORT) - all PACKED, fixed
// byte layout. See tools/export_run.py for the matching struct.Struct format.
// ----------------------------------------------------------------------------

struct PACKED RunReportMon
{
    u16 species;         // enum Species (form-specific IDs already encode form)
    u16 heldItem;         // enum Item
    u32 personality;
    u32 otId;
    u32 exp;
    u8 nickname[POKEMON_NAME_LENGTH + 1]; // raw GBA charmap, EOS-terminated
    u8 level;
    u8 ball;              // enum PokeBall
    u8 nature;
    u8 gender;
    u8 friendship;
    u8 ppBonuses;
    u8 metLocation;       // mapsec (also this mon's Nuzlocke encounter location)
    u8 metLevel;
    u8 slot;              // 1-based party slot at snapshot time
    u8 monFlags;          // RUN_REPORT_MON_FLAG_*
    u16 ability;           // enum Ability, resolved (not just the ability slot)
    u16 status;            // STATUS1_* bitfield
    u16 hp;
    u16 maxHp;
    u16 stats[5];          // Attack, Defense, Speed, Sp. Attack, Sp. Defense
    u16 moves[MAX_MON_MOVES];
    u32 ivPacked;          // GetMonData(mon, MON_DATA_IVS) layout: 5 bits each,
                           // HP/Atk/Def/Speed/SpAtk/SpDef, low bits first
    u8 evs[NUM_STATS];
    u8 pp[MAX_MON_MOVES];
};

struct PACKED RunDeathRecord
{
    u16 species;
    u8 level;
    u8 cause;             // enum RunDeathCause
    u16 mapSec;
    u16 opponentId;
    u8 badges;
    u8 recFlags;           // RUN_REPORT_DEATH_FLAG_*
    u16 playTimeMinutes;
};

struct PACKED RunWipeContext
{
    u16 mapSec;
    u8 mapGroup;
    u8 mapNum;
    u16 opponentTrainerId;
    u16 opponentSpecies;
    u8 opponentKind;       // enum RunWipeOpponentKind
    u8 battleOutcome;      // B_OUTCOME_*, 0 when not applicable (e.g. field poison)
    u16 lastBossTrainerId;
};

struct PACKED RunStatsSnapshot
{
    // Live counters, copied verbatim at finalize time.
    u16 encounters;
    u16 shinyEncounters;
    u16 dupesRerolled;
    u16 ballsThrown;
    u16 catchFailures;
    u16 shinyCatches;
    u16 encountersKilled;
    u16 encountersFled;
    u16 encountersRanFrom;
    u16 bossesDefeated;
    u16 levelToCapUses;
    // Reused existing counters, snapshotted so the exporter never has to
    // decrypt GAME_STAT_* itself (docs/SPEC.md: reuse existing counters).
    u16 totalBattles;      // GAME_STAT_TOTAL_BATTLES
    u16 wildBattles;       // GAME_STAT_WILD_BATTLES
    u16 trainerBattles;    // GAME_STAT_TRAINER_BATTLES
    u16 captures;          // GAME_STAT_POKEMON_CAPTURES
    u16 evolutions;        // GAME_STAT_EVOLVED_POKEMON
    u16 fishingEncounters; // GAME_STAT_FISHING_ENCOUNTERS
    u16 locationsCaught;   // Nuzlocke_CountLocationsCaught()
    u16 locationsUsed;     // popcount(NuzlockeState.locationUsed)
};

struct PACKED RunReportRecord
{
    // -- header / integrity --
    u32 magic;             // RUN_REPORT_RECORD_MAGIC
    u16 schemaVersion;      // RUN_REPORT_SCHEMA_VERSION
    u16 recordSize;         // sizeof(struct RunReportRecord) at write time
    u32 crc32;              // Crc32B over everything after this field
    u32 reportId;
    u32 runSeed;
    u32 newRunCounter;

    // -- identity --
    u16 projectVersion;     // PROJECT_VERSION_ID
    u16 rulesetVersion;     // RULESET_VERSION
    u16 randomizerVersion;  // RANDOMIZER_VERSION
    u16 genMask;            // bit (n-1) set == SETTING_GEN_n_ENABLED, n = 1..9
    u8 result;              // enum RunReportResult
    u8 preset;              // enum RulesetPreset
    u8 playerGender;
    u8 flags;               // RUN_REPORT_FLAG_*

    // -- player / time / progression --
    u8 playerName[PLAYER_NAME_LENGTH + 1]; // raw GBA charmap, EOS-terminated
    u32 playTimePacked;     // hours << 16 | minutes << 8 | seconds
    u16 badgeMask;
    u8 badgeCount;
    u8 finalCap;            // GetProgressionLevelCap()
    u8 progressionStep;     // index into caps.c's badge->cap ladder
    u8 partyCount;

    // -- full settings snapshot (informational; run identity is the
    // seed + version fields above plus genMask) --
    u8 settingsCount;       // NUM_SETTINGS at write time, <= RUN_REPORT_MAX_SETTINGS
    u8 settings[RUN_REPORT_MAX_SETTINGS];

    struct RunStatsSnapshot stats;
    struct RunWipeContext wipe; // zeroed for Victory

    struct RunReportMon party[PARTY_SIZE]; // Hall-of-Fame team, or final Wipe party

    u16 totalDeaths;        // NuzlockeState.deathCount - the true total
    u8 deathRecords;        // entries actually present below
    u8 pad0;
    struct RunDeathRecord deaths[RUN_REPORT_MAX_DEATHS];
};

struct PACKED RunReportBank
{
    u32 magic;              // RUN_REPORT_BANK_MAGIC
    u16 bankVersion;         // RUN_REPORT_BANK_VERSION
    u8 newestSlot;           // 0 or 1
    u8 slotsUsed;            // 0..RUN_REPORT_BANK_SLOTS
    struct RunReportRecord slot[RUN_REPORT_BANK_SLOTS];
};

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

// Idempotent: writes the finalized report to flash at most once per attempt
// (gated on gSaveBlock3Ptr->runReport.stateFlags & RUN_REPORT_STATE_FINALIZED).
// wipeOpponentKindHint is only consulted when result == RUN_RESULT_WIPE; pass
// RUN_WIPE_OPPONENT_FIELD_POISON from the field-poison path (which is not in
// a battle context, so it cannot be derived from battle globals) and
// RUN_WIPE_OPPONENT_UNKNOWN from the battle-loss path (derived internally
// from gBattleTypeFlags / TRAINER_BATTLE_PARAM).
void RunReport_Finalize(u8 result, u8 wipeOpponentKindHint);

// TRUE once this attempt's terminal report has finalized. Also doubles as
// the Wipe-condition idempotence guard.
bool32 RunReport_IsFinalized(void);

// TRUE if a Wipe report should finalize right now: the run is active,
// permadeath is on, and no usable Pokemon remain anywhere (docs/SPEC.md
// "Terminal Run Reports" - deliberately independent of
// SETTING_WHITEOUT_BEHAVIOR; see RUN_REPORT_FLAG_RUN_LOST for that).
bool32 RunReport_WipeConditionMet(void);

// ---- live counter hooks (see docs/CLAUDE_HANDOFF.md Phase 11E notes for
// the exact call sites) ----
void RunReport_NoteEncounterStart(bool32 shiny);
void RunReport_NoteDupeRerolled(void);
void RunReport_NoteBallThrown(void);
void RunReport_NoteCatchFailed(void);
void RunReport_NoteShinyCatch(void);
void RunReport_NoteEncounterKilled(void);
void RunReport_NoteEncounterFled(void);
void RunReport_NoteEncounterRanFrom(void);
void RunReport_NoteTrainerDefeated(u16 trainerId); // no-ops unless a boss class
void RunReport_NoteLevelToCapUsed(void);

// Appends a bounded death record (RUN_REPORT_MAX_DEATHS, keep-first). Called
// from nuzlocke.c's single MarkMonDead() sink, right where deathCount++
// already lives, so every permadeath path (battle, field poison, debug)
// reaches this once per mon.
void RunReport_NoteDeath(struct Pokemon *mon, u8 cause, u16 opponentId, bool32 opponentIsTrainer);

#endif // GUARD_RUN_REPORT_H
