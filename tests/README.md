# Production Rev 4.33 host-side unit tests (Unity)

Host-compiled (TDM-GCC / MinGW-w64) tests that run the **real** production
Rev 4.33 sources from the project root, unmodified apart from the `PC_SIDE`
guards. The hardware layer underneath and the build are shared from
`pc_side/core/test/` (`test_hw.c`, `test.mk`, Unity); this unit's fixtures are
`test_support.c/.h`.
The suites were carried over from `CRTS_Refactored`'s Rev4.33 (a later,
unreleased revision; see `../README.md`) and adapted where production behaves
differently.

```
mingw32-make summary     # build + run everything, aggregate report (VS Code: "Test: All Tests")
mingw32-make all         # build + run every suite
mingw32-make check       # gated: first failing suite fails the make
mingw32-make test_menu_cursor    # one suite
mingw32-make clean
```

## Suites

| Suite | Covers |
|---|---|
| `test_smoke` | Harness wiring: SFR array, RTI sim, Timer1 pacing service, CAN seams |
| `test_purge_retract` | `PurgeRetract` / `PurgeRetractWait`; the timeout goes to `ErrorState` |
| `test_coatseq_states` | Switch dispatch for every coating state and branch |
| `test_coatseq_errors` | Every route *into* `ErrorState` / `HeadErrorState`, and their bodies |
| `test_cameras` | `CameraMain1/2`, the two-wire camera protocol, trigger arming |
| `test_menu_cursor` | Press / hold / release of the menu cursor flags |

## Compiled vs stubbed

Compiled: `Subroutines.c`, `Subroutines1.c`, `mco.c`, `user.c`, `Interrupts.c`.
There is no `MenuSerialize.c` or `Packets.c` in production 4.33.

Not compiled:

| File | Why |
|---|---|
| `Controller.c` | Holds `main()` and the hardware bring-up; `host_firmware_init()` mirrors its init order |
| `EEProm.c`, `Flash.c` | Spin on command-complete bits; `test_hw.c` keeps the EEPROM image in RAM |
| `mcohw.c` | Spins on CAN transmit-buffer status bits that never set |
| `Command.c` | Serial console, reached only from the SCI ISRs |

## Harness notes

- **MCO time base.** Pinned to "always expired", so nothing pacing off it can
  hang a test.
- **`Timer1` busy-waits.** `Display()` and `PositionDisplay()` pace with
  `Timer1 = n; while (Timer1);`. A pacing thread zeroes `Timer1` and nothing
  else, so those waits end while `StateTime` stays under exact
  `advance_ticks()` control. `test_smoke` asserts that separation.
- **Menu setters.** `update_menu_var_by_str/_by_value` exist in the refactored
  firmware, not in production. The harness supplies them with this firmware's
  own idiom (`strncpy` + `getvalue`, or `value` + `getstrval`), so the suites
  read the same across revisions.

## Differences from the refactored 4.33 suites

Each is a real difference in behaviour, asserted as production does it:

- **Purge-retract timeout.** In production it goes to `ErrorState`
  ("Warn:TIMEOUT ERROR"). `PurgeRetractWaitErrorState` (110) doesn't exist here,
  so its tests were dropped.
- **Head / pump ON.** Production turns them on with `strncpy(" ON")` +
  `getvalue`, which works. The refactored revisions use
  `update_menu_var_by_str(&X, "ON")`, which misses the `" ON"` enum token and
  leaves them OFF; their suites pin that as a known bug. Here
  `test_trigstate_turns_head_on` and the clean-out pump test expect ON.
  **That bug is a regression introduced after this production release.**
- **Cursor flags.** Production keeps plain `char` flags. The host fix is the
  `#ifdef PC_SIDE` `== (char)-1` compare in `Subroutines1.c`. Built against the
  unguarded source, five `test_menu_cursor` tests fail.

## The revision-agnostic seam

Suites never name a revision's own signal variables; they go through
`test_support.h`:

| Accessor | Rev 4.33 | Rev4.34 |
|---|---|---|
| `set_actuator_moving()` | `gProcImg[OUT_digi_7]` | `*rpdo4_actuator_moving` |
| `set_purge_moving()` | `gProcImg[IN_digi_31]` | `*rpdo7_purge_moving` |
| `la_commanded_pos()` | `gProcImg[IN_digi_12]` | `tpdo3_actuator_1[0]` |
| `menu_data` | `&gProcImg[OUT_digi_0]` | `rpdo1_menu_data` |
| `camera_addr` | `&gProcImg[OUT_digi_8]` | `rpdo5_camera_addr` |
| `camera_cmds` | `&gProcImg[OUT_digi_10]` | `rpdo6_camera_cmds` |

## Known build noise (not a problem)

Every suite compile prints two warnings:

```
../Subroutines.h:240:1: warning: useless storage class specifier in empty declaration
../Subroutines.h:249:1: warning: useless storage class specifier in empty declaration
```

These come from the firmware, not the harness. `Subroutines.h` writes
`typedef struct MenuStruct { ... };` with no name after the brace, so the
`typedef` is dead; every use already spells `struct MenuStruct`. They're only
visible because test TUs compile with `-Wall`. Leave them: it's a shipping
header and the warning is cosmetic.

## Compiler flags that are load-bearing

- **`-funsigned-char`**: matches ICC12's default char signedness. Without it any
  `if (x == 0xFF)` on a raw byte never matches.
- **`-malign-data=abi`, and no `-fdata-sections`**: the menu code reaches
  adjacent globals by pointer arithmetic off the end of an array. That only
  holds while GCC lays the arrays out contiguously in declaration order, as
  ICC12 does.
- **`-Ddiag`**: uses the firmware's existing `#ifdef diag` to pin `HeadSpeed`
  to 8000. On the host the tachometer never reads, so `CkHeadRotation` would
  otherwise always time out.
