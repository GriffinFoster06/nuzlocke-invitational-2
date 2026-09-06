#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

// ============================================================================
// Phase 2 core randomizer wiring - docs/SPEC.md "Wild Pokemon randomization",
// "Fixed encounter-slot mapping", "Starter/Gift/Static Pokemon".
//
// Deterministic, power-matched species replacement derived from the run seed
// (include/ruleset.h GetRunSeed). Each category has its own salt so changing
// one category's outcome does not reshuffle another ("Separate deterministic
// randomization systems"). Selection and scoring live in src/power_score.c.
// ============================================================================

#include "wild_encounter.h"

struct Pokemon;
struct PokemonTemplate;

// Category enable checks (thin wrappers over the ruleset settings).
bool32 Randomizer_WildEnabled(void);
bool32 Randomizer_StarterEnabled(void);
bool32 Randomizer_GiftEnabled(void);
bool32 Randomizer_StaticEnabled(void);
bool32 Randomizer_LegendaryEnabled(void);

// Wild encounters. Returns the persistent replacement for the given
// (encounter table, slot); returns `vanilla` unchanged when randomization is
// off or the target is not a valid replaceable species.
enum Species Randomizer_WildSlotSpecies(const struct WildPokemonInfo *info, u32 slot, enum Species vanilla);

// Starters. `index` is 0..2; the three results are de-duplicated.
enum Species Randomizer_StarterSpecies(u32 index);
// Apply the SETTING_STARTER_IV_MODE spread to a freshly given starter.
void Randomizer_ApplyStarterIVs(struct Pokemon *mon);
// Freeze the generation settings: the first irreversible act of a run.
void Randomizer_MarkRunStarted(void);

// Script gifts (givemon / createmon, player side). Rewrites species + IV
// template in place per SETTING_GIFT_* ; no-op for eggs and when disabled.
void Randomizer_ApplyGiftTemplate(struct PokemonTemplate *monTemplate);

// Static encounters. `idx` disambiguates the two mons of a double battle.
// Premium-tier vanilla species route through the curated premium pool and obey
// SETTING_LEGENDARY_RANDOMIZATION instead of SETTING_STATIC_RANDOMIZATION.
enum Species Randomizer_StaticSpecies(enum Species vanilla, u8 level, u8 idx);

// Roaming legendaries (Latias/Latios). Premium pool, legendary toggle.
enum Species Randomizer_RoamerSpecies(enum Species vanilla, u8 level);

#endif // GUARD_RANDOMIZER_H
