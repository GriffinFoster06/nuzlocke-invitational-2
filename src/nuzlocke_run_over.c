// ============================================================================
// Phase 3 - Nuzlocke run-over flow (docs/SPEC.md "Whiteout").
//
// After a whiteout with no living Pokemon anywhere, CB2_WhiteOut routes
// gFieldCallback here instead of the normal "rush to the Pokemon Center"
// cutscene. We fade the field in, then run EventScript_NuzlockeRunOver
// (data/scripts/nuzlocke.inc), which shows the run summary and, on A,
// invokes StartNuzlockeNewAttempt -> CB2_NewGame with the ruleset preserved.
// ============================================================================

#include "global.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "nuzlocke.h"
#include "ruleset.h"

extern const u8 EventScript_NuzlockeRunOver[];

static void Task_NuzlockeRunOver(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        ScriptContext_SetupScript(EventScript_NuzlockeRunOver);
    }
}

void Nuzlocke_FieldCB_RunOver(void)
{
    Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    CreateTask(Task_NuzlockeRunOver, 10);
    LockPlayerFieldControls();
}

// special: fill gStringVar1/2/3 with seed / deaths / locations resolved by catch.
void BufferNuzlockeRunOverStats(void)
{
    ConvertIntToHexStringN(gStringVar1, GetRunSeed(), STR_CONV_MODE_LEADING_ZEROS, 8);
    ConvertIntToDecimalStringN(gStringVar2, Nuzlocke_GetDeathCount(), STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar3, Nuzlocke_CountLocationsCaught(), STR_CONV_MODE_LEFT_ALIGN, 3);
}

// special (waitstate=1): begin a fresh attempt. The ruleset config is stashed
// now and re-applied by Nuzlocke_ResetState() from inside NewGameInitData().
void StartNuzlockeNewAttempt(void)
{
    Nuzlocke_BeginRetry();
    SetMainCallback2(CB2_NewGame);
}
