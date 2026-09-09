#ifndef GUARD_EVOLVE_MENU_H
#define GUARD_EVOLVE_MENU_H

// ============================================================================
// Phase 7 - "Evolve command" (docs/SPEC.md).
//
// Evolve command: a party-menu action that evolves a Pokemon when its
// evolution is currently satisfied, keeping evolution player-controlled rather
// than silently triggered by Level to Cap.
//
// Evolution Assistance (teach the one move an evolution needs) was removed in
// the Phase 9.5 sweep - no evolution in the dataset requires a specific move
// any more, so it had no reachable use case. See docs/SPEC.md.
//
// Policy only. The UI wiring is CursorCb_Evolve in src/party_menu.c.
// ============================================================================

struct Pokemon;

enum EvolveCheck
{
    EVOLVE_CHECK_NONE,       // no evolution is reachable right now
    EVOLVE_CHECK_READY,      // an EVO_MODE_NORMAL evolution is satisfied - just do it
};

bool32 EvolveMenu_CommandEnabled(void);    // SETTING_EVOLVE_COMMAND

// Evaluate `mon`: READY when an EVO_MODE_NORMAL evolution is satisfied now.
enum EvolveCheck EvolveMenu_Check(struct Pokemon *mon);

// TRUE when the party-menu "EVOLVE" entry should be listed for this mon:
// the command is on, the mon isn't an egg or dead, and EvolveMenu_Check would
// return READY.
bool32 EvolveMenu_IsAvailable(struct Pokemon *mon);

#endif // GUARD_EVOLVE_MENU_H
