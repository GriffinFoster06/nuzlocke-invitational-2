#include "global.h"
#include "learnset_gen.h"
#include "move.h"
#include "pokemon.h"
#include "ruleset.h"
#include "test/test.h"
#include "constants/battle_move_effects.h"
#include "constants/ruleset.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------
// Phase 4 - generated 21-move 7/7/7 learnsets (docs/SPEC.md "Learnset size",
// "7/7/7 learnset composition", "Move-power progression").
// ---------------------------------------------------------------------------

static void UseDefaultLearnsetRuleset(void)
{
    SetRunSeed(0x0F4E2A11);
    SetRulesetSetting(SETTING_MOVE_RANDOMIZATION, 1);
    SetRulesetSetting(SETTING_LEARNSET_SIZE, 21);
    SetRulesetSetting(SETTING_LEARNSET_COMPOSITION, LRNCOMP_777);
    SetRulesetSetting(SETTING_MOVE_POWER_PROGRESSION, MVORDER_WEIGHTED_LATE);
    SetRulesetSetting(SETTING_BAN_OHKO_MOVES, 1);
    SetRulesetSetting(SETTING_BAN_EVASION_MOVES, 1);
    SetRulesetSetting(SETTING_BAN_SLEEP_MOVES, 1);
    SetRulesetSetting(SETTING_BAN_SELF_KO_MOVES, 1);
    LearnsetGen_Invalidate();
}

static u32 LearnsetLength(const struct LevelUpMove *ls)
{
    u32 n = 0;
    while (ls[n].move != LEVEL_UP_MOVE_END)
        n++;
    return n;
}

TEST("Generated learnset has 21 moves on the 1/4/7.../61 checkpoint grid")
{
    const struct LevelUpMove *ls;
    u32 i;

    UseDefaultLearnsetRuleset();
    ls = LearnsetGen_GetLearnset(SPECIES_ZIGZAGOON);

    EXPECT(ls != NULL);
    EXPECT_EQ(LearnsetLength(ls), 21);
    for (i = 0; i < 21; i++)
        EXPECT_EQ(ls[i].level, 1 + (i * 60) / 20);
}

TEST("Generated learnset is 7 STAB / 7 other damaging / 7 status")
{
    const struct LevelUpMove *ls;
    u32 i, stab = 0, otherDmg = 0, status = 0;
    enum Type t0, t1;

    UseDefaultLearnsetRuleset();
    ls = LearnsetGen_GetLearnset(SPECIES_MUDKIP);
    t0 = GetSpeciesType(SPECIES_MUDKIP, 0);
    t1 = GetSpeciesType(SPECIES_MUDKIP, 1);

    for (i = 0; i < 21; i++)
    {
        enum Move m = ls[i].move;
        if (GetMoveCategory(m) == DAMAGE_CATEGORY_STATUS)
            status++;
        else if (GetMoveType(m) == t0 || GetMoveType(m) == t1)
            stab++;
        else
            otherDmg++;
    }

    EXPECT_EQ(status, 7);
    EXPECT_EQ(stab, 7);
    EXPECT_EQ(otherDmg, 7);
}

TEST("Level-1 move is a damaging STAB move")
{
    const struct LevelUpMove *ls;
    enum Move m;

    UseDefaultLearnsetRuleset();
    ls = LearnsetGen_GetLearnset(SPECIES_TAILLOW);
    m = ls[0].move;

    EXPECT(GetMoveCategory(m) != DAMAGE_CATEGORY_STATUS);
    EXPECT((GetMoveType(m) == GetSpeciesType(SPECIES_TAILLOW, 0)
         || GetMoveType(m) == GetSpeciesType(SPECIES_TAILLOW, 1)));
}

TEST("Generation is deterministic for a (seed, species) pair")
{
    struct LevelUpMove first[32];
    const struct LevelUpMove *ls;
    u32 i, len;

    UseDefaultLearnsetRuleset();
    ls = LearnsetGen_GetLearnset(SPECIES_POOCHYENA);
    len = LearnsetLength(ls);
    for (i = 0; i <= len; i++)
        first[i] = ls[i];

    // Evict via unrelated lookups, then re-fetch.
    LearnsetGen_GetLearnset(SPECIES_WURMPLE);
    LearnsetGen_GetLearnset(SPECIES_SEEDOT);
    LearnsetGen_GetLearnset(SPECIES_ZIGZAGOON);
    LearnsetGen_GetLearnset(SPECIES_WINGULL);
    LearnsetGen_GetLearnset(SPECIES_MUDKIP);
    LearnsetGen_GetLearnset(SPECIES_TREECKO);
    LearnsetGen_GetLearnset(SPECIES_TORCHIC);
    LearnsetGen_GetLearnset(SPECIES_TAILLOW);

    ls = LearnsetGen_GetLearnset(SPECIES_POOCHYENA);
    EXPECT_EQ(LearnsetLength(ls), len);
    for (i = 0; i < len; i++)
    {
        EXPECT_EQ(ls[i].move, first[i].move);
        EXPECT_EQ(ls[i].level, first[i].level);
    }
}

TEST("A different run seed produces a different learnset")
{
    struct LevelUpMove a[32];
    const struct LevelUpMove *ls;
    u32 i, len, differences = 0;

    UseDefaultLearnsetRuleset();
    ls = LearnsetGen_GetLearnset(SPECIES_SHROOMISH);
    len = LearnsetLength(ls);
    for (i = 0; i < len; i++)
        a[i] = ls[i];

    SetRunSeed(0x99887766);
    LearnsetGen_Invalidate();
    ls = LearnsetGen_GetLearnset(SPECIES_SHROOMISH);

    for (i = 0; i < len; i++)
        if (ls[i].move != a[i].move)
            differences++;

    EXPECT(differences > 0);
}

TEST("Ban settings keep their categories out of generated learnsets")
{
    const struct LevelUpMove *ls;
    u32 sp, i;

    UseDefaultLearnsetRuleset();

    for (sp = 1; sp < 60; sp++)
    {
        ls = LearnsetGen_GetLearnset(sp);
        if (ls == NULL)
            continue;
        for (i = 0; ls[i].move != LEVEL_UP_MOVE_END; i++)
        {
            EXPECT(GetMoveEffect(ls[i].move) != EFFECT_OHKO);
            EXPECT(!IsExplosionMove(ls[i].move));
        }
    }
}

TEST("Learnset size setting is honored")
{
    const struct LevelUpMove *ls;

    UseDefaultLearnsetRuleset();
    SetRulesetSetting(SETTING_LEARNSET_SIZE, 12);
    LearnsetGen_Invalidate();

    ls = LearnsetGen_GetLearnset(SPECIES_ZIGZAGOON);
    EXPECT_EQ(LearnsetLength(ls), 12);
    EXPECT_EQ(ls[11].level, 1 + (11 * 60) / 11); // last checkpoint == 61
}

TEST("Randomization off falls back to the canonical learnset")
{
    UseDefaultLearnsetRuleset();
    SetRulesetSetting(SETTING_MOVE_RANDOMIZATION, 0);

    EXPECT(!LearnsetGen_IsActive());
    EXPECT_EQ(GetSpeciesLevelUpLearnset(SPECIES_ZIGZAGOON),
              gSpeciesInfo[SPECIES_ZIGZAGOON].levelUpLearnset);
}
