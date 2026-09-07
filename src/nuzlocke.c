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
#include "main.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "ruleset.h"
#include "string_util.h"
#include "nuzlocke.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/ruleset.h"
#include "constants/species.h"

#define NUZLOCKE_GRAVEYARD_BOX      (TOTAL_BOXES_COUNT - 1)
#define NUZLOCKE_DUPES_MAX_REROLLS  24

#define BITARR_GET(arr, i)    (((arr)[(i) >> 3] >> ((i) & 7)) & 1)
#define BITARR_SET(arr, i)    ((arr)[(i) >> 3] |=  (1 << ((i) & 7)))
#define BITARR_CLEAR(arr, i)  ((arr)[(i) >> 3] &= ~(1 << ((i) & 7)))

// Battle types that are never governed by the one-per-location rule.
#define NUZLOCKE_EXEMPT_BATTLE_FLAGS                                            \
    (BATTLE_TYPE_TRAINER | BATTLE_TYPE_SAFARI | BATTLE_TYPE_ROAMER             \
   | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_LINK | BATTLE_TYPE_PYRAMID            \
   | BATTLE_TYPE_GHOST | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE)

// Latched when an encounter's battle is set up (opposing mon already created).
static EWRAM_DATA bool8 sEncounterScripted = FALSE;
static EWRAM_DATA bool8 sEncounterShiny = FALSE;

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

    sEncounterScripted = FALSE;
    sEncounterShiny = FALSE;
}

void Nuzlocke_BeginRun(void)
{
    SetRulesetRunActive(TRUE);
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

// ---------------------------------------------------------------------------
// Setting predicates
// ---------------------------------------------------------------------------

bool32 Nuzlocke_PermadeathOn(void)
{
    return GetRulesetSetting(SETTING_PERMADEATH) != 0;
}

bool32 Nuzlocke_OneEncounterPerLocationOn(void)
{
    return GetRulesetSetting(SETTING_ONE_ENCOUNTER_PER_LOCATION) != 0;
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
    return GetRulesetSetting(SETTING_NICKNAME_MODE) == NICK_STRICT;
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
    sEncounterScripted = scripted;
    sEncounterShiny = IsMonShiny(&gParties[B_TRAINER_OPPONENT_A][0]);
}

// Apply an encounter's outcome to its location.
static void NuzlockeConsumeLocation(u32 tag, u32 outcome, bool32 allowShinySkip)
{
    if (allowShinySkip && sEncounterShiny && Nuzlocke_ShinyClauseOn())
        return; // a shiny is a free extra - it never touches the location

    if (outcome == B_OUTCOME_CAUGHT)
    {
        Nuzlocke_MarkLocationUsed(tag, TRUE);
        return;
    }

    // Strict default: killing, fleeing or failing to catch also consumes.
    if (outcome != 0
     && GetRulesetSetting(SETTING_ENCOUNTER_CONSUMED_MODE) == ENCCONSUMED_STRICT)
        Nuzlocke_MarkLocationUsed(tag, FALSE);
}

void Nuzlocke_HandleWildBattleEnd(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    if (gBattleTypeFlags & NUZLOCKE_EXEMPT_BATTLE_FLAGS)
        return;
    NuzlockeConsumeLocation(Nuzlocke_CurrentLocationTag(), gBattleOutcome, TRUE);
}

void Nuzlocke_HandleScriptedBattleEnd(void)
{
    // Static / legendary encounters consume their location (planning decision),
    // but the script - not this rule - decides whether they are catchable.
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    NuzlockeConsumeLocation(Nuzlocke_CurrentLocationTag(), gBattleOutcome, FALSE);
}

void Nuzlocke_HandleSafariBattleEnd(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return;
    NuzlockeConsumeLocation(Nuzlocke_CurrentLocationTag(), gBattleOutcome, TRUE);
}

bool32 Nuzlocke_CanCatchCurrentEncounter(void)
{
    u32 tag;

    if (!Nuzlocke_RunIsActive() || !Nuzlocke_OneEncounterPerLocationOn())
        return TRUE;
    if (sEncounterScripted)
        return TRUE;
    if (gBattleTypeFlags & NUZLOCKE_EXEMPT_BATTLE_FLAGS)
        return TRUE;

    tag = Nuzlocke_CurrentLocationTag();
    if (!Nuzlocke_LocationIsUsed(tag))
        return TRUE;
    // Location already resolved: only a shiny may still be caught.
    if (Nuzlocke_ShinyClauseOn() && IsMonShiny(&gParties[B_TRAINER_OPPONENT_A][0]))
        return TRUE;
    return FALSE;
}

// ---------------------------------------------------------------------------
// Dupes Clause
// ---------------------------------------------------------------------------

#define FAMILY_BITSET_BYTES  ROUND_BITS_TO_BYTES(NUM_SPECIES)

static bool32 SpeciesInRange(u32 s)
{
    return s != SPECIES_NONE && s < NUM_SPECIES;
}

// Grow `set` to the full evolutionary family closure of its current members
// using only cheap forward reads (GetSpeciesEvolutions is an O(1) pointer
// read). Walking the reverse edge for free is what makes GetSpeciesPreEvolution
// - a full O(N*evos) reverse scan - unnecessary here.
static void ExpandFamilyClosure(u8 *set, bool32 countForms)
{
    bool32 changed = TRUE;
    u32 s, j;

    while (changed)
    {
        changed = FALSE;

        for (s = 1; s < NUM_SPECIES; s++)
        {
            const struct Evolution *evos = GetSpeciesEvolutions(s);
            bool32 sIn = BITARR_GET(set, s);

            if (evos == NULL)
                continue;
            for (j = 0; evos[j].method != EVOLUTIONS_END; j++)
            {
                u32 tgt = SanitizeSpeciesId(evos[j].targetSpecies);

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

        if (countForms)
        {
            // Pull in every species sharing a National Dex number with a
            // current member. natDexNum already collapses regional forms, so
            // this also links branches like Meowth <-> Meowth-Galar (and thus
            // Persian / Perrserker) that no evolution row connects.
            for (s = 1; s < NUM_SPECIES; s++)
            {
                enum NationalDexOrder dex;

                if (!BITARR_GET(set, s))
                    continue;
                dex = SpeciesToNationalPokedexNum(s);
                for (j = 1; j < NUM_SPECIES; j++)
                {
                    if (!BITARR_GET(set, j) && SpeciesToNationalPokedexNum(j) == dex)
                    {
                        BITARR_SET(set, j);
                        changed = TRUE;
                    }
                }
            }
        }
    }
}

void Nuzlocke_MarkFamilyOwned(enum Species species)
{
    u8 set[FAMILY_BITSET_BYTES];
    u32 i;

    species = SanitizeSpeciesId(species);
    if (!SpeciesInRange(species))
        return;

    memset(set, 0, sizeof(set));
    BITARR_SET(set, species);
    ExpandFamilyClosure(set, GetRulesetSetting(SETTING_DUPES_COUNT_FORMS) != 0);

    for (i = 0; i < FAMILY_BITSET_BYTES; i++)
        gSaveBlock3Ptr->nuzlocke.familyOwned[i] |= set[i];
}

bool32 Nuzlocke_IsFamilyOwned(enum Species species)
{
    species = SanitizeSpeciesId(species);
    if (!SpeciesInRange(species))
        return FALSE;
    return BITARR_GET(gSaveBlock3Ptr->nuzlocke.familyOwned, species);
}

// Recompute familyOwned for one family from live party + PC membership. Only
// needed when SETTING_DUPES_COUNT_DEAD is off (bits are otherwise set-only).
static void RecheckFamilyOwnership(enum Species species)
{
    u8 set[FAMILY_BITSET_BYTES];
    u32 i, boxId, pos;
    bool32 aliveMember = FALSE;

    species = SanitizeSpeciesId(species);
    if (!SpeciesInRange(species))
        return;

    memset(set, 0, sizeof(set));
    BITARR_SET(set, species);
    ExpandFamilyClosure(set, GetRulesetSetting(SETTING_DUPES_COUNT_FORMS) != 0);

    for (i = 0; i < PARTY_SIZE && !aliveMember; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u32 s = GetMonData(mon, MON_DATA_SPECIES);

        if (s == SPECIES_NONE || GetMonData(mon, MON_DATA_SANITY_IS_EGG))
            continue;
        if (SpeciesInRange(s) && BITARR_GET(set, SanitizeSpeciesId(s))
         && GetMonData(mon, MON_DATA_HP) != 0)
            aliveMember = TRUE;
    }
    for (boxId = 0; boxId < TOTAL_BOXES_COUNT && !aliveMember; boxId++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(boxId, pos);
            u32 s = GetBoxMonData(boxMon, MON_DATA_SPECIES);

            if (s == SPECIES_NONE || GetBoxMonData(boxMon, MON_DATA_SANITY_IS_EGG))
                continue;
            // A boxed mon has no live HP field; "dead" is the only disqualifier.
            if (SpeciesInRange(s) && BITARR_GET(set, SanitizeSpeciesId(s))
             && !GetBoxMonData(boxMon, MON_DATA_IS_DEAD))
            {
                aliveMember = TRUE;
                break;
            }
        }
    }

    if (!aliveMember)
    {
        for (i = 1; i < NUM_SPECIES; i++)
        {
            if (BITARR_GET(set, i))
                BITARR_CLEAR(gSaveBlock3Ptr->nuzlocke.familyOwned, i);
        }
    }
}

void Nuzlocke_OnMonObtained(struct Pokemon *mon, bool32 fromWildCatch)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);

    Nuzlocke_MarkFamilyOwned(species);

    // A wild catch's location is handled by the battle-end hook. Every other
    // acquisition (script gift, fossil revival, gift/daycare egg) consumes the
    // location it is received in.
    if (!fromWildCatch && Nuzlocke_RunIsActive() && Nuzlocke_OneEncounterPerLocationOn())
        Nuzlocke_MarkLocationUsed(Nuzlocke_CurrentLocationTag(), TRUE);
}

bool32 Nuzlocke_DupesRerollActiveHere(void)
{
    if (!Nuzlocke_RunIsActive() || !Nuzlocke_DupesClauseOn())
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

static void MoveDeadMonToGraveyard(struct Pokemon *mon)
{
    s16 pos = GetFirstFreeBoxSpot(NUZLOCKE_GRAVEYARD_BOX);

    if (pos < 0)
        return; // graveyard full: the dead mon stays in the party, inert at 0 HP

    if (!gSaveBlock3Ptr->nuzlocke.graveyardBoxNamed)
    {
        static const u8 sGraveyardBoxName[] = _("GRAVEYARD");
        StringCopy(GetBoxNamePtr(NUZLOCKE_GRAVEYARD_BOX), sGraveyardBoxName);
        gSaveBlock3Ptr->nuzlocke.graveyardBoxNamed = TRUE;
    }

    SetBoxMonAt(NUZLOCKE_GRAVEYARD_BOX, pos, &mon->box);
    ZeroMonData(mon);
}

void Nuzlocke_MarkMonDead(struct Pokemon *mon)
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

    if (GetRulesetSetting(SETTING_DUPES_CLAUSE)
     && !GetRulesetSetting(SETTING_DUPES_COUNT_DEAD))
        RecheckFamilyOwnership(GetMonData(mon, MON_DATA_SPECIES));

    if (GetRulesetSetting(SETTING_GRAVEYARD_BOX))
        MoveDeadMonToGraveyard(mon);
}

void Nuzlocke_ProcessPostBattleDeaths(void)
{
    u32 i;

    if (!Nuzlocke_PermadeathOn() || !Nuzlocke_RunIsActive())
        return;
    if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI
                          | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_TRAINER_HILL
                          | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_RECORDED
                          | BATTLE_TYPE_FIRST_BATTLE))
        return;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (GetMonData(mon, MON_DATA_SANITY_IS_EGG))
            continue;
        if (GetMonData(mon, MON_DATA_HP) == 0)
            Nuzlocke_MarkMonDead(mon);
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
