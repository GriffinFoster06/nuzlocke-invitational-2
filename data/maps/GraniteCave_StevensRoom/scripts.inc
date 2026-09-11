GraniteCave_StevensRoom_MapScripts::
	.byte 0

GraniteCave_StevensRoom_EventScript_Steven::
	lock
	faceplayer
	goto_if_set FLAG_DELIVERED_STEVEN_LETTER, GraniteCave_StevensRoom_EventScript_GiveSteelWing
	msgbox GraniteCave_StevensRoom_Text_ImSteven, MSGBOX_DEFAULT
	checkitem ITEM_LETTER
	goto_if_eq VAR_RESULT, FALSE, GraniteCave_StevensRoom_EventScript_NoLegacyLetter
	removeitem ITEM_LETTER

GraniteCave_StevensRoom_EventScript_NoLegacyLetter::
	setflag FLAG_DELIVERED_STEVEN_LETTER

GraniteCave_StevensRoom_EventScript_GiveSteelWing::
	msgbox GraniteCave_StevensRoom_Text_ThankYouTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM_STEEL_WING
	goto_if_eq VAR_RESULT, FALSE, GraniteCave_StevensRoom_EventScript_BagFull
	setflag FLAG_REGISTERED_STEVEN_POKENAV
	setflag FLAG_HIDE_GRANITE_CAVE_STEVEN
	removeobject LOCALID_GRANITE_CAVE_STEVEN
	release
	end

GraniteCave_StevensRoom_EventScript_StevenExitNorth::
	applymovement LOCALID_GRANITE_CAVE_STEVEN, GraniteCave_StevensRoom_Movement_StevenExit
	waitmovement 0
	return

GraniteCave_StevensRoom_EventScript_StevenExitWestEast::
	applymovement LOCALID_PLAYER, GraniteCave_StevensRoom_Movement_PlayerTurnTowardExit
	applymovement LOCALID_GRANITE_CAVE_STEVEN, GraniteCave_StevensRoom_Movement_StevenExit
	waitmovement 0
	return

GraniteCave_StevensRoom_EventScript_StevenExitSouth::
	applymovement LOCALID_PLAYER, GraniteCave_StevensRoom_Movement_PlayerTurnTowardExit
	applymovement LOCALID_GRANITE_CAVE_STEVEN, GraniteCave_StevensRoom_Movement_StevenExitSouth
	waitmovement 0
	return

GraniteCave_StevensRoom_EventScript_BagFull::
	msgbox GraniteCave_StevensRoom_Text_OhBagIsFull, MSGBOX_DEFAULT
	release
	end

GraniteCave_StevensRoom_Movement_StevenExit:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	delay_8
	step_end

GraniteCave_StevensRoom_Movement_PlayerTurnTowardExit:
	delay_16
	delay_16
	delay_16
	walk_in_place_faster_up
	step_end

GraniteCave_StevensRoom_Movement_StevenExitSouth:
	walk_left
	walk_up
	walk_up
	walk_up
	walk_right
	walk_up
	walk_up
	delay_8
	step_end

GraniteCave_StevensRoom_Text_ImSteven:
	.string "My name is STEVEN.\p"
	.string "I'm interested in rare stones,\n"
	.string "so I travel here and there.$"

GraniteCave_StevensRoom_Text_ThankYouTakeThis:
	.string "STEVEN: It's good to meet a capable\n"
	.string "TRAINER. Please take this TM.$"

GraniteCave_StevensRoom_Text_CouldBecomeChampionLetsRegister:
	.string "STEVEN: Your POKéMON appear quite\n"
	.string "capable.\p"
	.string "If you keep training, you could even\n"
	.string "become the CHAMPION of the POKéMON\l"
	.string "LEAGUE one day. That's what I think.\p"
	.string "I know, since we've gotten to know each\n"
	.string "other, let's register one another in\l"
	.string "our POKéNAVS.\p"
	.string "… … … … … …$"

GraniteCave_StevensRoom_Text_RegisteredSteven:
	.string "Registered STEVEN\n"
	.string "in the POKéNAV.$"

GraniteCave_StevensRoom_Text_IveGotToHurryAlong:
	.string "Now, I've got to hurry along.$"

GraniteCave_StevensRoom_Text_OhBagIsFull:
	.string "Your BAG is full. Make room and\n"
	.string "come back for the TM.$"
