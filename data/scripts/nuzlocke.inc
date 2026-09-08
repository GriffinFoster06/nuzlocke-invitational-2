@ Phase 3 - Nuzlocke ruleset engine (docs/SPEC.md "Whiteout").
@ Shown once a run has ended: no living Pokemon anywhere after a whiteout.

EventScript_NuzlockeRunOver::
	lockall
	special BufferNuzlockeRunOverStats
	msgbox Text_NuzlockeRunOver, MSGBOX_DEFAULT
EventScript_NuzlockeRunOver_Confirm:
	msgbox Text_NuzlockeRunOverConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_NuzlockeRunOver_Confirm
	special StartNuzlockeNewAttempt
	releaseall
	end

Text_NuzlockeRunOver:
	.string "The run is over.\p"
	.string "Seed: {STR_VAR_1}\n"
	.string "Deaths: {STR_VAR_2}   Caught: {STR_VAR_3}$"

Text_NuzlockeRunOverConfirm:
	.string "Begin a new attempt?\p"
	.string "This wipes the current save and\n"
	.string "starts a fresh run.$"
