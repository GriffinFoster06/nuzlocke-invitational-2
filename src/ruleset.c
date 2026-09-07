// ============================================================================
// Nuzlocke-Randomizer ruleset: persistent settings model.
//
// Phase 1 scaffolding. This file owns the data model behind docs/SPEC.md's
// "Settings behavior": a flat value store in SaveBlock3, a metadata table
// (src/data/ruleset.h), the six presets, and the "any edit -> Custom" logic.
//
// NOTHING in gameplay reads these values yet. Phases 2+ add consumers that call
// GetRulesetSetting(); until then this only feeds the debug settings screen.
// ============================================================================

#include "global.h"
#include "learnset_gen.h"
#include "power_score.h"
#include "random.h"
#include "ruleset.h"
#include "ruleset_qol.h"

#include "data/ruleset.h"

STATIC_ASSERT(NUM_SETTINGS <= 255, RulesetTooManySettings);
STATIC_ASSERT(sizeof(struct RulesetSettings) < 400, RulesetSettingsUnexpectedlyLarge);

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Expand a preset into a full NUM_SETTINGS value array: descriptor defaults
// (== Invitational 2 Solo), then that preset's sparse overrides.
static void ExpandPreset(u32 preset, u8 *out)
{
    u32 i;

    for (i = 0; i < NUM_SETTINGS; i++)
        out[i] = sSettingDescriptors[i].defaultValue;

    if (preset < RULESET_NAMED_PRESET_COUNT)
    {
        const struct RulesetPresetOverride *ov = sPresetOverrideTables[preset];
        u32 count = sPresetOverrideCounts[preset];

        for (i = 0; i < count; i++)
            out[ov[i].settingId] = ov[i].value;
    }
}

// Which named preset does the current config exactly match? RULESET_PRESET_CUSTOM
// if none. Pure-display settings and the virtual preset/seed rows don't count.
static u32 ComputeMatchingPreset(void)
{
    u8 expanded[NUM_SETTINGS];
    u32 preset, i;

    for (preset = 0; preset < RULESET_NAMED_PRESET_COUNT; preset++)
    {
        bool32 match = TRUE;

        ExpandPreset(preset, expanded);
        for (i = 0; i < NUM_SETTINGS; i++)
        {
            if (i == SETTING_PRESET || i == SETTING_RUN_SEED)
                continue;
            if (sSettingDescriptors[i].flags & SETTING_FLAG_NOT_RULESET)
                continue;
            if (gSaveBlock3Ptr->ruleset.values[i] != expanded[i])
            {
                match = FALSE;
                break;
            }
        }

        if (match)
            return preset;
    }

    return RULESET_PRESET_CUSTOM;
}

// Overwrite the whole value store from a preset. Does NOT call EnsureInitialized
// (it is itself part of initialization).
static void ApplyPresetInternal(u32 preset)
{
    struct RulesetSettings *r = &gSaveBlock3Ptr->ruleset;
    u8 expanded[NUM_SETTINGS];
    u32 i;

    ExpandPreset(preset, expanded);
    for (i = 0; i < NUM_SETTINGS; i++)
        r->values[i] = expanded[i];

    PowerScore_Invalidate();
    LearnsetGen_Invalidate();

    if (preset < RULESET_NAMED_PRESET_COUNT)
    {
        r->displayedPreset = preset;
        r->lastNamedPreset = preset;
        r->values[SETTING_PRESET] = preset;
    }
    else
    {
        r->displayedPreset = ComputeMatchingPreset();
    }
}

// There is no save-migration system in this fork (see docs/PHASES.md notes), so
// defaults are applied lazily: any store whose version tag isn't current is
// (re)initialized to the default preset on first access.
static void RulesetSettings_EnsureInitialized(void)
{
    struct RulesetSettings *r = &gSaveBlock3Ptr->ruleset;

    if (r->rulesetVersion == RULESET_VERSION)
        return;

    ApplyPresetInternal(RULESET_DEFAULT_PRESET);
    r->lastNamedPreset = RULESET_DEFAULT_PRESET;
    r->runStarted = FALSE;
    r->runActive = FALSE;
    if (r->runSeed == 0)
        r->runSeed = Random32();
    r->rulesetVersion = RULESET_VERSION;
}

// ---------------------------------------------------------------------------
// Metadata access
// ---------------------------------------------------------------------------

const struct SettingDescriptor *GetSettingDescriptor(u32 settingId)
{
    if (settingId >= NUM_SETTINGS)
        settingId = 0;
    return &sSettingDescriptors[settingId];
}

const u8 *GetSettingCategoryName(u32 category)
{
    if (category >= SETTING_CAT_COUNT)
        category = 0;
    return sSettingCategoryNames[category];
}

const u8 *GetRulesetPresetName(u32 preset)
{
    if (preset >= RULESET_PRESET_COUNT)
        preset = RULESET_PRESET_CUSTOM;
    return sLbl_Preset[preset];
}

u32 CountSettingsInCategory(u32 category)
{
    u32 i, count = 0;

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (sSettingDescriptors[i].category == category)
            count++;
    }
    return count;
}

// Debug-time sanity check for the descriptor table (guards against a
// designated-initializer row being silently omitted / zeroed).
bool8 RulesetTablesAreValid(void)
{
    u32 i;

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        const struct SettingDescriptor *d = &sSettingDescriptors[i];

        if (d->name == NULL || d->description == NULL)
            return FALSE;
        if (d->category >= SETTING_CAT_COUNT)
            return FALSE;
        if (d->minValue > d->maxValue)
            return FALSE;
        if (d->defaultValue < d->minValue || d->defaultValue > d->maxValue)
            return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Value get / set
// ---------------------------------------------------------------------------

u8 GetRulesetSetting(u32 settingId)
{
    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS)
        return 0;
    return gSaveBlock3Ptr->ruleset.values[settingId];
}

// Resolves the value a menu should render for this row (the preset row shows the
// currently displayed preset, which may be Custom).
u8 GetSettingDisplayValue(u32 settingId)
{
    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS)
        return 0;
    if (settingId == SETTING_PRESET)
        return gSaveBlock3Ptr->ruleset.displayedPreset;
    return gSaveBlock3Ptr->ruleset.values[settingId];
}

void SetRulesetSetting(u32 settingId, u8 value)
{
    const struct SettingDescriptor *d;

    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS)
        return;

    if (settingId == SETTING_PRESET)
    {
        ApplyRulesetPreset(value);
        return;
    }

    if (!IsRulesetSettingEditable(settingId))
        return;

    d = &sSettingDescriptors[settingId];
    if (value < d->minValue)
        value = d->minValue;
    if (value > d->maxValue)
        value = d->maxValue;

    gSaveBlock3Ptr->ruleset.values[settingId] = value;
    RulesetSettings_RecomputeDisplayedPreset();
    // A species-pool / ability / form toggle may have moved; the Phase 2 power
    // cache re-checks its signature on the next EnsureBuilt.
    PowerScore_Invalidate();
    LearnsetGen_Invalidate();

    // docs/SPEC.md "Unlimited money": switching it on mid-run tops the wallet up.
    if (settingId == SETTING_UNLIMITED_MONEY && value != 0)
        Ruleset_ApplyUnlimitedMoneyGrant();
}

// Menu helper: step one option in the direction of `delta` (sign only), wrapping
// within the setting's range. Special-cased for the virtual preset/seed rows.
void NudgeRulesetSetting(u32 settingId, s32 delta)
{
    const struct SettingDescriptor *d;
    s32 range, value;

    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS || delta == 0)
        return;

    if (settingId == SETTING_PRESET)
    {
        s32 preset = gSaveBlock3Ptr->ruleset.displayedPreset;

        if (preset >= (s32)RULESET_NAMED_PRESET_COUNT)   // currently Custom
            preset = gSaveBlock3Ptr->ruleset.lastNamedPreset;

        preset += (delta > 0) ? 1 : -1;
        if (preset < 0)
            preset = RULESET_NAMED_PRESET_COUNT - 1;
        else if (preset >= (s32)RULESET_NAMED_PRESET_COUNT)
            preset = 0;

        ApplyRulesetPreset(preset);
        return;
    }

    if (settingId == SETTING_RUN_SEED)
        return;

    if (!IsRulesetSettingEditable(settingId))
        return;

    d = &sSettingDescriptors[settingId];
    range = (s32)d->maxValue - (s32)d->minValue + 1;
    if (range <= 1)
        return;

    value = (s32)gSaveBlock3Ptr->ruleset.values[settingId] - (s32)d->minValue;
    value += (delta > 0) ? 1 : -1;
    value %= range;
    if (value < 0)
        value += range;

    SetRulesetSetting(settingId, (u8)(value + d->minValue));
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void ApplyRulesetPreset(u32 preset)
{
    RulesetSettings_EnsureInitialized();
    if (preset >= RULESET_NAMED_PRESET_COUNT)
        return;
    ApplyPresetInternal(preset);
}

void RestoreRulesetCategory(u32 category)
{
    u8 expanded[NUM_SETTINGS];
    u32 i;

    RulesetSettings_EnsureInitialized();
    ExpandPreset(gSaveBlock3Ptr->ruleset.lastNamedPreset, expanded);

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (sSettingDescriptors[i].category != category)
            continue;
        if (i == SETTING_PRESET || i == SETTING_RUN_SEED)
            continue;
        gSaveBlock3Ptr->ruleset.values[i] = expanded[i];
    }

    RulesetSettings_RecomputeDisplayedPreset();
}

void RestoreRulesetAll(void)
{
    RulesetSettings_EnsureInitialized();
    ApplyPresetInternal(gSaveBlock3Ptr->ruleset.lastNamedPreset);
}

u32 RulesetSettings_RecomputeDisplayedPreset(void)
{
    RulesetSettings_EnsureInitialized();
    gSaveBlock3Ptr->ruleset.displayedPreset = ComputeMatchingPreset();
    return gSaveBlock3Ptr->ruleset.displayedPreset;
}

u32 GetDisplayedRulesetPreset(void)
{
    RulesetSettings_EnsureInitialized();
    return gSaveBlock3Ptr->ruleset.displayedPreset;
}

// ---------------------------------------------------------------------------
// Locking
// ---------------------------------------------------------------------------

bool8 IsRulesetSettingEditable(u32 settingId)
{
    const struct SettingDescriptor *d;

    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS)
        return FALSE;

    d = &sSettingDescriptors[settingId];
    if (d->flags & SETTING_FLAG_READ_ONLY)
        return FALSE;
    if (d->lockClass == SETTING_LOCK_GENERATION && gSaveBlock3Ptr->ruleset.runStarted)
        return FALSE;
    if (d->lockClass == SETTING_LOCK_RULES && gSaveBlock3Ptr->ruleset.runActive)
        return FALSE;
    return TRUE;
}

void SetRulesetRunStarted(bool8 started)
{
    RulesetSettings_EnsureInitialized();
    gSaveBlock3Ptr->ruleset.runStarted = started ? TRUE : FALSE;
}

void SetRulesetRunActive(bool8 active)
{
    RulesetSettings_EnsureInitialized();
    gSaveBlock3Ptr->ruleset.runActive = active ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// Run seed
// ---------------------------------------------------------------------------

u32 GetRunSeed(void)
{
    RulesetSettings_EnsureInitialized();
    return gSaveBlock3Ptr->ruleset.runSeed;
}

void SetRunSeed(u32 seed)
{
    RulesetSettings_EnsureInitialized();
    gSaveBlock3Ptr->ruleset.runSeed = seed;
}

void RerollRunSeed(void)
{
    RulesetSettings_EnsureInitialized();
    if (!gSaveBlock3Ptr->ruleset.runStarted)
        gSaveBlock3Ptr->ruleset.runSeed = Random32();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Called from NewGameInitData(): wipe the version tag and re-run lazy init, so a
// New Game always returns to the default preset with a fresh seed.
void ResetRulesetSettings(void)
{
    struct RulesetSettings *r = &gSaveBlock3Ptr->ruleset;

    r->rulesetVersion = 0;
    r->runSeed = 0;
    r->runStarted = FALSE;
    r->runActive = FALSE;
    RulesetSettings_EnsureInitialized();
}
