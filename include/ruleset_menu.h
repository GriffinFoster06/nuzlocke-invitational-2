#ifndef GUARD_RULESET_MENU_H
#define GUARD_RULESET_MENU_H

// Settings screen for the Nuzlocke-Randomizer ruleset. Set
// gMain.savedCallback before calling.
void CB2_InitRulesetMenu(void);

// Phase 11A.6: New-Game-only pre-run settings wizard. Shows only the
// generation-locked settings (plus preset/seed) and exits straight into
// CB2_NewGame on "Start Game" - no gMain.savedCallback needed. Call this in
// place of SetMainCallback2(CB2_NewGame) from the fresh-New-Game chain only;
// never from the Nuzlocke retry path (see src/ruleset_menu.c for details).
void RulesetMenu_EnterNewGameWizard(void);

// Phase 12A: writes the enabled-generation list ("1,2,3,...,9", or "none")
// into dst, EOS-terminated. Shared by the wizard confirmation screen and the
// in-run Run Information overlay (src/ruleset_field.c). dst must be able to
// hold the longest case: "1,2,3,4,5,6,7,8,9" (17 chars) + EOS.
void RulesetMenu_BuildEnabledGenString(u8 *dst);

#endif // GUARD_RULESET_MENU_H
