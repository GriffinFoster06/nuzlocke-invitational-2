#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

// ============================================================================
// Phase 2 core randomizer wiring - docs/SPEC.md "Wild Pokemon randomization",
// "Fixed encounter-slot mapping", "Starter/Gift/Static Pokemon".
//
// Deterministic, power-matched species replacement derived from the run seed
// (include/ruleset.h GetRunSeed). Each category has its own salt so changing
// one category's outcome does not reshuffle another ("Separate deterministic
// randomization systems"). Selection and scoring live in src/power_score.c.
// ============================================================================

#include "wild_encounter.h"

struct Pokemon;
struct PokemonTemplate;
struct TrainerMon;

// Category enable checks (thin wrappers over the ruleset settings).
bool32 Randomizer_WildEnabled(void);
bool32 Randomizer_StarterEnabled(void);
bool32 Randomizer_GiftEnabled(void);
bool32 Randomizer_StaticEnabled(void);
bool32 Randomizer_LegendaryEnabled(void);
bool32 Randomizer_TrainerEnabled(void);

// docs/SPEC.md "Trainer Pokemon": bosses (Leaders, Elite Four, Champion,
// Rival, Aqua/Magma Leader/Admin) use stricter power matching. Exposed for
// Phase 11E's boss-defeat / boss-opponent Run Report bookkeeping.
bool32 TrainerClassIsBoss(u8 trainerClass);

// Wild encounters. Returns the persistent replacement for the given
// (encounter table, slot); returns `vanilla` unchanged when randomization is
// off or the target is not a valid replaceable species.
enum Species Randomizer_WildSlotSpecies(const struct WildPokemonInfo *info, u32 slot, enum Species vanilla);
u32 Randomizer_WildRateSlot(const struct WildPokemonInfo *info, enum WildPokemonArea area,
                            u8 rod, u32 slot);
enum Species Randomizer_SpecialWildSpecies(enum Species vanilla, u32 sourceKey, u32 routeKey);
// Phase 11A.6: drop the small per-table wild-slot species cache (a locked
// setting moved, or the run seed changed - see include/run_rng.h).
void Randomizer_InvalidateWildSlotCache(void);

// Starters. `index` is 0..2; the three results are de-duplicated.
enum Species Randomizer_StarterSpecies(u32 index);
// Apply the SETTING_STARTER_IV_MODE spread to a freshly given starter.
void Randomizer_ApplyStarterIVs(struct Pokemon *mon);
// Freeze the generation settings: the first irreversible act of a run.
void Randomizer_MarkRunStarted(void);

// Script gifts (including eggs). Species and IVs use independent streams.
enum Species Randomizer_GiftSpecies(enum Species vanilla, u8 level, u32 sourceKey);
void Randomizer_ApplyGiftMonIVs(struct Pokemon *mon, enum Species vanilla, u32 sourceKey);
void Randomizer_ApplyGiftTemplate(struct PokemonTemplate *monTemplate, u32 sourceKey);

// Static encounters. `sourceKey` identifies the script/object event; callers
// disambiguate members of a double encounter in that key.
// Premium-tier vanilla species route through the curated premium pool and obey
// SETTING_LEGENDARY_RANDOMIZATION instead of SETTING_STATIC_RANDOMIZATION.
enum Species Randomizer_StaticSpecies(enum Species vanilla, u8 level, u32 sourceKey);

// Roaming legendaries (Latias/Latios). Premium pool, legendary toggle.
enum Species Randomizer_RoamerSpecies(enum Species vanilla, u8 level, u32 roamerId);

// Trainer parties are resolved as a unit so role-based Premium rules, Wallace's
// guarantee, party size, and cap-relative levels cannot be bypassed per slot.
u8 Randomizer_GetTrainerPartySize(u16 trainerId, u8 authoredPartySize);
void Randomizer_ApplyTrainerParty(struct TrainerMon *entries, const u32 *sourceIndices,
                                  u8 count, u16 trainerId, u8 trainerClass);

// ---- TMs (docs/SPEC.md "TMs") ---------------------------------------------
// The 50 TM->move assignments are drawn once from the ban-filtered move pool
// and cached, so scanning every TM index while rendering a list stays cheap.
// HMs are never randomized. All three accessors return the canonical mapping
// when SETTING_TM_RANDOMIZATION is off.

bool32 Randomizer_TmEnabled(void);

// `tmhmIndex` is the 1-based GetItemTMHMIndex() value; 0 yields MOVE_NONE.
enum Move Randomizer_TmMoveByIndex(u32 tmhmIndex);
enum Move Randomizer_TmMove(enum Item item);

// Reverse map: which TM (or HM) item teaches `move`, or ITEM_NONE.
enum Item Randomizer_TmItemForMove(enum Move move);

void Randomizer_InvalidateTms(void); // a ban / duplicate toggle moved

// ---- Move Tutors (docs/SPEC.md "Move Tutors") -----------------------------
// The ten standard Hoenn tutors. Keyed on the tutor's canonical move, so no
// table is needed. Battle Frontier tutors stay vanilla.

bool32 Randomizer_TutorEnabled(void);
enum Move Randomizer_TutorMove(enum Move vanilla);

// ---- Items (docs/SPEC.md "Items", "Item-pool safety") ---------------------
// Key items, HMs and anything with non-zero importance are never replaced and
// never produced; the replacement pool is ordinary items, berries and balls
// only. TMs are also left alone - they already carry a randomized move.

bool32 Randomizer_FieldItemEnabled(void);
bool32 Randomizer_HiddenItemEnabled(void);
bool32 Randomizer_GiftItemEnabled(void);

enum Item Randomizer_FieldItem(enum Item vanilla, u32 objectId);
enum Item Randomizer_HiddenItem(enum Item vanilla, u32 hiddenItemId);
enum Item Randomizer_GiftItem(enum Item vanilla);

#endif // GUARD_RANDOMIZER_H
