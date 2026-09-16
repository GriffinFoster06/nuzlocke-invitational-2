@ Phase 3 - Nuzlocke ruleset engine (docs/SPEC.md "Whiteout").
@ Shown once a run has ended: no living Pokemon anywhere after a whiteout.

EventScript_NuzlockeRunOver::
	lockall
	special BufferNuzlockeRunOverStats
	msgbox Text_NuzlockeRunOver, MSGBOX_DEFAULT
EventScript_NuzlockeRunOver_Confirm:
	msgbox Text_NuzlockeRunOverConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_NuzlockeRunOver_ExportReminder
	special StartNuzlockeNewAttempt
	releaseall
	end

@ Phase 12A (docs/CLAUDE_HANDOFF.md): NO used to loop this prompt silently
@ forever. It now explains why someone would say no - to export the Run
@ Report first - then re-asks, instead of looking like a broken choice.
EventScript_NuzlockeRunOver_ExportReminder:
	msgbox Text_NuzlockeRunOverExportReminder, MSGBOX_DEFAULT
	goto EventScript_NuzlockeRunOver_Confirm

Text_NuzlockeRunOver:
	.string "The run is over.\p"
	.string "Seed: {STR_VAR_1}\n"
	.string "Deaths: {STR_VAR_2}   Caught: {STR_VAR_3}\p"
	.string "A Run Report for this attempt was\n"
	.string "saved to this save file.\p"
	.string "To export it, run tools/export_run.py\n"
	.string "on this .sav file from a computer.$"

Text_NuzlockeRunOverConfirm:
	.string "Begin a new attempt?\p"
	.string "This wipes the current save and\n"
	.string "starts a fresh run.$"

Text_NuzlockeRunOverExportReminder:
	.string "This save still holds the Run Report\n"
	.string "for the attempt that just ended.\p"
	.string "If you want to export it, save and\n"
	.string "reset now instead of starting over.$"
