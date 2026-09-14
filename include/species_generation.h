#ifndef GUARD_SPECIES_GENERATION_H
#define GUARD_SPECIES_GENERATION_H

// ============================================================================
// Phase 11A.6 - generation classification (docs/SPEC.md "Generation
// filters"). Classifies a species/form by the generation in which THAT
// SPECIFIC species/form was introduced (Eevee = 1, Sylveon = 6, a regional
// form = the generation that introduced the form, Paradox species = 9) -
// never the generation of the family's oldest member.
//
// ROM-resident only: a small static exceptions list plus the
// isAlolanForm/isGalarianForm/.../natDexNum fields gSpeciesInfo already
// carries. No EWRAM cost.
// ============================================================================

// Returns 1-9, or 0 for SPECIES_NONE / an unrecognized id.
u8 GetSpeciesGeneration(enum Species species);

#endif // GUARD_SPECIES_GENERATION_H
