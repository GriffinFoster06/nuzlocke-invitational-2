#ifndef GUARD_RUN_RNG_H
#define GUARD_RUN_RNG_H

// ============================================================================
// Deterministic per-category run RNG (docs/SPEC.md "Run seed", "Separate
// deterministic randomization systems").
//
// Every randomization category derives its stream from the stored run seed
// plus a fixed category salt, so changing one category's outcome never
// reshuffles an unrelated one. Shared by src/randomizer.c (Phase 2) and
// src/learnset_gen.c (Phase 4).
// ============================================================================

#include "random.h"
#include "ruleset.h"

// Distinct per-category salts. ASCII mnemonics; values are arbitrary but must
// never change once a category has shipped (a seed's world would move).
#define SALT_WILD_SLOT    0x574C5344  // "WLSD"
#define SALT_WILD_ROUTE   0x574C5254  // "WLRT"
#define SALT_WILD_GLOBAL  0x574C4742  // "WLGB"
#define SALT_STARTER      0x53544152  // "STAR"
#define SALT_STARTER_IV   0x53544956  // "STIV"
#define SALT_GIFT         0x47494654  // "GIFT"
#define SALT_STATIC       0x53544154  // "STAT"
#define SALT_ROAMER       0x524F414D  // "ROAM"
#define SALT_LEARNSET     0x4C524E53  // "LRNS"
// Reserved for later phases: trainers, abilities, TMs, tutors, items.

static inline rng_value_t RunRng_Seed(u32 salt, u32 k0, u32 k1, u32 k2)
{
    const u32 pieces[5] = { GetRunSeed(), salt, k0, k1, k2 };
    return LocalRandomSeed(Crc32B((const u8 *)pieces, sizeof(pieces)));
}

#endif // GUARD_RUN_RNG_H
