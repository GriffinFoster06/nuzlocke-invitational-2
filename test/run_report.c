#include "global.h"
#include "pokemon.h"
#include "nuzlocke.h"
#include "ruleset.h"
#include "run_report.h"
#include "test/test.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// ============================================================================
// Phase 11E - Terminal Run Reports (docs/SPEC.md "Terminal Run Reports").
//
// Deliberately does NOT call RunReport_Finalize(): no existing test in this
// tree exercises actual flash writes (TryWriteSpecialSaveSector /
// ProgramFlashSectorAndVerify) - test/save.c only checks struct sizes - so
// the finalize-to-flash round trip is left to mGBA acceptance testing
// (docs/CLAUDE_HANDOFF.md Phase 11E). Everything below is EWRAM-only:
// live counters, the bounded death history, and the wipe-condition predicate.
// ============================================================================

TEST("Flash-resident Run Report structs match their documented byte sizes")
{
    EXPECT_EQ(sizeof(struct RunReportMon), 79);
    EXPECT_EQ(sizeof(struct RunDeathRecord), 12);
    EXPECT_EQ(sizeof(struct RunWipeContext), 12);
    EXPECT_EQ(sizeof(struct RunStatsSnapshot), 38);
    EXPECT_EQ(sizeof(struct RunReportRecord), 999);
    EXPECT_EQ(sizeof(struct RunReportBank), 2006);
}

static void ResetRunReportState(void)
{
    memset(&gSaveBlock3Ptr->runReport, 0, sizeof(gSaveBlock3Ptr->runReport));
}

TEST("Live counters only accumulate while the run is active, and saturate")
{
    ResetRunReportState();
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    SetRulesetRunActive(FALSE);

    RunReport_NoteBallThrown();
    RunReport_NoteCatchFailed();
    RunReport_NoteEncounterStart(FALSE);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.ballsThrown, 0);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.catchFailures, 0);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.encounters, 0);

    SetRulesetRunActive(TRUE);
    RunReport_NoteBallThrown();
    RunReport_NoteEncounterStart(TRUE);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.ballsThrown, 1);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.encounters, 1);
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.shinyEncounters, 1);

    gSaveBlock3Ptr->runReport.stats.ballsThrown = 0xFFFF;
    RunReport_NoteBallThrown();
    EXPECT_EQ(gSaveBlock3Ptr->runReport.stats.ballsThrown, 0xFFFF);
}

TEST("RunReport_IsFinalized reads the state flag directly")
{
    ResetRunReportState();
    EXPECT(!(RunReport_IsFinalized()));
    gSaveBlock3Ptr->runReport.stateFlags |= RUN_REPORT_STATE_FINALIZED;
    EXPECT((RunReport_IsFinalized()));
    ResetRunReportState();
}

TEST("Wipe condition requires an active strict run with no usable Pokemon")
{
    struct Pokemon mon;

    ResetRunReportState();
    Nuzlocke_ResetState();
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    SetRulesetSetting(SETTING_PERMADEATH, TRUE);
    Nuzlocke_BeginRun();
    EXPECT((Nuzlocke_RunIsActive()));

    memset(&gParties[B_TRAINER_PLAYER], 0, sizeof(gParties[B_TRAINER_PLAYER]));
    CreateMon(&mon, SPECIES_WOBBUFFET, 5, 12345, OTID_STRUCT_PRESET(0x1234));
    gParties[B_TRAINER_PLAYER][0] = mon;
    EXPECT(!(RunReport_WipeConditionMet())); // one living mon in the party

    Nuzlocke_MarkMonDeadFieldPoison(&gParties[B_TRAINER_PLAYER][0]);
    EXPECT((RunReport_WipeConditionMet()));

    gSaveBlock3Ptr->runReport.stateFlags |= RUN_REPORT_STATE_FINALIZED;
    EXPECT(!(RunReport_WipeConditionMet())); // idempotence guard
    ResetRunReportState();
}

TEST("Field-poison deaths are recorded with the right cause and no graveyard move")
{
    struct Pokemon mon;
    struct RunDeathRecordLive *rec;

    ResetRunReportState();
    Nuzlocke_ResetState();
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    SetRulesetSetting(SETTING_PERMADEATH, TRUE);
    Nuzlocke_BeginRun();

    memset(&gParties[B_TRAINER_PLAYER], 0, sizeof(gParties[B_TRAINER_PLAYER]));
    CreateMon(&mon, SPECIES_WOBBUFFET, 5, 12345, OTID_STRUCT_PRESET(0x1234));
    gParties[B_TRAINER_PLAYER][0] = mon;

    Nuzlocke_MarkMonDeadFieldPoison(&gParties[B_TRAINER_PLAYER][0]);

    // No immediate graveyard move: the party slot keeps its data (dead flag
    // set) until Nuzlocke_FinalizeDeadMons() runs - see src/field_poison.c.
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_WOBBUFFET);
    EXPECT((GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_DEAD)));

    EXPECT_EQ(gSaveBlock3Ptr->runReport.deathRecords, 1);
    rec = &gSaveBlock3Ptr->runReport.deaths[0];
    EXPECT_EQ(rec->species, SPECIES_WOBBUFFET);
    EXPECT_EQ(rec->cause, RUN_DEATH_CAUSE_FIELD_POISON);
    EXPECT_EQ(rec->level, 5);

    ResetRunReportState();
}

TEST("Death history is bounded and reports truncation past the cap")
{
    u32 i;

    ResetRunReportState();
    Nuzlocke_ResetState();
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    SetRulesetSetting(SETTING_PERMADEATH, TRUE);
    Nuzlocke_BeginRun();

    for (i = 0; i < RUN_REPORT_MAX_DEATHS + 3; i++)
    {
        struct Pokemon mon;
        CreateMon(&mon, SPECIES_WOBBUFFET, 5, i, OTID_STRUCT_PRESET(0x1234));
        RunReport_NoteDeath(&mon, RUN_DEATH_CAUSE_BATTLE, 0, FALSE);
    }

    EXPECT_EQ(gSaveBlock3Ptr->runReport.deathRecords, RUN_REPORT_MAX_DEATHS);
    EXPECT((gSaveBlock3Ptr->runReport.stateFlags & RUN_REPORT_STATE_DEATHS_TRUNCATED));

    ResetRunReportState();
}

TEST("Nuzlocke_FinalizeDeadMons moves dead mons to the graveyard and compacts")
{
    struct Pokemon mon1, mon2;

    ResetRunReportState();
    Nuzlocke_ResetState();
    EXPECT((ApplyRulesetPreset(RULESET_PRESET_RECOMMENDED)));
    SetRulesetSetting(SETTING_PERMADEATH, TRUE);
    SetRulesetSetting(SETTING_GRAVEYARD_BOX, TRUE);
    Nuzlocke_BeginRun();

    memset(&gParties[B_TRAINER_PLAYER], 0, sizeof(gParties[B_TRAINER_PLAYER]));
    CreateMon(&mon1, SPECIES_WOBBUFFET, 5, 1, OTID_STRUCT_PRESET(0x1234));
    CreateMon(&mon2, SPECIES_WYNAUT, 5, 2, OTID_STRUCT_PRESET(0x1234));
    gParties[B_TRAINER_PLAYER][0] = mon1;
    gParties[B_TRAINER_PLAYER][1] = mon2;

    Nuzlocke_MarkMonDeadFieldPoison(&gParties[B_TRAINER_PLAYER][0]);
    Nuzlocke_FinalizeDeadMons();

    // The dead mon's old slot 0 is now the surviving Wynaut (compacted).
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_WYNAUT);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_NONE);

    ResetRunReportState();
}
