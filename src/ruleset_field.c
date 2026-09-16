// ============================================================================
// Phase 9.5 Tier 2 - field / QoL ruleset predicates and the player-facing
// "RULES" submenu (docs/SPEC.md "Universal TM compatibility", "Move Tutors",
// "Portable healing", "Infinite Repel", "Set battle style").
//
// Same convention as src/ruleset_qol.c: keep the GetRulesetSetting() calls out
// of the big upstream files behind thin predicates. The submenu is a field
// overlay (windows + tasks, no CB2 swap) modelled on src/debug.c's menu.
// ============================================================================

#include "global.h"
#include "bike.h"
#include "caps.h"
#include "event_object_movement.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "item.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "nuzlocke.h"
#include "run_report.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "overworld.h"
#include "ruleset.h"
#include "ruleset_field.h"
#include "ruleset_menu.h"
#include "constants/items.h"
#include "constants/ruleset.h"
#include "constants/songs.h"

// ---------------------------------------------------------------------------
// Predicates (docs/SPEC.md sections named per function)
// ---------------------------------------------------------------------------

bool32 Ruleset_UniversalTmCompatOn(void)
{
    return GetRulesetSetting(SETTING_TM_COMPATIBILITY) == TMCOMPAT_UNIVERSAL;
}

bool32 Ruleset_UniversalTutorCompatOn(void)
{
    return GetRulesetSetting(SETTING_TUTOR_COMPATIBILITY) == TMCOMPAT_UNIVERSAL;
}

bool32 Ruleset_PortableHealOn(void)
{
    return GetRulesetSetting(SETTING_PORTABLE_HEAL) != 0;
}

// docs/SPEC.md "Portable healing": restores HP/PP/status, never the dead.
// HealPlayerParty() -> HealPokemon() already skips a permadeath-dead mon, so
// nothing extra is needed here.
void Ruleset_DoPortableHeal(void)
{
    HealPlayerParty();
}

bool32 Ruleset_InfiniteRepelOn(void)
{
    return GetRulesetSetting(SETTING_INFINITE_REPEL) != 0;
}

bool32 Ruleset_QuickTravelAvailable(void)
{
    return GetRulesetSetting(SETTING_QUICK_TRAVEL) != 0
        && FlagGet(FLAG_RECOVERED_DEVON_GOODS);
}

// Phase 12A (docs/CLAUDE_HANDOFF.md): nothing told the player Quick Travel had
// turned on. special, called from RusturfTunnel_EventScript_ResolveRescue
// right after FLAG_RECOVERED_DEVON_GOODS is set, so Ruleset_QuickTravelAvailable()
// already reflects the setting; gSpecialVar_Result gates the one-time msgbox.
void Ruleset_CheckQuickTravelJustUnlocked(void)
{
    gSpecialVar_Result = Ruleset_QuickTravelAvailable();
}

// docs/SPEC.md "Bikes".
bool32 Ruleset_BothBikesOwned(void)
{
    return CheckBagHasItem(ITEM_MACH_BIKE, 1) && CheckBagHasItem(ITEM_ACRO_BIKE, 1);
}

// docs/SPEC.md "Bikes": "Bike mode can be switched from the menu outside
// battle." If currently riding, switch in place via the same GetOnOffBike()
// primitive the Bag's bike-item-use flow already relies on - safe here
// because RulesetField_ShowMenu's caller (StartMenuRulesCallback) already
// calls FreezeObjectEvents() before opening this menu, the same precondition
// item-use's field callback expects. If not currently riding, retarget the
// registered quick-item instead of forcing an unwanted mount, so the next
// Select-button press (or manual Bag use) mounts the other bike.
void Ruleset_SwitchBikeMode(void)
{
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE))
    {
        GetOnOffBike(PLAYER_AVATAR_FLAG_ACRO_BIKE);
        gSaveBlock1Ptr->registeredItem = ITEM_ACRO_BIKE;
    }
    else if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_ACRO_BIKE))
    {
        GetOnOffBike(PLAYER_AVATAR_FLAG_MACH_BIKE);
        gSaveBlock1Ptr->registeredItem = ITEM_MACH_BIKE;
    }
    else if (gSaveBlock1Ptr->registeredItem == ITEM_MACH_BIKE)
    {
        gSaveBlock1Ptr->registeredItem = ITEM_ACRO_BIKE;
    }
    else if (gSaveBlock1Ptr->registeredItem == ITEM_ACRO_BIKE)
    {
        gSaveBlock1Ptr->registeredItem = ITEM_MACH_BIKE;
    }
    // else: Select is registered to something other than a bike (or nothing) -
    // leave it alone rather than clobbering the player's chosen quick-item.
}

bool32 Ruleset_InfiniteRepelActive(void)
{
    // GetRulesetSetting() runs the lazy-init, so the bit below is valid.
    return Ruleset_InfiniteRepelOn() && gSaveBlock3Ptr->ruleset.infiniteRepelActive;
}

void Ruleset_SetInfiniteRepelActive(bool32 active)
{
    (void)Ruleset_InfiniteRepelOn(); // force lazy-init before touching the store
    gSaveBlock3Ptr->ruleset.infiniteRepelActive = (active != 0);
}

bool32 Ruleset_ForceSetBattleStyleOn(void)
{
    return GetRulesetSetting(SETTING_FORCE_SET_BATTLE_STYLE) != 0;
}

// New Game: keep the stored Options value consistent with a forced ruleset so
// the Options screen and the battle agree from the first battle onward.
void Ruleset_ApplyForcedBattleStyle(void)
{
    if (Ruleset_ForceSetBattleStyleOn())
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET;
}

// Give / take one ruleset key item so the player's bag matches `wanted`.
static void ReconcileRulesetKeyItem(u16 itemId, bool32 wanted)
{
    bool32 have = CheckBagHasItem(itemId, 1);

    if (wanted && !have)
    {
        AddBagItem(itemId, 1);
    }
    else if (!wanted && have)
    {
        RemoveBagItem(itemId, 1);
        if (gSaveBlock1Ptr->registeredItem == itemId)
            gSaveBlock1Ptr->registeredItem = ITEM_NONE;
    }
}

// See ruleset_field.h. Cheap to call repeatedly - it only touches the bag when
// something is actually out of sync.
void Ruleset_GrantFieldKeyItems(void)
{
    ReconcileRulesetKeyItem(ITEM_RULES_HEAL, Ruleset_PortableHealOn());
    ReconcileRulesetKeyItem(ITEM_RULES_REPEL, Ruleset_InfiniteRepelOn());
}

// ---------------------------------------------------------------------------
// "RULES" submenu - field overlay
// ---------------------------------------------------------------------------

enum
{
    RF_ACT_RUNINFO,
    RF_ACT_SETTINGS,
    RF_ACT_HEAL,
    RF_ACT_REPEL,
    RF_ACT_SWITCH_BIKE,
    RF_ACT_CANCEL,
};

#define RF_MAX_ROWS 6
#define RF_NAME_LEN 24

struct RulesetFieldMenu
{
    u8 windowId;
    u8 listTaskId;
    bool8 listAlive;
    u8 numItems;
    u8 actions[RF_MAX_ROWS];
    struct ListMenuItem items[RF_MAX_ROWS];
    u8 names[RF_MAX_ROWS][RF_NAME_LEN];
};

static EWRAM_DATA struct RulesetFieldMenu *sRfMenu = NULL;

// Phase 12A: the combined "RUN INFO / SETTINGS" row split in two - RUN INFO
// now opens a read-only dashboard in place (see RfMenu_ShowRunInfo) instead
// of only reaching the settings browser (docs/CLAUDE_HANDOFF.md Phase 12A).
static const u8 sText_RfRunInfo[]    = _("RUN INFO");
static const u8 sText_RfSettings[]   = _("SETTINGS");
static const u8 sText_RfHeal[]       = _("HEAL PARTY");
static const u8 sText_RfRepelOn[]    = _("REPEL: ON");
static const u8 sText_RfRepelOff[]   = _("REPEL: OFF");
static const u8 sText_RfSwitchBike[] = _("SWITCH BIKE");
static const u8 sText_RfCancel[]     = _("CANCEL");
static const u8 sText_RfHealDone[]   = _("Your POKéMON were\nrestored to full health.");

// Phase 12A: Run Information overlay content (docs/SPEC.md "Hard level caps"
// requires the active cap visible somewhere in the interface; it previously
// had no UI call site at all).
static const u8 sText_RunInfoTitle[]      = _("RUN INFORMATION");
static const u8 sText_RunInfoPreset[]     = _("Preset: ");
static const u8 sText_RunInfoSeed[]       = _("Seed: 0x");
static const u8 sText_RunInfoGens[]       = _("Gens: ");
static const u8 sText_RunInfoBadges[]     = _("Badges: ");
static const u8 sText_RunInfoLevelCap[]   = _("   Level cap: ");
static const u8 sText_RunInfoDeaths[]     = _("Deaths: ");
static const u8 sText_RunInfoCaught[]     = _("   Caught: ");
static const u8 sText_RunInfoEncounters[] = _("Encounters: ");
static const u8 sText_RunInfoBosses[]     = _("   Bosses: ");
static const u8 sText_RunInfoReportSaved[]  = _("Run Report: saved to this save.");
static const u8 sText_RunInfoReportPending[] = _("Run Report: not finalized yet.");
static const u8 sText_RunInfoReturn[]     = _("B: return");

#define RUNINFO_LINE_HEIGHT 10

static const struct WindowTemplate sRfMenuWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 21,
    .height = 12, // Phase 12A: +2 tiles over the Phase 11D 5-row sizing (10)
                  // so a 6th row (SETTINGS) doesn't clip - verify in mGBA.
    .paletteNum = 15,
    .baseBlock = 1,
};

// Phase 12A: Run Information overlay - a wider, taller window reusing the
// same field-overlay convention (drawn over the frozen field, no CB2 swap).
// Built and printed as one static text block; B/A returns via the existing
// Task_RfMenuWaitMsg (IsFanfareTaskInactive() is TRUE when nothing played a
// fanfare, so it's safe to reuse for a plain "press a button" wait).
static const struct WindowTemplate sRfRunInfoWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 27,
    .height = 16,
    .paletteNum = 15,
    .baseBlock = 1,
};

static void Task_RfMenuInput(u8 taskId);
static void Task_RfMenuWaitMsg(u8 taskId);

static void RfMenu_BuildItems(void)
{
    u32 n = 0;

    StringCopy(sRfMenu->names[n], sText_RfRunInfo);
    sRfMenu->actions[n] = RF_ACT_RUNINFO;
    n++;

    StringCopy(sRfMenu->names[n], sText_RfSettings);
    sRfMenu->actions[n] = RF_ACT_SETTINGS;
    n++;

    if (Ruleset_PortableHealOn())
    {
        StringCopy(sRfMenu->names[n], sText_RfHeal);
        sRfMenu->actions[n] = RF_ACT_HEAL;
        n++;
    }

    if (Ruleset_InfiniteRepelOn())
    {
        StringCopy(sRfMenu->names[n],
                   Ruleset_InfiniteRepelActive() ? sText_RfRepelOn : sText_RfRepelOff);
        sRfMenu->actions[n] = RF_ACT_REPEL;
        n++;
    }

    if (Ruleset_BothBikesOwned())
    {
        StringCopy(sRfMenu->names[n], sText_RfSwitchBike);
        sRfMenu->actions[n] = RF_ACT_SWITCH_BIKE;
        n++;
    }

    StringCopy(sRfMenu->names[n], sText_RfCancel);
    sRfMenu->actions[n] = RF_ACT_CANCEL;
    n++;

    sRfMenu->numItems = n;
    for (u32 i = 0; i < n; i++)
    {
        sRfMenu->items[i].name = sRfMenu->names[i];
        sRfMenu->items[i].id = i;
    }
}

static void RfMenu_InitList(u16 selectedRow)
{
    struct ListMenuTemplate t = {0};

    t.items = sRfMenu->items;
    t.moveCursorFunc = ListMenuDefaultCursorMoveFunc;
    t.totalItems = sRfMenu->numItems;
    t.maxShowed = sRfMenu->numItems;
    t.windowId = sRfMenu->windowId;
    t.header_X = 0;
    t.item_X = 8;
    t.cursor_X = 0;
    t.upText_Y = 1;
    t.cursorPal = 2;
    t.fillValue = 1;
    t.cursorShadowPal = 3;
    t.lettersSpacing = 1;
    t.itemVerticalPadding = 0;
    t.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    t.fontId = FONT_NORMAL;
    t.cursorKind = CURSOR_BLACK_ARROW;

    sRfMenu->listTaskId = ListMenuInit(&t, 0, selectedRow);
    sRfMenu->listAlive = TRUE;
    CopyWindowToVram(sRfMenu->windowId, COPYWIN_FULL);
}

// Phase 12A: builds and prints the read-only Run Information overlay into
// windowId, one line at a time (StringAppend/ConvertIntTo*StringN into a
// reused scratch buffer, matching RulesetMenu_FormatRow's convention rather
// than juggling more STR_VAR slots than the format-string helpers support).
static void RfMenu_DrawRunInfo(u8 windowId)
{
    u8 line[40];
    u8 row = 0;
    u32 badges = 0, i;
    const struct RunStatsCounters *stats = RunReport_LiveStats();

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));

    AddTextPrinterParameterized(windowId, FONT_SMALL, sText_RunInfoTitle, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row += 2;

    StringCopy(line, sText_RunInfoPreset);
    StringAppend(line, GetRulesetPresetName(GetDisplayedRulesetPreset()));
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row++;

    StringCopy(line, sText_RunInfoSeed);
    ConvertIntToHexStringN(line + StringLength(line), GetRunSeed(), STR_CONV_MODE_LEADING_ZEROS, 8);
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row++;

    StringCopy(line, sText_RunInfoGens);
    RulesetMenu_BuildEnabledGenString(line + StringLength(line));
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row += 2;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (FlagGet(gBadgeFlags[i]))
            badges++;
    }
    StringCopy(line, sText_RunInfoBadges);
    ConvertIntToDecimalStringN(line + StringLength(line), badges, STR_CONV_MODE_LEFT_ALIGN, 1);
    StringAppend(line, sText_RunInfoLevelCap);
    ConvertIntToDecimalStringN(line + StringLength(line), GetProgressionLevelCap(), STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row++;

    // Nuzlocke_GetDeathCount() and the two RunStatsCounters fields below are
    // u16 (deathCount/encounters/bossesDefeated in include/global.h and
    // include/run_report.h); 5 digits covers the full u16 range so an
    // implausibly long run can never overflow ConvertIntToDecimalStringN's
    // fixed digit count into a garbled '?' (it truncates from the left, it
    // does not widen).
    StringCopy(line, sText_RunInfoDeaths);
    ConvertIntToDecimalStringN(line + StringLength(line), Nuzlocke_GetDeathCount(), STR_CONV_MODE_LEFT_ALIGN, 5);
    StringAppend(line, sText_RunInfoCaught);
    ConvertIntToDecimalStringN(line + StringLength(line), Nuzlocke_CountLocationsCaught(), STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row++;

    StringCopy(line, sText_RunInfoEncounters);
    ConvertIntToDecimalStringN(line + StringLength(line), stats->encounters, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringAppend(line, sText_RunInfoBosses);
    ConvertIntToDecimalStringN(line + StringLength(line), stats->bossesDefeated, STR_CONV_MODE_LEFT_ALIGN, 5);
    AddTextPrinterParameterized(windowId, FONT_SMALL, line, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row += 2;

    AddTextPrinterParameterized(windowId, FONT_SMALL,
        RunReport_IsFinalized() ? sText_RunInfoReportSaved : sText_RunInfoReportPending,
        1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);
    row++;

    AddTextPrinterParameterized(windowId, FONT_SMALL, sText_RunInfoReturn, 1, 1 + row * RUNINFO_LINE_HEIGHT, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(windowId, COPYWIN_FULL);
}

// Replaces the RULES list window with the (larger) Run Info window in place,
// then waits for any button before Task_RfMenuInput's caller tears back down
// to the field - mirrors the HEAL confirmation flow below.
static void RfMenu_ShowRunInfo(u8 taskId)
{
    if (sRfMenu->listAlive)
        DestroyListMenuTask(sRfMenu->listTaskId, NULL, NULL);
    sRfMenu->listAlive = FALSE;
    ClearStdWindowAndFrame(sRfMenu->windowId, TRUE);
    RemoveWindow(sRfMenu->windowId);

    sRfMenu->windowId = AddWindow(&sRfRunInfoWindowTemplate);
    DrawStdWindowFrame(sRfMenu->windowId, FALSE);
    RfMenu_DrawRunInfo(sRfMenu->windowId);

    gTasks[taskId].func = Task_RfMenuWaitMsg;
}

void RulesetField_ShowMenu(void)
{
    // Reconcile the key items here too, so a save made before this feature
    // existed picks them up the first time the player opens RULES.
    Ruleset_GrantFieldKeyItems();

    sRfMenu = AllocZeroed(sizeof(*sRfMenu));

    LoadMessageBoxAndBorderGfx();
    sRfMenu->windowId = AddWindow(&sRfMenuWindowTemplate);
    DrawStdWindowFrame(sRfMenu->windowId, FALSE);
    CopyWindowToVram(sRfMenu->windowId, COPYWIN_GFX);

    RfMenu_BuildItems();
    RfMenu_InitList(0);

    CreateTask(Task_RfMenuInput, 3);
}

// Rebuild the list in place (e.g. after toggling Repel) so a row label updates.
static void RfMenu_Redraw(void)
{
    u16 scroll = 0, row = 0;

    if (sRfMenu->listAlive)
        DestroyListMenuTask(sRfMenu->listTaskId, &scroll, &row);
    sRfMenu->listAlive = FALSE;
    FillWindowPixelBuffer(sRfMenu->windowId, PIXEL_FILL(1));
    RfMenu_BuildItems();
    if (row >= sRfMenu->numItems)
        row = sRfMenu->numItems - 1;
    RfMenu_InitList(row);
}

// Tear the overlay down. When returningToField, hand control back to the field
// (script + object events). SETTINGS passes FALSE because CB2_InitRulesetMenu
// rebuilds the screen itself and its saved callback restores the field.
static void RfMenu_TearDown(u8 taskId, bool32 returningToField)
{
    if (sRfMenu->listAlive)
        DestroyListMenuTask(sRfMenu->listTaskId, NULL, NULL);
    ClearStdWindowAndFrame(sRfMenu->windowId, TRUE);
    RemoveWindow(sRfMenu->windowId);
    DestroyTask(taskId);
    TRY_FREE_AND_SET_NULL(sRfMenu);

    ScriptContext_Enable();
    UnfreezeObjectEvents();
}

static void RfMenu_OpenRulesetScreen(u8 taskId)
{
    RfMenu_TearDown(taskId, FALSE);
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gMain.state = 0;
    SetMainCallback2(CB2_InitRulesetMenu);
}

static void Task_RfMenuInput(u8 taskId)
{
    s32 input = ListMenu_ProcessInput(sRfMenu->listTaskId);

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        RfMenu_TearDown(taskId, TRUE);
        return;
    }

    if (input == LIST_NOTHING_CHOSEN || input == LIST_CANCEL)
        return;

    switch (sRfMenu->actions[input])
    {
    case RF_ACT_RUNINFO:
        PlaySE(SE_SELECT);
        RfMenu_ShowRunInfo(taskId);
        break;
    case RF_ACT_SETTINGS:
        PlaySE(SE_SELECT);
        RfMenu_OpenRulesetScreen(taskId);
        break;
    case RF_ACT_HEAL:
        Ruleset_DoPortableHeal();
        PlayFanfare(MUS_HEAL);
        if (sRfMenu->listAlive)
            DestroyListMenuTask(sRfMenu->listTaskId, NULL, NULL);
        sRfMenu->listAlive = FALSE;
        FillWindowPixelBuffer(sRfMenu->windowId, PIXEL_FILL(1));
        AddTextPrinterParameterized(sRfMenu->windowId, FONT_NORMAL, sText_RfHealDone, 0, 1, 0, NULL);
        CopyWindowToVram(sRfMenu->windowId, COPYWIN_GFX);
        gTasks[taskId].func = Task_RfMenuWaitMsg;
        break;
    case RF_ACT_REPEL:
        PlaySE(SE_SELECT);
        Ruleset_SetInfiniteRepelActive(!Ruleset_InfiniteRepelActive());
        RfMenu_Redraw();
        break;
    case RF_ACT_SWITCH_BIKE:
        PlaySE(SE_SELECT);
        Ruleset_SwitchBikeMode();
        RfMenu_TearDown(taskId, TRUE);
        break;
    case RF_ACT_CANCEL:
    default:
        PlaySE(SE_SELECT);
        RfMenu_TearDown(taskId, TRUE);
        break;
    }
}

static void Task_RfMenuWaitMsg(u8 taskId)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON) && IsFanfareTaskInactive())
    {
        PlaySE(SE_SELECT);
        RfMenu_TearDown(taskId, TRUE);
    }
}
