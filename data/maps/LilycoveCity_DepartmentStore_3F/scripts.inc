LilycoveCity_DepartmentStore_3F_MapScripts::
	.byte 0

LilycoveCity_DepartmentStore_3F_EventScript_ClerkLeft::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart LilycoveCity_DepartmentStore_3F_Pokemart_Vitamins
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_3F_Pokemart_Vitamins:
	.2byte ITEM_PROTEIN
	.2byte ITEM_CALCIUM
	.2byte ITEM_IRON
	.2byte ITEM_ZINC
	.2byte ITEM_CARBOS
	.2byte ITEM_HP_UP
	pokemartlistend

LilycoveCity_DepartmentStore_3F_EventScript_ClerkRight::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart LilycoveCity_DepartmentStore_3F_Pokemart_StatBoosters
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_3F_Pokemart_StatBoosters:
	.2byte ITEM_X_SPEED
	.2byte ITEM_X_SP_ATK
	.2byte ITEM_X_ATTACK
	.2byte ITEM_X_DEFENSE
	.2byte ITEM_DIRE_HIT
	.2byte ITEM_GUARD_SPEC
	.2byte ITEM_X_ACCURACY
	pokemartlistend

@ docs/SPEC.md "Lilycove evolution-item sellers": guarantees every normal
@ evolution item any included Pokemon can need, so a randomized field-item pool
@ can never make an evolution impossible. Items stay gated behind reaching
@ Lilycove ("Delayed is acceptable. Impossible is not.").
LilycoveCity_DepartmentStore_3F_EventScript_EvolutionClerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	dynmultipush LilycoveCity_DepartmentStore_3F_Text_EvoCategoryStones, 0
	dynmultipush LilycoveCity_DepartmentStore_3F_Text_EvoCategoryTrade, 1
	dynmultipush LilycoveCity_DepartmentStore_3F_Text_EvoCategoryRare, 2
	dynmultistack 0, 0, FALSE, 4, 0, 0, DYN_MULTICHOICE_CB_NONE
	switch VAR_RESULT
	case 0, LilycoveCity_DepartmentStore_3F_EventScript_EvoMartStones
	case 1, LilycoveCity_DepartmentStore_3F_EventScript_EvoMartTrade
	case 2, LilycoveCity_DepartmentStore_3F_EventScript_EvoMartRare
	goto LilycoveCity_DepartmentStore_3F_EventScript_EvoClerkDone

LilycoveCity_DepartmentStore_3F_EventScript_EvoMartStones::
	pokemart LilycoveCity_DepartmentStore_3F_Pokemart_EvoStones
	goto LilycoveCity_DepartmentStore_3F_EventScript_EvoClerkDone

LilycoveCity_DepartmentStore_3F_EventScript_EvoMartTrade::
	pokemart LilycoveCity_DepartmentStore_3F_Pokemart_EvoTrade
	goto LilycoveCity_DepartmentStore_3F_EventScript_EvoClerkDone

LilycoveCity_DepartmentStore_3F_EventScript_EvoMartRare::
	pokemart LilycoveCity_DepartmentStore_3F_Pokemart_EvoRare
	goto LilycoveCity_DepartmentStore_3F_EventScript_EvoClerkDone

LilycoveCity_DepartmentStore_3F_EventScript_EvoClerkDone::
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
LilycoveCity_DepartmentStore_3F_Pokemart_EvoStones:
	.2byte ITEM_FIRE_STONE
	.2byte ITEM_WATER_STONE
	.2byte ITEM_THUNDER_STONE
	.2byte ITEM_LEAF_STONE
	.2byte ITEM_MOON_STONE
	.2byte ITEM_SUN_STONE
	.2byte ITEM_SHINY_STONE
	.2byte ITEM_DUSK_STONE
	.2byte ITEM_DAWN_STONE
	.2byte ITEM_ICE_STONE
	.2byte ITEM_EVERSTONE
	pokemartlistend

	.align 2
LilycoveCity_DepartmentStore_3F_Pokemart_EvoTrade:
	.2byte ITEM_LINKING_CORD
	.2byte ITEM_KINGS_ROCK
	.2byte ITEM_METAL_COAT
	.2byte ITEM_DRAGON_SCALE
	.2byte ITEM_UPGRADE
	.2byte ITEM_DUBIOUS_DISC
	.2byte ITEM_PROTECTOR
	.2byte ITEM_ELECTIRIZER
	.2byte ITEM_MAGMARIZER
	.2byte ITEM_REAPER_CLOTH
	.2byte ITEM_DEEP_SEA_TOOTH
	.2byte ITEM_DEEP_SEA_SCALE
	.2byte ITEM_PRISM_SCALE
	.2byte ITEM_SACHET
	.2byte ITEM_WHIPPED_DREAM
	.2byte ITEM_OVAL_STONE
	pokemartlistend

	.align 2
LilycoveCity_DepartmentStore_3F_Pokemart_EvoRare:
	.2byte ITEM_RAZOR_CLAW
	.2byte ITEM_RAZOR_FANG
	.2byte ITEM_BLACK_AUGURITE
	.2byte ITEM_PEAT_BLOCK
	.2byte ITEM_METAL_ALLOY
	.2byte ITEM_AUSPICIOUS_ARMOR
	.2byte ITEM_MALICIOUS_ARMOR
	.2byte ITEM_CHIPPED_POT
	.2byte ITEM_CRACKED_POT
	.2byte ITEM_UNREMARKABLE_TEACUP
	.2byte ITEM_MASTERPIECE_TEACUP
	.2byte ITEM_GALARICA_CUFF
	.2byte ITEM_GALARICA_WREATH
	.2byte ITEM_SWEET_APPLE
	.2byte ITEM_TART_APPLE
	.2byte ITEM_SYRUPY_APPLE
	.2byte ITEM_SCROLL_OF_DARKNESS
	.2byte ITEM_SCROLL_OF_WATERS
	.2byte ITEM_STRAWBERRY_SWEET
	.2byte ITEM_BERRY_SWEET
	.2byte ITEM_LOVE_SWEET
	.2byte ITEM_STAR_SWEET
	.2byte ITEM_CLOVER_SWEET
	.2byte ITEM_FLOWER_SWEET
	.2byte ITEM_RIBBON_SWEET
	pokemartlistend

LilycoveCity_DepartmentStore_3F_EventScript_TriathleteM::
	msgbox LilycoveCity_DepartmentStore_3F_Text_ItemsBestForTougheningPokemon, MSGBOX_NPC
	end

LilycoveCity_DepartmentStore_3F_EventScript_PokefanM::
	msgbox LilycoveCity_DepartmentStore_3F_Text_WantMoreEndurance, MSGBOX_NPC
	end

LilycoveCity_DepartmentStore_3F_EventScript_Woman::
	msgbox LilycoveCity_DepartmentStore_3F_Text_GaveCarbosToSpeedUpMon, MSGBOX_NPC
	end

LilycoveCity_DepartmentStore_3F_Text_ItemsBestForTougheningPokemon:
	.string "For quickly toughening up POKéMON,\n"
	.string "items are the best.\p"
	.string "PROTEIN boosts ATTACK,\n"
	.string "and CALCIUM raises SP. ATK.$"

LilycoveCity_DepartmentStore_3F_Text_WantMoreEndurance:
	.string "I want my POKéMON to have more\n"
	.string "endurance.\p"
	.string "I'm trying to decide whether to raise\n"
	.string "DEFENSE with IRON, or SP. DEF with ZINC.$"

LilycoveCity_DepartmentStore_3F_Text_GaveCarbosToSpeedUpMon:
	.string "I gave a CARBOS to my POKéMON,\n"
	.string "and its SPEED went up.$"

LilycoveCity_DepartmentStore_3F_Text_EvoCategoryStones:
	.string "EVOLUTION STONES$"

LilycoveCity_DepartmentStore_3F_Text_EvoCategoryTrade:
	.string "TRADE ITEMS$"

LilycoveCity_DepartmentStore_3F_Text_EvoCategoryRare:
	.string "RARE EVOLUTION ITEMS$"

