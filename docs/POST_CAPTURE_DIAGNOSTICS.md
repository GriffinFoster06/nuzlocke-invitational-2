# Post-capture forced-down diagnostics

## Status

**ROOT CAUSE NOT PROVEN.** No gameplay or input-reset fix is authorized until
debugging observes the first corrupting transition or write.

The intermittent symptom follows a successful capture: after the battle,
Pokédex/nickname and party-or-storage handling return to the field, then the
player can walk Down continuously as if it were held. Normal direction input
does not recover control; menus or saving can also be unavailable. It has been
observed after a grass encounter and must not be attributed to fishing without
evidence.

## Existing evidence

- `pokeemerald.ss1` has SHA-256
  `6c99050720d72ece3ca57dce3f179e240a39e315afc4ca05378078a2bcd1ad84` and
  embedded ROM CRC32 `0x5d0b39f8`, matching `test-0907-1634.gba`.
- `pokeemerald.ss2` has SHA-256
  `0eed69c7d4a595c7c20bd23ae2ee670aeb1e8e291d5e7f4d451e07df6a577ce8` and
  embedded ROM CRC32 `0xfd4c0fe9`, matching `checkpoint-after-phase9.gba`.
  It is the stronger stuck-state candidate.
- In `ss2`, `KEYINPUT` is `0x03fd`: B is physically pressed and Down is not.
  The saved PC is `0x08179ee6`, in the matching optimized `ReadKeys` loop.
  `r4=0x030066e0` is matching `gMain`, while `r6=0x03006698` is not
  `REG_KEYINPUT`, `r7=1` is not `KEYS_MASK=0x3ff`, `r8=0x02001004` points into
  battle-global storage, and `r9=0x080dbadd` is inside matching
  `FreeBattleResources`.
- With matching `gMain`, `heldKeysRaw=heldKeys=0x72bd`, which includes
  `DPAD_DOWN`; `newKeysRaw`, `newKeys`, and `newAndRepeatedKeys` are zero.

This proves the immediate mechanism: corrupted persistent registers in the
optimized input loop repeatedly write a false held-key value containing Down.
It does not identify the capture transition that corrupted those registers or
the stack.

## Read-only state inspector

`tools/debug/inspect_capture_state.py` validates PNG chunk CRCs, the `gbAs`
payload version (`0x01000007`), decompressed length (`0x61000`), and the
embedded ROM CRC32 against the ROM supplied with `--rom`. It never writes a
state. It reads CPU GPRs from serialized offsets `0x20..0x5c`, CPSR at `0x60`,
and `KEYINPUT` at `0x530`. It only reads `gMain` when a matching address is
explicitly supplied; for this build, `gMain=0x030066e0` maps to state offset
`0x1f6e0`, and its key fields use the offsets in `include/main.h`.

Run the two historical states only with their matching ROMs:

```sh
python3 tools/debug/inspect_capture_state.py pokeemerald.ss1 --rom test-0907-1634.gba --gmain-address 0x030066e0
python3 tools/debug/inspect_capture_state.py pokeemerald.ss2 --rom checkpoint-after-phase9.gba --gmain-address 0x030066e0
```

The tool must reject `pokeemerald.gba` for either historical state. Preserve
the state hashes above; do not open them with a mismatched ROM or apply current
symbols to them.

## Reproduction and GDB procedure

Build with representative optimization and symbols, then run mGBA's GDB stub:

```sh
make clean
make DINFO=1 -j$(sysctl -n hw.ncpu)
/Applications/mGBA.app/Contents/MacOS/mGBA -g pokeemerald.gba
/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb pokeemerald.elf
```

In GDB, use `target remote :2345`, resolve every breakpoint from this exact
ELF with `info address`, and record registers plus the requested memory at:

1. wild battle start;
2. every successful-capture branch, including Pokédex, nickname, party/PC, and
   full-party swap/cancel returns;
3. immediately before and after `FreeBattleResources` and the battle callback
   handoff;
4. `CB2_ReturnToField`, field initialization, the first overworld frame, and
   the first bad movement frame.

At each point record `r0`–`r12`, `sp`, `lr`, `pc`, `cpsr`; stack and saved
register slots; `KEYINPUT`; all `gMain` key fields; callbacks, battle state,
tasks, script contexts, field callbacks, avatar flags/transition/preventStep,
object movement/facing/coordinates, and relevant encounter/randomizer/Nuzlocke
state. Once a new bad state has intact symbols, use watchpoints on the matching
`gMain` key fields and the first identified damaged stack or saved-register
slot to find the earliest write. Do not use a broad key clear: it would mask
the evidence.

## Capture matrix

Reproduce across grass, Old/Good/Super Rod, and static/scripted captures; free
party slot, direct PC, swap accept, swap decline/cancel, and storage rollover;
nickname accepted, declined with B, mandatory, and cancellation; new and owned
Pokédex entries; Nuzlocke/randomizer off and on; and catch EXP/evolution paths.
After each capture test standing, directions, Start, save, interaction, map
transition, and 60 seconds of control. Also retain non-capture flee and KO
controls. On reproduction, save and hash both the pre-capture and stuck states,
then inspect them with this tool and the matching ROM.

## Candidate classes

The current candidates are ABI/saved-register or stack corruption across
capture cleanup; capture-only screen/party/storage return paths; lifetime or
bounds errors in battle resources/tasks/naming/copies/Nuzlocke/randomized
species; and map/avatar/fishing state only if a new occurrence has intact input
loop registers. A permanent correction requires an approved plan amendment
naming the exact transition, file/function, minimal fix, and risks.
