// ============================================================================
// Nuzlocke-Randomizer ruleset settings screen.
//
// Reached from Start Menu -> Run Info and the debug menu. Data-driven from the
// descriptor table in src/data/ruleset.h via the accessors in src/ruleset.c.
//
//   Up/Down     move between settings
//   Left/Right  change the highlighted setting's value (-> preset recomputes)
//   L/R         switch category page
//   Start       cycle to the next named preset and apply it
//   Select      restore the current category to the last applied preset
//   A           activate the highlighted value or action
//   B           exit
// ============================================================================

#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "line_break.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random_mon_generation.h"
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
#include "constants/characters.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define RSMENU_MAX_ROWS 24          // largest category is well under this
#define RSMENU_ROW_TEXT_LEN 56
#define RSMENU_NAME_PAD 16          // min column the value starts at, in chars
#define RSMENU_VISIBLE_ROWS 4       // rows the shorter list window can show

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
    bool8 seedEditing;
    u8 seedNibble;
    u32 seedEditValue;
    bool8 speciesBansOpen;
    u16 banSpecies;
    bool8 restoreConfirm;
    // Phase 11A.6: New-Game-only pre-run wizard mode (see
    // RulesetMenu_EnterNewGameWizard). Restricts the visible rows to
    // generation-locked settings and adds the Start-Game confirmation
    // overlay below; the in-run Start Menu -> RULES path never sets this.
    bool8 wizardMode;
    bool8 wizardConfirmOpen;
};

static EWRAM_DATA struct RulesetMenuState *sState = NULL;
// Set by RulesetMenu_EnterNewGameWizard() just before switching into
// CB2_InitRulesetMenu; consumed once at state-4 allocation so a later,
// ordinary Start Menu -> RULES open is never accidentally in wizard mode.
static bool8 sPendingWizardMode = FALSE;

static void Task_RulesetMenuFadeInReal(u8 taskId);
static void Task_RulesetMenuProcessInput(u8 taskId);
static void Task_RulesetMenuFadeOut(u8 taskId);
static void Task_RulesetMenuFadeOutToNewGame(u8 taskId);

// docs/SPEC.md "Run seed": Run Information shows the active ruleset, its stored
// format versions (so a seed stays reproducible across ROM updates), and the
// run seed itself. The header shows both versions; Preset/Seed shows the seed.
static const u8 sText_HeaderFmt[]     = _("{STR_VAR_1} R{STR_VAR_2}/G{STR_VAR_3}");
static const u8 sText_CategoryFmt[]   = _("{STR_VAR_1}/{STR_VAR_2}  {STR_VAR_3}");
static const u8 sText_Locked[]        = _(" (L)");
static const u8 sText_Controls[]      = _("{DPAD_LEFTRIGHT} change   L/R page   START preset");
static const u8 sText_SeedPrefix[]    = _("0x");
static const u8 sText_Open[]          = _("Open");
static const u8 sText_Confirm[]       = _("Confirm");
static const u8 sText_SeedEdit[]      = _("Edit 0x{STR_VAR_1} nibble {STR_VAR_2}\n{DPAD_LEFTRIGHT} select {DPAD_UPDOWN} change A save B cancel");
static const u8 sText_BanControls[]   = _("National Dex order\nA toggle  START clear all  B return");
static const u8 sText_RestoreConfirm[] = _("Press A again to restore every setting; B cancels.");

// Phase 11A.6: New-Game-only wizard confirmation overlay (see
// RulesetMenu_EnterNewGameWizard / RulesetMenu_DrawWizardConfirm below).
static const u8 sText_WizardConfirmFmt[] = _("Preset: {STR_VAR_1}\nSeed: 0x{STR_VAR_2}\nGenerations: {STR_VAR_3}");
static const u8 sText_WizardControls[]  = _("A start game   B back");
static const u8 sText_WizardBlocked[]   = _("Enable at least one generation to start.");
static const u8 sText_WizardNone[]      = _("none");

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
    // Full 20-row screen budget: header rows 0-4, list 4-14, description 13-19
    // (1-tile std frames overlap on the shared rows). Two small header lines and
    // a 3-line description window mean neither overlaps the other any more.
    [RSWIN_HEADER] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 1, .width = 28, .height = 3,
        .paletteNum = 15, .baseBlock = 1,
    },
    [RSWIN_LIST] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 5, .width = 28, .height = 9,
        .paletteNum = 15, .baseBlock = 1 + 28 * 3,
    },
    [RSWIN_DESC] = {
        .bg = 0, .tilemapLeft = 1, .tilemapTop = 14, .width = 28, .height = 5,
        .paletteNum = 15, .baseBlock = 1 + 28 * 3 + 28 * 9,
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
        ConvertIntToHexStringN(valueBuf + 2, sState->seedEditing ? sState->seedEditValue : GetRunSeed(), STR_CONV_MODE_LEADING_ZEROS, 8);
    }
    else if (settingId == SETTING_PRESET)
    {
        StringCopy(valueBuf, GetRulesetPresetName(value));
    }
    else if (settingId == SETTING_SPECIES_BANS)
    {
        StringCopy(valueBuf, sText_Open);
    }
    else if (settingId == SETTING_RESTORE_ALL)
    {
        StringCopy(valueBuf, sText_Confirm);
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

    if (settingId != SETTING_RUN_SEED && settingId != SETTING_RESTORE_ALL
     && !IsRulesetSettingEditable(settingId))
        StringAppend(dst, sText_Locked);
}

static void RulesetMenu_DrawHeader(void)
{
    FillWindowPixelBuffer(sState->windowIds[RSWIN_HEADER], PIXEL_FILL(1));

    // Line 1: active preset and both persisted format/algorithm versions.
    StringCopy(gStringVar1, GetRulesetPresetName(GetDisplayedRulesetPreset()));
    ConvertIntToDecimalStringN(gStringVar2, GetSavedRulesetVersion(), STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, GetSavedRandomizerVersion(), STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, sText_HeaderFmt);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_HEADER], FONT_SMALL, gStringVar4, 0, 1, TEXT_SKIP_DRAW, NULL);

    // Line 2: current category page, on its own line so nothing overlaps.
    ConvertIntToDecimalStringN(gStringVar1, sState->category + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, SETTING_CAT_COUNT, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringCopy(gStringVar3, GetSettingCategoryName(sState->category));
    StringExpandPlaceholders(gStringVar4, sText_CategoryFmt);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_HEADER], FONT_SMALL, gStringVar4, 0, 13, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sState->windowIds[RSWIN_HEADER], COPYWIN_FULL);
}

static void RulesetMenu_DrawDescription(s32 row)
{
    u16 settingId;
    const struct SettingDescriptor *d;
    u8 descBuf[128];

    if (row < 0 || row >= sState->rowCount)
        return;

    settingId = sState->rowSettingIds[row];
    d = GetSettingDescriptor(settingId);

    FillWindowPixelBuffer(sState->windowIds[RSWIN_DESC], PIXEL_FILL(1));

    if (sState->seedEditing)
    {
        ConvertIntToHexStringN(gStringVar1, sState->seedEditValue, STR_CONV_MODE_LEADING_ZEROS, 8);
        ConvertIntToDecimalStringN(gStringVar2, sState->seedNibble + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringExpandPlaceholders(descBuf, sText_SeedEdit);
        AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, descBuf, 0, 0, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
        return;
    }
    if (sState->restoreConfirm)
    {
        AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, sText_RestoreConfirm, 0, 0, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
        return;
    }

    // Descriptions are single sentences that overrun one line; wrap them into
    // the top two rows and keep the controls hint on the last row.
    StringCopy(descBuf, d->description);
    BreakStringAutomatic(descBuf, WindowWidthPx(sState->windowIds[RSWIN_DESC]), 2, FONT_SMALL, HIDE_SCROLL_PROMPT);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, descBuf, 0, 0, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, sText_Controls, 0, 26, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
}

static void RulesetMenu_MoveCursor(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    RulesetMenu_DrawDescription(itemIndex);
}

// Phase 11A.6: true when `category` has at least one row the wizard would
// show (a SETTING_LOCK_GENERATION setting, or SETTING_PRESET) - used both by
// the row filter below and by the wizard's L/R category-skip logic, so the
// two never disagree about which categories are non-empty.
static bool32 RulesetMenu_RowVisibleInWizard(u32 settingId)
{
    return settingId == SETTING_PRESET
        || GetSettingDescriptor(settingId)->lockClass == SETTING_LOCK_GENERATION;
}

static bool32 CategoryHasWizardRows(u32 category)
{
    u32 i;

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (GetSettingDescriptor(i)->category != category
         || (GetSettingDescriptor(i)->flags & SETTING_FLAG_HIDDEN))
            continue;
        if (RulesetMenu_RowVisibleInWizard(i))
            return TRUE;
    }
    return FALSE;
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
        if (GetSettingDescriptor(i)->category != sState->category
         || (GetSettingDescriptor(i)->flags & SETTING_FLAG_HIDDEN))
            continue;
        // Phase 11A.6: the New-Game-only wizard (see
        // RulesetMenu_EnterNewGameWizard) shows only settings that must be
        // locked before the seed rolls, plus SETTING_PRESET itself (its own
        // lockClass is SETTING_LOCK_NONE, since it stays nudgeable mid-run
        // via the ordinary RULES menu, but choosing a preset is still part
        // of the pre-run flow, so the wizard always shows that one row).
        if (sState->wizardMode && !RulesetMenu_RowVisibleInWizard(i))
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
    template.fontId = FONT_NARROW;   // narrower glyphs so name + value + lock fit
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

static s32 RulesetMenu_CurrentRow(void)
{
    u16 scroll = 0, row = 0;

    ListMenuGetScrollAndRow(sState->listTaskId, &scroll, &row);
    return scroll + row;
}

static void RulesetMenu_RefreshVisibleRows(void)
{
    u32 i;

    for (i = 0; i < sState->rowCount; i++)
        RulesetMenu_FormatRow(i);
    RedrawListMenu(sState->listTaskId);
    RulesetMenu_DrawHeader();
}

static bool32 RulesetMenu_CanListSpecies(enum Species species)
{
    return species > SPECIES_NONE && species < NUM_SPECIES
        && species != SPECIES_EGG && IsSpeciesEnabled(species)
        && IsRandomSpeciesFormSafe(species);
}

static u32 RulesetMenu_SpeciesOrderKey(enum Species species)
{
    return (u32)SpeciesToNationalPokedexNum(species) * NUM_SPECIES + species;
}

static enum Species RulesetMenu_FindOrderedSpecies(enum Species current, bool32 forward)
{
    u32 currentKey = current == SPECIES_NONE ? 0 : RulesetMenu_SpeciesOrderKey(current);
    enum Species best = SPECIES_NONE, wrap = SPECIES_NONE, species;
    u32 bestKey = 0xFFFFFFFF, wrapKey = 0xFFFFFFFF;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        u32 key;
        if (!RulesetMenu_CanListSpecies(species))
            continue;
        key = RulesetMenu_SpeciesOrderKey(species);
        if (forward)
        {
            if (key > currentKey && key < bestKey) { best = species; bestKey = key; }
            if (key < wrapKey) { wrap = species; wrapKey = key; }
        }
        else
        {
            if (key < currentKey && (best == SPECIES_NONE || key > bestKey)) { best = species; bestKey = key; }
            if (wrap == SPECIES_NONE || key > wrapKey) { wrap = species; wrapKey = key; }
        }
    }
    return best == SPECIES_NONE ? wrap : best;
}

static void RulesetMenu_DrawSpeciesBans(void)
{
    enum Species species = sState->banSpecies;
    u8 line[48];
    u32 i;

    FillWindowPixelBuffer(sState->windowIds[RSWIN_LIST], PIXEL_FILL(1));
    for (i = 0; i < RSMENU_VISIBLE_ROWS && species != SPECIES_NONE; i++)
    {
        line[0] = i == 0 ? CHAR_GREATER_THAN : CHAR_SPACE;
        line[1] = CHAR_SPACE;
        line[2] = CHAR_SPACE;
        line[3] = CHAR_SPACE;
        line[4] = EOS;
        if (Ruleset_IsSpeciesBanned(species))
            line[2] = CHAR_X;
        StringAppend(line, GetSpeciesName(species));
        AddTextPrinterParameterized(sState->windowIds[RSWIN_LIST], FONT_NARROW, line, 0, i * 16, TEXT_SKIP_DRAW, NULL);
        species = RulesetMenu_FindOrderedSpecies(species, TRUE);
        if (species == sState->banSpecies)
            break;
    }
    CopyWindowToVram(sState->windowIds[RSWIN_LIST], COPYWIN_FULL);
    FillWindowPixelBuffer(sState->windowIds[RSWIN_DESC], PIXEL_FILL(1));
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL, sText_BanControls, 0, 0, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
}

static void RulesetMenu_OpenSpeciesBans(void)
{
    if (!IsRulesetSettingEditable(SETTING_SPECIES_BANS))
    {
        PlaySE(SE_FAILURE);
        return;
    }
    sState->speciesBansOpen = TRUE;
    sState->banSpecies = RulesetMenu_FindOrderedSpecies(SPECIES_NONE, TRUE);
    RulesetMenu_DrawSpeciesBans();
}

static void RulesetMenu_ProcessSpeciesBans(void)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sState->speciesBansOpen = FALSE;
        RulesetMenu_BuildCategory(FALSE);
    }
    else if (JOY_NEW(DPAD_UP) || JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        sState->banSpecies = RulesetMenu_FindOrderedSpecies(sState->banSpecies, JOY_NEW(DPAD_DOWN));
        RulesetMenu_DrawSpeciesBans();
    }
    else if (JOY_NEW(A_BUTTON))
    {
        bool8 banned = !Ruleset_IsSpeciesBanned(sState->banSpecies);
        PlaySE(Ruleset_SetSpeciesBanned(sState->banSpecies, banned) ? SE_SELECT : SE_FAILURE);
        RulesetMenu_DrawSpeciesBans();
        RulesetMenu_DrawHeader();
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(Ruleset_ClearSpeciesBans() ? SE_SELECT : SE_FAILURE);
        RulesetMenu_DrawSpeciesBans();
        RulesetMenu_DrawHeader();
    }
}

static void RulesetMenu_ProcessSeedEditor(void)
{
    u32 shift = (7 - sState->seedNibble) * 4;
    u32 digit = (sState->seedEditValue >> shift) & 0xF;

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sState->seedEditing = FALSE;
    }
    else if (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT))
    {
        PlaySE(SE_SELECT);
        if (JOY_NEW(DPAD_LEFT))
            sState->seedNibble = sState->seedNibble == 0 ? 7 : sState->seedNibble - 1;
        else
            sState->seedNibble = (sState->seedNibble + 1) & 7;
    }
    else if (JOY_NEW(DPAD_UP) || JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        digit = (digit + (JOY_NEW(DPAD_UP) ? 1 : 15)) & 0xF;
        sState->seedEditValue = (sState->seedEditValue & ~(0xFu << shift)) | (digit << shift);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (SetRunSeed(sState->seedEditValue))
        {
            PlaySE(SE_SELECT);
            sState->seedEditing = FALSE;
        }
        else
            PlaySE(SE_FAILURE);
    }
    RulesetMenu_RefreshVisibleRows();
    RulesetMenu_DrawDescription(RulesetMenu_CurrentRow());
}

// Phase 11A.6: New-Game-only wizard confirmation overlay. Modeled on the
// species-ban sub-screen above: draws directly into RSWIN_LIST/RSWIN_DESC
// (no ListMenu) and is dispatched from Task_RulesetMenuProcessInput before
// any other input path, exactly like sState->speciesBansOpen.
static bool32 RulesetMenu_AnyGenerationEnabled(void)
{
    u32 gen;

    for (gen = 0; gen < 9; gen++)
    {
        if (GetRulesetSetting(SETTING_GEN_1_ENABLED + gen))
            return TRUE;
    }
    return FALSE;
}

static void RulesetMenu_BuildEnabledGenString(u8 *dst)
{
    u32 gen;
    u32 n = 0;

    for (gen = 0; gen < 9; gen++)
    {
        if (GetRulesetSetting(SETTING_GEN_1_ENABLED + gen))
        {
            if (n > 0)
                dst[n++] = CHAR_COMMA;
            dst[n++] = CHAR_0 + (gen + 1); // gens are 1-9: always a single digit
        }
    }
    dst[n] = EOS;
    if (n == 0)
        StringCopy(dst, sText_WizardNone);
}

static void RulesetMenu_DrawWizardConfirm(void)
{
    u8 genBuf[32];

    FillWindowPixelBuffer(sState->windowIds[RSWIN_LIST], PIXEL_FILL(1));
    StringCopy(gStringVar1, GetRulesetPresetName(GetDisplayedRulesetPreset()));
    ConvertIntToHexStringN(gStringVar2, GetRunSeed(), STR_CONV_MODE_LEADING_ZEROS, 8);
    RulesetMenu_BuildEnabledGenString(genBuf);
    StringCopy(gStringVar3, genBuf);
    StringExpandPlaceholders(gStringVar4, sText_WizardConfirmFmt);
    AddTextPrinterParameterized(sState->windowIds[RSWIN_LIST], FONT_NARROW, gStringVar4, 0, 0, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sState->windowIds[RSWIN_LIST], COPYWIN_FULL);

    FillWindowPixelBuffer(sState->windowIds[RSWIN_DESC], PIXEL_FILL(1));
    AddTextPrinterParameterized(sState->windowIds[RSWIN_DESC], FONT_SMALL,
        RulesetMenu_AnyGenerationEnabled() ? sText_WizardControls : sText_WizardBlocked,
        0, 0, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sState->windowIds[RSWIN_DESC], COPYWIN_FULL);
}

static void RulesetMenu_ProcessWizardConfirm(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sState->wizardConfirmOpen = FALSE;
        RulesetMenu_BuildCategory(FALSE);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        // Fail-safe UI guard (docs/SPEC.md): a fully-empty generation mask
        // must never reach gameplay, even though every randomizer pool
        // already falls back to vanilla on its own if it somehow did.
        if (!RulesetMenu_AnyGenerationEnabled())
        {
            PlaySE(SE_FAILURE);
            return;
        }
        PlaySE(SE_SELECT);
        RulesetSettings_MarkPreconfigured();
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_RulesetMenuFadeOutToNewGame;
    }
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

// Phase 11A.6: wizard "Start Game" exit path. Unlike Task_RulesetMenuFadeOut
// (which returns to whatever screen opened the ordinary RULES menu), this
// always proceeds into CB2_NewGame - the wizard is only ever entered from
// the New-Game chain (see RulesetMenu_EnterNewGameWizard), never mid-run.
static void Task_RulesetMenuFadeOutToNewGame(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyListMenuTask(sState->listTaskId, NULL, NULL);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        TRY_FREE_AND_SET_NULL(sState);
        SetMainCallback2(CB2_NewGame);
    }
}

static void Task_RulesetMenuProcessInput(u8 taskId)
{
    s32 settingId;

    if (sState->wizardConfirmOpen)
    {
        RulesetMenu_ProcessWizardConfirm(taskId);
        return;
    }
    if (sState->speciesBansOpen)
    {
        RulesetMenu_ProcessSpeciesBans();
        return;
    }
    if (sState->seedEditing)
    {
        RulesetMenu_ProcessSeedEditor();
        return;
    }
    if (sState->restoreConfirm)
    {
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(RestoreRulesetAll() ? SE_SELECT : SE_FAILURE);
            sState->restoreConfirm = FALSE;
            RulesetMenu_BuildCategory(FALSE);
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sState->restoreConfirm = FALSE;
            RulesetMenu_DrawDescription(RulesetMenu_CurrentRow());
        }
        return;
    }

    if (!sState->wizardMode && JOY_NEW(B_BUTTON))
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_RulesetMenuFadeOut;
        return;
    }

    if (JOY_NEW(L_BUTTON) || JOY_NEW(R_BUTTON))
    {
        bool8 forward = JOY_NEW(R_BUTTON) ? TRUE : FALSE;
        u32 tries;

        PlaySE(SE_SELECT);
        // Phase 11A.6: in wizard mode, skip any category that has nothing the
        // wizard would show, instead of landing on a visibly empty page.
        // Bounded by SETTING_CAT_COUNT so a fully-empty config (should not
        // happen; SETTING_PRESET's own category always qualifies) cannot loop.
        for (tries = 0; tries < SETTING_CAT_COUNT; tries++)
        {
            if (forward)
                sState->category = (sState->category + 1 == SETTING_CAT_COUNT) ? 0 : sState->category + 1;
            else
                sState->category = (sState->category == 0) ? SETTING_CAT_COUNT - 1 : sState->category - 1;
            if (!sState->wizardMode || CategoryHasWizardRows(sState->category))
                break;
        }
        RulesetMenu_BuildCategory(FALSE);
        return;
    }

    if (JOY_NEW(START_BUTTON))
    {
        if (sState->wizardMode)
        {
            PlaySE(SE_SELECT);
            sState->wizardConfirmOpen = TRUE;
            RulesetMenu_DrawWizardConfirm();
        }
        else
        {
            PlaySE(NudgeRulesetSetting(SETTING_PRESET, 1) ? SE_SELECT : SE_FAILURE);
            RulesetMenu_BuildCategory(FALSE);
        }
        return;
    }

    if (JOY_NEW(SELECT_BUTTON))
    {
        PlaySE(RestoreRulesetCategory(sState->category) ? SE_SELECT : SE_FAILURE);
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
        else if (settingId == SETTING_SPECIES_BANS || settingId == SETTING_RESTORE_ALL)
        {
            // action rows do not have left/right values
        }
        else
        {
            PlaySE(NudgeRulesetSetting(settingId, delta) ? SE_SELECT : SE_FAILURE);
            if (settingId == SETTING_PRESET)
                RulesetMenu_BuildCategory(FALSE);
            else
                RulesetMenu_RefreshVisibleRows();
        }
        return;
    }

    if (JOY_NEW(A_BUTTON))
    {
        if (settingId == SETTING_RUN_SEED)
        {
            if (GetRulesetSetting(SETTING_SEED_MODE) == SEEDMODE_MANUAL)
            {
                if (IsRulesetSettingEditable(SETTING_RUN_SEED))
                {
                    PlaySE(SE_SELECT);
                    sState->seedEditing = TRUE;
                    sState->seedNibble = 0;
                    sState->seedEditValue = GetRunSeed();
                    RulesetMenu_DrawDescription(RulesetMenu_CurrentRow());
                }
                else
                    PlaySE(SE_FAILURE);
            }
            else
            {
                PlaySE(RerollRunSeed() ? SE_SELECT : SE_FAILURE);
                RulesetMenu_RefreshVisibleRows();
            }
        }
        else if (settingId == SETTING_PRESET)
        {
            PlaySE(NudgeRulesetSetting(SETTING_PRESET, 1) ? SE_SELECT : SE_FAILURE);
            RulesetMenu_BuildCategory(FALSE);
        }
        else if (settingId == SETTING_SPECIES_BANS)
        {
            RulesetMenu_OpenSpeciesBans();
        }
        else if (settingId == SETTING_RESTORE_ALL)
        {
            PlaySE(SE_SELECT);
            sState->restoreConfirm = TRUE;
            RulesetMenu_DrawDescription(RulesetMenu_CurrentRow());
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
        // Phase 11A.6: consumed once here, never left set for a later,
        // ordinary Start Menu -> RULES open. Category 0 (SETTING_CAT_PRESET_SEED)
        // always has at least one wizard-visible row (SETTING_PRESET itself),
        // so no extra "find a non-empty starting category" search is needed.
        sState->wizardMode = sPendingWizardMode;
        sPendingWizardMode = FALSE;
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

// Phase 11A.6: entry point for the New-Game-only pre-run settings wizard
// (see docs/SPEC.md / the struct RulesetMenuState comment above). Call this
// in place of SetMainCallback2(CB2_NewGame) from the fresh-New-Game chain
// (src/main_menu.c Task_NewGameBirchSpeech_Cleanup) only. Do NOT call it
// from the Nuzlocke whiteout-retry path (src/nuzlocke_run_over.c) - that
// flow must keep going straight to CB2_NewGame so it silently reuses its
// already-stashed settings, untouched by this wizard.
//
// ResetRulesetSettings() here gives the wizard a fresh default preset and a
// newly auto-rolled seed to show/edit; RulesetSettings_MarkPreconfigured()
// (called from RulesetMenu_ProcessWizardConfirm on Start Game) then tells
// NewGameInitData() to skip its own unconditional reset, so the player's
// wizard edits survive into the actual save init.
void RulesetMenu_EnterNewGameWizard(void)
{
    ResetRulesetSettings();
    sPendingWizardMode = TRUE;
    SetMainCallback2(CB2_InitRulesetMenu);
}
