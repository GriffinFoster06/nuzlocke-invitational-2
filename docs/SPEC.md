# ROM Specification

This document is the authoritative product design. Historical Invitational 2
and Randolocke behavior can inform choices only where this specification does
not decide them; neither is an authority over an explicit rule below.

This is a solo, replay-focused randomized Emerald adventure. There is no
Tournament mode, no competitive final-team locking, and no persistent or
cross-run species-ban system. A completed run must never change the species or
other randomization pools used by a later fresh run. The Hall of Fame is an
informational and celebratory solo-run archive only.

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
- The player-facing default is **Recommended**, representing the way the
  developers genuinely intend the game to be played.
- It uses: Hardcore Nuzlocke rules, balanced species randomization, randomized
  moves, randomized abilities, randomized TMs/Tutors/items, Gen 9 mechanics,
  level caps, Level to Cap, Infinite Repel, HM-free progression, linearized
  campaign, maximum-strength fair AI, and the other defaults specified below.
- New Game should begin promptly with Recommended selected instead of forcing
  the player through a large configuration wall.

## Core player-facing presets
- **Recommended** — main default and intended solo replay experience.
- **Modern Emerald** — normal Emerald species/trainers rather than
  randomized ones; modern Gen 9 battle engine and QoL; story streamlining
  can remain enabled.
- **Randomizer** — full randomization and QoL without mandatory Nuzlocke
  restrictions.
- **Custom / Advanced** — supported settings can be individually changed.
  Randolocke-style or other legacy configuration may remain here when useful,
  but does not require equal player-facing prominence.
- Choosing a preset fills all settings automatically. Changing an individual
  setting changes the displayed ruleset to Custom / Advanced. Players can
  restore an individual category or the entire configuration to its preset
  defaults.
- Phase 12 may simplify, hide, reorganize, or remove redundant player-facing
  settings while preserving supported underlying functionality. Feature
  completeness does not require every historical option to have equal
  prominence.

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
  Seed setting may be available without tournament-specific restrictions.
  The seed is shown in Run Information. The ruleset version is also stored so
  a seed remains reproducible after future ROM updates. Same
  ROM/randomizer-version + seed + settings should generate the same world.
  Previous completed runs and Hall-of-Fame records never influence generation
  for a fresh run.

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

## Premium species category
Whenever a project requirement refers to a "legendary" restriction or pool,
it means the broader **Premium** category: Legendary Pokémon, Mythical
Pokémon, Ultra Beasts, Paradox Pokémon, and equivalent restricted or
high-power special species designated by the project.

Premium species may appear at randomized premium/legendary static encounter
locations, on Elite Four teams, and on Champion Wallace's team. They may not
normally appear on ordinary trainers, the rival, Wally, Team Aqua or Team
Magma members, admins, or Gym Leaders. Wallace is guaranteed at least one
Premium species. Each Elite Four member may naturally roll zero or more; no
individual Elite Four member is guaranteed one.

## Wild Pokémon randomization
Default: ON. Every wild encounter slot receives a randomized species. Weak
Pokémon are replaced by approximately weak Pokémon; strong by approximately
strong. Completely unrestricted randomization exists as a non-default
setting. Encounter rates remain those of the original slot unless
encounter-rate randomization is independently enabled. Encounter levels
remain progression appropriate. If a generated wild encounter level would be
above the active legal cap, it is clamped to that cap.

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
selections should not duplicate each other. Starter previews may show species
and type icons. They must not reveal the randomized ability before selection;
the ability remains unknown until the Pokémon is obtained and inspected. Full
randomized learnsets need not be exposed before choosing.

## Starter IVs
Recommended default: exactly 5 perfect IVs and 1 random IV (which stat is
random is itself random).
Settings: 5 perfect + 1 random (default) / 3 perfect / Natural IVs / All 31 /
Custom floor.

## Gift Pokémon
Default: randomized, power-appropriate replacements with 3 guaranteed perfect
IVs for ordinary gift Pokémon, separately configurable from starter IVs.

## Static Pokémon
Default: randomized. Static encounters retain their fixed locations; species
generated once per run. Ordinary static encounters use appropriate power
matching. Every canonical premium/legendary static encounter slot exposed by
the ROM uses the Premium Species pool exclusively, including Rayquaza,
Groudon, Kyogre, the Regis, Latios/Latias premium encounters, Mew, Deoxys,
Ho-Oh, Lugia, and every other exposed event or premium static slot. Each slot
selects its replacement deterministically and independently from the run seed;
only an eligible Premium species is valid, its original species remains
eligible, and duplicate Premium species across separate slots are allowed.
The replacement changes only the encountered species: the slot's location,
event, and progression behavior remain intact. Ordinary wild encounter slots
never generate Premium species. If a generated static encounter level would be
above the active legal cap, it is clamped to that cap. The over-cap/ineligible
system remains useful defensive infrastructure, but normal randomizer
generation must not routinely create unusable wild or static encounters.

## Trainer Pokémon
Default: randomized. Applies to regular trainers, rival, Wally, Team
Aqua/Magma, admins, Gym Leaders, Elite Four, Champion. Each trainer slot
receives a power-appropriate replacement. Party sizes remain authored unless
party-size randomization is explicitly selected. Bosses use stricter power
matching than ordinary route trainers. Trainer species remain fixed for the
run. Slots are randomized individually: bosses do not receive artificially
coherent teams, strategic synergy, physical/special balance, defensive cores,
coverage requirements, or a minimum team-quality floor. A boss may naturally
roll an excellent team or a terrible one.

## Trainer levels
Default: deterministic scaling around the level-cap progression; levels are
never randomly rerolled. For each ordinary mandatory or route trainer
Pokémon, determine approximately how far its vanilla level was below the
corresponding upcoming vanilla boss ace and preserve approximately that offset
below the current custom cap. This retains the trainer's original difficulty
relationship to the upcoming boss. Every Gym Leader Pokémon is exactly at that
Gym's active cap. Every Elite Four Pokémon and every Champion Wallace Pokémon
is exactly at the final pre-Champion cap. Difficulty should come primarily
from randomized species, moves, abilities, and intelligent AI.

## Trainer movesets
A trainer Pokémon uses the four most recently learned moves available from its
randomized learnset at its current level, following normal Pokémon
move-learning behavior. Do not construct or optimize trainer movesets, and do
not improve boss movesets separately.

## Default cap progression
The cap equals the next Gym Leader's highest level. The progression is:
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

## Hard level caps
Default: ON. Pokémon cannot gain levels past the active cap; additional EXP
cannot accidentally push a Pokémon over. Current cap visible in the
interface.
Modes: Hard cap (default) / Soft cap / Warning only / Off.

## Caught Pokémon above the cap
Acquisition levels are clamped to the current progression cap whenever caps are
enabled. Soft-cap and warning-only modes can still produce an over-cap Pokémon
through later growth. When over-cap ineligibility is enabled, such a Pokémon
cannot enter battle or provide encounter-manipulation advantages until
progression raises the cap sufficiently. As a safety exception, the restriction
is not applied when the party has no cap-legal battler.

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
threshold / current cap. Not required by Recommended.

## Randomized level-up moves
Default: ON. Pokémon do not use canonical learnsets. Learnsets are generated
once per run and remain deterministic.

## Learnset size
Every species has exactly 21 standardized level-up move-learning
opportunities, using the same 21 learning checkpoints. A species cannot have
a duplicate move within its generated 21-move learnset.

## Learnset composition
The default is weighted random generation, not literal 7/7/7. The pool is the
valid supported Gen 9 move pool after excluding nonfunctional, internal, and
configured-blacklist moves. STAB moves have increased probability but are not
guaranteed. Stronger damaging moves become progressively more likely at later
checkpoints. Particularly powerful utility or status moves may likewise be
weighted later. These are probabilities, not hard restrictions: weak or
terrible moves may occur late and unusually strong moves may rarely occur
early.

There is no guarantee of coverage, recovery, setup, damaging/status ratios,
physical-versus-special role optimization, or any general quality floor.
Bosses receive no special moveset improvement. The randomizer must allow both
spectacularly bad and spectacularly good Pokémon and must not deliberately
prevent either result. For example, both of these are legal outcomes:

- Charizard with Constrict / Splash / Nightmare / Smog and Truant.
- Slaking with Spore / Shift Gear / Population Bomb / Wicked Blow and Huge
  Power.

## Move-power progression
Default: higher-power attacks tend to be learned later (weighted, not
rigidly sorted — a rare unusually strong early roll can still occur). Status
moves are evaluated separately from raw base power, and exceptionally
powerful utility/status moves may also be biased later. No quality threshold
is imposed at any checkpoint.
Optional mode: fully random move order.

## Move pool
Moves through Gen 9. Signature attacks can appear on unrelated Pokémon
unless the move literally cannot function outside its original
implementation. Blank/internal moves, Struggle, and broken
placeholder/debug effects excluded. Configurable move blacklist for OHKO
moves, Evasion, Sleep moves, Self-KO moves, other unwanted effects.

## Move Reminder
Default: disabled/free reminder not available, preserving the strategic
importance of forgetting moves. A forgotten randomized level-up move is a
meaningful loss unless reacquired through another legitimate source.
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
Move-gated evolutions were converted to level evolutions, so randomized Tutors
cannot softlock evolution progress.

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
Default: Shedinja retains Wonder Guard; Wonder Guard does not randomly appear
elsewhere by default. Custom chaos mode may permit full Wonder Guard
randomization.

## Items
Default: field items randomized, gift items randomized. Hidden field items
treated as field items. Item results remain fixed for the run. Duplicate
items allowed. Key items required for progression are never destroyed by
randomization. Story-critical traversal items are either protected or
replaced by automatic progression systems. Shops NOT randomized by default
(optional in Custom). Field-item rewards are unrestricted by reward tier; do
not tier-balance them.

## Item-pool safety
Required key items do not become Potions. Ordinary consumable pickups cannot
replace progression flags. Evolution availability guaranteed independently
of random field-item luck. Items with no legitimate purpose under Recommended
are excluded from the ordinary randomized field-item pool, including:

- ordinary HP-healing and status-curing medicines;
- Revives;
- temporary X-item-style battle boosters and other non-held, battle-only
  consumables that cannot legally be used;
- Repels, because Infinite Repel replaces them;
- Rare Candies, because Level to Cap replaces them;
- sell-only treasure, because money is unlimited;
- redundant PP-restoration consumables, because Portable Heal restores PP
  outside battle;
- Nature Mints, Ability Capsule, Ability Patch, Bottle Caps, and Hyper
  Training items;
- items used solely for Mega Evolution, Primal battle-gimmick
  transformations, Z-Moves, Dynamax/Gigantamax, Terastallization, or another
  deliberately absent battle gimmick;
- every Poké Ball other than the Master Ball.

Functional items remain eligible, including berries that can be held,
ordinary held items, permanent stat boosters such as Protein, PP Up, PP Max,
Toxic Orb and similar held items, and useful Gen 1-9 non-key items that work
without violating the rules. An allowed ordinary item need not also be sold;
items such as Toxic Orb may exist only through randomized pickups.

## Modern held items
Held items through Gen 9 supported with Gen 9 effect behavior. Randomized
item pools can include useful modern held items.

## Vendor and modern-item integration
Supported Gen 4-9 items must have logical game integration, but not every item
must be sold. Items that logically belong in shops, especially modern Poké
Balls, should be placed and priced as though they had existed in Emerald
originally.

## Unlimited money
Default: ON. Money is not intended to be a limiting resource — purchases
effectively do not exhaust spending ability. Removes money farming. The
Oldale money-refill NPC can remain as a redundant convenience/easter egg.

## Poké Ball availability
The Master Ball is the only Poké Ball permitted in the randomized field-item
pool. Every other Ball comes from progression-based availability rather than
randomized pickups. Unlimited money does not make every Ball available
immediately, but it makes every currently unlocked Ball effectively unlimited
without resource grinding.

Use Emerald's existing shop progression as the anchor, assigning unlocks at
logical points based on each Ball's power and usefulness. The ordinary Poké
Ball unlocks when catching unlocks; Great Ball and comparable early/mid tiers
come later; Timer Ball, Repeat Ball, and comparable specialty Balls unlock at
a sensible early/midgame point; Ultra Ball arrives at its appropriate later
point. By the time Ultra Ball is available, essentially every normal supported
specialty Ball should also be obtainable. Mechanically normal modern options
such as Apricorn Balls, Dream Ball, and Beast Ball may have late availability.
Exclude Cherish, Safari, Sport, or other unusual Balls when their actual
implementation is event/location-specific or inappropriate for normal shops.

The Master Ball is never sold. The original Aqua Hideout Master Ball location
becomes an ordinary randomized field item and may roll a Master Ball only by
chance. Exactly one Master Ball is guaranteed immediately before the
post-Rayquaza Premium encounters unlock. That guarantee does not prevent an
additional Master Ball from appearing randomly.

## 999 Poké Ball NPC
Default: present at the moment catching first unlocks. The player receives 999
ordinary Poké Balls (not Great or Ultra Balls). Ordinary Poké Balls may remain
purchasable afterward. This removes tedious early capture-resource management
without destroying Ball progression.

## Catch rates
Default: moderately increased catch rates.
Settings: Vanilla / Moderate Boost (default) / Large Boost / Guaranteed.

## Fishing
On a fishable tile with a non-null fishing table for the current map and time
of day, Old, Good, and Super Rod casts always bite and start an encounter after
an untimed A-button prompt. Invalid tiles and missing tables remain invalid.
Rod-specific tables, relative slot weights, level generation, randomizer and
Nuzlocke behavior, catch probability, and presentation are preserved except
for random no-bite and input-timing failures.

## Ball shortcut
Default: ON. Outside the shortcut, normal Bag selection remains available.
The L button cycles forward through Ball types currently possessed and wraps
from the last available type to the first. During a legal wild encounter, the
R button throws the currently selected Ball. The shortcut never creates an
unowned Ball and never automatically chooses the Master Ball merely because
one is available; the player must deliberately select it. No Ball-quantity
display is required for the shortcut.

## Nuzlocke rules start gate
**Nuzlocke rules (permadeath and one-encounter-per-location) do not begin
until the player has received their first Poké Balls.** Before that
point, wild encounters do not consume the route/location under the
one-encounter-per-location rule, and fainting does not trigger permadeath
— there's no way to catch or meaningfully lose anything yet, so there's
nothing to protect. Once Poké Balls are obtained for the first time, all
Nuzlocke rules activate normally and apply going forward, including for
the current location if it's still unresolved at that moment.

## Nuzlocke permadeath
Default: ON (once the start gate above has passed). If a Pokémon faints,
it is permanently dead. Dead Pokémon cannot battle, be revived into legal
use, use field abilities, activate encounter-manipulation abilities, or be
used for any other gameplay advantage. Dead Pokémon can be automatically
moved to a designated Graveyard PC box after battle; the player may still
inspect them.

## One encounter per location
Default: ON (once the start gate above has passed). Only the first valid
encounter for each location can be caught. Location identity uses the
actual location tag — a different location tag means a new encounter.
Once resolved, that location is marked used. Strict default: killing,
running from, or failing to catch the valid encounter all consume the
location.

## Dupes Clause
Default: ON. Dupes history covers evolutionary families that have previously
been the player's valid encounter. A family enters that history whether the
valid encounter was caught, killed, fled, or the player ran from it. Family
means the entire evolutionary family; split evolutions count as one family,
and forms/regional variants of the same family count as duplicates.

A future encounter from a recorded family is a Dupes Clause encounter and
does not consume the new location's encounter opportunity. Running from a
dupe is always safe. The game continues searching or allowing encounters until
a non-dupe valid encounter occurs. If the eligible pool is exhausted, it must
handle that state safely rather than loop forever.

## Shiny Clause
Default: ON. A shiny may always be caught regardless of whether the location
encounter was already consumed, Dupes Clause, or any previous encounter. It is
a bonus encounter: catching it neither consumes nor replaces the location's
normal Nuzlocke encounter opportunity. Normal shiny odds unless explicitly
changed in Custom.

## Nicknames
Default: optional/off. A separately selectable rule can require every obtained
Pokémon to be nicknamed and prevent bypassing the prompt.

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
defeating an opposing Pokémon. Can be changed when strict rules are not locked.

## Portable healing
Default: ON. Available from the menu; restores HP/PP/status; does not
revive dead Pokémon. Removes Pokémon Center detours (healing already free
outside battles).

## Infinite Repel
Default: ON/available. Toggleable outside battle; prevents ordinary random
encounters while enabled; player disables it when deliberately seeking a
new-location encounter. Static/scripted encounters still occur. Visible
indicator shows current state. Do not add an unused-encounter warning.

## Expanded Bag
Default: ON. Bag capacity is effectively unlimited for normal gameplay: it has
enough unique slots for the complete supported obtainable item pool, permits
stack sizes up to 999 where appropriate, and does not fail in ordinary play
because the player collected too many distinct useful items.

## Bikes
The player permanently receives both the Mach Bike and Acro Bike; returning to
Rydel to exchange them is unnecessary. Bike mode can be switched from the menu
outside battle.

## Type icons
Where technically appropriate, display Pokémon type icons next to Pokémon
names in the Party, PC, Summary, starter selection, player battle HUD, and
enemy battle HUD. Species typing is public information and may be shown for
enemy Pokémon.

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

**Global rule: every evolution is level-based except item/stone
evolutions.** Friendship, time-of-day, specific-move-known, location,
party-member-present, stat-comparison, weather, region, beauty, and any
other non-item evolution condition is replaced with a specific level
threshold instead. Item/stone evolutions (Thunder Stone, Water Stone,
Fire Stone, Leaf Stone, Moon Stone, Sun Stone, Shiny Stone, Dusk Stone,
Dawn Stone, Ice Stone, etc.) are the only evolutions that keep a
non-level trigger. Trade evolutions also become plain level-based under
this rule, not item-based — a held item during trade is not itself a
stone.

Some evolutions may still be legitimately progression-gated by when a
required stone becomes available (e.g. a stone not available until
Lilycove is fine). Rule: **Delayed is acceptable. Impossible is not.**

**Eevee specifically:** Vaporeon/Jolteon/Flareon/Leafeon/Glaceon keep
their existing stones (Water/Thunder/Fire/Leaf/Ice). The three currently
non-stone Eeveelutions get stones unused by the others:
- Espeon → Sun Stone
- Umbreon → Moon Stone
- Sylveon → Shiny Stone

**Evolution Assistance:** Phase 9.5 verified that the final evolution dataset
contains no specific-move-known requirements. Evolution Assistance was
therefore unnecessary and was removed. The Evolve command below remains.

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

## Move-dependent evolution anti-softlock (completed historical context)
Originally: the Lilycove Tutor Mansion was removed only because a
guaranteed replacement system existed — a Pokémon requiring a specific
move to evolve could always obtain that move through an Evolution
Assistance function, restricted specifically to evolution-necessary moves
(not a general free Move Reminder). Under the new global rule, move-gated
evolutions no longer exist as a category (they are level-based now). Phase 9.5
verified the final dataset and removed the now-unused Evolution Assistance
infrastructure; the separate Evolve command remains available.

## Trade evolutions
Converted into deterministic single-player evolutions using a specific
level threshold, per the global rule above — not an item or held-item
substitute. No Link Cable / second game necessary.

## Species-specific evolution fixes
- **Shelmet and Karrablast**: Verdanturf forced trades removed; both get
  deterministic single-player level-based evolutions (party-member
  requirement removed per the global rule, not replaced with an item).
- **Mantyke**: Fallarbor free Remoraid workaround removed; deterministic
  single-player evolution level (party-member requirement removed, not
  replaced with an item).
- **Gimmighoul**: 999-coin NPC removed; deterministic level-based evolution.
- **Galarian Yamask**: Route 111 statue removed; deterministic level-based
  evolution.
- **Bisharp**: Route 123 three-Leader's-Crest event removed; Kingambit gets
  a deterministic single-player level-based requirement (not a
  repeated-item-collection trick).
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

## Premium encounter balancing
Premium static slots use the project's eligible Premium Species pool, not
ordinary power matching or a mixed high-power pool. A slot may remain its
original species, but may never become an ordinary non-Premium Pokémon.
Ordinary wild encounter slots do not draw from this restricted pool. Duplicate
Premium encounters are allowed unless another explicit rule in this
specification says otherwise.

## Generation 9 mechanics
Required modern battle baseline; ON in Recommended. Includes the modern
physical/special split, Fairy type, modern type chart, move effects, abilities,
held items, damage calculation, critical-hit behavior, burn, paralysis,
weather, terrain, priority, multi-hit behavior, targeting, switching
interactions, status/type immunities, and end-of-turn processing.

## Battle gimmicks
Mega Evolution, Primal battle-gimmick transformations, Z-Moves, Dynamax,
Gigantamax, and Terastallization are not part of this game and must not be
exposed in Recommended gameplay. Items that exist solely for those systems
must not enter the normal item pool.

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
know the randomizer's general weighted-generation rules without knowing which
four specific moves a given opposing Pokémon carries. It cannot assume that a
Pokémon has STAB, coverage, status, setup, recovery, or any minimum move
quality. It can hedge against plausible threats but cannot magically choose
the correct counter to a secret move every turn.

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
Default: heavily linearized. The individual cuts below define this project's
implementation.

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
- **First rival battle** (Route 103): retained; no forced walk back to
  Birch's lab afterward; Pokédex/Balls/Running can be awarded
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
  rooms and randomized items can remain; the original Master Ball pickup is
  an ordinary randomized field item; eastern sea routes unlock after the
  submarine escapes.
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

## Terminal Run Reports
Every strict run finalizes one comprehensive, informational-only Run Report at
either terminal outcome: **Victory** (Champion defeated / Hall of Fame
reached) or **Wipe** (no usable Pokémon remain and the run is lost). Reports
never affect later runs, randomization pools, or species eligibility.

Both outcomes record a report/schema version, project/ROM version,
randomizer/ruleset version, run seed, preset/ruleset, relevant generation
settings, player name where useful, terminal result, play time, progression
state, and a unique run/report identifier.

Victory reports contain the complete Hall-of-Fame team. Wipe reports capture
the complete final party immediately before run-loss cleanup destroys relevant
state. For each Pokémon, reports preserve every reasonably available,
player-relevant persistent attribute needed to faithfully describe or
reconstruct it: party slot; species and form; nickname; gender; shiny state;
level and useful experience; types; Nature; ability; held item; moves with
current/max PP and useful PP bonuses; IVs; EVs; calculated stats; meaningful
current HP, status, friendship, and Poké Ball; met/caught level and location;
Nuzlocke encounter location and encounter metadata; alive/dead/legal state;
and other stable useful attributes. The external representation may include
both stable IDs and human-readable names.

Wipe reports capture, where practical, the wipe location/map, badge count,
active cap, story/progression checkpoint, last major boss defeated, opponent
type, trainer/boss identity, reliably available opponent team, final
knockout/cause, and play time before cleanup occurs.

Reports track useful, inexpensive run statistics that cannot reliably be
reconstructed later, rather than meaningless counters. This includes where
practical encounters obtained; successful catches; failed, killed, fled, and
player-run-from encounters; Dupes Clause and shiny encounters; shiny catches;
total deaths, death order, and individual death details; trainer and wild
battles; Gym Leaders and major bosses defeated; Balls thrown and capture
attempts; evolutions; Level to Cap uses; badge progression; final cap; and
play time. Individual death records may include the Pokémon, level, location,
opponent, cause, and progression checkpoint.

Canonical finalized report state lives in save data. The latest finalized
report remains recoverable long enough for manual export, including across
practical run-reset/new-run handling; the exact save architecture is designed
in Phase 11C2. A standalone repository tool must read an ordinary `.sav` and
export its finalized report without modifying that save. Its versioned JSON
uses human-readable names for species, forms, moves, abilities, items,
locations, and settings; preferred non-overwriting filenames are
`run_<seed>_victory.json` and `run_<seed>_wipe.json`.

For the preferred supported-mGBA experience, an optional companion integration
automatically exports the finalized JSON exactly once when a Victory or Wipe
report finalizes. Core ROM gameplay never depends on mGBA, and manual
`.sav`-to-JSON export remains the portable fallback. A unique report ID and
ready state prevent repeated save events from exporting the same report more
than once. The ROM must not claim that an external JSON file was created until
host-side tooling confirms it.

## Postgame
Nuzlocke run formally ends at Champion by default. Player can optionally
continue into postgame for exploration; permadeath can remain active if
desired. Postgame and completed-run records never influence randomization in a
later fresh run.

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
