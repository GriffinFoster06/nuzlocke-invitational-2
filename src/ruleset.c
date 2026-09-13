// ============================================================================
// Nuzlocke-Randomizer ruleset: persistent settings model.
//
// This file owns the data model behind docs/SPEC.md's "Settings behavior": a
// flat value store in SaveBlock3, metadata, the three named presets, explicit
// Custom identity, protected-run locks, seeds, and exact-species bans.
// ============================================================================

#include "global.h"
#include "ability_gen.h"
#include "learnset_gen.h"
#include "power_score.h"
#include "randomizer.h"
#include "random.h"
#include "ruleset.h"
#include "ruleset_field.h"
#include "ruleset_qol.h"

#include "data/ruleset.h"

STATIC_ASSERT(NUM_SETTINGS <= 255, RulesetTooManySettings);
STATIC_ASSERT(sizeof(struct RulesetSettings) < 400, RulesetSettingsUnexpectedlyLarge);

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Expand a preset into a full NUM_SETTINGS value array: Recommended descriptor
// defaults followed by that named preset's sparse overrides.
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
    memset(r->speciesBans, 0, sizeof(r->speciesBans));

    PowerScore_Invalidate();
    LearnsetGen_Invalidate();
    AbilityGen_Invalidate();
    Randomizer_InvalidateTms();

    if (preset < RULESET_NAMED_PRESET_COUNT)
    {
        r->displayedPreset = preset;
        r->lastNamedPreset = preset;
        r->values[SETTING_PRESET] = preset;
    }
    else
    {
        r->displayedPreset = RULESET_PRESET_CUSTOM;
        r->values[SETTING_PRESET] = RULESET_PRESET_CUSTOM;
    }
}

// There is no save-migration system in this fork (see docs/PHASES.md notes), so
// defaults are applied lazily: any store whose version tag isn't current is
// (re)initialized to the default preset on first access.
static void RulesetSettings_EnsureInitialized(void)
{
    struct RulesetSettings *r = &gSaveBlock3Ptr->ruleset;

    if (r->rulesetVersion == RULESET_VERSION)
    {
        // Version 3 introduced an explicit initialization bit so seed zero is
        // distinguishable from an old/uninitialized tail field.
        if (!r->seedInitialized)
        {
            r->runSeed = Random32();
            r->seedInitialized = TRUE;
        }
        return;
    }

    ApplyPresetInternal(RULESET_DEFAULT_PRESET);
    r->lastNamedPreset = RULESET_DEFAULT_PRESET;
    r->runStarted = FALSE;
    r->runActive = FALSE;
    // docs/SPEC.md "Infinite Repel": available and toggled on by default.
    r->infiniteRepelActive = (r->values[SETTING_INFINITE_REPEL] != 0);
    r->runSeed = Random32();
    r->seedInitialized = TRUE;
    r->randomizerVersion = RANDOMIZER_VERSION;
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
        if (sSettingDescriptors[i].category == category
         && !(sSettingDescriptors[i].flags & SETTING_FLAG_HIDDEN))
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

bool8 SetRulesetSetting(u32 settingId, u8 value)
{
    const struct SettingDescriptor *d;

    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS)
        return FALSE;

    if (settingId == SETTING_PRESET)
    {
        return ApplyRulesetPreset(value);
    }

    if (!IsRulesetSettingEditable(settingId))
        return FALSE;
    if (settingId == SETTING_RUN_SEED || settingId == SETTING_SPECIES_BANS
     || settingId == SETTING_RESTORE_ALL)
        return FALSE;

    d = &sSettingDescriptors[settingId];
    if (value < d->minValue)
        value = d->minValue;
    if (value > d->maxValue)
        value = d->maxValue;

    if (gSaveBlock3Ptr->ruleset.values[settingId] == value)
        return TRUE;

    gSaveBlock3Ptr->ruleset.values[settingId] = value;
    gSaveBlock3Ptr->ruleset.displayedPreset = RULESET_PRESET_CUSTOM;
    gSaveBlock3Ptr->ruleset.values[SETTING_PRESET] = RULESET_PRESET_CUSTOM;
    // A species-pool / ability / form toggle may have moved; the Phase 2 power
    // cache re-checks its signature on the next EnsureBuilt.
    PowerScore_Invalidate();
    LearnsetGen_Invalidate();
    AbilityGen_Invalidate();
    Randomizer_InvalidateTms();

    // docs/SPEC.md "Unlimited money": switching it on mid-run tops the wallet up.
    if (settingId == SETTING_UNLIMITED_MONEY && value != 0)
        Ruleset_ApplyUnlimitedMoneyGrant();

    // SPEC "Portable healing" / "Infinite Repel": hand over or reclaim the
    // SELECT-registerable key item to match the new setting value.
    if (settingId == SETTING_PORTABLE_HEAL || settingId == SETTING_INFINITE_REPEL)
        Ruleset_GrantFieldKeyItems();
    return TRUE;
}

// Menu helper: step one option in the direction of `delta` (sign only), wrapping
// within the setting's range. Special-cased for the virtual preset/seed rows.
bool8 NudgeRulesetSetting(u32 settingId, s32 delta)
{
    const struct SettingDescriptor *d;
    s32 range, value;

    RulesetSettings_EnsureInitialized();
    if (settingId >= NUM_SETTINGS || delta == 0)
        return FALSE;

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

        return ApplyRulesetPreset(preset);
    }

    if (settingId == SETTING_RUN_SEED)
        return FALSE;

    if (!IsRulesetSettingEditable(settingId))
        return FALSE;

    d = &sSettingDescriptors[settingId];
    range = (s32)d->maxValue - (s32)d->minValue + 1;
    if (range <= 1)
        return FALSE;

    value = (s32)gSaveBlock3Ptr->ruleset.values[settingId] - (s32)d->minValue;
    value += (delta > 0) ? 1 : -1;
    value %= range;
    if (value < 0)
        value += range;

    return SetRulesetSetting(settingId, (u8)(value + d->minValue));
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

static bool8 ValueChangeIsAllowed(u32 settingId, u8 newValue)
{
    if (settingId == SETTING_PRESET || settingId == SETTING_RUN_SEED
     || (sSettingDescriptors[settingId].flags & SETTING_FLAG_NOT_RULESET))
        return TRUE;
    if (gSaveBlock3Ptr->ruleset.values[settingId] == newValue)
        return TRUE;
    return IsRulesetSettingEditable(settingId);
}

static bool8 SpeciesBansCanBeCleared(void)
{
    u32 i;

    for (i = 0; i < sizeof(gSaveBlock3Ptr->ruleset.speciesBans); i++)
    {
        if (gSaveBlock3Ptr->ruleset.speciesBans[i] != 0)
            return !gSaveBlock3Ptr->ruleset.runStarted;
    }
    return TRUE;
}

bool8 ApplyRulesetPreset(u32 preset)
{
    u8 expanded[NUM_SETTINGS];
    u32 i;

    RulesetSettings_EnsureInitialized();
    if (preset >= RULESET_NAMED_PRESET_COUNT)
        return FALSE;
    // Presets and whole-configuration restores are aggregate mutations. Once
    // either protected lifecycle has begun, do not permit them to clear bans,
    // rewrite hidden slots, or merely relabel the live configuration.
    if (gSaveBlock3Ptr->ruleset.runStarted || gSaveBlock3Ptr->ruleset.runActive)
        return FALSE;
    ExpandPreset(preset, expanded);
    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (!ValueChangeIsAllowed(i, expanded[i]))
            return FALSE;
    }
    if (!SpeciesBansCanBeCleared())
        return FALSE;
    ApplyPresetInternal(preset);
    return TRUE;
}

bool8 RestoreRulesetCategory(u32 category)
{
    u8 expanded[NUM_SETTINGS];
    u32 i;

    RulesetSettings_EnsureInitialized();
    if (category >= SETTING_CAT_COUNT)
        return FALSE;
    ExpandPreset(gSaveBlock3Ptr->ruleset.lastNamedPreset, expanded);

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (sSettingDescriptors[i].category != category)
            continue;
        if (i == SETTING_PRESET || i == SETTING_RESTORE_ALL)
            continue;
        // A category restore is also aggregate: a protected row makes the
        // entire action unavailable, even when that row already equals its
        // preset value. This prevents a locked action from changing Custom
        // identity or hidden values around the lock.
        if (!IsRulesetSettingEditable(i))
            return FALSE;
    }
    if (category == SETTING_CAT_SPECIES_POOL && !SpeciesBansCanBeCleared())
        return FALSE;

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        if (sSettingDescriptors[i].category != category)
            continue;
        if (i == SETTING_PRESET || i == SETTING_RUN_SEED)
            continue;
        gSaveBlock3Ptr->ruleset.values[i] = expanded[i];
    }
    if (category == SETTING_CAT_SPECIES_POOL)
        memset(gSaveBlock3Ptr->ruleset.speciesBans, 0, sizeof(gSaveBlock3Ptr->ruleset.speciesBans));

    gSaveBlock3Ptr->ruleset.displayedPreset = RULESET_PRESET_CUSTOM;
    gSaveBlock3Ptr->ruleset.values[SETTING_PRESET] = RULESET_PRESET_CUSTOM;
    PowerScore_Invalidate();
    LearnsetGen_Invalidate();
    AbilityGen_Invalidate();
    Randomizer_InvalidateTms();
    return TRUE;
}

bool8 RestoreRulesetAll(void)
{
    RulesetSettings_EnsureInitialized();
    return ApplyRulesetPreset(gSaveBlock3Ptr->ruleset.lastNamedPreset);
}

u32 RulesetSettings_RecomputeDisplayedPreset(void)
{
    RulesetSettings_EnsureInitialized();
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

bool8 SetRunSeed(u32 seed)
{
    RulesetSettings_EnsureInitialized();
    if (gSaveBlock3Ptr->ruleset.runStarted)
        return FALSE;
    gSaveBlock3Ptr->ruleset.runSeed = seed;
    gSaveBlock3Ptr->ruleset.seedInitialized = TRUE;
    LearnsetGen_Invalidate();
    AbilityGen_Invalidate();
    Randomizer_InvalidateTms();
    return TRUE;
}

bool8 RerollRunSeed(void)
{
    RulesetSettings_EnsureInitialized();
    if (gSaveBlock3Ptr->ruleset.runStarted)
        return FALSE;
    return SetRunSeed(Random32());
}

u16 GetSavedRulesetVersion(void)
{
    RulesetSettings_EnsureInitialized();
    return gSaveBlock3Ptr->ruleset.rulesetVersion;
}

u16 GetSavedRandomizerVersion(void)
{
    RulesetSettings_EnsureInitialized();
    return gSaveBlock3Ptr->ruleset.randomizerVersion;
}

bool8 Ruleset_IsSpeciesBanned(enum Species species)
{
    RulesetSettings_EnsureInitialized();
    if (species <= SPECIES_NONE || species >= NUM_SPECIES)
        return FALSE;
    return (gSaveBlock3Ptr->ruleset.speciesBans[species >> 3] >> (species & 7)) & 1;
}

bool8 Ruleset_SetSpeciesBanned(enum Species species, bool8 banned)
{
    u8 mask;

    RulesetSettings_EnsureInitialized();
    if (species <= SPECIES_NONE || species >= NUM_SPECIES || gSaveBlock3Ptr->ruleset.runStarted)
        return FALSE;
    mask = 1 << (species & 7);
    if (banned)
        gSaveBlock3Ptr->ruleset.speciesBans[species >> 3] |= mask;
    else
        gSaveBlock3Ptr->ruleset.speciesBans[species >> 3] &= ~mask;
    gSaveBlock3Ptr->ruleset.displayedPreset = RULESET_PRESET_CUSTOM;
    gSaveBlock3Ptr->ruleset.values[SETTING_PRESET] = RULESET_PRESET_CUSTOM;
    PowerScore_Invalidate();
    return TRUE;
}

bool8 Ruleset_ClearSpeciesBans(void)
{
    RulesetSettings_EnsureInitialized();
    if (gSaveBlock3Ptr->ruleset.runStarted)
        return FALSE;
    memset(gSaveBlock3Ptr->ruleset.speciesBans, 0, sizeof(gSaveBlock3Ptr->ruleset.speciesBans));
    gSaveBlock3Ptr->ruleset.displayedPreset = RULESET_PRESET_CUSTOM;
    gSaveBlock3Ptr->ruleset.values[SETTING_PRESET] = RULESET_PRESET_CUSTOM;
    PowerScore_Invalidate();
    return TRUE;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Called from NewGameInitData(): wipe the version tag and re-run lazy init, so a
// New Game always returns to the default preset with a fresh seed.
void ResetRulesetSettings(void)
{
    struct RulesetSettings *r = &gSaveBlock3Ptr->ruleset;

    memset(r, 0, sizeof(*r));
    RulesetSettings_EnsureInitialized();
}
