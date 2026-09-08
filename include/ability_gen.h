#ifndef GUARD_ABILITY_GEN_H
#define GUARD_ABILITY_GEN_H

// ============================================================================
// Phase 2 - randomized abilities (docs/SPEC.md "Abilities", "Ability
// consistency through evolution", "Shedinja").
//
// When SETTING_ABILITY_RANDOMIZATION is on, every species' ability slots are
// replaced by deterministic picks from the eligible-ability pool. The pick is
// keyed on the species' *evolution-family root* plus the slot index, so a
// family shares one ability per slot and evolving never changes the ability a
// Pokemon has (a mon stores only its slot, and evolution never touches it).
// SETTING_ABILITY_EVO_CONSISTENCY == 0 keys on the species instead.
//
// A vanilla slot holding ABILITY_NONE stays ABILITY_NONE, so a one-ability
// species does not silently gain three.
//
// Hook point: GetSpeciesAbility() in src/pokemon.c - the single accessor
// behind GetAbilityBySpecies() / GetMonAbility(), so battle, summary, Pokedex
// and AI all agree for free.
// ============================================================================

bool32 AbilityGen_IsActive(void); // SETTING_ABILITY_RANDOMIZATION

// The randomized ability for (species, slot), or ABILITY_NONE when the vanilla
// slot is empty, the species is exempt (Shedinja), or the pool is unusable -
// the caller then falls back to the canonical ability.
enum Ability AbilityGen_Get(enum Species species, u8 slot);

void AbilityGen_EnsureBuilt(void); // rebuild family map / pool if settings moved
void AbilityGen_Invalidate(void);  // force the above on next call

#endif // GUARD_ABILITY_GEN_H
