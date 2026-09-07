#ifndef GUARD_CAPS_H
#define GUARD_CAPS_H

#if B_EXP_CAP_TYPE != EXP_CAP_NONE && B_EXP_CAP_TYPE != EXP_CAP_HARD && B_EXP_CAP_TYPE != EXP_CAP_SOFT
#error "Invalid choice for B_EXP_CAP_TYPE, must be of [EXP_CAP_NONE, EXP_CAP_HARD, EXP_CAP_SOFT]"
#endif

#if B_EXP_CAP_TYPE == EXP_CAP_HARD || B_EXP_CAP_TYPE == EXP_CAP_SOFT
#if B_LEVEL_CAP_TYPE != LEVEL_CAP_FLAG_LIST && B_LEVEL_CAP_TYPE != LEVEL_CAP_VARIABLE
#error "Invalid choice for B_LEVEL_CAP_TYPE, must be of [LEVEL_CAP_FLAG_LIST, LEVEL_CAP_VARIABLE]"
#endif
#if B_LEVEL_CAP_TYPE == LEVEL_CAP_VARIABLE && B_LEVEL_CAP_VARIABLE == 0
#error "B_LEVEL_CAP_TYPE set to LEVEL_CAP_VARIABLE, but no variable chosen for B_LEVEL_CAP_VARIABLE, set B_LEVEL_CAP_VARIABLE to a valid event variable"
#endif
#endif

#if B_EV_CAP_TYPE != EV_CAP_NONE && B_EV_CAP_TYPE != EV_CAP_FLAG_LIST && B_EV_CAP_TYPE != EV_CAP_VARIABLE && B_EV_CAP_TYPE != EV_CAP_NO_GAIN
#error "Invalid choice for B_EV_CAP_TYPE, must be one of [EV_CAP_NONE, EV_CAP_FLAG_LIST, EV_CAP_VARIABLE, EV_CAP_NO_GAIN]"
#endif

// docs/SPEC.md "Default cap progression": the cap in force from the eighth badge
// until the Champion is beaten. Also the ceiling Phase 7 lowers any higher
// evolution level down to (src/data/evolution_fixes.h).
#define PRE_CHAMPION_LEVEL_CAP 63

u32 GetProgressionLevelCap(void);        // docs/SPEC.md "Default cap progression" - always meaningful
u32 GetCurrentLevelCap(void);            // enforcement cap: progression cap in hard mode, else MAX_LEVEL
u32 GetSoftLevelCapExpValue(u32 level, u32 expValue);
u32 GetCurrentEVCap(void);
bool32 IsLevelOverCap(u32 level);        // TRUE if level is past the progression cap (and caps aren't off)
u8 Caps_ClampLevel(u8 level);            // clamp a found-Pokemon level to the progression cap

#endif /* GUARD_CAPS_H */
