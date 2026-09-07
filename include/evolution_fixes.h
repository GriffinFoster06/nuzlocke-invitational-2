#ifndef GUARD_EVOLUTION_FIXES_H
#define GUARD_EVOLUTION_FIXES_H

// ============================================================================
// Phase 7 - Evolution overhaul (docs/SPEC.md "Evolution system", "Trade
// evolutions", "Species-specific evolution fixes").
//
// A handful of species cannot reach their final form in a single-player,
// randomized, Nuzlocke run: link-trade-only evolutions (Karrablast/Shelmet),
// evolutions that need a specific *other* species in the party or defeated
// (Mantyke, Kingambit), a 999-item counter (Gholdengo), an unimplemented
// overworld trigger (Galarian Yamask/Runerigus), a Hisui-region gate that can
// never be true in a Hoenn game (Ursaluna), a purely luck-gated requirement
// (Sirfetch'd), and one evolution level above the pre-Champion cap (Hydreigon).
//
// Rather than scatter edits across src/data/pokemon/species_info/gen_*.h (the
// worst possible shape for staying rebasable against upstream master), this is
// a single override table consulted from GetSpeciesEvolutions() - the one and
// only reader of gSpeciesInfo[].evolutions. The override, when present,
// completely replaces that species' evolution array.
//
// The table itself is src/data/evolution_fixes.h.
// ============================================================================

#include "constants/species.h"

struct Evolution;

// Returns the replacement evolution array for `species`, or NULL if this
// species keeps its stock evolutions. The returned pointer has static storage
// duration and is EVOLUTIONS_END-terminated, exactly like the stock arrays.
const struct Evolution *EvoFix_GetOverride(enum Species species);

#endif // GUARD_EVOLUTION_FIXES_H
