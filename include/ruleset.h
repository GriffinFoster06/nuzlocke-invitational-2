#ifndef GUARD_RULESET_H
#define GUARD_RULESET_H

#include "constants/ruleset.h"

// Metadata for one setting. One row per SettingId, in enum order, in
// src/data/ruleset.h. `defaultValue` is the Recommended value; the other named
// presets are expressed as sparse overrides of those defaults.
struct SettingDescriptor
{
    const u8 *name;
    const u8 *description;
    const u8 *const *optionLabels;  // BOOL/ENUM: labels[value]. NUMBER: NULL.
    u8 category;                    // enum SettingCategory
    u8 lockClass;                  // enum SettingLock
    u8 type;                       // enum SettingType
    u8 minValue;
    u8 maxValue;
    u8 defaultValue;
    u8 flags;                     // SETTING_FLAG_*
};

// One entry of a preset's override list: "for this preset, this setting
// differs from the Recommended default".
struct RulesetPresetOverride
{
    u16 settingId;
    u8 value;
};

// ---- descriptor / metadata access ----
const struct SettingDescriptor *GetSettingDescriptor(u32 settingId);
const u8 *GetSettingCategoryName(u32 category);
const u8 *GetRulesetPresetName(u32 preset);
u8 GetSettingDisplayValue(u32 settingId);          // resolves the string/number to show
u32 CountSettingsInCategory(u32 category);
bool8 RulesetTablesAreValid(void);                 // debug-time descriptor-table sanity check

// ---- value get / set ----
u8 GetRulesetSetting(u32 settingId);
bool8 SetRulesetSetting(u32 settingId, u8 value);  // false when invalid or locked
bool8 NudgeRulesetSetting(u32 settingId, s32 delta);

// ---- presets ----
bool8 ApplyRulesetPreset(u32 preset);
bool8 RestoreRulesetCategory(u32 category);
bool8 RestoreRulesetAll(void);
u32 RulesetSettings_RecomputeDisplayedPreset(void); // compatibility: preserves explicit named/Custom identity
u32 GetDisplayedRulesetPreset(void);

// ---- locking ----
bool8 IsRulesetSettingEditable(u32 settingId);
void SetRulesetRunStarted(bool8 started);
void SetRulesetRunActive(bool8 active);

// ---- run seed ----
u32 GetRunSeed(void);
bool8 SetRunSeed(u32 seed);
bool8 RerollRunSeed(void);
u16 GetSavedRulesetVersion(void);
u16 GetSavedRandomizerVersion(void);

// Exact-form, per-save generation bans. Bans never imply relatives/forms.
bool8 Ruleset_IsSpeciesBanned(enum Species species);
bool8 Ruleset_SetSpeciesBanned(enum Species species, bool8 banned);
bool8 Ruleset_ClearSpeciesBans(void);

// ---- lifecycle ----
void ResetRulesetSettings(void);                  // New Game: re-apply the default preset

#endif // GUARD_RULESET_H
