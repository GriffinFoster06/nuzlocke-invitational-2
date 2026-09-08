// ============================================================================
// Phase 7 - evolution override table. See include/evolution_fixes.h for the
// rationale. Each row completely replaces one species' stock evolution array;
// GetSpeciesEvolutions() returns it verbatim.
//
// Phase 9.5's global evolution sweep (docs/SPEC.md "Evolution system": every
// evolution is level-based except item/stone evolutions) moved every other case
// that used to live here into the species dataset itself
// (src/data/pokemon/species_info/gen_*_families.h, tagged "// NUZLOCKE:"), so
// only one genuinely bespoke entry is left. Prefer a dataset edit over a new
// row here: this table is linear-scanned on every GetSpeciesEvolutions() call,
// and an override must restate a species' *entire* evolution array, which
// silently goes stale if the dataset changes underneath it.
// ============================================================================

#ifndef EVOLUTION
#define EVOLUTION(...) (const struct Evolution[]) { __VA_ARGS__, { EVOLUTIONS_END }, }
#endif
#ifndef CONDITIONS
#define CONDITIONS(...) ((const struct EvolutionParam[]) { __VA_ARGS__, {CONDITIONS_END} })
#endif

// ---- Zweilous: the dataset evolves it at level 64, one above the pre-Champion
// level cap, so Hydreigon would be unreachable inside the cap. This is a caps
// interaction, not an evolution-condition one, which is why it stays here.
static const struct Evolution sEvoFix_Zweilous[] =
    EVOLUTION({EVO_LEVEL, PRE_CHAMPION_LEVEL_CAP, SPECIES_HYDREIGON});

struct EvoFixEntry
{
    u16 species;
    const struct Evolution *evolutions;
};

// Linear-scanned (tiny). No ordering requirement.
static const struct EvoFixEntry sEvoFixTable[] =
{
    { SPECIES_ZWEILOUS, sEvoFix_Zweilous },
};
