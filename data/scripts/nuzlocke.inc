@ Phase 3 - Nuzlocke ruleset engine (docs/SPEC.md "Whiteout").
@ Shown once a run has ended: no living Pokemon anywhere after a whiteout.

EventScript_NuzlockeRunOver::
	lockall
	special BufferNuzlockeRunOverStats
	msgbox Text_NuzlockeRunOver, MSGBOX_DEFAULT
	special StartNuzlockeNewAttempt
	releaseall
	end

Text_NuzlockeRunOver:
	.string "The run is over.\p"
	.string "Seed: {STR_VAR_1}\n"
	.string "Deaths: {STR_VAR_2}   Caught: {STR_VAR_3}\p"
	.string "Press A to begin a new attempt.$"
