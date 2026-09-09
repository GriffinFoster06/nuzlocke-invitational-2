// ============================================================================
// Phase 7 - "Evolve command" policy. See include/evolve_menu.h.
// UI wiring is CursorCb_Evolve in src/party_menu.c.
//
// Evolution Assistance was removed in the Phase 9.5 sweep: once every move-gated
// evolution in the species dataset became plain EVO_LEVEL (docs/SPEC.md
// "Evolution system"), EVOLVE_CHECK_NEEDS_MOVE was unreachable. See the
// "Move-dependent evolution anti-softlock" note in docs/SPEC.md.
// ============================================================================

#include "global.h"
#include "evolve_menu.h"
#include "nuzlocke.h"       // Nuzlocke_MonIsDead
#include "pokemon.h"
#include "ruleset.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"

bool32 EvolveMenu_CommandEnabled(void)
{
    return GetRulesetSetting(SETTING_EVOLVE_COMMAND) != 0;
}

enum EvolveCheck EvolveMenu_Check(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    const struct Evolution *evos;
    bool32 canStopEvo = TRUE;

    if (GetMonData(mon, MON_DATA_IS_EGG))
        return EVOLVE_CHECK_NONE;

    evos = GetSpeciesEvolutions(species);
    if (evos[0].method == EVOLUTIONS_END)
        return EVOLVE_CHECK_NONE;

    // A level/param evolution that is satisfied right now.
    if (GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, &canStopEvo, CHECK_EVO) != SPECIES_NONE)
        return EVOLVE_CHECK_READY;

    return EVOLVE_CHECK_NONE;
}

bool32 EvolveMenu_IsAvailable(struct Pokemon *mon)
{
    if (!EvolveMenu_CommandEnabled())
        return FALSE;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    if (Nuzlocke_MonIsDead(mon))
        return FALSE;

    return EvolveMenu_Check(mon) != EVOLVE_CHECK_NONE;
}
