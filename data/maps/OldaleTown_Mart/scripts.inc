OldaleTown_Mart_MapScripts::
	.byte 0

OldaleTown_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	goto_if_set FLAG_ADVENTURE_STARTED, OldaleTown_Mart_ExpandedItems
	pokemart OldaleTown_Mart_Pokemart_Basic
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
OldaleTown_Mart_Pokemart_Basic:
	.2byte ITEM_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	pokemartlistend

OldaleTown_Mart_ExpandedItems::
	pokemart OldaleTown_Mart_Pokemart_Expanded
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
OldaleTown_Mart_Pokemart_Expanded:
	.2byte ITEM_POKE_BALL
	.2byte ITEM_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	pokemartlistend

@ docs/SPEC.md "999 Poke Ball NPC": replaces Randolocke's 999 Ultra Ball NPC.
@ Hands over 999 ordinary Poke Balls once, gated on SETTING_BALL_NPC_999.
OldaleTown_Mart_EventScript_Woman::
	lock
	faceplayer
	goto_if_set FLAG_ADVENTURE_STARTED, OldaleTown_Mart_EventScript_BallNpc999
	msgbox OldaleTown_Mart_Text_PokeBallsAreSoldOut, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_BallNpc999::
	special TryGiveBallNpc999
	switch VAR_RESULT
	case 1, OldaleTown_Mart_EventScript_BallNpc999Gave
	case 2, OldaleTown_Mart_EventScript_BallNpc999AlreadyGot
	case 3, OldaleTown_Mart_EventScript_BallNpc999BagFull
	msgbox OldaleTown_Mart_Text_ImGoingToBuyPokeBalls, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_BallNpc999Gave::
	playfanfare MUS_OBTAIN_ITEM
	waitfanfare
	msgbox OldaleTown_Mart_Text_Take999PokeBalls, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_BallNpc999AlreadyGot::
	msgbox OldaleTown_Mart_Text_HopeThePokeBallsHelped, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_BallNpc999BagFull::
	msgbox OldaleTown_Mart_Text_MakeRoomForPokeBalls, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_Boy::
	msgbox OldaleTown_Mart_Text_RestoreHPWithPotion, MSGBOX_NPC
	end

OldaleTown_Mart_Text_PokeBallsAreSoldOut:
	.string "The clerk says they're all sold out.\n"
	.string "I can't buy any POKé BALLS.$"

OldaleTown_Mart_Text_ImGoingToBuyPokeBalls:
	.string "I'm going to buy a bunch of POKé BALLS\n"
	.string "and catch a bunch of POKéMON!$"

OldaleTown_Mart_Text_Take999PokeBalls:
	.string "You're taking on a NUZLOCKE?\p"
	.string "Then running out of POKé BALLS is the\n"
	.string "last thing you should be worrying about.\p"
	.string "Here, take 999 of them. Go catch your\n"
	.string "team and give it everything you've got!$"

OldaleTown_Mart_Text_HopeThePokeBallsHelped:
	.string "Those 999 POKé BALLS should see you\n"
	.string "through. Good luck out there!$"

OldaleTown_Mart_Text_MakeRoomForPokeBalls:
	.string "Your BAG's too full for me to hand over\n"
	.string "that many POKé BALLS. Make some room\l"
	.string "and come back!$"

OldaleTown_Mart_Text_RestoreHPWithPotion:
	.string "If a POKéMON gets hurt and loses its HP\n"
	.string "and faints, it won't be able to battle.\p"
	.string "To prevent your POKéMON from fainting,\n"
	.string "restore its HP with a POTION.$"

