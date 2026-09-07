// ============================================================================
// Phase 7 - evolution override table. See include/evolution_fixes.h for the
// rationale. Each row completely replaces one species' stock evolution array;
// GetSpeciesEvolutions() returns it verbatim.
//
// Keep this list short and auditable. Anything that is already reachable in a
// Hoenn single-player game (stone evolutions, IF_IN_MAP maps that exist here,
// IF_WEATHER rain routes, Kubfu's Scroll *items*, step counters) is NOT listed
// here - it only needs the Lilycove seller (data/maps/LilycoveCity_Department
// Store_3F) to guarantee the items, not a data change.
// ============================================================================

#ifndef EVOLUTION
#define EVOLUTION(...) (const struct Evolution[]) { __VA_ARGS__, { EVOLUTIONS_END }, }
#endif
#ifndef CONDITIONS
#define CONDITIONS(...) ((const struct EvolutionParam[]) { __VA_ARGS__, {CONDITIONS_END} })
#endif

// ---- Karrablast / Shelmet: the only two hard link-trade locks in the repo.
// Stock: reciprocal EVO_TRADE with IF_TRADE_PARTNER_SPECIES, no fallback row.
static const struct Evolution sEvoFix_Karrablast[] =
    EVOLUTION({EVO_LEVEL, 32, SPECIES_ESCAVALIER});
static const struct Evolution sEvoFix_Shelmet[] =
    EVOLUTION({EVO_LEVEL, 32, SPECIES_ACCELGOR});

// ---- Mantyke: stock needs a Remoraid in the party (uncatchable under a
// randomized dupes-clause Nuzlocke). Plain level evo instead.
#if P_GEN_4_CROSS_EVOS
static const struct Evolution sEvoFix_Mantyke[] =
    EVOLUTION({EVO_LEVEL, 32, SPECIES_MANTINE});
#endif

// ---- Gimmighoul (both forms): stock needs 999 Gimmighoul Coins in the bag.
static const struct Evolution sEvoFix_Gimmighoul[] =
    EVOLUTION({EVO_LEVEL, 45, SPECIES_GHOLDENGO});

// ---- Galarian Yamask: stock is EVO_SCRIPT_TRIGGER (EVO_TRIGGER_TABLET_CURSE)
// only, and no map in data/ ever calls tryspecialevo. Mirror normal Yamask's
// level-34 threshold.
static const struct Evolution sEvoFix_YamaskGalar[] =
    EVOLUTION({EVO_LEVEL, 34, SPECIES_RUNERIGUS});

// ---- Bisharp: stock needs to defeat 3 Bisharp holding a Leader's Crest.
#if P_GEN_9_CROSS_EVOS
static const struct Evolution sEvoFix_Bisharp[] =
    EVOLUTION({EVO_LEVEL, 60, SPECIES_KINGAMBIT});
#endif

// ---- Zweilous: stock evolves at level 64, one above the pre-Champion cap.
static const struct Evolution sEvoFix_Zweilous[] =
    EVOLUTION({EVO_LEVEL, PRE_CHAMPION_LEVEL_CAP, SPECIES_HYDREIGON});

// ---- Ursaring: stock is Peat Block + IF_REGION REGION_HISUI + night, and
// GetCurrentRegion() is always REGION_HOENN here, so Ursaluna is impossible.
// Keep the Peat Block requirement, drop the region/time gate. Preserve the
// EVO_NONE bloodmoon marker (offspring-generation only, never a real evo).
#if P_GEN_8_CROSS_EVOS
static const struct Evolution sEvoFix_Ursaring[] =
    EVOLUTION({EVO_ITEM, ITEM_PEAT_BLOCK, SPECIES_URSALUNA},
              {EVO_NONE, 0, SPECIES_URSALUNA_BLOODMOON});
#endif

// ---- Galarian Farfetch'd: stock needs 3 critical hits in one battle - pure
// luck, and a Nuzlocke affords few attempts. Lower to a single crit.
static const struct Evolution sEvoFix_FarfetchdGalar[] =
    EVOLUTION({EVO_BATTLE_END, 0, SPECIES_SIRFETCHD, CONDITIONS({IF_CRITICAL_HITS_GE, 1})});

struct EvoFixEntry
{
    u16 species;
    const struct Evolution *evolutions;
};

// Linear-scanned (tiny). No ordering requirement.
static const struct EvoFixEntry sEvoFixTable[] =
{
    { SPECIES_KARRABLAST,         sEvoFix_Karrablast     },
    { SPECIES_SHELMET,            sEvoFix_Shelmet        },
#if P_GEN_4_CROSS_EVOS
    { SPECIES_MANTYKE,            sEvoFix_Mantyke        },
#endif
    { SPECIES_GIMMIGHOUL_CHEST,   sEvoFix_Gimmighoul     },
    { SPECIES_GIMMIGHOUL_ROAMING, sEvoFix_Gimmighoul     },
    { SPECIES_YAMASK_GALAR,       sEvoFix_YamaskGalar    },
#if P_GEN_9_CROSS_EVOS
    { SPECIES_BISHARP,            sEvoFix_Bisharp        },
#endif
    { SPECIES_ZWEILOUS,           sEvoFix_Zweilous       },
#if P_GEN_8_CROSS_EVOS
    { SPECIES_URSARING,           sEvoFix_Ursaring       },
#endif
    { SPECIES_FARFETCHD_GALAR,    sEvoFix_FarfetchdGalar },
};
