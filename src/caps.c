#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "caps.h"
#include "pokemon.h"
#include "ruleset.h"
#include "constants/ruleset.h"

// ============================================================================
// Phase 4 - Level caps (docs/SPEC.md "Default cap progression", "Hard level
// caps"). Two distinct notions:
//   GetProgressionLevelCap() - the spec's per-badge cap, ALWAYS meaningful,
//     used for UI, the Level-to-Cap target, and clamping found Pokemon.
//   GetCurrentLevelCap()     - the ENFORCEMENT cap the upstream call sites see;
//     equals the progression cap only in hard-cap mode, otherwise MAX_LEVEL so
//     those sites (battle exp clamp, TryIncrementMonLevel, daycare, rare candy)
//     become no-ops without being touched.
// The runtime mode comes from SETTING_CAP_MODE, not from include/config/caps.h.
// ============================================================================

// docs/SPEC.md "Default cap progression". The value returned is the one at the
// FIRST unset flag, i.e. the row keyed by FLAG_BADGEnn_GET is the cap that
// applies while that badge is still missing. Returns 0 once the Champion is
// beaten - the caller substitutes the post-Champion cap.
static u32 ProgressionCapFromFlags(void)
{
    static const u16 sLevelCapFlagMap[][2] =
    {
        {FLAG_BADGE01_GET, 14}, // before Roxanne
        {FLAG_BADGE02_GET, 21}, // after Roxanne / before Brawly
        {FLAG_BADGE03_GET, 24}, // before Wattson
        {FLAG_BADGE04_GET, 29}, // before Flannery
        {FLAG_BADGE05_GET, 36}, // before Norman
        {FLAG_BADGE06_GET, 43}, // before Winona
        {FLAG_BADGE07_GET, 47}, // before Tate & Liza
        {FLAG_BADGE08_GET, 50}, // before Juan
        {FLAG_IS_CHAMPION,  PRE_CHAMPION_LEVEL_CAP}, // eight badges / before Elite Four
    };

    for (u32 i = 0; i < ARRAY_COUNT(sLevelCapFlagMap); i++)
    {
        if (!FlagGet(sLevelCapFlagMap[i][0]))
            return sLevelCapFlagMap[i][1];
    }
    return 0;
}

u32 GetProgressionLevelCap(void)
{
    u32 cap = ProgressionCapFromFlags();

    if (cap == 0) // Champion beaten
    {
        cap = GetRulesetSetting(SETTING_POST_CHAMPION_CAP);
        if (cap < 63) // guard against a malformed stored value
            cap = 63;
    }
    if (cap > MAX_LEVEL)
        cap = MAX_LEVEL;
    return cap;
}

u32 GetCurrentLevelCap(void)
{
    if (GetRulesetSetting(SETTING_CAP_MODE) == CAPMODE_HARD)
        return GetProgressionLevelCap();
    return MAX_LEVEL;
}

bool32 IsLevelOverCap(u32 level)
{
    if (GetRulesetSetting(SETTING_CAP_MODE) == CAPMODE_OFF)
        return FALSE;
    return level > GetProgressionLevelCap();
}

// docs/SPEC.md "Caught Pokemon above the cap": found Pokemon should simply never
// exist above the cap. Callers apply this to wild / gift / static levels. No-op
// when caps are turned off entirely.
u8 Caps_ClampLevel(u8 level)
{
    u32 cap;

    if (GetRulesetSetting(SETTING_CAP_MODE) == CAPMODE_OFF)
        return level;

    cap = GetProgressionLevelCap();
    if (level > cap)
        return cap;
    return level;
}

u32 GetSoftLevelCapExpValue(u32 level, u32 expValue)
{
    static const u32 sExpScalingDown[5] = { 4, 8, 16, 32, 64 };
    static const u32 sExpScalingUp[5]   = { 16, 8, 4, 2, 1 };

    u32 mode = GetRulesetSetting(SETTING_CAP_MODE);
    u32 cap = GetProgressionLevelCap();
    u32 levelDifference;

    if (mode == CAPMODE_OFF || mode == CAPMODE_WARNING)
        return expValue;

    if (level < cap)
    {
        if (B_LEVEL_CAP_EXP_UP)
        {
            levelDifference = cap - level;
            if (levelDifference > ARRAY_COUNT(sExpScalingUp) - 1)
                return expValue + (expValue / sExpScalingUp[ARRAY_COUNT(sExpScalingUp) - 1]);
            else
                return expValue + (expValue / sExpScalingUp[levelDifference]);
        }
        return expValue;
    }

    if (mode == CAPMODE_HARD)
        return 0;

    // CAPMODE_SOFT: reduced experience past the cap.
    levelDifference = level - cap;
    if (levelDifference > ARRAY_COUNT(sExpScalingDown) - 1)
        return expValue / sExpScalingDown[ARRAY_COUNT(sExpScalingDown) - 1];
    return expValue / sExpScalingDown[levelDifference];
}

u32 GetCurrentEVCap(void)
{
    static const u16 sEvCapFlagMap[][2] = {
        // Define EV caps for each milestone
        {FLAG_BADGE01_GET, MAX_TOTAL_EVS *  1 / 17},
        {FLAG_BADGE02_GET, MAX_TOTAL_EVS *  3 / 17},
        {FLAG_BADGE03_GET, MAX_TOTAL_EVS *  5 / 17},
        {FLAG_BADGE04_GET, MAX_TOTAL_EVS *  7 / 17},
        {FLAG_BADGE05_GET, MAX_TOTAL_EVS *  9 / 17},
        {FLAG_BADGE06_GET, MAX_TOTAL_EVS * 11 / 17},
        {FLAG_BADGE07_GET, MAX_TOTAL_EVS * 13 / 17},
        {FLAG_BADGE08_GET, MAX_TOTAL_EVS * 15 / 17},
        {FLAG_IS_CHAMPION, MAX_TOTAL_EVS},
    };

    if (B_EV_CAP_TYPE == EV_CAP_FLAG_LIST)
    {
        for (u32 evCap = 0; evCap < ARRAY_COUNT(sEvCapFlagMap); evCap++)
        {
            if (!FlagGet(sEvCapFlagMap[evCap][0]))
                return sEvCapFlagMap[evCap][1];
        }
    }
    else if (B_EV_CAP_TYPE == EV_CAP_VARIABLE)
    {
        return VarGet(B_EV_CAP_VARIABLE);
    }
    else if (B_EV_CAP_TYPE == EV_CAP_NO_GAIN)
    {
        return 0;
    }

    return MAX_TOTAL_EVS;
}
