#ifndef GUARD_EVOLVE_MENU_H
#define GUARD_EVOLVE_MENU_H

// ============================================================================
// Phase 7 - "Evolve command" + "Evolution Assistance" (docs/SPEC.md).
//
// Evolve command: a party-menu action that evolves a Pokemon when its
// evolution is currently satisfied, keeping evolution player-controlled rather
// than silently triggered by Level to Cap.
//
// Evolution Assistance: when the ONLY thing standing between a Pokemon and its
// evolution is a move it never rolled in its randomized learnset, offer to
// teach exactly that move (nothing else - this is not a general Move Reminder).
//
// Policy only. The UI wiring is CursorCb_Evolve in src/party_menu.c.
// ============================================================================

struct Pokemon;

enum EvolveCheck
{
    EVOLVE_CHECK_NONE,       // no evolution is reachable right now
    EVOLVE_CHECK_READY,      // an EVO_MODE_NORMAL evolution is satisfied - just do it
    EVOLVE_CHECK_NEEDS_MOVE, // blocked only by a missing move; *outMove is what to teach
};

bool32 EvolveMenu_CommandEnabled(void);    // SETTING_EVOLVE_COMMAND
bool32 EvolveMenu_AssistEnabled(void);     // SETTING_EVOLUTION_ASSISTANCE

// Evaluate `mon`. When the result is EVOLVE_CHECK_NEEDS_MOVE, *outMove receives
// the evolution-required move to offer (only if EVOLUTION_ASSISTANCE is on).
enum EvolveCheck EvolveMenu_Check(struct Pokemon *mon, u16 *outMove);

// TRUE when the party-menu "EVOLVE" entry should be listed for this mon:
// the command is on, the mon isn't an egg or dead, and EvolveMenu_Check would
// return READY or NEEDS_MOVE.
bool32 EvolveMenu_IsAvailable(struct Pokemon *mon);

#endif // GUARD_EVOLVE_MENU_H
