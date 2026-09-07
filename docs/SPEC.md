# ROM Specification

Defaults follow this priority rule:
1. **Invitational 2 behavior whenever known.**
2. **Randolocke behavior when Invitational 2 does not establish it.**
3. **Our design choice only where neither establishes a default.**

The official Invitational rules explicitly establish permadeath, location-tag
encounters, evolutionary-family Dupes Clause, nicknames, Shiny Clause, no
battle items, balanced Pokémon/move/ability randomization, automatic Gym
caps, over-cap illegality, whiteout resets, Level to Cap, infinite Repels,
HM-free traversal, linearized gameplay, Gen 9 mechanics, and the Hall-of-Fame
species removal rule. Randolocke supplies the fallback defaults for things
such as the 21-move 7/7/7 learnsets, universal compatibility, randomized
items/Tutors, map additions, catch-rate boost, and various QoL features.

## Core game
- Pokémon Emerald is the underlying world and campaign.
- Pokémon, moves, abilities, items, evolutions, types, and battle effects are
  expanded through Generation 9.
- The game remains visually and structurally recognizable as Emerald rather
  than becoming a completely new region.
- The main campaign is redesigned around: getting a new encounter,
  evaluating/team-building around that encounter, fighting trainers, fighting
  bosses, moving immediately toward the next meaningful encounter or battle.
- Grinding, forced backtracking, HM-party management, repetitive cutscenes,
  tutorials, fetch quests, and other downtime are aggressively removed.
- All important Emerald encounter locations remain available even when their
  associated story busywork is removed.
- All Pokémon included in the ROM can be fully evolved in a single-player game.

## Default preset
- The default overall preset is Invitational 2 Solo.
- It uses: Hardcore Nuzlocke rules, balanced species randomization, randomized
  moves, randomized abilities, randomized TMs/Tutors/items where Invitational
  does not contradict Randolocke, Gen 9 mechanics, level caps, Level to Cap,
  Infinite Repel, HM-free progression, linearized campaign, maximum-strength
  fair AI.
- A separate Invitational 2 Tournament preset adds the cross-run
  Hall-of-Fame species exclusion system.

## Additional presets
- **Invitational 2 Solo** — main default; designed for one player repeatedly
  attempting the challenge.
- **Invitational 2 Tournament** — same gameplay rules; adds persistent
  Hall-of-Fame species exclusions for later fresh runs; adds final-team
  locking/export information.
- **Randolocke** — uses Randolocke's documented defaults as closely as
  possible; retains our improved fair AI.
- **Modern Emerald** — normal Emerald species/trainers rather than
  randomized ones; modern Gen 9 battle engine and QoL; story streamlining
  can remain enabled.
- **Randomizer** — full randomization and QoL without mandatory Nuzlocke
  restrictions.
- **Custom** — every supported setting can be individually changed.
- Choosing a preset fills all settings automatically. Changing an individual
  setting changes the displayed ruleset to Custom. Players can restore an
  individual category or the entire configuration to its preset defaults.

## Settings behavior
- Settings that determine the generated world are chosen before starting the
  save (species randomization, learnset generation, ability randomization,
  item randomization, seed, encounter mapping method). Generation settings
  become locked once the run starts.
- QoL/display settings can generally remain changeable during the run.
- Nuzlocke rule settings are locked during an active strict run so the player
  cannot disable permadeath after losing something.
- Every setting should include a short description of exactly what changing
  it does.

## Run seed
- Every New Game generates a run seed; the player can alternatively enter a
  specific seed. The seed is stored with the save. Reloading, resetting, or
  reopening the ROM does not change anything generated for that save.
  Starting a new attempt normally creates a new seed. An optional Retry Same
  Seed setting can exist outside strict Invitational tournament play. The
  seed is shown in Run Information. The ruleset version is also stored so a
  seed remains reproducible after future ROM updates. Same
  ROM/randomizer-version + seed + settings should generate the same world.

## Separate deterministic randomization systems
Species replacements, trainer replacements, abilities, learnsets, TMs,
tutors, items, static encounters, legendary encounters — these should be
deterministic but logically separate so changing one category does not
unnecessarily reshuffle every unrelated category.

## Pokémon roster
Pokémon through Gen 9, regional forms (Alolan/Galarian/Hisuian/Paldean),
modern evolutions of old Pokémon, Ultra Beasts, Paradox Pokémon, Legendary
and Mythical Pokémon, special species/forms where their battle implementation
functions correctly. Invalid/internal/debug forms are automatically excluded.

## Species-pool settings
Individual species can be banned. Entire categories can be allowed or
excluded: Legendary, Mythical, Ultra Beast, Paradox, Regional forms.
Extremely powerful restricted Pokémon use a separate pool from ordinary
Pokémon. Default uses a curated high-power pool instead of allowing every box
legendary to replace ordinary strong Pokémon.

## Wild Pokémon randomization
Default: ON. Every wild encounter slot receives a randomized species. Weak
Pokémon are replaced by approximately weak Pokémon; strong by approximately
strong. Completely unrestricted randomization exists as a non-default
setting. Encounter rates remain those of the original slot unless
encounter-rate randomization is independently enabled. Encounter levels
remain progression appropriate.

## Species power matching
Default: Balanced. Randolocke's similar-BST philosophy is retained, but the
matching system can use a more accurate intrinsic power score considering:
BST, evolutionary stage, stat distribution, offensive efficiency, defensive
efficiency, speed, special species classification, and whether a Pokémon's
canonical design is artificially restrained by a crippling ability (e.g.
Slaking, Archeops). Randomized ability is NOT included when selecting the
replacement species — species is selected first, ability is randomized
afterward (this still permits lucky broken combinations).
Settings: Strict balanced / Normal balanced (default) / Loose balanced /
BST-only / Unrestricted chaos.

## Evolutionary-stage matching
Default: Prefer similar evolutionary stage. Unevolved → unevolved, middle →
middle, final → final preferentially; single-stage Pokémon treated
appropriately. May be broken if necessary to satisfy the power range. A
strict same-stage setting is available.

## Fixed encounter-slot mapping
Default: Randolocke-style persistent encounter-slot mapping. Each
encounter-table/Porymap slot has a fixed randomized replacement for the
entire save — not rerolled whenever the encounter occurs. Multiple vanilla
slots containing the same Pokémon can map to different replacements (e.g.
separate Zigzagoon slots on two routes need not become the same Pokémon),
dramatically increasing species diversity.
Optional mapping modes: Encounter-slot mapping (default) / Route-species
mapping / Global species-to-species mapping.

## Starter randomization
Default: ON. Starter choices are randomized; power kept within an
appropriate starter-level range. Player still chooses from three starters;
selections should not duplicate each other. Starter previews show species,
type, randomized ability. Full randomized learnsets need not be exposed
before choosing.

## Starter IVs
Invitational default: exactly 5 perfect IVs and 1 random IV (which stat is
random is itself random).
Settings: 5 perfect + 1 random (default) / 3 perfect / Natural IVs / All 31 /
Custom floor.

## Gift Pokémon
Default: randomized, power-appropriate replacements. Randolocke fallback
applies since Invitational doesn't establish a separate gift-IV rule: 3
guaranteed perfect IVs by default for ordinary gift Pokémon (separately
configurable from starter IVs).

## Static Pokémon
Default: randomized. Static encounters retain their fixed locations; species
generated once per run. Ordinary static encounters use appropriate power
matching; legendary/static premium encounters use the separate high-power
pool.

## Trainer Pokémon
Default: randomized. Applies to regular trainers, rival, Wally, Team
Aqua/Magma, admins, Gym Leaders, Elite Four, Champion. Each trainer slot
receives a power-appropriate replacement. Party sizes remain authored unless
party-size randomization is explicitly selected. Bosses use stricter power
matching than ordinary route trainers. Trainer species remain fixed for the
run.

## Trainer levels
Default: scaled around the level-cap progression. Ordinary trainers
generally below the area's boss cap; strong trainers approach the cap; Gym
Leader ace establishes the area's actual cap. Trainer levels do not randomly
fluctuate by seed.

## Default cap progression
Invitational requires the cap to equal the next Gym Leader's highest level.
Until exact Invitational 2 intermediate values are recovered, Randolocke
values are the fallback:
- Before Roxanne: 14
- After Roxanne / before Brawly: 21
- Before Wattson: 24
- Before Flannery: 29
- Before Norman: 36
- Before Winona: 43
- Before Tate & Liza: 47
- Before Juan: 50
- After eight badges / before Elite Four: 63

After completing the Elite Four/Champion, the general cap can rise to 100.
If future Invitational evidence provides different intermediate values, the
Invitational preset takes priority.

## Hard level caps
Default: ON. Pokémon cannot gain levels past the active cap; additional EXP
cannot accidentally push a Pokémon over. Current cap visible in the
interface.
Modes: Hard cap (default) / Soft cap / Warning only / Off.

## Caught Pokémon above the cap
Can still be caught; marked Over Cap / Ineligible; cannot enter battle while
above the current legal cap; cannot be used for encounter-manipulation
abilities or other Nuzlocke gameplay advantages; automatically become legal
once progression raises the cap sufficiently.

## Level to Cap
Default: ON. Available from the party menu and for PC Pokémon. Immediately
sets the Pokémon to the current cap — does NOT simply skip the move-learning
process. The game records every randomized move-learning level crossed,
brings the Pokémon to the cap first, then presents all moves it should have
learned during those skipped levels, in chronological order. No move
opportunity is lost. Grants no artificial EVs. Does not automatically evolve
the Pokémon — evolution is handled separately so instant leveling cannot
accidentally bypass evolution decisions.

Example: Pokémon goes from level 18 to 24, with new moves scheduled at 20
and 23. It reaches 24, then is offered the level-20 move, then the level-23
move.

## Endless Candy
Removed — Level to Cap makes an infinite +1-level item redundant.

## Optional Level to Next Breakpoint
Available as optional QoL: level only to next move / next evolution
threshold / current cap. Not required by the Invitational preset.

## Randomized level-up moves
Default: ON. Pokémon do not use canonical learnsets. Generated learnsets
remain fixed for the run.

## Learnset size
Randolocke fallback (Invitational's exact implementation not publicly
established): 21 randomized level-up moves per species. Every species
receives the same number of move opportunities via the same 21 standardized
learning checkpoints.

## 7/7/7 learnset composition
Default: ON. Seven STAB damaging moves, seven additional damaging moves,
seven non-damaging/status moves. STAB moves must correspond to at least one
current type (dual-types can draw from either). Non-damaging moves can
include setup, recovery, status, screens, hazards, speed control,
disruption, protection, support — distributed through the level progression
rather than necessarily occurring in blocks. Alternative smarter/weighted
compositions available in Custom settings. Literal 7/7/7 remains default
because it's Randolocke's established fallback.

## Move-power progression
Default: higher-power attacks tend to be learned later (weighted, not
rigidly sorted — a rare unusually strong early roll can still occur). Status
moves evaluated separately from raw base power. Randolocke v1.1 made
higher-BP-later learning its default.
Optional mode: fully random move order.

## Move pool
Moves through Gen 9. Signature attacks can appear on unrelated Pokémon
unless the move literally cannot function outside its original
implementation. Blank/internal moves, Struggle, and broken
placeholder/debug effects excluded. Configurable move blacklist for OHKO
moves, Evasion, Sleep moves, Self-KO moves, other unwanted effects.
Invitational preset can use any verified Invitational-specific exclusions
when identified.

## Move Reminder
Default: disabled/free reminder not available, following the
Invitational-style strategic importance of forgetting moves. A forgotten
randomized level-up move is a meaningful loss unless reacquired through
another legitimate source.
Settings: Disabled (default) / Normal Emerald reminder / Free reminder /
Previously learned moves only.

## TMs
Default: randomized. TM move assignments generated at New Game, never
reroll after reopening. Duplicate randomized TM moves allowed. HMs are not
part of ordinary TM randomization. TM move information visible before use.

## Universal TM compatibility
Default: ON. Every Pokémon can learn every randomized TM. Optional
restrictive compatibility systems can exist under Custom.

## Move Tutors
Default: randomized. Every standard Tutor's move is randomized; assignments
remain fixed through the run. Universal compatibility applies by default.
Evolution-assistance moves handled separately so randomized Tutors cannot
softlock evolutions.

## Abilities
Default: randomized. Abilities from all supported generations can appear
and remain fixed throughout the run. Extremely strong combinations are
permitted — the game does not automatically nerf a Pokémon for rolling
Huge Power, Speed Boost, Adaptability, Magic Guard, etc. Discovering broken
combinations is part of the randomizer.

## Ability consistency through evolution
Default: ON. Evolution does not arbitrarily reroll a completely unrelated
ability — a Pokémon retains the corresponding randomized family/slot
ability when evolving.

## Shedinja
Randolocke fallback (Invitational exact behavior unknown). Default: Shedinja
retains Wonder Guard; Wonder Guard does not randomly appear elsewhere by
default. Custom chaos mode may permit full Wonder Guard randomization.

## Items
Default: field items randomized, gift items randomized. Hidden field items
treated as field items. Item results remain fixed for the run. Duplicate
items allowed. Key items required for progression are never destroyed by
randomization. Story-critical traversal items are either protected or
replaced by automatic progression systems. Shops NOT randomized by default
(optional in Custom).

## Item-pool safety
Required key items do not become Potions. Ordinary consumable pickups cannot
replace progression flags. Evolution availability guaranteed independently
of random field-item luck.

## Modern held items
Held items through Gen 9 supported with Gen 9 effect behavior. Randomized
item pools can include useful modern held items.

## Unlimited money
Default: ON. Money is not intended to be a limiting resource — purchases
effectively do not exhaust spending ability. Removes money farming. The
Oldale money-refill NPC can remain as a redundant convenience/easter egg.

## Poké Ball availability
Unlimited money does NOT mean every Ball type is available immediately.
Shops retain progression-based Ball inventories (e.g. Great Balls only once
shops that sell them are reachable). Once a type is available, unlimited
money means the player may purchase as many as desired.

## 999 Poké Ball NPC
Default: present. Replaces Randolocke's 999 Ultra Ball NPC — gives 999
ordinary Poké Balls (not Great/Ultra). Removes tedious early capture-resource
management without destroying Ball progression.

## Catch rates
Randolocke fallback (Invitational doesn't publish a catch-rate setting).
Default: moderately increased catch rates.
Settings: Vanilla / Moderate boost (default) / Large boost / Guaranteed
capture.

## R-button Ball shortcut
Default: ON. Pressing R in a wild battle provides a fast Ball-throwing
shortcut using an appropriate Ball already in inventory (never creates a
Ball the player doesn't possess).

## Nuzlocke permadeath
Default: ON. If a Pokémon faints, it is permanently dead. Dead Pokémon
cannot battle, be revived into legal use, use field abilities, activate
encounter-manipulation abilities, or be used for any other gameplay
advantage. Dead Pokémon can be automatically moved to a designated Graveyard
PC box after battle; the player may still inspect them.

## One encounter per location
Default: ON. Only the first valid encounter for each location can be
caught. Location identity uses the actual location tag — a different
location tag means a new encounter. Once resolved, that location is marked
used. Strict default: killing, running from, or failing to catch the valid
encounter all consume the location.

## Dupes Clause
Default: ON. A Pokémon from an evolutionary family the player has already
caught does not count as the route encounter — the game rerolls until a
non-dupe appears. Family means the entire evolutionary family; split
evolutions still count as one family; forms/regional variants of the same
family count as duplicates under the strict Invitational default. Dead
Pokémon still count as previously owned for Dupes Clause purposes.

## Shiny Clause
Default: ON. A shiny may be caught regardless of whether the location
encounter has already been consumed, and doesn't invalidate the previously
caught encounter. Normal shiny odds unless explicitly changed in Custom.

## Nicknames
Default: mandatory. Every obtained Pokémon must be nicknamed; strict mode
prevents bypassing the prompt.

## No battle items
Default: enforced. Trainer battle Bag access is disabled — no Potions, Full
Restores, X Items, status medicine, Revives, manually used berries. Held
items function normally. Wild encounters still permit Poké Balls (Bag can
expose Balls in wild battles while hiding prohibited combat consumables).

## Whiteout
Default: run lost. No usable Pokémon + whiteout ends the attempt; player
returns to a run-over screen and can immediately begin another. No limit on
attempts. Normal default generates a new seed for the new attempt.

## Set battle style
Default: ON as the best-fit hardcore rule. No free switch offered after
defeating an opposing Pokémon. Can be changed outside strict Invitational
presets.

## Portable healing
Default: ON. Available from the menu; restores HP/PP/status; does not
revive dead Pokémon. Removes Pokémon Center detours (healing already free
outside battles).

## Infinite Repel
Default: ON/available. Toggleable outside battle; prevents ordinary random
encounters while enabled; player disables it when deliberately seeking a
new-location encounter. Static/scripted encounters still occur. Visible
indicator shows current state.

## Expanded Bag
Default: ON — expanded enough to comfortably support the much larger Gen
1-9 item pool.

## IV / EV / Nature display
Default: ON for all. Exact IV and EV per stat shown in Summary (no external
calculator needed). Nature shown with boosted/reduced/neutral stat
indicated. Natures are random/natural by default; no default nature
editing.

## Pokémon Summary improvements
Shows species, nickname, level, current legality under cap, type(s),
ability + description, nature, actual stats, IVs, EVs, moves (with type,
category, base power, accuracy, PP). Dead Pokémon and over-cap Pokémon
clearly marked.

## HM-free traversal
Default: ON. Pokémon do not need to know HMs to use field abilities
(Cut, Rock Smash, Strength, Surf, Dive, Waterfall, other required field
interactions). Story/badge progress grants traversal permission. HM moves
may still exist as normal combat moves and can be forgotten like ordinary
moves.

## Evolution system
Every Pokémon must be capable of reaching its final evolution in
single-player. No Pokémon can be permanently blocked because its original
game required trading, another player, Pokémon HOME, another version, a
feature not present in Emerald, or a region-specific environmental object.
Some evolutions may still be legitimately progression-gated (e.g. a stone
not available until Lilycove is fine). Rule: **Delayed is acceptable.
Impossible is not.**

## Evolve command
Default: ON. Available from the Pokémon menu; shows whether an evolution is
currently available. Evolution remains player-controlled rather than
silently triggered by Level to Cap. Split evolutions allow the player to
choose among valid outcomes when the original mechanic permits a choice.

## Lilycove evolution-item sellers
Implemented. Lilycove Department Store provides guaranteed access to all
normal evolution items required by included Pokémon, so random field items
can't make evolution impossible. Evolution stones/items remain
progression-gated by reaching Lilycove. Scroll of Darkness/Waters can also
be made available here if Kubfu continues to use the canonical Scroll
method. Replaces the arbitrary Mossdeep white-rock workaround.

## Move-dependent evolution anti-softlock
The Lilycove Tutor Mansion is removed ONLY because a guaranteed replacement
exists: a Pokémon requiring a specific move to evolve can always obtain that
move through an Evolution Assistance function, restricted specifically to
evolution-necessary moves (not a general free Move Reminder). Randomized
learnsets/TMs therefore cannot permanently prevent an evolution.

## Trade evolutions
Converted into deterministic single-player evolutions with appropriate
level/item requirements. No Link Cable / second game necessary.

## Species-specific evolution fixes
- **Shelmet and Karrablast**: Verdanturf forced trades removed; both get
  deterministic single-player level-based evolutions.
- **Mantyke**: Fallarbor free Remoraid workaround removed; deterministic
  single-player evolution level.
- **Gimmighoul**: 999-coin NPC removed; deterministic level-based evolution.
- **Galarian Yamask**: Route 111 statue removed; deterministic level-based
  evolution.
- **Bisharp**: Route 123 three-Leader's-Crest event removed; Kingambit gets
  a deterministic single-player requirement, primarily level/progression
  based.
- **High-level evolutions generally**: if a canonical evolution level
  exceeds the final pre-Champion cap, the requirement is lowered to an
  appropriate reachable threshold (Randolocke changes Zweilous to 63
  specifically; generalize this principle to every affected Pokémon).

## Map additions (all implemented by default, inherited from Randolocke)
- **Route 103 Old Rod**: Old Rod sailor moved to Route 103 for earlier
  fishing access.
- **Littleroot water**: gains accessible water with its own encounter where
  location tagging supports it.
- **Oldale grass**: gains grass and an encounter table.
- **Scorched Slab**: populated with wild Pokémon by default.

## Late-game unlocks (implemented)
- **Regi caves**: unlock after Rayquaza resolves the Sootopolis
  Groudon/Kyogre crisis — Regirock/Regice/Registeel static slots available
  before Gym 8 (species randomized).
- **Kyogre/Groudon Weather Institute unlock**: abnormal-weather system
  (Marine Cave / Terra Cave) activates immediately after the
  Rayquaza/Sootopolis event rather than waiting for postgame (species
  randomized).
- **Slateport legendary-map NPC**: appears after the Sootopolis crisis, near
  the ferry building; sells access to Latios/Latias, Mew/Faraway Island,
  Deoxys/Birth Island, Ho-Oh and Lugia/Navel Rock (species randomized).
  Unlimited money means the purchase price isn't the meaningful gate — the
  Sootopolis progression flag is.

## Legendary encounter balancing
Static legendary locations use a curated premium species pool and don't
necessarily contain the original legendary; they should yield powerful
species comparable to the intended encounter tier. Ordinary low-level route
slots don't draw from this restricted pool. Duplicate premium encounters can
be allowed by default unless a future Invitational rule establishes
otherwise.

## Generation 9 mechanics
Default: ON. Modern physical/special split, Fairy type, modern type chart,
move effects, abilities, held items, damage calculation, critical-hit
behavior, burn, paralysis, weather, terrain, priority, multi-hit behavior,
targeting, switching interactions, status/type immunities, end-of-turn
processing.

## Battle gimmicks
Default OFF: Mega Evolution, Primal Reversion, Z-Moves, Dynamax,
Gigantamax, Terastallization. Custom mode can expose any correctly
implemented mechanic, but none are part of the default hack identity.

## Maximum-strength fair AI
Default: Pro Fair. The AI should play as intelligently as technically
feasible — difficulty from good decision-making, not hidden-information
cheating. Should behave closer to a strong competitive player than an
Emerald NPC. Expansion already provides sophisticated viability, switching,
prediction and switch-in systems; its explicit Omniscient mode (reading
hidden moves/items/abilities) is specifically excluded.

### Information the AI knows
Rules of Pokémon; current battlefield state; visible species/typings/base
stats; visible HP/status/stat stages; weather/terrain/screens/hazards/Trick
Room/Tailwind; revealed moves/abilities/items; previously revealed opposing
Pokémon; information legitimately inferable from turn order or damage.

### Information the AI does not know
Unrevealed player moves/held item/ability; player IVs/EVs/nature; unseen
reserve Pokémon (unless team-preview reveals them); the move or switch the
player selected this turn; future critical hits, damage rolls, or secondary
effect rolls; hidden randomizer data merely because the engine internally
contains it.

### AI battle memory
Remembers moves/items/abilities/Pokémon once revealed, keeps that
information across switches, updates incorrect inferences when later
evidence contradicts them.

### AI uncertainty
Unrevealed information represented as possibilities/probabilities. AI can
know the randomizer's general rules (e.g. that Pokémon tend to have STAB,
coverage, and status tools under 7/7/7 generation) without knowing which
four specific moves a given opposing Pokémon carries. It can hedge against
plausible threats but cannot magically choose the correct counter to a
secret move every turn.

### AI damage reasoning
Identifies guaranteed KOs, likely KOs, 2HKOs; considers accuracy, priority,
recoil, defensive setup, weather/terrain/screens; uses ranges when hidden
player stats prevent exact calculation; does not know the upcoming damage
roll.

### AI switching
Voluntarily switches out of bad positions; selects intelligent switch-ins
using immunities/resistances; preserves valuable Pokémon; escapes Encore,
Perish Song, Yawn, severe stat reductions, bad trapping (where escape
remains legal); uses Regenerator/Natural Cure/absorb abilities; accounts for
hazards before switching (doesn't repeatedly switch into lethal hazards).

### AI strategic play
Understands setup, recovery, sacrifice plays, revenge killing, preserving
win conditions, hazards and hazard removal, speed control, status,
pivoting, Choice-locking once revealed/inferred, Focus Sash once revealed,
Protect mind games without reading inputs, short multi-turn plans rather
than choosing exclusively by immediate damage.

### AI prediction
Can predict likely switches/attacks from known information; can
double-switch; can choose coverage expecting an obvious switch. Predictions
intentionally retain uncertainty — the AI never knows whether a prediction
is correct before actions resolve.

### AI doubles behavior
Evaluates both allied Pokémon together; coordinates targets; understands
double-target KOs, spread moves, Protect, Wide Guard, Quick Guard, Fake
Out, Follow Me/Rage Powder, Helping Hand, Tailwind, Trick Room,
weather/terrain combinations, ally-ability activation; avoids stupid
friendly fire but can intentionally hit an ally when beneficial; considers
board position rather than treating each AI Pokémon independently.

### AI difficulty settings
Vanilla / Improved / Expert / Pro Fair (default). Increasing difficulty
improves reasoning. No standard difficulty setting grants omniscience.

## Story streamlining — overall rule
Default: heavily linearized. Official Invitational 2 confirms "linearized
gameplay" but doesn't publicly document every script edit, so the
individual cuts below are our implementation of that requirement.

Preserve: new encounter locations, important trainers, boss battles, major
rival/villain battles, useful static encounters.

Remove or compress: fetch quests, long dialogue, tutorials, walking back
through already-cleared routes merely to trigger story flags, repeated
trips between NPCs, HM errands, story puzzles whose only purpose is slowing
navigation, repetitive faction exposition.

## General story QoL
Cutscenes can be fast-forwarded/skipped. Text runs at maximum speed by
default; no forced slow text. Running available immediately, including
indoors. Repeated ferry/travel animations shortened. PokéNav/Match Call and
mandatory phone-call interruptions removed. TV/news interruptions removed.
Contest and Daycare tutorials never block progression. Pokémon
Center/shop/HM-use explanations removed after the first or entirely. Long
badge/HM explanation dialogue reduced to brief notifications. Important
items can be delivered automatically when the associated battle/story flag
completes.

## Quick Travel
Visited towns become available through a quick-travel map. Does not allow
travelling to locations not yet legitimately reached. Primarily eliminates
story-mandated backtracking through already-cleared terrain — new routes
still need to be traversed normally because their encounters and trainers
matter. When the story requires returning to a previously visited town
immediately after a boss/event, the game can offer a direct travel prompt.

## Map-by-map story cuts
*(Full area-by-area breakdown — see original design conversation for
complete detail per location. Preserved here at summary level; expand each
area into its own task when that area is actually being implemented,
Phase 10.)*

- **Littleroot opening**: moving-truck sequence shortened/skipped;
  clock-setting removed (configurable in settings); Mother's dialogue and
  Birch introduction shortened; forced rival-house trip removed; starter
  selection reached almost immediately; Birch rescue tutorial battle
  shortenable/skippable in strict mode.
- **First rival battle** (Route 103): retained; walk back to
  Birch's lab afterward retained so the player gets 5x random items; Pokédex/Balls/Running can be automatically given.
  automatically or via immediate transition.
- **Petalburg/Wally tutorial**: Norman's story gate remains; Wally's
  catching tutorial removed (he leaves offscreen); later meaningful Wally
  battles remain.
- **Petalburg Woods**: retained (encounter location); Aqua grunt battle
  retained; Devon-researcher dialogue shortened.
- **Rustboro/Roxanne**: Gym normal; post-Gym Devon theft story advances
  immediately to Route 116/Rusturf Tunnel; unnecessary cutscenes removed.
- **Route 116/Rusturf Tunnel**: retained; Aqua grunt battle and Peeko
  rescue remain; Devon Goods recovery auto-resolved after the grunt fight
  (no escort/backtrack chain); Devon President meeting compressed/skipped.
- **Briney ferry**: unlocked once Peeko is rescued; no repeat walk through
  Petalburg Woods; Quick Travel available; animations shortened.
- **Dewford**: Brawly and Granite Cave remain; Steven-letter fetch quest no
  longer a major obstacle (auto-completed with minimal dialogue, no repeat
  Devon trip); Slateport travel available immediately after.
- **Slateport**: beach trainers optional; Shipyard "find Stern" detour
  removed; Museum available immediately; Aqua museum battles remain;
  Archie/Stern dialogue compressed; Devon delivery auto-completed.
- **Route 110/Mauville**: rival and Wally battles remain; Wattson remains;
  Trick House/side content optional; Bike Shop gives BOTH bikes at once
  (switchable from menu, no repeat trips); Rock Smash unlocks automatically.
- **Route 117/Verdanturf**: optional; no Shelmet/Karrablast errand; Rusturf
  restoration optional flavor.
- **Routes 111/112/Fiery Path**: retained; repeated forced traversal after
  events eliminated via Quick Travel.
- **Fallarbor/Route 114**: retained; no Remoraid workaround; dialogue
  shortened.
- **Meteor Falls**: retained; Aqua/Magma confrontation shortened; direct
  transition/Quick Travel to Mt. Chimney afterward (no manual retrace).
- **Mt. Chimney**: Aqua/Magma and Maxie battles remain; speeches shortened;
  Jagged Pass immediately becomes the route forward.
- **Jagged Pass/Lavaridge**: retained; Flannery remains; Go-Goggles awarded
  immediately (no extended cutscene); desert access available; Quick Travel
  to Petalburg available after 4 badges.
- **Petalburg/Norman**: no long Wally-family sequence; Surf unlocks
  automatically after victory (no detour into Wally's parents' house).
- **Routes 118/119**: traversed normally (encounters); no unnecessary
  pauses.
- **Weather Institute**: Aqua battle section retained, shortened to direct
  combat route; Castform reward remains; proceeds directly to rival battle.
- **Route 119 rival**: retained; rewards immediate.
- **Fortree/Route 120/Devon Scope**: encounter access remains; Scope
  sequence compressed; Gym immediately accessible after.
- **Winona**: Gym remains; rewards immediate.
- **Route 121/Lilycove**: new encounters remain; rival battle remains;
  Department Store important for guaranteed evolution items.
- **Mt. Pyre**: retained; villain battles retained; orb-theft scene
  compressed; Magma Hideout flag set immediately after; Quick Travel
  offered.
- **Magma Hideout**: retained; grunt/admin/boss battles retained; maze
  shortened substantially; unnecessary Strength/boulder busywork
  removed/simplified; Maxie encounter remains; next Aqua phase unlocks
  immediately.
- **Aqua Hideout**: retained; important trainers/admin battle retained;
  teleporter maze simplified to essentially direct; optional side
  rooms/items (Master Ball) can remain; eastern sea routes unlock after
  the submarine escapes.
- **Routes 124/Mossdeep**: new encounters remain; Gym remains; Tate & Liza
  keep their double-battle identity.
- **Mossdeep Space Center**: Magma takeover retained (culminates in a
  meaningful battle); excess filler grunts reduced; Steven double battle
  remains; dialogue condensed; Dive permission granted immediately after
  (no separate Steven's-house trip).
- **Seafloor Cavern**: retained; Aqua battles and Archie boss remain;
  Strength/boulder/current puzzles heavily simplified into a relatively
  direct path.
- **Kyogre awakening**: shortened; immediately directs player to
  Sootopolis (no pointless sailing through cleared routes).
- **Sootopolis crisis**: Steven/Wallace conversations condensed; no
  multi-NPC fetch chain; player immediately told to reach Sky Pillar and
  wake Rayquaza.
- **Sky Pillar**: retained; long Mach Bike cracked-floor puzzle removed
  from mandatory progression (direct climb available; optional puzzle for
  bonus items); awakening cutscene shortened/skippable.
- **Return from Sky Pillar**: automatic transition/Quick Travel to
  Sootopolis after waking Rayquaza; Gym 8 opens immediately.
- **Post-Rayquaza premium encounters**: Regi caves, Weather Institute
  events, and Slateport legendary-map NPC all become available here —
  none mandatory to continue.
- **Juan/Gym 8**: retained; final level cap becomes 63; Waterfall unlocks
  automatically (no HM party member needed).
- **Ever Grande**: newly relevant routes traversed normally; Waterfall
  works via badge permission; badge-check sequence shortened.
- **Victory Road**: retained; major trainers and final Wally battle
  retained; Rock Smash/Strength/HM busywork removed; maze shortened;
  optional branches can still contain randomized items; permanent shortcut
  opens after exit.
- **Pokémon League**: no repetitive badge-check presentation; player can
  manage team/Level-to-Cap/held items/healing before entering; Elite Four
  gauntlet remains; Portable Heal remains legal outside battle; no battle
  items once combat starts; Champion is the endpoint.

## Hall of Fame
Final six Pokémon recorded, along with species, nicknames, abilities,
moves, held items, IVs, EVs, nature, seed, and ruleset — useful for
recreating/exporting the tournament team.

## Postgame
Nuzlocke run formally ends at Champion by default. Player can optionally
continue into postgame for exploration; permadeath can remain active if
desired. Postgame does not alter the recorded Hall-of-Fame tournament team.

## Fast battle presentation
Faster battle introductions, HP-bar movement, reduced pauses, faster
status/weather animation pacing; optional animation speed settings; battle
animations can remain enabled by default (randomized moves are part of the
spectacle); a fully animation-off option remains available.

## Core design rule for anything not explicitly listed
Ask whether it creates a new encounter, a meaningful battle, a meaningful
team-building decision, or a meaningful resource decision. If yes,
preserve it. If it's merely walking back somewhere already cleared, talking
to NPCs for exposition, delivering an item, watching repeated exposition,
carrying an HM slave, or grinding (levels/money/capture supplies), or
solving an old navigation puzzle solely to trigger the next flag — then
streamline or remove it.

Intended rhythm: **new location/encounter → team decision → trainers →
boss → immediate path to next new location/encounter → repeat.**
