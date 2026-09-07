// ============================================================================
// Phase 8 - QoL / economy ruleset predicates (docs/SPEC.md "Catch rates",
// "R-button Ball shortcut", "Unlimited money"). See include/ruleset_qol.h.
// ============================================================================

#include "global.h"
#include "event_data.h"
#include "item.h"
#include "money.h"
#include "ruleset.h"
#include "ruleset_qol.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/ruleset.h"

#define BALL_NPC_999_COUNT 999

u32 Ruleset_CatchOddsMultiplier(void)
{
    switch (GetRulesetSetting(SETTING_CATCH_RATE))
    {
    case CATCHRATE_MODERATE: return 2;
    case CATCHRATE_LARGE:    return 4;
    default:                 return 1; // CATCHRATE_VANILLA, CATCHRATE_GUARANTEED
    }
}

bool32 Ruleset_CatchGuaranteed(void)
{
    return GetRulesetSetting(SETTING_CATCH_RATE) == CATCHRATE_GUARANTEED;
}

bool32 Ruleset_RButtonBallShortcutOn(void)
{
    return GetRulesetSetting(SETTING_R_BUTTON_BALL_SHORTCUT) != 0;
}

bool32 Ruleset_UnlimitedMoneyOn(void)
{
    return GetRulesetSetting(SETTING_UNLIMITED_MONEY) != 0;
}

void Ruleset_ApplyUnlimitedMoneyGrant(void)
{
    if (Ruleset_UnlimitedMoneyOn() && GetMoney(&gSaveBlock1Ptr->money) < MAX_MONEY)
        SetMoney(&gSaveBlock1Ptr->money, MAX_MONEY);
}

// docs/SPEC.md "999 Poke Ball NPC". special. gSpecialVar_Result:
//   0 = feature off, 1 = just handed over, 2 = already claimed, 3 = no bag room.
void TryGiveBallNpc999(void)
{
    if (GetRulesetSetting(SETTING_BALL_NPC_999) == 0)
        gSpecialVar_Result = 0;
    else if (FlagGet(FLAG_RULESET_BALL_NPC_999_CLAIMED))
        gSpecialVar_Result = 2;
    else if (!AddBagItem(ITEM_POKE_BALL, BALL_NPC_999_COUNT))
        gSpecialVar_Result = 3;
    else
    {
        FlagSet(FLAG_RULESET_BALL_NPC_999_CLAIMED);
        gSpecialVar_Result = 1;
    }
}
