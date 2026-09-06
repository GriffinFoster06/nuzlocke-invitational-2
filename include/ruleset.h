#ifndef GUARD_RULESET_H
#define GUARD_RULESET_H

#include "constants/ruleset.h"

// Metadata for one setting. One row per SettingId, in enum order, in
// src/data/ruleset.h. `defaultValue` is the Invitational 2 Solo value - the
// other named presets are expressed as sparse overrides of these defaults.
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
// differs from the Invitational 2 Solo default".
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
void SetRulesetSetting(u32 settingId, u8 value);   // clamps, stores, recomputes displayed preset
void NudgeRulesetSetting(u32 settingId, s32 delta); // menu helper: step + wrap within range

// ---- presets ----
void ApplyRulesetPreset(u32 preset);              // fill defaults + overrides, update displayed preset
void RestoreRulesetCategory(u32 category);        // reset one category to the last named preset
void RestoreRulesetAll(void);                     // reset everything to the last named preset
u32 RulesetSettings_RecomputeDisplayedPreset(void); // returns + stores the matching preset (or CUSTOM)
u32 GetDisplayedRulesetPreset(void);

// ---- locking ----
bool8 IsRulesetSettingEditable(u32 settingId);
void SetRulesetRunStarted(bool8 started);          // Phase 2 will call this
void SetRulesetRunActive(bool8 active);            // Phase 3 will call this

// ---- run seed ----
u32 GetRunSeed(void);
void SetRunSeed(u32 seed);
void RerollRunSeed(void);

// ---- lifecycle ----
void ResetRulesetSettings(void);                  // New Game: re-apply the default preset

#endif // GUARD_RULESET_H
