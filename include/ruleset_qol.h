#ifndef GUARD_RULESET_QOL_H
#define GUARD_RULESET_QOL_H

// ============================================================================
// Phase 8 - thin predicates over the QoL / economy ruleset settings
// (docs/SPEC.md "Catch rates", "R-button Ball shortcut", "Unlimited money").
// Same convention as src/nuzlocke.c's Nuzlocke_*On() helpers: keep the
// GetRulesetSetting() calls out of the big upstream files.
// ============================================================================

#include "constants/ruleset.h"

// docs/SPEC.md "Catch rates". Integer multiplier applied to the computed
// capture odds: 1 (Vanilla), 2 (Moderate), 4 (Large). Guaranteed is a separate
// predicate because it short-circuits the whole calculation.
u32 Ruleset_CatchOddsMultiplier(void);
bool32 Ruleset_CatchGuaranteed(void);

// docs/SPEC.md "R-button Ball shortcut".
bool32 Ruleset_RButtonBallShortcutOn(void);

// docs/SPEC.md "Unlimited money".
bool32 Ruleset_UnlimitedMoneyOn(void);
// Call after the ruleset is applied (New Game) or the setting is switched on:
// tops the wallet up so the counter reads as "not a resource".
void Ruleset_ApplyUnlimitedMoneyGrant(void);

// docs/SPEC.md "999 Poke Ball NPC". Script special backing
// OldaleTown_EventScript_BallNpc999. Sets gSpecialVar_Result:
//   0 = feature off, 1 = just handed over, 2 = already claimed, 3 = no bag room.
void TryGiveBallNpc999(void);

#endif // GUARD_RULESET_QOL_H
