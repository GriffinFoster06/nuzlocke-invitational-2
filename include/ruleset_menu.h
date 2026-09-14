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

#endif // GUARD_RULESET_MENU_H
