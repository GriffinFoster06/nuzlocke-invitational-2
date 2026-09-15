#!/usr/bin/env python3
"""Export a finalized Phase 11E Run Report from an ordinary .sav to JSON.

Read-only: this program never writes to the source .sav. It parses the
report bank at flash sector SECTOR_ID_RUN_REPORT (see include/save.h /
include/run_report.h) and emits one versioned, human-readable JSON document
per finalized report, expanding stable IDs (species, moves, items, abilities,
balls, map sections, settings) into names by parsing this repository's own
constants headers - so names can never drift from the ROM that produced the
save.

    python3 tools/export_run.py <save-file> [-o DIR] [--slot N] [--all]
                                 [--force] [--json-only]
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
import zlib
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Save/flash layout (include/save.h) - must match the ROM build exactly.
# ---------------------------------------------------------------------------
SECTOR_SIZE = 0x1000
SECTOR_DATA_SIZE = 3968
SAVE_BLOCK_3_CHUNK_SIZE = 116
SECTOR_ID_RUN_REPORT = 30  # SECTOR_ID_TRAINER_HILL, see include/save.h
SPECIAL_SECTOR_SENTINEL = 0xB39D
FLASH_SIZE = 128 * 1024

# ---------------------------------------------------------------------------
# Report bank/record format (include/run_report.h) - keep in lockstep with
# the C structs. RUN_REPORT_SCHEMA_VERSION gates which format this parses.
# ---------------------------------------------------------------------------
RUN_REPORT_BANK_MAGIC = b"RUNR"
RUN_REPORT_RECORD_MAGIC = b"RREC"
RUN_REPORT_BANK_VERSION = 1
SUPPORTED_SCHEMA_VERSIONS = (1,)

RUN_REPORT_MAX_DEATHS = 24
RUN_REPORT_MAX_SETTINGS = 128
RUN_REPORT_BANK_SLOTS = 2
PARTY_SIZE = 6
MAX_MON_MOVES = 4
NUM_STATS = 6

RUN_RESULT_NAMES = {0: "none", 1: "victory", 2: "wipe"}
DEATH_CAUSE_NAMES = {0: "unknown", 1: "battle", 2: "field_poison", 3: "debug"}
WIPE_OPPONENT_KIND_NAMES = {0: "unknown", 1: "wild", 2: "trainer", 3: "boss", 4: "field_poison"}
BATTLE_OUTCOME_NAMES = {
    0: "none", 1: "won", 2: "lost", 3: "drew", 4: "ran",
    5: "player_teleported", 6: "mon_fled", 7: "caught",
    8: "no_safari_balls", 9: "forfeited", 10: "mon_teleported",
}
STAT_NAMES = ("hp", "attack", "defense", "speed", "sp_attack", "sp_defense")
NATURE_NAMES = (
    "hardy", "lonely", "brave", "adamant", "naughty", "bold", "docile", "relaxed",
    "impish", "lax", "timid", "hasty", "serious", "jolly", "naive", "modest", "mild",
    "quiet", "bashful", "rash", "calm", "gentle", "sassy", "careful", "quirky",
)
RUN_REPORT_MON_FLAG_SHINY = 1 << 0
RUN_REPORT_MON_FLAG_DEAD = 1 << 1
RUN_REPORT_MON_FLAG_EGG = 1 << 2
RUN_REPORT_MON_FLAG_OVER_CAP_INELIGIBLE = 1 << 3
RUN_REPORT_DEATH_FLAG_SHINY = 1 << 0
RUN_REPORT_DEATH_FLAG_OPPONENT_TRAINER = 1 << 1
RUN_REPORT_FLAG_RUN_LOST = 1 << 0
RUN_REPORT_FLAG_DEATHS_TRUNCATED = 1 << 1

# struct RunReportMon, PACKED, 79 bytes - include/run_report.h field order:
# species, heldItem, personality, otId, exp, nickname, level, ball, nature,
# gender, friendship, ppBonuses, metLocation, metLevel, slot, monFlags,
# ability, status, hp, maxHp, stats[5], moves[4], ivPacked, evs[6], pp[4].
MON_STRUCT = struct.Struct("<HHIII13s" + "B" * 10 + "HHHH5H4HI6s4s")
assert MON_STRUCT.size == 79, MON_STRUCT.size

# struct RunDeathRecord, PACKED, 12 bytes.
DEATH_STRUCT = struct.Struct("<HBBHHBBH")
assert DEATH_STRUCT.size == 12, DEATH_STRUCT.size

# struct RunWipeContext, PACKED, 12 bytes.
WIPE_STRUCT = struct.Struct("<HBBHHBBH")
assert WIPE_STRUCT.size == 12, WIPE_STRUCT.size

# struct RunStatsSnapshot, PACKED, 19 x u16 = 38 bytes.
STATS_STRUCT = struct.Struct("<19H")
assert STATS_STRUCT.size == 38, STATS_STRUCT.size

# struct RunReportRecord header, up to (not including) settings[]/stats/wipe/
# party/deaths, which are handled as sub-structs below.
RECORD_HEADER_STRUCT = struct.Struct(
    "<"
    "I"   # magic
    "H"   # schemaVersion
    "H"   # recordSize
    "I"   # crc32
    "I"   # reportId
    "I"   # runSeed
    "I"   # newRunCounter
    "H"   # projectVersion
    "H"   # rulesetVersion
    "H"   # randomizerVersion
    "H"   # genMask
    "B"   # result
    "B"   # preset
    "B"   # playerGender
    "B"   # flags
    "8s"  # playerName
    "I"   # playTimePacked
    "H"   # badgeMask
    "B"   # badgeCount
    "B"   # finalCap
    "B"   # progressionStep
    "B"   # partyCount
    "B"   # settingsCount
)
RECORD_TAIL_STRUCT = struct.Struct("<HBB")  # totalDeaths, deathRecords, pad0

RECORD_SIZE = (
    RECORD_HEADER_STRUCT.size
    + RUN_REPORT_MAX_SETTINGS
    + STATS_STRUCT.size
    + WIPE_STRUCT.size
    + MON_STRUCT.size * PARTY_SIZE
    + RECORD_TAIL_STRUCT.size
    + DEATH_STRUCT.size * RUN_REPORT_MAX_DEATHS
)
assert RECORD_SIZE == 999, RECORD_SIZE

BANK_HEADER_STRUCT = struct.Struct("<IHBB")  # magic, bankVersion, newestSlot, slotsUsed
BANK_SIZE = BANK_HEADER_STRUCT.size + RECORD_SIZE * RUN_REPORT_BANK_SLOTS
assert BANK_SIZE == 2006, BANK_SIZE


def fail(message: str) -> None:
    raise ValueError(message)


# ---------------------------------------------------------------------------
# GBA charmap decoding (charmap.txt)
# ---------------------------------------------------------------------------

_CHARMAP_LINE_RE = re.compile(r"^'((?:\\.|[^'\\]))'\s*=\s*([0-9A-Fa-f]{2})\b")
_CHARMAP_ESCAPES = {"\\'": "'", "\\\\": "\\", "\\l": "\u2191", "\\p": "\u00b6", "\\n": "\n"}


def load_charmap(repo: Path | None) -> dict[int, str]:
    if repo is None:
        return {}
    path = repo / "charmap.txt"
    if not path.is_file():
        return {}
    table: dict[int, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = _CHARMAP_LINE_RE.match(line.strip())
        if not m:
            continue
        raw_char, hex_byte = m.group(1), m.group(2)
        byte = int(hex_byte, 16)
        if byte == 0xFF:
            continue  # EOS - never a printable character in our decoder
        char = _CHARMAP_ESCAPES.get(raw_char, raw_char)
        table.setdefault(byte, char)
    return table


def decode_gba_string(data: bytes, charmap: dict[int, str]) -> str:
    out = []
    for byte in data:
        if byte == 0xFF:  # EOS
            break
        out.append(charmap.get(byte, f"\\x{byte:02X}"))
    return "".join(out)


# ---------------------------------------------------------------------------
# Generic C enum parser, used to expand species/move/item/ability/ball/mapsec/
# setting IDs into names straight from this repository's own headers. A
# single forward pass in file order is correct because standard C only
# allows an enumerator's value to reference an earlier enumerator in the
# same enum.
# ---------------------------------------------------------------------------

_ENUM_BLOCK_RE = re.compile(r"enum\b([^{;]*)\{(.*?)\}\s*;", re.DOTALL)
_COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.DOTALL)
_ENTRY_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*(?:=\s*(.*))?$")


def _split_top_level_commas(text: str) -> list[str]:
    parts, current, depth = [], [], 0
    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(ch)
    tail = "".join(current).strip()
    if tail:
        parts.append(tail)
    return parts


def parse_enum_constants(header_text: str, expect_prefix: str, enum_name: str | None = None) -> dict[str, int]:
    """Finds the enum block whose declared name matches `enum_name` (a whole
    word, so "SettingId" does not also match "SettingIdSomethingElse"), or -
    for anonymous enums / when no name matches - the first block whose first
    member starts with `expect_prefix`. Several ruleset.h enums share the
    SETTING_ prefix, which is why the name check exists at all."""
    text = _COMMENT_RE.sub("", header_text)
    name_re = re.compile(rf"\b{re.escape(enum_name)}\b") if enum_name else None
    fallback_entries = None
    for header, body in _ENUM_BLOCK_RE.findall(text):
        entries = [e.strip() for e in _split_top_level_commas(body) if e.strip()]
        if not entries:
            continue
        if name_re is not None and name_re.search(header):
            return _resolve_enum_entries(entries)
        first = _ENTRY_RE.match(entries[0])
        if fallback_entries is None and first and first.group(1).startswith(expect_prefix):
            fallback_entries = entries
    if fallback_entries is not None:
        return _resolve_enum_entries(fallback_entries)
    return {}


def _resolve_enum_entries(entries: list[str]) -> dict[str, int]:
    # A single forward pass in file order is correct because standard C only
    # allows an enumerator's value to reference an earlier enumerator in the
    # same enum.
    values: dict[str, int] = {}
    current = -1
    for entry in entries:
        m = _ENTRY_RE.match(entry)
        if not m:
            continue
        name, rhs = m.group(1), m.group(2)
        if rhs is None:
            current += 1
        else:
            rhs = rhs.strip()
            try:
                current = int(rhs, 0)
            except ValueError:
                current = values.get(rhs, current + 1)
        values[name] = current
    return values


def _load_enum(
    repo: Path | None, relative_path: str, prefix: str, sanity: dict[str, int], enum_name: str | None = None
) -> dict[int, str]:
    """Returns {value: name}. Empty dict (graceful fallback) if the header is
    missing, unparsable, or fails its sanity checks - callers then fall back
    to bare numeric IDs rather than risk silently wrong names."""
    if repo is None:
        return {}
    path = repo / "include" / "constants" / relative_path
    if not path.is_file():
        return {}
    try:
        values = parse_enum_constants(path.read_text(encoding="utf-8", errors="replace"), prefix, enum_name)
    except (OSError, ValueError):
        return {}
    for name, expected in sanity.items():
        if values.get(name) != expected:
            print(
                f"export_run.py: warning: {relative_path} sanity check failed for {name} "
                f"(expected {expected}, got {values.get(name)}) - names disabled for this category",
                file=sys.stderr,
            )
            return {}
    return {value: name for name, value in values.items()}


class NameTables:
    """Lazily-loaded name tables. All lookups degrade to None (never a guess)
    when the repository headers are unavailable or --json-only is passed."""

    def __init__(self, repo: Path | None):
        self.repo = repo
        self.species = _load_enum(repo, "species.h", "SPECIES_", {"SPECIES_NONE": 0, "SPECIES_BULBASAUR": 1}, "Species")
        self.moves = _load_enum(repo, "moves.h", "MOVE_", {"MOVE_NONE": 0, "MOVE_POUND": 1}, "Move")
        self.items = _load_enum(repo, "items.h", "ITEM_", {"ITEM_NONE": 0, "ITEM_POKE_BALL": 1}, "Item")
        self.abilities = _load_enum(repo, "abilities.h", "ABILITY_", {"ABILITY_NONE": 0, "ABILITY_STENCH": 1}, "Ability")
        self.balls = _load_enum(repo, "pokeball.h", "BALL_", {"BALL_STRANGE": 0, "BALL_POKE": 1}, "PokeBall")
        self.mapsecs = _load_enum(repo, "region_map_sections.h", "MAPSEC_", {"MAPSEC_LITTLEROOT_TOWN": 0})
        self.settings = _load_enum(repo, "ruleset.h", "SETTING_", {"SETTING_PRESET": 0}, "SettingId")
        self.charmap = load_charmap(repo)

    def named(self, table: dict[int, str], value: int) -> dict[str, Any]:
        name = table.get(value)
        return {"id": value, "name": name}

    def text(self, data: bytes) -> str:
        return decode_gba_string(data, self.charmap)


# ---------------------------------------------------------------------------
# Binary parsing: bank -> record -> JSON
# ---------------------------------------------------------------------------


def read_sector(sav: bytes, sector_id: int) -> bytes:
    offset = sector_id * SECTOR_SIZE
    if offset + SECTOR_SIZE > len(sav):
        fail(f"save file too short to contain sector {sector_id} (need {offset + SECTOR_SIZE} bytes)")
    return sav[offset : offset + SECTOR_SIZE]


def unpack_bank(sav: bytes) -> dict[str, Any]:
    sector = read_sector(sav, SECTOR_ID_RUN_REPORT)
    sentinel = struct.unpack_from("<I", sector, 0)[0]
    if sentinel != SPECIAL_SECTOR_SENTINEL:
        fail(
            "no Run Report found in this save (sector sentinel mismatch - "
            "the run has not finalized a Victory or Wipe yet)"
        )
    payload = sector[4:]
    magic, bank_version, newest_slot, slots_used = BANK_HEADER_STRUCT.unpack_from(payload, 0)
    if magic != struct.unpack("<I", RUN_REPORT_BANK_MAGIC)[0]:
        fail(f"bad Run Report bank magic: 0x{magic:08x}")
    if bank_version != RUN_REPORT_BANK_VERSION:
        fail(f"unsupported Run Report bank version: {bank_version}")
    if slots_used == 0:
        fail("Run Report bank is present but empty (slotsUsed == 0)")

    records_raw = payload[BANK_HEADER_STRUCT.size : BANK_HEADER_STRUCT.size + RECORD_SIZE * RUN_REPORT_BANK_SLOTS]
    return {
        "newest_slot": newest_slot,
        "slots_used": slots_used,
        "records_raw": [records_raw[i * RECORD_SIZE : (i + 1) * RECORD_SIZE] for i in range(RUN_REPORT_BANK_SLOTS)],
    }


def unpack_mon(raw: bytes, names: NameTables) -> dict[str, Any] | None:
    (
        species, held_item, personality, ot_id, exp, nickname_raw,
        level, ball, nature, gender, friendship, pp_bonuses,
        met_location, met_level, slot, mon_flags,
        ability, status, hp, max_hp,
        atk, defense, speed, sp_atk, sp_def,
        move1, move2, move3, move4,
        iv_packed, evs_raw, pp_raw,
    ) = MON_STRUCT.unpack(raw)

    if species == 0:
        return None

    ivs = {stat: (iv_packed >> (5 * i)) & 0x1F for i, stat in enumerate(STAT_NAMES)}
    evs = {stat: value for stat, value in zip(STAT_NAMES, evs_raw)}
    moves = [move1, move2, move3, move4]
    pp = list(pp_raw)

    return {
        "slot": slot,
        "species": names.named(names.species, species),
        "nickname": names.text(nickname_raw),
        "level": level,
        "experience": exp,
        "nature": {"id": nature, "name": NATURE_NAMES[nature] if nature < len(NATURE_NAMES) else None},
        "ability": names.named(names.abilities, ability),
        "held_item": names.named(names.items, held_item) if held_item else None,
        "gender": {0: "male", 1: "female", 2: "genderless"}.get(gender, "unknown"),
        "personality": personality,
        "ot_id": ot_id,
        "moves": [
            {"move": names.named(names.moves, m), "pp": p, "pp_bonus": (pp_bonuses >> (2 * i)) & 0x3}
            for i, (m, p) in enumerate(zip(moves, pp))
            if m != 0
        ],
        "ivs": ivs,
        "evs": evs,
        "stats": {
            "hp": hp, "max_hp": max_hp, "attack": atk, "defense": defense,
            "speed": speed, "sp_attack": sp_atk, "sp_defense": sp_def,
        },
        "status": "none" if status == 0 else f"0x{status:04x}",
        "friendship": friendship,
        "ball": names.named(names.balls, ball),
        "met": {"level": met_level, "location": names.named(names.mapsecs, met_location)},
        "nuzlocke": {"encounter_location": names.named(names.mapsecs, met_location)},
        "state": {
            "shiny": bool(mon_flags & RUN_REPORT_MON_FLAG_SHINY),
            "alive": not bool(mon_flags & RUN_REPORT_MON_FLAG_DEAD),
            "egg": bool(mon_flags & RUN_REPORT_MON_FLAG_EGG),
            "over_cap_ineligible": bool(mon_flags & RUN_REPORT_MON_FLAG_OVER_CAP_INELIGIBLE),
        },
    }


def unpack_death(raw: bytes, names: NameTables) -> dict[str, Any]:
    species, level, cause, map_sec, opponent_id, badges, rec_flags, play_time_minutes = DEATH_STRUCT.unpack(raw)
    opponent_is_trainer = bool(rec_flags & RUN_REPORT_DEATH_FLAG_OPPONENT_TRAINER)
    return {
        "species": names.named(names.species, species),
        "level": level,
        "cause": DEATH_CAUSE_NAMES.get(cause, "unknown"),
        "location": names.named(names.mapsecs, map_sec),
        "opponent": (
            {"kind": "trainer", "trainer_id": opponent_id}
            if opponent_is_trainer
            else {"kind": "wild", "species": names.named(names.species, opponent_id)} if opponent_id else None
        ),
        "shiny": bool(rec_flags & RUN_REPORT_DEATH_FLAG_SHINY),
        "badges": badges,
        "play_time_minutes": play_time_minutes,
    }


def unpack_record(raw: bytes, names: NameTables) -> dict[str, Any]:
    if len(raw) != RECORD_SIZE:
        fail(f"internal error: record slice is {len(raw)} bytes, expected {RECORD_SIZE}")

    header = RECORD_HEADER_STRUCT.unpack_from(raw, 0)
    (
        magic, schema_version, record_size, crc32_stored, report_id, run_seed, new_run_counter,
        project_version, ruleset_version, randomizer_version, gen_mask,
        result, preset, player_gender, flags,
        player_name_raw, play_time_packed, badge_mask, badge_count, final_cap,
        progression_step, party_count, settings_count,
    ) = header
    offset = RECORD_HEADER_STRUCT.size

    if magic != struct.unpack("<I", RUN_REPORT_RECORD_MAGIC)[0]:
        fail(f"bad Run Report record magic: 0x{magic:08x}")
    if schema_version not in SUPPORTED_SCHEMA_VERSIONS:
        fail(f"unsupported Run Report schema version: {schema_version}")
    if record_size != RECORD_SIZE:
        fail(f"record size mismatch: record says {record_size}, exporter expects {RECORD_SIZE}")

    # crc32 is defined as "Crc32B over everything after the crc32 field" -
    # i.e. starting at reportId, a fixed, known offset.
    crc_start = 4 + 2 + 2 + 4  # magic + schemaVersion + recordSize + crc32
    computed_crc = crc32b(raw[crc_start:])
    if computed_crc != crc32_stored:
        fail(f"CRC32 mismatch: stored 0x{crc32_stored:08x}, computed 0x{computed_crc:08x} (corrupt record)")

    settings_raw = raw[offset : offset + RUN_REPORT_MAX_SETTINGS]
    offset += RUN_REPORT_MAX_SETTINGS
    stats_raw = raw[offset : offset + STATS_STRUCT.size]
    offset += STATS_STRUCT.size
    wipe_raw = raw[offset : offset + WIPE_STRUCT.size]
    offset += WIPE_STRUCT.size
    party_raw = raw[offset : offset + MON_STRUCT.size * PARTY_SIZE]
    offset += MON_STRUCT.size * PARTY_SIZE
    total_deaths, death_records, _pad0 = RECORD_TAIL_STRUCT.unpack_from(raw, offset)
    offset += RECORD_TAIL_STRUCT.size
    deaths_raw = raw[offset : offset + DEATH_STRUCT.size * RUN_REPORT_MAX_DEATHS]

    (
        encounters, shiny_encounters, dupes_rerolled, balls_thrown, catch_failures, shiny_catches,
        encounters_killed, encounters_fled, encounters_ran_from, bosses_defeated, level_to_cap_uses,
        total_battles, wild_battles, trainer_battles, captures, evolutions, fishing_encounters,
        locations_caught, locations_used,
    ) = STATS_STRUCT.unpack(stats_raw)

    (
        wipe_map_sec, wipe_map_group, wipe_map_num, wipe_opponent_trainer_id, wipe_opponent_species,
        wipe_opponent_kind, wipe_battle_outcome, wipe_last_boss_trainer_id,
    ) = WIPE_STRUCT.unpack(wipe_raw)

    result_name = RUN_RESULT_NAMES.get(result, "unknown")

    party = [
        mon
        for i in range(PARTY_SIZE)
        if (mon := unpack_mon(party_raw[i * MON_STRUCT.size : (i + 1) * MON_STRUCT.size], names)) is not None
    ]
    deaths = [
        unpack_death(deaths_raw[i * DEATH_STRUCT.size : (i + 1) * DEATH_STRUCT.size], names)
        for i in range(min(death_records, RUN_REPORT_MAX_DEATHS))
    ]

    gen_list = [n for n in range(1, 10) if gen_mask & (1 << (n - 1))]
    settings = [
        {**names.named(names.settings, i), "value": settings_raw[i]}
        for i in range(min(settings_count, RUN_REPORT_MAX_SETTINGS))
    ]

    hours = (play_time_packed >> 16) & 0xFFFF
    minutes = (play_time_packed >> 8) & 0xFF
    seconds = play_time_packed & 0xFF

    badge_names = [f"badge_{i + 1}" for i in range(8) if badge_mask & (1 << i)]

    doc: dict[str, Any] = {
        "schema": {"run_report_schema": schema_version, "exporter": "export_run.py 1.0"},
        "game": {
            "project_version": f"{(project_version >> 12) & 0xF}.{(project_version >> 6) & 0x3F}.{project_version & 0x3F}",
            "ruleset_version": ruleset_version,
            "randomizer_version": randomizer_version,
            "game": "EMERALD",
        },
        "run": {
            "report_id": f"0x{report_id:08x}",
            "seed": f"0x{run_seed:08x}",
            "seed_decimal": run_seed,
            "new_run_counter": new_run_counter,
            "preset": {"id": preset},
            "generations_enabled": gen_list,
            "locked_settings": settings,
            "player": {
                "name": names.text(player_name_raw),
                "gender": {0: "male", 1: "female"}.get(player_gender, "unknown"),
            },
        },
        "outcome": {
            "result": result_name,
            "run_lost": bool(flags & RUN_REPORT_FLAG_RUN_LOST),
            "play_time": {"hours": hours, "minutes": minutes, "seconds": seconds, "text": f"{hours}:{minutes:02d}:{seconds:02d}"},
        },
        "progression": {
            "badges": badge_count,
            "badge_list": badge_names,
            "progression_step": progression_step,
            "final_level_cap": final_cap,
            "champion": progression_step > badge_count,
        },
        "statistics": {
            "encounters": encounters,
            "shiny_encounters": shiny_encounters,
            "dupes_rerolled": dupes_rerolled,
            "balls_thrown": balls_thrown,
            "catch_failures": catch_failures,
            "shiny_catches": shiny_catches,
            "encounters_killed": encounters_killed,
            "encounters_fled": encounters_fled,
            "encounters_ran_from": encounters_ran_from,
            "bosses_defeated": bosses_defeated,
            "level_to_cap_uses": level_to_cap_uses,
            "total_battles": total_battles,
            "wild_battles": wild_battles,
            "trainer_battles": trainer_battles,
            "captures": captures,
            "evolutions": evolutions,
            "fishing_encounters": fishing_encounters,
            "locations_caught": locations_caught,
            "locations_used": locations_used,
        },
        "hall_of_fame_team": party if result == 1 else None,
        "final_party": party if result == 2 else None,
        "death_history": {
            "total": total_deaths,
            "recorded": len(deaths),
            "truncated": bool(flags & RUN_REPORT_FLAG_DEATHS_TRUNCATED),
            "entries": deaths,
        },
        "wipe_context": (
            {
                "map_section": names.named(names.mapsecs, wipe_map_sec),
                "map_group": wipe_map_group,
                "map_num": wipe_map_num,
                "opponent_kind": WIPE_OPPONENT_KIND_NAMES.get(wipe_opponent_kind, "unknown"),
                "opponent_trainer_id": wipe_opponent_trainer_id or None,
                "opponent_species": names.named(names.species, wipe_opponent_species) if wipe_opponent_species else None,
                "battle_outcome": BATTLE_OUTCOME_NAMES.get(wipe_battle_outcome, "none"),
                "last_boss_trainer_id": wipe_last_boss_trainer_id or None,
            }
            if result == 2
            else None
        ),
    }
    return doc


def crc32b(data: bytes) -> int:
    # Mirrors src/random.c Crc32B() exactly: a bit-reversed CRC-32 variant,
    # NOT the same table as zlib.crc32. Implemented directly to match.
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            mask = -(crc & 1) & 0xFFFFFFFF
            crc = (crc >> 1) ^ (0xEDB88320 & mask)
    return (~crc) & 0xFFFFFFFF


def export_reports(sav_path: Path, out_dir: Path, repo: Path | None, slot_filter: int | None, export_all: bool, force: bool) -> list[Path]:
    sav = sav_path.read_bytes()
    if len(sav) < FLASH_SIZE:
        fail(f"save file is too short to be a 128 KiB GBA flash save ({len(sav)} bytes)")

    bank = unpack_bank(sav)
    names = NameTables(repo)

    if export_all:
        slots = list(range(bank["slots_used"]))
    elif slot_filter is not None:
        slots = [slot_filter]
    else:
        slots = [bank["newest_slot"]]

    written = []
    for slot in slots:
        raw = bank["records_raw"][slot]
        try:
            doc = unpack_record(raw, names)
        except ValueError as error:
            if export_all:
                print(f"export_run.py: warning: slot {slot}: {error}", file=sys.stderr)
                continue
            raise
        seed_hex = doc["run"]["seed"][2:]
        result = doc["outcome"]["result"]
        filename = f"run_{seed_hex}_{result}.json"
        path = out_dir / filename
        if path.exists() and not force:
            fail(f"{path} already exists (pass --force to write a non-overwriting alternate name)")
        if path.exists() and force:
            path = out_dir / f"run_{seed_hex}_{result}.{doc['run']['report_id'][2:]}.json"
        out_dir.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
        written.append(path)
    return written


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("save", type=Path, help="ordinary .sav file (never modified)")
    parser.add_argument("-o", "--out-dir", type=Path, default=Path("."), help="directory to write JSON into (default: cwd)")
    parser.add_argument("--slot", type=int, choices=range(RUN_REPORT_BANK_SLOTS), help="export a specific bank slot instead of the newest")
    parser.add_argument("--all", action="store_true", help="export every valid slot in the bank")
    parser.add_argument("--force", action="store_true", help="write a report-ID-suffixed file instead of refusing an existing one")
    parser.add_argument("--json-only", action="store_true", help="skip repository name-expansion (raw IDs only); use outside a checkout")
    parser.add_argument("--repo", type=Path, default=None, help="repository root for name expansion (default: parent of tools/)")
    args = parser.parse_args()

    repo = None
    if not args.json_only:
        repo = args.repo if args.repo is not None else Path(__file__).resolve().parent.parent
        if not (repo / "include" / "run_report.h").is_file():
            print(f"export_run.py: warning: {repo} does not look like the project checkout - names disabled", file=sys.stderr)
            repo = None

    try:
        before = args.save.stat()
        written = export_reports(args.save, args.out_dir, repo, args.slot, args.all, args.force)
        after = args.save.stat()
        if before.st_mtime != after.st_mtime or before.st_size != after.st_size:
            fail("internal error: the source save file changed during export - aborting")
    except (OSError, ValueError, struct.error) as error:
        print(f"export_run.py: error: {error}", file=sys.stderr)
        return 1

    for path in written:
        print(f"wrote {path}")
    if not written:
        print("export_run.py: nothing exported", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
