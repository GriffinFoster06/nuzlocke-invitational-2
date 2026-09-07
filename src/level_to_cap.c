// ============================================================================
// Phase 4 - "Level to Cap" policy (docs/SPEC.md "Level to Cap",
// "Optional Level to Next Breakpoint"). See include/level_to_cap.h.
//
// This file only decides the target level and applies it. Presenting the
// skipped level-up moves in chronological order is done by the existing
// party-menu catch-up loop (Task_TryLearnNewMoves in src/party_menu.c), which
// the cursor callback hands off to.
// ============================================================================

#include "global.h"
#include "caps.h"
#include "level_to_cap.h"
#include "nuzlocke.h"
#include "pokemon.h"
#include "ruleset.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"

// Lowest generated / canonical level-up move level strictly above `fromLevel`,
// or 0 if there is none at or below the cap.
static u32 NextLevelUpMoveLevel(enum Species species, u32 fromLevel, u32 cap)
{
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);
    u32 best = 0;

    for (u32 i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
    {
        u32 lvl = learnset[i].level;
        if (lvl > fromLevel && lvl <= cap && (best == 0 || lvl < best))
            best = lvl;
    }
    return best;
}

// Lowest level-based evolution threshold strictly above `fromLevel`, or 0.
static u32 NextEvolutionLevel(enum Species species, u32 fromLevel, u32 cap)
{
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);
    u32 best = 0;

    for (u32 i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
    {
        u32 lvl;

        if (evolutions[i].method != EVO_LEVEL && evolutions[i].method != EVO_LEVEL_BATTLE_ONLY)
            continue;

        lvl = evolutions[i].param;
        if (lvl > fromLevel && lvl <= cap && (best == 0 || lvl < best))
            best = lvl;
    }
    return best;
}

u8 LevelToCap_GetTargetLevel(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 curLevel = GetMonData(mon, MON_DATA_LEVEL);
    u32 cap = GetProgressionLevelCap();
    u32 target = cap;
    u32 breakpoint;

    if (curLevel >= cap)
        return curLevel;

    switch (GetRulesetSetting(SETTING_LEVEL_TO_BREAKPOINT))
    {
    case BREAKPT_NEXT_MOVE:
        breakpoint = NextLevelUpMoveLevel(species, curLevel, cap);
        if (breakpoint != 0)
            target = breakpoint;
        break;
    case BREAKPT_NEXT_EVO:
        breakpoint = NextEvolutionLevel(species, curLevel, cap);
        if (breakpoint != 0)
            target = breakpoint;
        break;
    case BREAKPT_OFF:
    case BREAKPT_CAP:
    default:
        break;
    }

    if (target > cap)
        target = cap;
    return target;
}

bool32 LevelToCap_IsAvailable(struct Pokemon *mon)
{
    if (!GetRulesetSetting(SETTING_LEVEL_TO_CAP))
        return FALSE;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    if (Nuzlocke_MonIsDead(mon))
        return FALSE;

    return GetMonData(mon, MON_DATA_LEVEL) < LevelToCap_GetTargetLevel(mon);
}

void LevelToCap_ApplyLevel(struct Pokemon *mon, u8 target)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 exp;

    if (target > MAX_LEVEL)
        target = MAX_LEVEL;
    if (target <= GetMonData(mon, MON_DATA_LEVEL))
        return;

    // Mirrors the Rare Candy path in PokemonUseItemEffects (src/pokemon.c):
    // set EXP to the target level's threshold, then recompute stats/level.
    // No EV grant, no friendship bump, no evolution.
    exp = gExperienceTables[gSpeciesInfo[species].growthRate][target];
    SetMonData(mon, MON_DATA_EXP, &exp);
    CalculateMonStats(mon);
}
