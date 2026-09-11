#!/usr/bin/env python3
"""Read, validate, and report the diagnostic fields of an mGBA GBA savestate.

The program is deliberately read-only: it parses PNG chunks and the compressed
``gbAs`` payload in memory and never writes a state, ROM, or sidecar file.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
import zlib


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
GBA_STATE_SIZE = 0x61000
GBA_STATE_VERSION_MAGIC = 0x01000007
GBA_STATE_ROM_CRC_OFFSET = 0x08
GBA_STATE_GPRS_OFFSET = 0x20
GBA_STATE_CPSR_OFFSET = 0x60
GBA_STATE_IO_OFFSET = 0x400
GBA_STATE_IWRAM_OFFSET = 0x19000
REG_KEYINPUT_OFFSET = 0x130
IWRAM_START = 0x03000000
IWRAM_SIZE = 0x8000


def fail(message: str) -> "None":
    raise ValueError(message)


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def find_gbas_payload(png: bytes) -> bytes:
    if not png.startswith(PNG_SIGNATURE):
        fail("not a PNG mGBA savestate")

    offset = len(PNG_SIGNATURE)
    payload = None
    while offset < len(png):
        if offset + 12 > len(png):
            fail(f"truncated PNG chunk header at 0x{offset:x}")
        length = struct.unpack_from(">I", png, offset)[0]
        kind = png[offset + 4:offset + 8]
        data_start = offset + 8
        data_end = data_start + length
        if data_end + 4 > len(png):
            fail(f"truncated {kind.decode('ascii', 'replace')} chunk at 0x{offset:x}")
        chunk = png[data_start:data_end]
        expected_crc = struct.unpack_from(">I", png, data_end)[0]
        actual_crc = zlib.crc32(kind)
        actual_crc = zlib.crc32(chunk, actual_crc) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            fail(f"bad PNG CRC for {kind.decode('ascii', 'replace')} at 0x{offset:x}")
        if kind == b"gbAs":
            if payload is not None:
                fail("multiple gbAs chunks")
            payload = chunk
        offset = data_end + 4

    if offset != len(png):
        fail("trailing partial PNG data")
    if payload is None:
        fail("no gbAs chunk")
    try:
        return zlib.decompress(payload)
    except zlib.error as error:
        fail(f"cannot decompress gbAs payload: {error}")


def serialized_iwram_offset(address: int, size: int) -> int:
    if not (IWRAM_START <= address and address + size <= IWRAM_START + IWRAM_SIZE):
        fail(f"gMain address 0x{address:08x} is outside IWRAM")
    return GBA_STATE_IWRAM_OFFSET + address - IWRAM_START


def report_gmain(state: bytes, address: int) -> None:
    # These offsets come directly from include/main.h. Do not report them unless
    # the caller supplied a matching build's gMain address.
    base = serialized_iwram_offset(address, 0x43A)
    callback1, callback2, saved_callback = (read_u32(state, base + offset) for offset in (0, 4, 8))
    held_raw, new_raw, held, new, repeated, repeat_counter, watched, watched_mask = (
        read_u16(state, base + offset) for offset in (0x28, 0x2A, 0x2C, 0x2E, 0x30, 0x32, 0x34, 0x36)
    )
    flags = state[base + 0x439]
    print(f"gMain address: 0x{address:08x} (serialized offset 0x{base:x})")
    print(f"gMain callbacks: callback1=0x{callback1:08x} callback2=0x{callback2:08x} saved=0x{saved_callback:08x}")
    print(
        "gMain keys: "
        f"heldKeysRaw=0x{held_raw:04x} newKeysRaw=0x{new_raw:04x} "
        f"heldKeys=0x{held:04x} newKeys=0x{new:04x} "
        f"newAndRepeatedKeys=0x{repeated:04x} keyRepeatCounter=0x{repeat_counter:04x} "
        f"watchedKeysPressed=0x{watched:04x} watchedKeysMask=0x{watched_mask:04x}"
    )
    print(f"gMain state: state={state[base + 0x438]} inBattle={(flags >> 1) & 1} flags=0x{flags:02x}")


def inspect(state_path: Path, rom_path: Path, gmain_address: int | None) -> None:
    png = state_path.read_bytes()
    state = find_gbas_payload(png)
    if len(state) != GBA_STATE_SIZE:
        fail(f"unexpected decompressed gbAs length: 0x{len(state):x}, expected 0x{GBA_STATE_SIZE:x}")
    if read_u32(state, 0) != GBA_STATE_VERSION_MAGIC:
        fail(f"unsupported GBA state version magic: 0x{read_u32(state, 0):08x}")

    state_crc = read_u32(state, GBA_STATE_ROM_CRC_OFFSET)
    rom_crc = zlib.crc32(rom_path.read_bytes()) & 0xFFFFFFFF
    if state_crc != rom_crc:
        fail(
            f"ROM CRC mismatch: state=0x{state_crc:08x}, "
            f"{rom_path}=0x{rom_crc:08x}; do not use these symbols"
        )

    title = state[0x10:0x1C].rstrip(b"\0").decode("ascii", "replace")
    game_code = state[0x1C:0x20].rstrip(b"\0").decode("ascii", "replace")
    registers = [read_u32(state, GBA_STATE_GPRS_OFFSET + 4 * index) for index in range(16)]
    print(f"state: {state_path}")
    print(f"sha256: {hashlib.sha256(png).hexdigest()}")
    print(f"gbAs: version=0x{read_u32(state, 0):08x} length=0x{len(state):x} title={title!r} game_code={game_code!r}")
    print(f"ROM CRC32: 0x{state_crc:08x} (validated against {rom_path})")
    print("CPU: " + " ".join(f"r{index}=0x{value:08x}" for index, value in enumerate(registers[:13])))
    print(f"CPU: sp=0x{registers[13]:08x} lr=0x{registers[14]:08x} pc=0x{registers[15]:08x} cpsr=0x{read_u32(state, GBA_STATE_CPSR_OFFSET):08x}")
    print(f"KEYINPUT: 0x{read_u16(state, GBA_STATE_IO_OFFSET + REG_KEYINPUT_OFFSET):04x}")
    if gmain_address is None:
        print("gMain keys: not read (pass --gmain-address from the matching ELF/map)")
    else:
        report_gmain(state, gmain_address)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("state", type=Path, help="mGBA PNG savestate")
    parser.add_argument("--rom", required=True, type=Path, help="ROM that must match the state's embedded CRC32")
    parser.add_argument("--gmain-address", type=lambda value: int(value, 0), help="gMain address from the matching ELF/map")
    args = parser.parse_args()
    try:
        inspect(args.state, args.rom, args.gmain_address)
    except (OSError, ValueError, zlib.error, struct.error) as error:
        print(f"inspect_capture_state.py: error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
