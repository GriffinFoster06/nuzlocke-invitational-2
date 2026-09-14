// ============================================================================
// Phase 3 - Nuzlocke ruleset engine.
//
// Owns persistent run state (gSaveBlock3Ptr->nuzlocke) and enforces
// docs/SPEC.md's "Nuzlocke permadeath", "One encounter per location",
// "Dupes Clause", "Shiny Clause", "Nicknames", "No battle items" and
// "Whiteout". See include/nuzlocke.h for the API contract and the design
// notes referenced there.
//
// Consumers call the small predicate/hook functions here; this file never
// reaches into battle/menu internals itself. Species replacement stays the
// Phase 2 randomizer's job - the Dupes Clause only re-picks an encounter
// SLOT and asks Randomizer_WildSlotSpecies() what that slot resolves to.
// ============================================================================

#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "item.h"
#include "main.h"
#include "naming_screen.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "ruleset.h"
#include "script.h"
#include "string_util.h"
#include "nuzlocke.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"

#define NUZLOCKE_GRAVEYARD_BOX      (TOTAL_BOXES_COUNT - 1)
// The graveyard PC box name. MUST fit boxNames[] (BOX_NAME_LENGTH + 1 bytes,
// terminator included) - a longer string here overruns into boxWallpapers[0]
// and corrupts the PC (see the box-open crash this replaced). The static
// assert makes that a build error, not a runtime one.
static const u8 sGraveyardBoxName[] = _("HEAVEN");
STATIC_ASSERT(sizeof(sGraveyardBoxName) <= BOX_NAME_LENGTH + 1, GraveyardBoxNameFitsBoxNameSlot);

#define BITARR_GET(arr, i)    (((arr)[(i) >> 3] >> ((i) & 7)) & 1)
#define BITARR_SET(arr, i)    ((arr)[(i) >> 3] |=  (1 << ((i) & 7)))
#define BITARR_CLEAR(arr, i)  ((arr)[(i) >> 3] &= ~(1 << ((i) & 7)))

// Battle types that are never governed by the one-per-location rule.
#define NUZLOCKE_EXEMPT_ENCOUNTER_FLAGS                                         \
    (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED             \
   | BATTLE_TYPE_GHOST | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE)

struct NuzlockeEncounterTarget
{
    u8 present:1;
    u8 shiny:1;
    u8 duplicate:1;
    u8 ordinaryValid:1;
};

struct NuzlockeEncounterState
{
    bool8 active;
    bool8 ordinaryCaught;
    u16 location;
    struct NuzlockeEncounterTarget targets[2];
};

// Latched after the opposing party has been created and before battle setup.
static EWRAM_DATA struct NuzlockeEncounterState sEncounter = {0};

struct PendingNickname
{
    bool8 active;
    bool8 naming;
    bool8 inBox;
    u8 boxId;
    u8 position;
};

static EWRAM_DATA struct PendingNickname sPendingNickname = {0};

// Set by Nuzlocke_BeginRetry(); consumed by Nuzlocke_ResetState() on the next
// New Game so a fresh attempt keeps the same ruleset config.
static EWRAM_DATA bool8 sRetryPending = FALSE;
static EWRAM_DATA struct RulesetSettings sRetrySettings = {0};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Nuzlocke_ResetState(void)
{
    memset(&gSaveBlock3Ptr->nuzlocke, 0, sizeof(gSaveBlock3Ptr->nuzlocke));
    memset(&sPendingNickname, 0, sizeof(sPendingNickname));

    if (sRetryPending)
    {
        u32 keepSeed = sRetrySettings.runSeed;

        gSaveBlock3Ptr->ruleset = sRetrySettings;
        gSaveBlock3Ptr->ruleset.runStarted = FALSE;
        gSaveBlock3Ptr->ruleset.runActive = FALSE;
        if (GetRulesetSetting(SETTING_NEW_SEED_ON_RETRY))
            RerollRunSeed();
        else
            SetRunSeed(keepSeed);
        sRetryPending = FALSE;
    }

    memset(&sEncounter, 0, sizeof(sEncounter));
}

void Nuzlocke_BeginRun(void)
{
    bool32 strict = GetRulesetSetting(SETTING_PERMADEATH)
                 || GetRulesetSetting(SETTING_ONE_ENCOUNTER_PER_LOCATION)
                 || GetRulesetSetting(SETTING_WHITEOUT_BEHAVIOR) != WHITEOUT_VANILLA
                 || GetRulesetSetting(SETTING_NO_BATTLE_ITEMS)
                 || GetRulesetSetting(SETTING_FORCE_SET_BATTLE_STYLE)
                 || GetRulesetSetting(SETTING_NICKNAME_MODE) != NICK_OPTIONAL
                 || GetRulesetSetting(SETTING_CAP_MODE) != CAPMODE_OFF;

    SetRulesetRunActive(strict);
}

void Nuzlocke_BeginRetry(void)
{
    sRetrySettings = gSaveBlock3Ptr->ruleset;
    sRetryPending = TRUE;
}

bool32 Nuzlocke_RunIsActive(void)
{
    return gSaveBlock3Ptr->ruleset.runActive;
}

bool32 Nuzlocke_RunIsOver(void)
{
    return gSaveBlock3Ptr->nuzlocke.runOver;
}

// Run once per load (CB2_ContinueSavedGame). Repairs PC state that a save
// written before the graveyard-box name overflow could carry:
//  - out-of-range box wallpaper ids (the overflow wrote EOS/0xFF into
//    boxWallpapers[0]; opening the PC then indexes sWallpapers[] out of
//    bounds and jumps through a garbage pointer),
//  - a graveyard box name with no terminator inside its own slot (it only
//    "worked" because the stray 0xFF in the next field terminated it).
// Harmless on a clean save: nothing is out of range, so nothing changes.
void Nuzlocke_RepairStorage(void)
{
    SanitizeBoxWallpapers();

    if (gPokemonStoragePtr->currentBox >= TOTAL_BOXES_COUNT)
        gPokemonStoragePtr->currentBox = 0;

    if (gSaveBlock3Ptr->nuzlocke.graveyardBoxNamed)
    {
        StringCopy(GetBoxNamePtr(NUZLOCKE_GRAVEYARD_BOX), sGraveyardBoxName);
        SetBoxWallpaperSky(NUZLOCKE_GRAVEYARD_BOX);
    }
}

// ---------------------------------------------------------------------------
// Setting predicates
// ---------------------------------------------------------------------------

// docs/SPEC.md "Nuzlocke rules start gate": permadeath and one-encounter-per-
// location do not begin until the player has received their first Poke Balls -
// before that there is nothing to catch and nothing worth protecting. The gate
// latches the first time the player holds any Poke Ball (Birch's gift, a shop,
// the 999-ball NPC, Pickup, ...) and never closes again for the attempt.
// Nuzlocke_ResetState() clears the bit on a new game / retry.
bool32 Nuzlocke_RulesGateOpen(void)
{
    if (gSaveBlock3Ptr->nuzlocke.rulesGateOpen)
        return TRUE;

    if (HasAtLeastOnePokeBall())
    {
        gSaveBlock3Ptr->nuzlocke.rulesGateOpen = TRUE;
        return TRUE;
    }

    return FALSE;
}

bool32 Nuzlocke_PermadeathOn(void)
{
    return GetRulesetSetting(SETTING_PERMADEATH) != 0 && Nuzlocke_RulesGateOpen();
}

bool32 Nuzlocke_OneEncounterPerLocationOn(void)
{
    return GetRulesetSetting(SETTING_ONE_ENCOUNTER_PER_LOCATION) != 0 && Nuzlocke_RulesGateOpen();
}

bool32 Nuzlocke_DupesClauseOn(void)
{
    return GetRulesetSetting(SETTING_DUPES_CLAUSE) != 0;
}

bool32 Nuzlocke_ShinyClauseOn(void)
{
    return GetRulesetSetting(SETTING_SHINY_CLAUSE) != 0;
}

bool32 Nuzlocke_ForcedNicknamesOn(void)
{
    return GetRulesetSetting(SETTING_NICKNAME_MODE) != NICK_OPTIONAL;
}

bool32 Nuzlocke_StrictNicknamesOn(void)
{
    return Nuzlocke_ForcedNicknamesOn();
}

// Script-facing wrapper: the starter hand-off in Birch's lab is a plain
// scripted gift, so it doesn't go through the caught-mon / hatched-egg paths
// that already honour Nuzlocke_ForcedNicknamesOn(). See
// LittlerootTown_ProfessorBirchsLab_EventScript_GiveStarterEvent.
bool16 AreNicknamesForced(void)
{
    return Nuzlocke_ForcedNicknamesOn();
}

static struct BoxPokemon *GetPendingNicknameMon(void)
{
    if (!sPendingNickname.active)
        return NULL;
    if (sPendingNickname.inBox)
        return GetBoxedMonPtr(sPendingNickname.boxId, sPendingNickname.position);
    return &gParties[B_TRAINER_PLAYER][sPendingNickname.position].box;
}

static bool32 MonStillNeedsMandatoryNickname(struct BoxPokemon *boxMon)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);

    if (species == SPECIES_NONE || GetBoxMonData(boxMon, MON_DATA_IS_EGG))
        return FALSE;
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, nickname);
    return StringCompare(nickname, GetSpeciesName(species)) == 0;
}

static void CommitMandatoryNickname(void)
{
    struct BoxPokemon *boxMon = GetPendingNicknameMon();

    if (boxMon != NULL)
        SetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar2);
    memset(&sPendingNickname, 0, sizeof(sPendingNickname));
    SetMainCallback2(CB2_ReturnToField);
}

void Nuzlocke_QueueMandatoryNickname(struct Pokemon *mon)
{
    u32 personality;

    if (!Nuzlocke_ForcedNicknamesOn() || GetMonData(mon, MON_DATA_IS_EGG))
        return;
    personality = GetMonData(mon, MON_DATA_PERSONALITY);

    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE
         && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_PERSONALITY) == personality)
        {
            sPendingNickname.active = TRUE;
            sPendingNickname.inBox = FALSE;
            sPendingNickname.position = i;
            return;
        }
    }

    for (u32 boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        for (u32 position = 0; position < IN_BOX_COUNT; position++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, position);
            if (GetBoxMonData(boxMon, MON_DATA_SPECIES) != SPECIES_NONE
             && GetBoxMonData(boxMon, MON_DATA_PERSONALITY) == personality)
            {
                sPendingNickname.active = TRUE;
                sPendingNickname.inBox = TRUE;
                sPendingNickname.boxId = boxId;
                sPendingNickname.position = position;
                return;
            }
        }
    }
}

void Nuzlocke_TryPromptMandatoryNickname(void)
{
    struct BoxPokemon *boxMon;
    enum Species species;

    if (!sPendingNickname.active || sPendingNickname.naming
     || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return;

    boxMon = GetPendingNicknameMon();
    if (boxMon == NULL || !MonStillNeedsMandatoryNickname(boxMon))
    {
        memset(&sPendingNickname, 0, sizeof(sPendingNickname));
        return;
    }

    species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar2);
    sPendingNickname.naming = TRUE;
    DoNamingScreen(NAMING_SCREEN_NICKNAME, gStringVar2, species,
                   GetBoxMonGender(boxMon), GetBoxMonData(boxMon, MON_DATA_PERSONALITY),
                   CommitMandatoryNickname);
}

// ---------------------------------------------------------------------------
// Location tags
// ---------------------------------------------------------------------------

u32 Nuzlocke_CurrentLocationTag(void)
{
    return GetCurrentRegionMapSectionId();
}

bool32 Nuzlocke_LocationIsUsed(u32 tag)
{
    if (tag >= MAPSEC_COUNT)
        return FALSE;
    return BITARR_GET(gSaveBlock3Ptr->nuzlocke.locationUsed, tag);
}

bool32 Nuzlocke_LocationResolvedByCatch(u32 tag)
{
    if (tag >= MAPSEC_COUNT)
        return FALSE;
    return BITARR_GET(gSaveBlock3Ptr->nuzlocke.locationCaught, tag);
}

void Nuzlocke_MarkLocationUsed(u32 tag, bool32 byCatch)
{
    if (tag >= MAPSEC_COUNT)
        return;
    BITARR_SET(gSaveBlock3Ptr->nuzlocke.locationUsed, tag);
    if (byCatch)
        BITARR_SET(gSaveBlock3Ptr->nuzlocke.locationCaught, tag);
}

void Nuzlocke_ClearLocation(u32 tag)
{
    if (tag >= MAPSEC_COUNT)
        return;
    BITARR_CLEAR(gSaveBlock3Ptr->nuzlocke.locationUsed, tag);
    BITARR_CLEAR(gSaveBlock3Ptr->nuzlocke.locationCaught, tag);
}

void Nuzlocke_NoteWildEncounterStart(bool32 scripted)
{
    u32 i;
    bool32 locationOpen;

    (void)scripted;
    memset(&sEncounter, 0, sizeof(sEncounter));
    sEncounter.active = TRUE;
    sEncounter.location = Nuzlocke_CurrentLocationTag();
    locationOpen = Nuzlocke_OneEncounterPerLocationOn()
                && !Nuzlocke_LocationIsUsed(sEncounter.location);

    for (i = 0; i < ARRAY_COUNT(sEncounter.targets); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        enum Species species = GetMonData(mon, MON_DATA_SPECIES);
        struct NuzlockeEncounterTarget *target = &sEncounter.targets[i];

        if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_SANITY_IS_EGG))
            continue;

        target->present = TRUE;
        target->shiny = Nuzlocke_ShinyClauseOn() && IsMonShiny(mon);
        target->duplicate = Nuzlocke_DupesClauseOn() && Nuzlocke_IsFamilyOwned(species);
        target->ordinaryValid = locationOpen && !target->shiny && !target->duplicate;

        // Encounter history is about the encounter, not the eventual outcome.
        // Snapshot duplicate status first so this encounter does not disqualify itself.
        if (target->ordinaryValid)
            Nuzlocke_MarkFamilyOwned(species);
    }
}

// Apply an encounter's outcome to its location.
static void NuzlockeConsumeLocation(u32 outcome)
{
    bool32 hasOrdinaryEncounter = FALSE;
    u32 i;

    if (!sEncounter.active)
        return;

    for (i = 0; i < ARRAY_COUNT(sEncounter.targets); i++)
        hasOrdinaryEncounter |= sEncounter.targets[i].ordinaryValid;

    if (!hasOrdinaryEncounter)
        return;

    if (outcome == B_OUTCOME_CAUGHT && sEncounter.ordinaryCaught)
    {
        Nuzlocke_MarkLocationUsed(sEncounter.location, TRUE);
        return;
    }

    // Strict default: killing, fleeing or failing to catch also consumes.
    if (outcome != 0
     && GetRulesetSetting(SETTING_ENCOUNTER_CONSUMED_MODE) == ENCCONSUMED_STRICT)
        Nuzlocke_MarkLocationUsed(sEncounter.location, FALSE);
}

static void NuzlockeFinishEncounter(void)
{
    NuzlockeConsumeLocation(gBattleOutcome);
    memset(&sEncounter, 0, sizeof(sEncounter));
}

void Nuzlocke_HandleWildBattleEnd(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    if (gBattleTypeFlags & NUZLOCKE_EXEMPT_ENCOUNTER_FLAGS)
        return;
    NuzlockeFinishEncounter();
}

void Nuzlocke_HandleScriptedBattleEnd(void)
{
    // Static / legendary encounters consume their location (planning decision),
    // but the script - not this rule - decides whether they are catchable.
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    NuzlockeFinishEncounter();
}

void Nuzlocke_HandleSafariBattleEnd(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    NuzlockeFinishEncounter();
}

static u32 GetEncounterTargetIndex(enum BattlerId battler)
{
    u32 partyIndex;

    if (battler >= MAX_BATTLERS_COUNT || IsOnPlayerSide(battler))
        return ARRAY_COUNT(sEncounter.targets);
    partyIndex = gBattlerPartyIndexes[battler];
    if (partyIndex >= ARRAY_COUNT(sEncounter.targets))
        return ARRAY_COUNT(sEncounter.targets);
    return partyIndex;
}

bool32 Nuzlocke_CanCatchBattler(enum BattlerId battler)
{
    u32 index;

    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return TRUE;
    if (gBattleTypeFlags & NUZLOCKE_EXEMPT_ENCOUNTER_FLAGS)
        return TRUE;

    index = GetEncounterTargetIndex(battler);
    if (!sEncounter.active || index >= ARRAY_COUNT(sEncounter.targets))
        return FALSE;
    return sEncounter.targets[index].shiny || sEncounter.targets[index].ordinaryValid;
}

bool32 Nuzlocke_CanCatchCurrentEncounter(void)
{
    enum BattlerId battler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

    if (!IsBattlerAlive(battler))
        battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    return Nuzlocke_CanCatchBattler(battler);
}

void Nuzlocke_NoteCaughtBattler(enum BattlerId battler)
{
    u32 index = GetEncounterTargetIndex(battler);

    if (sEncounter.active && index < ARRAY_COUNT(sEncounter.targets))
        sEncounter.ordinaryCaught = sEncounter.targets[index].ordinaryValid;
}

// ---------------------------------------------------------------------------
// Dupes Clause
// ---------------------------------------------------------------------------

#define FAMILY_BITSET_BYTES  ROUND_BITS_TO_BYTES(NUM_SPECIES)

// Invariant: any species ID that did not come off a live struct Pokemon must
// pass IsSpeciesEnabled() before it reaches a gSpeciesInfo accessor. The
// sanitizing accessors (GetSpeciesEvolutions, SpeciesToNationalPokedexNum, ...)
// assert on a disabled ID, and SPECIES_LUGIA_SHADOW (1435) is a reserved
// upstream form ID with no data entry sitting in the middle of the range.
static bool32 SpeciesInRange(u32 s)
{
    return s != SPECIES_NONE && s < NUM_SPECIES && IsSpeciesEnabled(s);
}

// Grow `set` to the full evolutionary family closure of its current members
// using only cheap forward reads (GetSpeciesEvolutions is an O(1) pointer
// read). Walking the reverse edge for free is what makes GetSpeciesPreEvolution
// - a full O(N*evos) reverse scan - unnecessary here.
static void ExpandFamilyClosure(u8 *set)
{
    bool32 changed = TRUE;
    u32 s, j;

    while (changed)
    {
        changed = FALSE;

        for (s = 1; s < NUM_SPECIES; s++)
        {
            const struct Evolution *evos;
            bool32 sIn;

            if (!IsSpeciesEnabled(s))
                continue;

            evos = GetSpeciesEvolutions(s);
            sIn = BITARR_GET(set, s);

            if (evos == NULL)
                continue;
            for (j = 0; evos[j].method != EVOLUTIONS_END; j++)
            {
                u32 tgt = evos[j].targetSpecies;

                if (!SpeciesInRange(tgt))
                    continue;
                if (sIn && !BITARR_GET(set, tgt))
                {
                    BITARR_SET(set, tgt);
                    changed = TRUE;
                }
                else if (!sIn && BITARR_GET(set, tgt))
                {
                    BITARR_SET(set, s);
                    sIn = TRUE;
                    changed = TRUE;
                }
            }
        }

        // Pull in every species sharing a National Dex number with a current
        // member. This links regional forms and their evolution branches.
        for (s = 1; s < NUM_SPECIES; s++)
        {
            enum NationalDexOrder dex;

            if (!BITARR_GET(set, s))
                continue;
            dex = SpeciesToNationalPokedexNum(s);
            for (j = 1; j < NUM_SPECIES; j++)
            {
                if (!IsSpeciesEnabled(j))
                    continue;
                if (!BITARR_GET(set, j) && SpeciesToNationalPokedexNum(j) == dex)
                {
                    BITARR_SET(set, j);
                    changed = TRUE;
                }
            }
        }
    }
}

void Nuzlocke_MarkFamilyOwned(enum Species species)
{
    u8 set[FAMILY_BITSET_BYTES];
    u32 i;

    if (!SpeciesInRange(species))
        return;

    memset(set, 0, sizeof(set));
    BITARR_SET(set, species);
    ExpandFamilyClosure(set);

    for (i = 0; i < FAMILY_BITSET_BYTES; i++)
        gSaveBlock3Ptr->nuzlocke.familyOwned[i] |= set[i];
}

bool32 Nuzlocke_IsFamilyOwned(enum Species species)
{
    if (!SpeciesInRange(species))
        return FALSE;
    return BITARR_GET(gSaveBlock3Ptr->nuzlocke.familyOwned, species);
}

void Nuzlocke_OnMonObtained(struct Pokemon *mon, bool32 fromWildCatch)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);

    // A wild encounter was classified and recorded when it began. In
    // particular, a bonus shiny must not enter ordinary Dupes history.
    if (!fromWildCatch)
        Nuzlocke_MarkFamilyOwned(species);

    // A wild catch's location is handled by the battle-end hook. Every other
    // acquisition (script gift, fossil revival, gift/daycare egg) consumes the
    // location it is received in.
    if (!fromWildCatch && Nuzlocke_RunIsActive() && Nuzlocke_OneEncounterPerLocationOn())
        Nuzlocke_MarkLocationUsed(Nuzlocke_CurrentLocationTag(), TRUE);
}

bool32 Nuzlocke_DupesRerollActiveHere(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_RulesGateOpen() || !Nuzlocke_DupesClauseOn())
        return FALSE;
    // No point avoiding dupes on a route whose encounter is already spent.
    if (Nuzlocke_OneEncounterPerLocationOn()
     && Nuzlocke_LocationIsUsed(Nuzlocke_CurrentLocationTag()))
        return FALSE;
    return TRUE;
}

// ---------------------------------------------------------------------------
// Permadeath
// ---------------------------------------------------------------------------

bool32 Nuzlocke_MonIsDead(struct Pokemon *mon)
{
    if (!Nuzlocke_PermadeathOn())
        return FALSE;
    return GetMonData(mon, MON_DATA_IS_DEAD) != 0;
}

bool32 Nuzlocke_MonCanBattle(struct Pokemon *mon)
{
    return !Nuzlocke_MonIsDead(mon);
}

bool32 Nuzlocke_MonCanProvideGameplayBenefit(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE
     || GetMonData(mon, MON_DATA_SANITY_IS_EGG))
        return FALSE;
    return !Nuzlocke_MonIsDead(mon);
}

static void MoveDeadMonToGraveyard(struct Pokemon *mon)
{
    s16 pos = GetFirstFreeBoxSpot(NUZLOCKE_GRAVEYARD_BOX);

    if (pos < 0)
        return; // graveyard full: the dead mon stays in the party, inert at 0 HP

    if (!gSaveBlock3Ptr->nuzlocke.graveyardBoxNamed)
    {
        StringCopy(GetBoxNamePtr(NUZLOCKE_GRAVEYARD_BOX), sGraveyardBoxName);
        SetBoxWallpaperSky(NUZLOCKE_GRAVEYARD_BOX);
        gSaveBlock3Ptr->nuzlocke.graveyardBoxNamed = TRUE;
    }

    SetBoxMonAt(NUZLOCKE_GRAVEYARD_BOX, pos, &mon->box);
    ZeroMonData(mon);
}

static void MarkMonDead(struct Pokemon *mon, bool32 moveToGraveyard)
{
    bool8 dead = TRUE;

    if (!Nuzlocke_PermadeathOn())
        return;
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE
     || GetMonData(mon, MON_DATA_SANITY_IS_EGG)
     || GetMonData(mon, MON_DATA_IS_DEAD))
        return;

    SetMonData(mon, MON_DATA_IS_DEAD, &dead);
    if (gSaveBlock3Ptr->nuzlocke.deathCount < 0xFFFF)
        gSaveBlock3Ptr->nuzlocke.deathCount++;

    if (moveToGraveyard && GetRulesetSetting(SETTING_GRAVEYARD_BOX))
        MoveDeadMonToGraveyard(mon);
}

void Nuzlocke_MarkMonDead(struct Pokemon *mon)
{
    MarkMonDead(mon, TRUE);
}

void Nuzlocke_RecordBattleFaint(enum BattlerId battler)
{
    if (!Nuzlocke_PermadeathOn() || !Nuzlocke_RunIsActive())
        return;
    if (battler >= gBattlersCount || GetBattlerTrainer(battler) != B_TRAINER_PLAYER)
        return;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED
                          | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE))
        return;
    if (gBattlerPartyIndexes[battler] < PARTY_SIZE)
        MarkMonDead(&gParties[B_TRAINER_PLAYER][gBattlerPartyIndexes[battler]], FALSE);
}

void Nuzlocke_ProcessPostBattleDeaths(void)
{
    u32 i;

    if (!Nuzlocke_PermadeathOn() || !Nuzlocke_RunIsActive())
        return;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_CATCH_TUTORIAL
                          | BATTLE_TYPE_RECORDED | BATTLE_TYPE_FIRST_BATTLE))
        return;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (GetMonData(mon, MON_DATA_SANITY_IS_EGG))
            continue;
        if (GetMonData(mon, MON_DATA_HP) == 0)
            MarkMonDead(mon, FALSE);
    }

    if (GetRulesetSetting(SETTING_GRAVEYARD_BOX))
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

            if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE
             && GetMonData(mon, MON_DATA_IS_DEAD))
                MoveDeadMonToGraveyard(mon);
        }
    }

    if (GetRulesetSetting(SETTING_GRAVEYARD_BOX))
        CompactPartySlots();
    CalculatePlayerPartyCount();
}

u32 Nuzlocke_GetDeathCount(void)
{
    return gSaveBlock3Ptr->nuzlocke.deathCount;
}

u32 Nuzlocke_CountLocationsCaught(void)
{
    u32 i, count = 0;

    for (i = 0; i < MAPSEC_COUNT; i++)
    {
        if (BITARR_GET(gSaveBlock3Ptr->nuzlocke.locationCaught, i))
            count++;
    }
    return count;
}

// ---------------------------------------------------------------------------
// No battle items
// ---------------------------------------------------------------------------

bool32 Nuzlocke_BattleItemsBlocked(void)
{
    if (!GetRulesetSetting(SETTING_NO_BATTLE_ITEMS))
        return FALSE;
    // Wild battles still expose Poke Balls (handled by Nuzlocke_BattleBagBallsOnly);
    // only trainer-battle combat items are forbidden outright.
    return (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
}

bool32 Nuzlocke_BattleBagBallsOnly(void)
{
    if (!GetRulesetSetting(SETTING_NO_BATTLE_ITEMS))
        return FALSE;
    if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK))
        return FALSE;
    return TRUE;
}

// ---------------------------------------------------------------------------
// Whiteout / run over
// ---------------------------------------------------------------------------

static bool32 AnyLivingMonAnywhere(void)
{
    u32 i, boxId, pos;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE
         || GetMonData(mon, MON_DATA_SANITY_IS_EGG))
            continue;
        if (GetMonData(mon, MON_DATA_HP) != 0 && !GetMonData(mon, MON_DATA_IS_DEAD))
            return TRUE;
    }
    for (boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, pos);

            if (GetBoxMonData(boxMon, MON_DATA_SPECIES) == SPECIES_NONE
             || GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG))
                continue;
            if (!GetBoxMonData(boxMon, MON_DATA_IS_DEAD))
                return TRUE;
        }
    }

    for (i = 0; i < DAYCARE_MON_COUNT; i++)
    {
        struct BoxPokemon *boxMon = &gSaveBlock1Ptr->daycare.mons[i].mon;

        if (GetBoxMonData(boxMon, MON_DATA_SPECIES) != SPECIES_NONE
         && !GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG)
         && !GetBoxMonData(boxMon, MON_DATA_IS_DEAD))
            return TRUE;
    }
#if IS_FRLG
    if (GetBoxMonData(&gSaveBlock1Ptr->route5DayCareMon.mon, MON_DATA_SPECIES) != SPECIES_NONE
     && !GetBoxMonData(&gSaveBlock1Ptr->route5DayCareMon.mon, MON_DATA_SANITY_IS_EGG)
     && !GetBoxMonData(&gSaveBlock1Ptr->route5DayCareMon.mon, MON_DATA_IS_DEAD))
        return TRUE;
#endif
    return FALSE;
}

bool32 Nuzlocke_ShouldEndRunOnWhiteout(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_PermadeathOn())
        return FALSE;
    if (GetRulesetSetting(SETTING_WHITEOUT_BEHAVIOR) != WHITEOUT_RUN_OVER)
        return FALSE;
    return !AnyLivingMonAnywhere();
}

void Nuzlocke_SetRunOver(void)
{
    gSaveBlock3Ptr->nuzlocke.runOver = TRUE;
}
