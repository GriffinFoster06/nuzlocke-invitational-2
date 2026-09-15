#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"

// Regression coverage for a reported A -> B -> A voluntary-switch oscillation: the AI
// switches out of a bad matchup, then immediately undoes its own decision on the very next
// turn with no meaningful change in board state. See docs/SPEC.md "AI switching". This file
// covers the two general principles the fix must preserve; new file (rather than editing
// upstream test/battle/ai/ai_switching.c) to keep future upstream merges clean.

AI_SINGLE_BATTLE_TEST("Switch AI: after a voluntary switch into a good matchup, AI does not immediately switch back with unchanged state")
{
    PASSES_RANDOMLY(SHOULD_SWITCH_HASBADODDS_PERCENTAGE, 100, RNG_AI_SWITCH_HASBADODDS);
    GIVEN {
        ASSUME(GetSpeciesType(SPECIES_RHYDON, 0) == TYPE_GROUND);
        ASSUME(GetSpeciesType(SPECIES_PELIPPER, 0) == TYPE_WATER);
        ASSUME(GetSpeciesType(SPECIES_PELIPPER, 1) == TYPE_FLYING);
        ASSUME(GetMoveType(MOVE_THUNDERBOLT) == TYPE_ELECTRIC);
        ASSUME(GetMoveType(MOVE_EARTHQUAKE) == TYPE_GROUND);

        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_SMART_SWITCHING | AI_FLAG_SMART_MON_CHOICES | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_ELECTRODE) { Moves(MOVE_THUNDERBOLT, MOVE_THUNDER_WAVE, MOVE_THUNDER_SHOCK); }
        // Pelipper is 4x weak to Electric and gets OHKO'd; Rhydon is Ground/Rock and takes
        // zero damage from any Electric move, so once switched in there is no state-based
        // reason to switch back to Pelipper.
        OPPONENT(SPECIES_PELIPPER) { Moves(MOVE_EARTHQUAKE); }
        OPPONENT(SPECIES_RHYDON) { Moves(MOVE_EARTHQUAKE); Ability(ABILITY_ROCK_HEAD); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDERBOLT); EXPECT_SWITCH(opponent, 1); }
        TURN { MOVE(player, MOVE_THUNDERBOLT); EXPECT_MOVE(opponent, MOVE_EARTHQUAKE); }
    }
}

AI_SINGLE_BATTLE_TEST("Switch AI: AI still switches again when the player's switch materially changes the matchup (legitimate pivot)")
{
    PASSES_RANDOMLY(SHOULD_SWITCH_HASBADODDS_PERCENTAGE, 100, RNG_AI_SWITCH_HASBADODDS);
    GIVEN {
        ASSUME(GetSpeciesType(SPECIES_RHYDON, 0) == TYPE_GROUND);
        ASSUME(GetSpeciesType(SPECIES_RHYDON, 1) == TYPE_ROCK);
        ASSUME(GetSpeciesType(SPECIES_PELIPPER, 0) == TYPE_WATER);
        ASSUME(GetSpeciesType(SPECIES_PELIPPER, 1) == TYPE_FLYING);
        ASSUME(GetMoveType(MOVE_THUNDERBOLT) == TYPE_ELECTRIC);
        ASSUME(GetMoveType(MOVE_SURF) == TYPE_WATER);

        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT | AI_FLAG_SMART_SWITCHING | AI_FLAG_SMART_MON_CHOICES | AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_ELECTRODE) { Moves(MOVE_THUNDERBOLT, MOVE_THUNDER_WAVE, MOVE_THUNDER_SHOCK); }
        // Kingdra's Surf is 4x super effective on Rhydon (Water/Ground+Rock); Zigzagoon here
        // is immune to it via a forced Water Absorb, so the player switching to Kingdra is a
        // genuine, event-driven reason for the AI to pivot again.
        PLAYER(SPECIES_KINGDRA) { Moves(MOVE_SURF); }
        OPPONENT(SPECIES_PELIPPER) { Moves(MOVE_EARTHQUAKE); }
        OPPONENT(SPECIES_RHYDON) { Moves(MOVE_EARTHQUAKE); Ability(ABILITY_ROCK_HEAD); }
        OPPONENT(SPECIES_ZIGZAGOON) { Ability(ABILITY_WATER_ABSORB); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDERBOLT); EXPECT_SWITCH(opponent, 1); } // Pelipper -> Rhydon
        TURN { SWITCH(player, 1); EXPECT_SWITCH(opponent, 2); }              // player pivots to Kingdra; Rhydon -> Zigzagoon
    }
}
