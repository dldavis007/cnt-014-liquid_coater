# Rev4.33 host-side unit tests (Unity)

Host-compiled (TDM-GCC/MinGW-w64) tests that run the **real** Rev4.33 firmware
sources — nothing is copied, stubbed out or shadowed. `-DPC_SIDE` selects the
host path inside the real headers; `stubs/hardware_stubs.c` supplies only the
hardware layer underneath.

## Running

```
mingw32-make summary     # build + run everything, aggregate report
mingw32-make all         # build + run every suite
mingw32-make check       # gated: first failing suite fails the make
mingw32-make test_purge_retract    # one suite
mingw32-make clean
```

Or in VS Code: **Test: All Tests**.

## Suites

| Suite | Covers |
|---|---|
| `test_smoke` | Harness wiring: SFR array, RTI sim, Timer1 pacing service, CAN seams |
| `test_purge_retract` | `PurgeRetract` / `PurgeRetractWait` / `PurgeRetractWaitErrorState` |
| `test_coatseq_states` | Switch dispatch for every coating state and branch |
| `test_coatseq_errors` | Every route *into* an error state, and each error state's body |
| `test_cameras` | `CameraMain1/2`, the two-wire camera protocol, trigger arming |

## What is and isn't compiled

Compiled (the real thing): `Subroutines.c`, `Subroutines1.c`, `mco.c`, `user.c`,
`Interrupts.c`, `MenuSerialize.c`, `Packets.c`.

Not compiled, stubbed in `hardware_stubs.c` instead:

| File | Why |
|---|---|
| `Controller.c` | Holds `main()` and the hardware bring-up; `host_firmware_init()` mirrors its init order |
| `EEProm.c`, `Flash.c` | Absolute-address access + spin on command-complete bits |
| `mcohw.c` | Spins on CAN transmit-buffer status bits that never set |
| `Command.c` | Serial console, reached only from the SCI ISRs |

## Two things that would otherwise hang the suites

**1. MCO time base.** `MCOHW_GetTime()` returns 0 and `MCOHW_IsTimeExpired()`
returns 1, so anything pacing off it falls straight through.

**2. `Timer1` busy-waits — the Rev4.33-specific one.** Rev4.33 paces its
outgoing CAN display frames with the production idiom

```c
Timer1 = RTI_One_Sec * .05;  while ( Timer1 );
```

in `Display()`, `PositionDisplay()` and `Packets.c`'s `sendPackets()`. `Timer1`
is decremented **only** by `RTI_Int_Handler()`, so on the host those spins never
end and the first `Display()` in the coating sequence hangs the suite. (Rev4.34
paces off `MCOHW_GetTime` instead, so it never had this problem.)

We do **not** `#ifdef` those waits out of the firmware — the pacing code should
stay on the host path rather than compiling a different program than the one
that ships. Instead a background **pacing service** (`pacing_thread_start()`,
started automatically by `host_firmware_init()`) services `Timer1` and nothing
else. That keeps `StateTime` and every other firmware timer under deterministic
`advance_ticks()` control — using the full RTI ISR here would advance
`StateTime` by ~100 ticks per `Display()` call and make every timeout assertion
racy. `test_smoke` asserts exactly this separation.

## The revision-agnostic seam

Suites never name a revision's own signal variables. They go through
`stubs/test_support.h`:

| Accessor | Rev4.33 | Rev4.34 |
|---|---|---|
| `set_actuator_moving()` | `gProcImg[OUT_digi_7]` | `*rpdo4_actuator_moving` |
| `set_purge_moving()` | `gProcImg[IN_digi_31]` | `*rpdo7_purge_moving` |
| `la_commanded_pos()` | `gProcImg[IN_digi_12]` | `tpdo3_actuator_1[0]` |
| `menu_data` | `&gProcImg[OUT_digi_0]` | `rpdo1_menu_data` |
| `camera_addr` | `&gProcImg[OUT_digi_8]` | `rpdo5_camera_addr` |
| `camera_cmds` | `&gProcImg[OUT_digi_10]` | `rpdo6_camera_cmds` |

That seam is why porting these suites from Rev4.34 was a reimplementation of one
block in `hardware_stubs.c` rather than a rewrite of every test.

## Known build noise (not a problem)

Every suite compile prints two warnings:

```
../SourceFiles/Subroutines.h:247:1: warning: useless storage class specifier in empty declaration
../SourceFiles/Subroutines.h:256:1: warning: useless storage class specifier in empty declaration
```

These are **pre-existing firmware, not caused by the test harness**, and they are
harmless. `Subroutines.h` writes

```c
typedef struct MenuStruct { ... };     /* no declarator after the brace */
typedef struct MenuStack  { ... };
```

A `typedef` with no name after the closing brace declares no alias, so the
`typedef` keyword is dead — `MenuStruct` and `MenuStack` never become type
names. Nothing depends on them being types: every use in the tree already spells
them `struct MenuStruct` / `struct MenuStack` (the only bare `MenuStruct` is
inside a comment in `MenuSerialize.h`).

They are visible here and not in the production build only because test TUs
compile with `-Wall` while the production TUs use `-w`. Deliberately left
unfixed: it is a shipping firmware header, the warning is cosmetic, and there is
no dedicated `-Wno-` flag for it. Do not "fix" this by accident during a port.

## Compiler flags that are load-bearing

- `-funsigned-char` — matches the ICC12 default char signedness. Without it any
  `if (x == 0xFF)` on a raw byte silently never matches.
- `-malign-data=abi`, and **no** `-fdata-sections` — the menu code reaches
  adjacent globals by pointer arithmetic off the end of an array. That only
  holds while GCC lays the arrays out contiguously in declaration order, the way
  ICC12 does. `-fdata-sections` lets the linker reorder them; GCC's default
  alignment pads between them.
- `-Ddiag` — pins `HeadSpeed` to 8000 in `RTI_Int_Handler`. The head tachometer
  is a timer input-capture on the target, so on the host `HeadSpeed` stays 0,
  `CkHeadRotation` always times out into `HeadErrorState`, and the sequence
  could never get past step 2.
