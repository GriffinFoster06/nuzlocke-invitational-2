#ifndef GUARD_NUZLOCKE_H
#define GUARD_NUZLOCKE_H

// ============================================================================
// Phase 3 - Nuzlocke ruleset engine (docs/SPEC.md "Nuzlocke permadeath",
// "One encounter per location", "Dupes Clause", "Shiny Clause", "Nicknames",
// "No battle items", "Whiteout").
//
// Self-contained subsystem. Reads the Phase 1 ruleset settings via
// GetRulesetSetting(); owns persistent run state in gSaveBlock3Ptr->nuzlocke
// (see struct NuzlockeState in include/global.h). Touches the Phase 2
// randomizer only through its public Randomizer_WildSlotSpecies().
// ============================================================================

#include "wild_encounter.h"

struct Pokemon;

// ---- lifecycle ----
void Nuzlocke_ResetState(void);        // New Game: wipe run state (+ apply a pending retry)
void Nuzlocke_BeginRun(void);          // starter given: the run becomes active
void Nuzlocke_BeginRetry(void);        // run-over screen: stash ruleset, next New Game is a retry
bool32 Nuzlocke_RunIsActive(void);
bool32 Nuzlocke_RunIsOver(void);

// ---- setting predicates (thin wrappers, safe before a run starts) ----
bool32 Nuzlocke_PermadeathOn(void);
bool32 Nuzlocke_OneEncounterPerLocationOn(void);
bool32 Nuzlocke_DupesClauseOn(void);
bool32 Nuzlocke_ShinyClauseOn(void);
bool32 Nuzlocke_ForcedNicknamesOn(void);   // NICK_MANDATORY or NICK_STRICT
bool32 Nuzlocke_StrictNicknamesOn(void);   // NICK_STRICT: the prompt cannot be escaped
bool16 AreNicknamesForced(void);           // script special wrapper for Nuzlocke_ForcedNicknamesOn

// ---- location tags (docs/SPEC.md "One encounter per location") ----
u32 Nuzlocke_CurrentLocationTag(void);
bool32 Nuzlocke_LocationIsUsed(u32 tag);
bool32 Nuzlocke_LocationResolvedByCatch(u32 tag);
void Nuzlocke_MarkLocationUsed(u32 tag, bool32 byCatch);
void Nuzlocke_ClearLocation(u32 tag);      // debug menu

// Called once a wild / scripted encounter's battle is being set up, with the
// opposing mon already in gParties[B_TRAINER_OPPONENT_A][0].
void Nuzlocke_NoteWildEncounterStart(bool32 scripted);
// Battle-end hooks (read gBattleOutcome; the map is still the encounter's map).
void Nuzlocke_HandleWildBattleEnd(void);
void Nuzlocke_HandleScriptedBattleEnd(void);
void Nuzlocke_HandleSafariBattleEnd(void);
// TRUE if the player may throw a ball at the current wild encounter.
bool32 Nuzlocke_CanCatchCurrentEncounter(void);

// ---- Dupes Clause (docs/SPEC.md "Dupes Clause") ----
void Nuzlocke_MarkFamilyOwned(enum Species species);
bool32 Nuzlocke_IsFamilyOwned(enum Species species);
// A Pokemon was obtained. Marks its whole evolutionary family owned and, for a
// non-wild-catch acquisition during an active run, consumes the current
// location (script gifts, fossil revival, gift/daycare eggs).
void Nuzlocke_OnMonObtained(struct Pokemon *mon, bool32 fromWildCatch);
// TRUE if a fresh route encounter should be re-rolled to dodge owned families.
bool32 Nuzlocke_DupesRerollActiveHere(void);

// ---- permadeath (docs/SPEC.md "Nuzlocke permadeath") ----
bool32 Nuzlocke_MonIsDead(struct Pokemon *mon);
void Nuzlocke_MarkMonDead(struct Pokemon *mon);       // field poison, debug
void Nuzlocke_ProcessPostBattleDeaths(void);          // ReturnFromBattleToOverworld
u32 Nuzlocke_GetDeathCount(void);
u32 Nuzlocke_CountLocationsCaught(void);

// ---- no battle items (docs/SPEC.md "No battle items") ----
bool32 Nuzlocke_BattleItemsBlocked(void);   // current battle: Bag combat items disabled
bool32 Nuzlocke_BattleBagBallsOnly(void);   // current battle: restrict Bag to the Ball pocket

// ---- whiteout / run over (docs/SPEC.md "Whiteout") ----
bool32 Nuzlocke_ShouldEndRunOnWhiteout(void);
void Nuzlocke_SetRunOver(void);
void Nuzlocke_FieldCB_RunOver(void);        // gFieldCallback after the whiteout warp
// Script specials backing the run-over screen (data/scripts/nuzlocke.inc).
void BufferNuzlockeRunOverStats(void);
void StartNuzlockeNewAttempt(void);

#endif // GUARD_NUZLOCKE_H
