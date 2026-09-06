// ============================================================================
// Nuzlocke-Randomizer ruleset settings screen (Phase 1 scaffolding).
//
// Reached from the debug menu. Data-driven from the descriptor table in
// src/data/ruleset.h via the accessors in src/ruleset.c - adding a setting in a
// later phase needs no change here.
//
//   Up/Down     move between settings
//   Left/Right  change the highlighted setting's value (-> preset recomputes)
//   L/R         switch category page
//   Start       cycle to the next named preset and apply it
//   Select      restore the current category to the last applied preset
//   A           (seed row) reroll the run seed; (preset row) next preset
//   B           exit
// ============================================================================

#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "ruleset.h"
#include "ruleset_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define RSMENU_MAX_ROWS 24          // largest category is well under this
#define RSMENU_ROW_TEXT_LEN 56
#define RSMENU_NAME_PAD 20          // column the value starts at, in chars
#define RSMENU_VISIBLE_ROWS 5

enum
{
    RSWIN_HEADER,
    RSWIN_LIST,
    RSWIN_DESC,
    RSWIN_COUNT,
};

struct RulesetMenuState
{
    u8 category;
    u8 windowIds[RSWIN_COUNT];
    u8 listTaskId;
    u8 rowCount;
    u16 rowSettingIds[RSMENU_MAX_ROWS];
    struct ListMenuItem listItems[RSMENU_MAX_ROWS];
    u8 rowText[RSMENU_MAX_ROWS][RSMENU_ROW_TEXT_LEN];
    u16 scrollOffset;
    u16 selectedRow;
};

static EWRAM_DATA struct RulesetMenuState *sState = NULL;

static void Task_RulesetMenuFadeInReal(u8 taskId);
static void Task_RulesetMenuProcessInput(u8 taskId);
static void Task_RulesetMenuFadeOut(u8 taskId);

static const u8 sText_HeaderFmt[]   = _("RULESET  {STR_VAR_1}");
static const u8 sText_CategoryFmt[] = _("{STR_VAR_1}/{STR_VAR_2}  {STR_VAR_3}");
static const u8 sText_Locked[]      = _("  (locked)");
static const u8 sText_Controls[]    = _("{DPAD_LEFTRIGHT}value  L R page  START preset  SELECT reset");
static const u8 sText_SeedPrefix[]  = _("0x");

static const struct BgTemplate sRulesetMenuBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sRulesetMenuWindowTemplates[] =
{
    [RSWIN_HEADER] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 1, .width = 28, .height = 2,
        .paletteNum = 15, .baseBlock = 1,
    },
    [RSWIN_LIST] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 4, .width = 28, .height = 11,
        .paletteNum = 15, .baseBlock = 1 + 28 * 2,
    },
    [RSWIN_DESC] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 16, .width = 28, .height = 3,
        .paletteNum = 15, .baseBlock = 1 + 28 * 2 + 28 * 11,
    },
    DUMMY_WIN_TEMPLATE,
};

static const u16 sRulesetMenuBgPal[] = { RGB(6, 8, 15) };

// ---------------------------------------------------------------------------

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Build the "Name............Value (locked)" string for one visible row.
static void RulesetMenu_FormatRow(u8 row)
{
    u16 settingId = sState->rowSettingIds[row];
    const struct SettingDescriptor *d = GetSettingDescriptor(settingId);
    u8 *dst = sState->rowText[row];
    u8 value = GetSettingDisplayValue(settingId);
    u8 valueBuf[24];

    StringCopyPadded(dst, d->name, CHAR_SPACE, RSMENU_NAME_PAD);

    if (settingId == SETTING_RUN_SEED)
    {
        StringCopy(valueBuf, sText_SeedPrefix);
        ConvertIntToHexStringN(valueBuf + 2, GetRunSeed(), STR_CONV_MODE_LEADING_ZEROS, 8);
    }
    else if (settingId == SETTING_PRESET)
    {
        StringCopy(valueBuf, GetRulesetPresetName(value));
    }
    else if (d->optionLabels != NULL)
    {
        StringCopy(valueBuf, d->optionLabels[value]);
    }
    else
    {
        ConvertIntToDecimalStringN(valueBuf, value, STR_CONV_MODE_LEFT_ALIGN, 3);
    }

    StringAppend(dst, valueBuf);

    if (settingId != SETTING_RUN_SEED && !IsRulesetSettingEditable(settingId))
        StringAppend(dst, sText_Locked);
}

static void RulesetMenu_DrawHeader(void)
{
    FillWindowPixelBuffer(sState->windowIds[RSWIN_HEADER], PIXEL_FILL(1));

    StringCopy(gStringVar1, GetRulesetPresetName(GetDisplayedRulesetPreset()));
    StringExpandPlaceholders(gStringVar4, sText_HeaderFmt);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_HEADER], FONT_NORMAL, gStringVar4, 0, 0, TEXT_SKIP_DRAW, NULL);

    ConvertIntToDecimalStringN(gStringVar1, sState->category + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, SETTING_CAT_COUNT, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringCopy(gStringVar3, GetSettingCategoryName(sState->category));
    StringExpandPlaceholders(gStringVar4, sText_CategoryFmt);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_HEADER], FONT_SMALL, gStringVar4, 104, 4, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sState->windowIds[RSWIN_HEADER], COPYWIN_FULL);
}

static void RulesetMenu_DrawDescription(s32 row)
{
    u16 settingId;
    const struct SettingDescriptor *d;

    if (row < 0 || row >= sState->rowCount)
        return;

    settingId = sState->rowSettingIds[row];
    d = GetSettingDescriptor(settingId);

    FillWindowPixelBuffer(sState->windowIds[RSWIN_DESC], PIXEL_FILL(1));
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, d->description, 0, 0, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, sText_Controls, 0, 12, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
}

static void RulesetMenu_MoveCursor(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    RulesetMenu_DrawDescription(itemIndex);
}

// Populate rowSettingIds/listItems/rowText for the current category and (re)init
// the list-menu task.
static void RulesetMenu_BuildCategory(bool8 firstBuild)
{
    struct ListMenuTemplate template;
    u32 i;
    u8 count = 0;

    for (i = 0; i < NUM_SETTINGS && count < RSMENU_MAX_ROWS; i++)
    {
        if (GetSettingDescriptor(i)->category != sState->category)
            continue;
        sState->rowSettingIds[count] = i;
        count++;
    }
    sState->rowCount = count;

    for (i = 0; i < count; i++)
    {
        RulesetMenu_FormatRow(i);
        sState->listItems[i].name = sState->rowText[i];
        sState->listItems[i].id = i;
    }

    if (!firstBuild)
        DestroyListMenuTask(sState->listTaskId, NULL, NULL);

    sState->scrollOffset = 0;
    sState->selectedRow = 0;

    FillWindowPixelBuffer(sState->windowIds[RSWIN_LIST], PIXEL_FILL(1));

    template.items = sState->listItems;
    template.moveCursorFunc = RulesetMenu_MoveCursor;
    template.itemPrintFunc = NULL;
    template.totalItems = count;
    template.maxShowed = (count < RSMENU_VISIBLE_ROWS) ? count : RSMENU_VISIBLE_ROWS;
    template.windowId = sState->windowIds[RSWIN_LIST];
    template.header_X = 0;
    template.item_X = 8;
    template.cursor_X = 0;
    template.upText_Y = 1;
    template.cursorPal = 2;
    template.fillValue = 1;
    template.cursorShadowPal = 3;
    template.lettersSpacing = 0;
    template.itemVerticalPadding = 0;
    template.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    template.fontId = FONT_NORMAL;
    template.cursorKind = CURSOR_BLACK_ARROW;
    template.textNarrowWidth = 0;
    template.isDynamic = FALSE;

    sState->listTaskId = ListMenuInit(&template, sState->scrollOffset, sState->selectedRow);

    RulesetMenu_DrawHeader();
    RulesetMenu_DrawDescription(0);
}

static s32 RulesetMenu_CurrentSettingId(void)
{
    u16 scroll = 0, row = 0;

    ListMenuGetScrollAndRow(sState->listTaskId, &scroll, &row);
    return sState->rowSettingIds[scroll + row];
}

static void RulesetMenu_RefreshVisibleRows(void)
{
    u32 i;

    for (i = 0; i < sState->rowCount; i++)
        RulesetMenu_FormatRow(i);
    RedrawListMenu(sState->listTaskId);
    RulesetMenu_DrawHeader();
}

// ---------------------------------------------------------------------------

static void Task_RulesetMenuFadeInReal(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_RulesetMenuProcessInput;
}

static void Task_RulesetMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(sState->listTaskId, NULL, NULL);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        TRY_FREE_AND_SET_NULL(sState);
        SetMainCallback2(gMain.savedCallback);
    }
}

static void Task_RulesetMenuProcessInput(u8 taskId)
{
    s32 settingId;

    if (JOY_NEW(B_BUTTON))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_RulesetMenuFadeOut;
        return;
    }

    if (JOY_NEW(L_BUTTON) || JOY_NEW(R_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (JOY_NEW(L_BUTTON))
            sState->category = (sState->category == 0) ? SETTING_CAT_COUNT - 1 : sState->category - 1;
        else
            sState->category = (sState->category + 1 == SETTING_CAT_COUNT) ? 0 : sState->category + 1;
        RulesetMenu_BuildCategory(FALSE);
        return;
    }

    if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_SELECT);
        NudgeRulesetSetting(SETTING_PRESET, 1);
        RulesetMenu_BuildCategory(FALSE);
        return;
    }

    if (JOY_NEW(SELECT_BUTTON))
    {
        PlaySE(SE_SELECT);
        RestoreRulesetCategory(sState->category);
        RulesetMenu_RefreshVisibleRows();
        return;
    }

    settingId = RulesetMenu_CurrentSettingId();

    if (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT))
    {
        s32 delta = JOY_NEW(DPAD_LEFT) ? -1 : 1;

        if (settingId == SETTING_RUN_SEED)
        {
            // nothing to nudge; A rerolls
        }
        else if (IsRulesetSettingEditable(settingId) || settingId == SETTING_PRESET)
        {
            PlaySE(SE_SELECT);
            NudgeRulesetSetting(settingId, delta);
            if (settingId == SETTING_PRESET)
                RulesetMenu_BuildCategory(FALSE);
            else
                RulesetMenu_RefreshVisibleRows();
        }
        else
        {
            PlaySE(SE_FAILURE);
        }
        return;
    }

    if (JOY_NEW(A_BUTTON))
    {
        if (settingId == SETTING_RUN_SEED)
        {
            PlaySE(SE_SELECT);
            RerollRunSeed();
            RulesetMenu_RefreshVisibleRows();
        }
        else if (settingId == SETTING_PRESET)
        {
            PlaySE(SE_SELECT);
            NudgeRulesetSetting(SETTING_PRESET, 1);
            RulesetMenu_BuildCategory(FALSE);
        }
        return;
    }

    // Up/Down (and nothing else) go to the list menu; it calls our moveCursorFunc.
    ListMenu_ProcessInput(sState->listTaskId);
}

// ---------------------------------------------------------------------------

void CB2_InitRulesetMenu(void)
{
    u32 i;

    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sRulesetMenuBgTemplates, ARRAY_COUNT(sRulesetMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        InitWindows(sRulesetMenuWindowTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        LoadMessageBoxAndBorderGfx();
        LoadPalette(sRulesetMenuBgPal, BG_PLTT_ID(0), sizeof(sRulesetMenuBgPal));
        gMain.state++;
        break;
    case 4:
        sState = AllocZeroed(sizeof(*sState));
        sState->category = 0;

        for (i = 0; i < RSWIN_COUNT; i++)
        {
            sState->windowIds[i] = i;
            PutWindowTilemap(i);
            FillWindowPixelBuffer(i, PIXEL_FILL(1));
            DrawStdWindowFrame(i, FALSE);
        }

        // Descriptor-table sanity check (see docs/PHASES.md verification step).
        if (!RulesetTablesAreValid())
        {
            FillWindowPixelBuffer(RSWIN_HEADER, PIXEL_FILL(1));
            AddTextPrinterParameterized(RSWIN_HEADER, FONT_NORMAL, COMPOUND_STRING("RULESET TABLE INVALID"), 0, 0, TEXT_SKIP_DRAW, NULL);
            CopyWindowToVram(RSWIN_HEADER, COPYWIN_FULL);
        }

        RulesetMenu_BuildCategory(TRUE);

        for (i = 0; i < RSWIN_COUNT; i++)
            CopyWindowToVram(i, COPYWIN_FULL);

        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 5:
    {
        u8 taskId = CreateTask(Task_RulesetMenuFadeInReal, 0);
        (void)taskId;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        gMain.state = 0;
        return;
    }
    }
}
