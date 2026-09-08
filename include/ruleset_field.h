#ifndef GUARD_RULESET_FIELD_H
#define GUARD_RULESET_FIELD_H

// ============================================================================
// Phase 9.5 Tier 2 - field / QoL ruleset predicates and the player-facing
// "RULES" submenu (docs/SPEC.md "Universal TM compatibility", "Move Tutors",
// "Portable healing", "Infinite Repel", "Set battle style").
//
// Same convention as src/ruleset_qol.c and src/nuzlocke.c's *_On() helpers:
// keep the GetRulesetSetting() calls out of the big upstream files.
// ============================================================================

#include "constants/ruleset.h"

struct Pokemon;

// docs/SPEC.md "Universal TM compatibility" / "Move Tutors". When on, every
// Pokemon can be taught every (randomized) TM / Tutor move regardless of its
// canonical teachable learnset.
bool32 Ruleset_UniversalTmCompatOn(void);
bool32 Ruleset_UniversalTutorCompatOn(void);

// docs/SPEC.md "Portable healing". Menu command that restores HP/PP/status
// (never revives the dead).
bool32 Ruleset_PortableHealOn(void);
void Ruleset_DoPortableHeal(void);

// docs/SPEC.md "Infinite Repel". *On == the feature is enabled at all;
// *Active == enabled AND the player currently has it toggled on, i.e. ordinary
// wild encounters are suppressed right now.
bool32 Ruleset_InfiniteRepelOn(void);
bool32 Ruleset_InfiniteRepelActive(void);
void Ruleset_SetInfiniteRepelActive(bool32 active);

// docs/SPEC.md "Set battle style". When on, no free switch is offered after
// defeating an opposing Pokemon, and the Options battle-style row is locked.
bool32 Ruleset_ForceSetBattleStyleOn(void);
void Ruleset_ApplyForcedBattleStyle(void);   // New Game: sync optionsBattleStyle

// The "RULES" start-menu submenu, drawn over the field. Caller has already
// hidden the start menu and frozen object events (see StartMenuRulesCallback).
void RulesetField_ShowMenu(void);

#endif // GUARD_RULESET_FIELD_H
