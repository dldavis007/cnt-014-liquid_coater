# Production Rev 4.33 PC-side host

Runs the **real** production Rev 4.33 firmware loop (`doevents()`) as a Windows
executable, with live logging and a UDP CAN bus. Built by GCC with
`-DPC_SIDE`; ImageCraft never sees anything in this folder.

## Layout

- `core/`: the host shared by every HCS12 + MicroCANopen unit. It covers the
  UDP CAN bus and `MCOHW_*` layer, the RTI thread, the EEPROM image, the stall
  detector and reset relaunch, the `--hw-*` options and the shared `core.mk`
  build. Change it there, not per unit. The API is in `core/pc_core.h`.
- `main.c`: this unit's part. It holds the `pc_side_unit` settings (ports, RTI
  handler, `COPCTL`/`CANRFLG`, transmit timeout), the init order mirroring
  `Controller.c`, test
  seeds and logging.
- `Makefile`: this unit's firmware sources and `-D` flags, then
  `include core/core.mk`.

`core/` will become a git submodule. Clone with
`git clone --recurse-submodules <url>`. If `core/` comes up empty, run
`git submodule update --init`.

## Running

```
mingw32-make run          # build + run (Ctrl+C to quit)
mingw32-make              # build only
mingw32-make clean
```

Or in VS Code: **PC_SIDE: Build & Run**, or **PC_SIDE: Tests, Build & Run** to
start the host only after every test suite passes.

The hub and emulators live in `C:\Working_Projects\can_emulators`
(`python can_hub_gui.py`, then e.g. `coater_la_purge_emulator.py`). Defaults:
this host binds `:20010` and sends to the hub on `:20100`. Pass both as args to
skip the prompts (`pc_side_host.exe 20010 20100`).

## How it maps onto the target

`main.c` mirrors `Controller.c`'s `main()`: `InitPorts()`, `InitInterrupts()`,
`EEInit()`, `INTR_ON()`, the EEPROM loads, `InitCANOpen()`, then the
`while(1) { doevents(); }` loop. `doevents()` has no internal loop in 4.33, so
the loop lives in the host's `main()`, exactly as on the target.

Skipped: `InitPLL()`, `PWMInit()` and `AtoDInit()`, which spin on status bits.

## EEPROM

`-DPC_EEPROM` points `EE_begin` at a 2 KB host array, saved to `eeprom.bin` in
the working directory on every `EEWrite`. The firmware's own `Load_Variables`,
`Save_Variables` and `RestoreDefaults` run unchanged. With no file, the array
starts erased (`0xFF`), so the first boot saves the compiled-in defaults.
Delete `eeprom.bin` to start fresh.

Compiled: `Subroutines.c`, `Subroutines1.c`, `mco.c`, `user.c`, `Interrupts.c`,
all from the project root.

## The RTI simulation thread

`core/host_rti.c` drives the production `RTI_Int_Handler()` at `RTI_One_Sec`,
counting ticks from `QueryPerformanceCounter`. It is **load-bearing**:
`Display()` and `PositionDisplay()` pace their CAN frames with
`Timer1 = n; while (Timer1);`, and `Timer1` only moves under the RTI ISR. Ticks
are deferred while `g_intr_masked` is set.

## Stall detector and reset

The console **title bar** carries `loop=`, the coating state and the RTI rate.
`[STALL]` is logged if `doevents()` stops returning. A `ResetProc` spin
(`COPCTL = 0x01`, for example after an NMT reset-node 0x81) relaunches the host
on the same ports.

## Two gates the UDP bus can't satisfy

- **`VSEL_PORT & CAM_ON`** is a GPIO, and `doevents()` discards the start
  trigger without it. The line that asserts it in `main.c` is left commented.
- **`HeadSpeed`** comes from a tachometer input-capture. `-Ddiag` uses the
  firmware's existing `#ifdef diag` to pin it to 8000, or `CkHeadRotation` always
  times out.

## ICC12 vs GCC: the `char` comparison trap

ICC12 compares a `char` as 8 bits with the constant truncated, so a flag holding
0xFF matches `== -1` on the target. Under GCC with `-funsigned-char` it promotes
to 255, and `255 == -1` is false. The menu cursor flags (`CursorUpFlag`,
`CursorDownFlag`, `SelectFlag`) are tri-state with release detected by
`flag == -1`, so on the host one press left the cursor free-running.

Production 4.33 keeps the target line **byte-for-byte**. The host gets the
cast through a guard in `Subroutines1.c`:

```c
#ifdef PC_SIDE   // host: plain char is unsigned under GCC; the cast matches the -1 store
            else if ( CursorDownFlag == (char)-1 && ... )
#else
            else if ( CursorDownFlag == -1 && ... )
#endif
```

Proof that the target is unchanged: the edited sources rebuild with the
original `HEAD.mak` flags to a `HEAD.s19` **byte-identical to the shipped
`HEAD.s19`**. The pristine sources reproduce it too, which shows this folder is
the code that shipped.

## NULL pointers: benign on the HCS12, fatal here

Address 0 on the target is SFR space, so a NULL dereference reads registers and
carries on. On Windows it's a SIGSEGV. `NullVar` is an uninitialised global, so
`NullVar.str_enum == NULL`, and menu rows pointing at it reach `strlen(NULL)`.
`main.c` (and `hardware_stubs.c`) set `NullVar.str_enum = ""`, which reproduces
the target's blank-row outcome with no firmware change.
