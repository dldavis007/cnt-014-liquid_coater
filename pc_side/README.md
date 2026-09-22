# Rev4.33 PC-side host

Runs the **real** Rev4.33 firmware main loop (`doevents()`) as a Windows
executable, with live logging and a UDP CAN bus — so the logic can be driven and
observed without the HCS12 or NOICE. Built by GCC with `-DPC_SIDE`; ImageCraft
never sees `main.c` or `pc_side_host.c`.

## Running

```
mingw32-make run          # build + run (Ctrl+C to quit)
mingw32-make              # build only
mingw32-make clean
```

Or in VS Code: **PC-Side: Build & Run**, or **PC-SIDE: Tests, Build & Run** to
gate the host build on a clean test run first.

Bring the bus up first, in another terminal:

```
python ../../../../CAN_RECEIVER_CODE/can_udp_hub.py
```

then any emulators you want on it, from `emulators/`. Defaults: this host binds
`:20010` and sends to the hub on `:20100`. Pass them as args to override
(`pc_side_host.exe 20010 20100`), otherwise it prompts.

## How it maps onto the target

`main.c` mirrors `SourceFiles/Controller.c`'s `main()` — `InitPorts()`,
`InitInterrupts()`, `EEInit()`, `INTR_ON()`, `InitCANOpen()`, then the
`while(1) { doevents(); }` loop. Rev4.33's `doevents()` has no internal loop, so
the loop lives in the host's `main()` exactly as on the target.

Skipped: `InitPLL()`, `PWMInit()`, `AtoDInit()` (spin on status bits that never
change on a PC) and the EEPROM loads (`-DSKIP_EEPROM_LOAD`; they use absolute
addresses Windows cannot map), so the unit comes up on its compiled-in defaults.

## The RTI simulation thread

`pc_side_host.c` runs a thread that drives the production `RTI_Int_Handler()`,
deriving the tick count from `QueryPerformanceCounter` rather than from wakeup
count — a 1 ms `Sleep` does not take 1 ms on Windows, and a one-tick-per-wakeup
thread would run every firmware timeout long by the shortfall.

On Rev4.33 this thread is **load-bearing, not a convenience**: `Display()`,
`PositionDisplay()` and `sendPackets()` pace their CAN frames with
`Timer1 = <n>; while (Timer1);`, and `Timer1` moves only under the RTI ISR.
Without it the first `Display()` call spins forever. The console title bar
reports the achieved rate; if it drops below `1.00x`, everything timed is
running long by `1/rate`.

Ticks are skipped while `g_intr_masked` is set, so the firmware's existing
`INTR_OFF()`/`INTR_ON()` critical sections mean something here too.

## Two gates the UDP bus cannot satisfy

- **`VSEL_PORT & CAM_ON`** is a GPIO on the target, so nothing on the bus can set
  it — and `doevents()` silently discards the start trigger without it. The line
  that asserts it in `main.c` is left commented; uncomment to force it.
- **`HeadSpeed`** comes from a tachometer input-capture. `-Ddiag` pins it to 8000
  in `RTI_Int_Handler`, otherwise `CkHeadRotation` always times out into
  `HeadErrorState`.

## ICC12 vs GCC: the `char` comparison trap

**Read this before adding a `-f*-char` flag or "fixing" a `== -1` on a `char`.**

ICC12 compiles a `char` comparison as an **8-bit compare with the constant
truncated**. Both of these emit the identical instruction `C1 FF`:

```
c == -1     ->  ldab _c ; cmpb #65535     (truncates to 0xFF)
c == 0xFF   ->  ldab _c ; cmpb #255
```

So on the target, a `char` holding `0xFF` matches `== -1` **and** `== 0xFF`.
GCC follows the C standard instead: it promotes the `char` to `int` first, so it
can only satisfy one of those at a time, and which one depends on the signedness
flag:

| | `flag = -1` stores | `flag == -1` | `flag == 0xFF` |
|---|---|---|---|
| ICC12 (target) | `0xFF` | true | true |
| GCC `-funsigned-char` | `255` | **false** | true |
| GCC `-fsigned-char` | `-1` | true | **false** |

We build with `-funsigned-char`. Rev4.33 relies on *both* idioms, so neither GCC
setting is faithful — this is a property of the host build, not a firmware bug.

This bit us once, for real. The menu cursor flags (`CursorUpFlag`,
`CursorDownFlag`, `SelectFlag` in `Subroutines1.c`) are tri-state: `0` idle,
`1` fresh press, `-1` handled-and-auto-repeating. Release is detected by
`flag == -1`, which under `-funsigned-char` compared `255 == -1` and was dead —
so the flag never returned to `0`, stayed truthy, and the cursor scrolled the
whole menu at the `MenuTime` rate (~3 Hz) forever after one button press.

The first fix was `== (char)-1`, which puts the literal through the same
conversion the assignment used. Verified with the real ICC12 at `C:\iccv712`,
compiling the real `Subroutines1.c` with the project's own CFLAGS: the object
file was **byte-identical** with and without the casts.

The three flags are now declared `signed char` instead, and the comparisons are
plain `== -1`. That fixes the cause rather than each comparison site: `signed
char` is signed on every compiler, so nothing depends on ICC12's 8-bit compare
or on GCC's `-funsigned-char`. Prefer either form over changing the compiler
flag.

`bb_probe.py` is the headless regression probe for it — it drives a single Up
pulse over UDP with no GUI and reports whether the cursor settles after release:

```
mingw32-make && .\pc_side_host.exe 20910 20900     # one terminal
python bb_probe.py                                  # another
```

It uses ports 20910/20900 so it never disturbs a running `can_udp_hub.py` bus.

## NULL pointers: benign on the HCS12, fatal here

Address 0 on the target is the **SFR block** (`PORTA` is at 0x0000). It is
ordinary readable memory with no MMU, so dereferencing a NULL pointer does not
trap — it just reads registers. On Windows page 0 is unmapped, so the identical
code takes a hard SIGSEGV. Any latent NULL deref in this firmware is therefore
invisible on hardware and instantly fatal on the host.

One such case is live and is shimmed in `main.c`:

`NullVar` is an uninitialised global (BSS), so `NullVar.str_enum == NULL`. A menu
row that sets a variable column in `Pos[]` but points `VarPntr[]` at that
sentinel reaches `getstrval()`, which does `strlen(var->str_enum)`. On the target
that reads SFR space and, because `len_str == 0`, the eventual `strncpy` copies
nothing and the row renders blank. Here it killed the host the moment you opened
the menu.

`main.c` sets `NullVar.str_enum = ""` before bring-up, which reproduces that
blank-row outcome exactly (`strlen() == 0`, enum branch skipped) with **no
firmware change**. `tests/stubs/hardware_stubs.c` does the same.

### Pre-existing menu-table bug this exposed (NOT fixed)

The CAMERAS menu (`Menuc` index 8, `Index {0,0,0,5}`) is inconsistent:

```
" CAMERA 1", " CAMERA 2", " EXIT"
Pos      = 15, 15, 0, ...            <- rows 1-2 declare a variable at column 15
VarPntr  = &NullVar, &NullVar, ...   <- but point at the sentinel
FunctPtr = &NullFunction, &NullFunction, &ExitMenu
```

So both camera rows declare a variable that does not exist *and* do nothing when
selected. Compare the STATUS MENU, which does the same job correctly with
`16,16,16,14` and `&Rev, &disp_add1, &disp_add2, &SerialNum`.

This looks unfinished rather than broken-by-regression: on hardware the rows
simply render blank. Fixing it is a product decision — either point those slots
at `disp_add1`/`disp_add2` (as the STATUS MENU does) or drop `Pos` to `0,0` —
so it has been left exactly as found.

## Diagnostics

The console **title bar** carries `loop=`, the coating state, and the RTI rate.
It is written with `SetConsoleTitleA` (kernel32, not stdout), so it keeps
updating even when the terminal has blocked our output — which is what separates
"terminal stuck" from "`doevents()` genuinely hung".

Logging is async and drop-on-full: the firmware and CAN threads only format and
enqueue, never block on a slow terminal. Windows QuickEdit is disabled at
startup for the same reason — one stray click in the window would otherwise
freeze the host silently.
