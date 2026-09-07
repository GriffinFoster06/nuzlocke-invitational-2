#ifndef GUARD_LEVEL_TO_CAP_H
#define GUARD_LEVEL_TO_CAP_H

// ============================================================================
// Phase 4 - "Level to Cap" (docs/SPEC.md "Level to Cap"). A party-menu command
// that jumps a Pokemon straight to the current progression cap (or, with the
// optional "Level to Breakpoint" QoL setting, to the next move / evolution
// level), then hands off to the existing party-menu move-catch-up loop so every
// skipped level-up move is still offered in chronological order.
//
// Policy only. The UI wiring lives in src/party_menu.c (CursorCb_LevelToCap).
// ============================================================================

struct Pokemon;

// TRUE when the party-menu entry should be shown for this mon: the setting is
// on, the mon isn't an egg or dead, and it is actually below its target level.
bool32 LevelToCap_IsAvailable(struct Pokemon *mon);

// The level this mon would be taken to, honoring SETTING_LEVEL_TO_BREAKPOINT.
// Always <= the progression cap. May equal the current level (nothing to do).
u8 LevelToCap_GetTargetLevel(struct Pokemon *mon);

// Set the mon's EXP (and recompute stats) so its level becomes `target`.
// No EVs, no friendship change, no evolution - evolution is handled separately
// so instant leveling can't bypass an evolution decision.
void LevelToCap_ApplyLevel(struct Pokemon *mon, u8 target);

#endif // GUARD_LEVEL_TO_CAP_H
