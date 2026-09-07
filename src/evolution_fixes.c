// ============================================================================
// Phase 7 - Evolution overhaul. See include/evolution_fixes.h.
// ============================================================================

#include "global.h"
#include "evolution_fixes.h"
#include "caps.h"
#include "pokemon.h"
#include "constants/pokemon.h"
#include "constants/items.h"

#include "data/evolution_fixes.h"

const struct Evolution *EvoFix_GetOverride(enum Species species)
{
    for (u32 i = 0; i < ARRAY_COUNT(sEvoFixTable); i++)
    {
        if (sEvoFixTable[i].species == species)
            return sEvoFixTable[i].evolutions;
    }
    return NULL;
}
