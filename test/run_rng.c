#include "global.h"
#include "random.h"
#include "run_rng.h"
#include "ruleset.h"
#include "test/test.h"
#include "constants/ruleset.h"

// ============================================================================
// Phase 11A.6: seed-quality verification for the deterministic per-category
// run RNG (include/run_rng.h). See docs/SPEC.md "Run seed".
// ============================================================================

// A golden-value regression test: RunRng_Seed funnels
// {GetRunSeed(), GetSavedRandomizerVersion(), salt, k0, k1, k2} through
// Crc32B and seeds an SFC32 stream from the digest. These four draws were
// computed independently (a standalone Python re-implementation of Crc32B
// and SFC32, cross-checked against zlib.crc32 for the CRC step, and against
// this test's own prior RANDOMIZER_VERSION == 4 values before they were
// replaced) for the exact tuple below, at RANDOMIZER_VERSION == 5 (Phase
// 13A's Ball field-item-pool fix). If this test ever fails after a change to
// RunRng_Seed, Crc32B, or the SFC32 stepping, that change silently reshuffled
// every existing run seed's entire world and must be paired with a
// RANDOMIZER_VERSION bump (see src/ruleset.c's version-history comment) - it
// should never be "fixed" by just updating these constants.
TEST("RunRng_Seed golden values are stable for a fixed (seed, version, salt) tuple")
{
    rng_value_t state;

    SetRunSeed(0x1234ABCD);
    EXPECT_EQ(GetSavedRandomizerVersion(), RANDOMIZER_VERSION);
    EXPECT_EQ(RANDOMIZER_VERSION, 5);

    state = RunRng_Seed(SALT_WILD_SLOT, 1, 2, 3);
    EXPECT_EQ(LocalRandom32(&state), 0x2AE11DB7u);
    EXPECT_EQ(LocalRandom32(&state), 0xA398E77Du);
    EXPECT_EQ(LocalRandom32(&state), 0x44A4EAFAu);
    EXPECT_EQ(LocalRandom32(&state), 0x5E33189Au);
}

// A category's stream must depend on its own salt/keys only - not on any
// other category's salt, and not on call order (docs/SPEC.md "Separate
// deterministic randomization systems").
TEST("RunRng_Seed streams are independent per salt and per key")
{
    rng_value_t a, b, c;

    SetRunSeed(0xCAFEF00Du);

    a = RunRng_Seed(SALT_WILD_SLOT, 5, 0, 0);
    b = RunRng_Seed(SALT_STARTER, 5, 0, 0);
    EXPECT_NE(LocalRandom32(&a), LocalRandom32(&b));

    a = RunRng_Seed(SALT_WILD_SLOT, 5, 0, 0);
    c = RunRng_Seed(SALT_WILD_SLOT, 6, 0, 0);
    EXPECT_NE(LocalRandom32(&a), LocalRandom32(&c));
}

// Re-deriving the same (seed, version, salt, keys) tuple must always produce
// the same stream - this is the Deterministic World Principle's foundation:
// every downstream generator (wild slots, starters, trainers, learnsets,
// abilities) trusts RunRng_Seed to be a pure function of its inputs.
TEST("RunRng_Seed is a pure function of its inputs")
{
    rng_value_t a, b;
    u32 i;

    SetRunSeed(0x0BADF00Du);
    a = RunRng_Seed(SALT_TRAINER, 11, 22, 33);
    b = RunRng_Seed(SALT_TRAINER, 11, 22, 33);

    for (i = 0; i < 8; i++)
        EXPECT_EQ(LocalRandom32(&a), LocalRandom32(&b));
}
