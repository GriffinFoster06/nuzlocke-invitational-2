#ifndef GUARD_TEST_FIXTURES_H
#define GUARD_TEST_FIXTURES_H

#ifndef TEST_FIXTURES
#define TEST_FIXTURES 0
#endif

// Developer-only test-scenario fixture generator.
//
// This file (and src/test_fixtures.c) only exist in the `make fixtures`
// build (-DTEST_FIXTURES=1). They are never compiled into a normal or
// release ROM: see docs handoff / AGENTS.md for the tools/test_scenario and
// tools/generate_test_fixture host-side workflow that drives this.
//
// At boot, CB2_TestFixtureBoot reads the requested scenario id out of
// gFixtureRequest (a well-known byte the host tool patches into the ROM
// image before running it headlessly), runs that scenario's builder
// function using the project's normal save/Pokemon/ruleset/Nuzlocke
// helpers, saves the result with the real save machinery, and then dumps
// the resulting 128 KB flash image over the mGBA debug-print channel so the
// host can reassemble it into an ordinary .sav fixture file.
#if TEST_FIXTURES

void CB2_TestFixtureBoot(void);

#endif // TEST_FIXTURES

#endif // GUARD_TEST_FIXTURES_H
