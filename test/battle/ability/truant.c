#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Truant alternates action and loafing turns")
{
    GIVEN {
        PLAYER(SPECIES_SLAKING) { Ability(ABILITY_TRUANT); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        MESSAGE("Slaking is loafing around!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
    }
}

SINGLE_BATTLE_TEST("Truant resumes with an action turn after switching out and back in")
{
    GIVEN {
        PLAYER(SPECIES_SLAKING) { Ability(ABILITY_TRUANT); Moves(MOVE_TACKLE); }
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        SEND_IN_MESSAGE("Wobbuffet");
        SEND_IN_MESSAGE("Slaking");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        NOT MESSAGE("Slaking is loafing around!");
    }
}
