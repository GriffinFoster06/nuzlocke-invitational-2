// ============================================================================
// Phase 7 - "Evolve command" + "Evolution Assistance" policy.
// See include/evolve_menu.h. UI wiring is CursorCb_Evolve in src/party_menu.c.
// ============================================================================

#include "global.h"
#include "evolve_menu.h"
#include "move.h"           // GetMoveType
#include "nuzlocke.h"       // Nuzlocke_MonIsDead
#include "party_menu.h"     // MonKnowsMove
#include "pokemon.h"
#include "regions.h"    // GetCurrentRegion
#include "ruleset.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"

bool32 EvolveMenu_CommandEnabled(void)
{
    return GetRulesetSetting(SETTING_EVOLVE_COMMAND) != 0;
}

bool32 EvolveMenu_AssistEnabled(void)
{
    return GetRulesetSetting(SETTING_EVOLUTION_ASSISTANCE) != 0;
}

static bool32 MonKnowsMoveOfType(struct Pokemon *mon, u32 type)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move != MOVE_NONE && GetMoveType(move) == type)
            return TRUE;
    }
    return FALSE;
}

// The one move we hand out to satisfy an IF_KNOWS_MOVE_TYPE evolution. Only
// Fairy is a live requirement today (Sylveon); anything else returns MOVE_NONE
// so the assist offer is simply withheld rather than guessed at.
static enum Move CuratedMoveForType(u32 type)
{
    if (type == TYPE_FAIRY)
        return MOVE_DISARMING_VOICE;
    return MOVE_NONE;
}

// Scan one evolution entry's condition list. Returns the evolution-required
// move to teach if the entry is blocked *only* by a move the mon lacks, or
// MOVE_NONE if teaching a move can't unblock this entry (level/item/friendship/
// time/weather/map gates, or an already-satisfied move condition).
static enum Move FindAssistMove(struct Pokemon *mon, const struct EvolutionParam *params)
{
    enum Move candidate = MOVE_NONE;

    if (params == NULL)
        return MOVE_NONE;

    for (u32 i = 0; params[i].condition != CONDITIONS_END; i++)
    {
        switch (params[i].condition)
        {
        case IF_KNOWS_MOVE:              // Ambipom, Tangrowth, Sudowoodo, ...
        case IF_USED_MOVE_X_TIMES:       // Annihilape (Rage Fist), Wyrdeer
            if (!MonKnowsMove(mon, params[i].arg1))
                candidate = params[i].arg1;
            break;
        case IF_KNOWS_MOVE_TYPE:         // Sylveon (Fairy)
            if (!MonKnowsMoveOfType(mon, params[i].arg1))
            {
                enum Move mv = CuratedMoveForType(params[i].arg1);
                if (mv != MOVE_NONE)
                    candidate = mv;
                else
                    return MOVE_NONE;
            }
            break;
        case IF_RECOIL_DAMAGE_GE:        // Basculegion M/F
            if (!MonKnowsMove(mon, MOVE_DOUBLE_EDGE))
                candidate = MOVE_DOUBLE_EDGE;
            break;
        // Gates a move can't open, but which don't make teaching the move
        // pointless either: the mon still needs the move once the gate is met,
        // so keep scanning instead of withholding the offer.
        //
        // IF_MIN_FRIENDSHIP matters concretely - Sylveon is
        // {IF_MIN_FRIENDSHIP, ...}, {IF_KNOWS_MOVE_TYPE, TYPE_FAIRY}, and
        // withholding here meant the only Fairy-move offer was never reachable.
        // The rest are inherent traits (M/F, PID and nature splits).
        case IF_MIN_FRIENDSHIP:
        case IF_MIN_OVERWORLD_STEPS:
        case IF_GENDER:
        case IF_PID_MODULO_100_GT:
        case IF_PID_MODULO_100_EQ:
        case IF_PID_MODULO_100_LT:
        case IF_PID_UPPER_MODULO_10_GT:
        case IF_PID_UPPER_MODULO_10_EQ:
        case IF_PID_UPPER_MODULO_10_LT:
        case IF_NATURE:
        case IF_AMPED_NATURE:
        case IF_LOW_KEY_NATURE:
        case IF_ATK_GT_DEF:
        case IF_ATK_EQ_DEF:
        case IF_ATK_LT_DEF:
            break;
        // Region gates are constant for this ROM, so resolve them instead of
        // withholding: Mime Jr. -> Mr. Mime is {IF_KNOWS_MOVE, MIMIC} plus
        // {IF_NOT_REGION, REGION_GALAR}, which always holds in Hoenn. An
        // unsatisfiable region entry (Mr. Mime-Galar) drops out here so we
        // never promise an evolution that cannot fire.
        case IF_REGION:
            if (GetCurrentRegion() != params[i].arg1)
                return MOVE_NONE;
            break;
        case IF_NOT_REGION:
            if (GetCurrentRegion() == params[i].arg1)
                return MOVE_NONE;
            break;
        // Anything else (time, weather, held item, map, beauty, party
        // contents, ...) is a gate a move can't open. Withhold.
        default:
            return MOVE_NONE;
        }
    }
    return candidate;
}

enum EvolveCheck EvolveMenu_Check(struct Pokemon *mon, u16 *outMove)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    const struct Evolution *evos;
    bool32 canStopEvo = TRUE;
    u32 level;

    if (outMove != NULL)
        *outMove = MOVE_NONE;

    if (GetMonData(mon, MON_DATA_IS_EGG))
        return EVOLVE_CHECK_NONE;

    evos = GetSpeciesEvolutions(species);
    if (evos[0].method == EVOLUTIONS_END)
        return EVOLVE_CHECK_NONE;

    // A level/param evolution that is satisfied right now.
    if (GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, &canStopEvo, CHECK_EVO) != SPECIES_NONE)
        return EVOLVE_CHECK_READY;

    if (!EvolveMenu_AssistEnabled())
        return EVOLVE_CHECK_NONE;

    level = GetMonData(mon, MON_DATA_LEVEL);
    for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
    {
        enum Move move;

        if (evos[i].method != EVO_LEVEL && evos[i].method != EVO_LEVEL_BATTLE_ONLY)
            continue;
        if (evos[i].param != 0 && level < evos[i].param)
            continue; // level gate not met yet - a move won't help today

        move = FindAssistMove(mon, evos[i].params);
        if (move != MOVE_NONE)
        {
            if (outMove != NULL)
                *outMove = move;
            return EVOLVE_CHECK_NEEDS_MOVE;
        }
    }
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

    return EvolveMenu_Check(mon, NULL) != EVOLVE_CHECK_NONE;
}
