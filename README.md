# 12/48 Liquid Coater — production Rev 4.33

This folder is the **true production Rev 4.33**. Rebuilt with its own `HEAD.mak`
flags, the unmodified sources reproduce the shipped `HEAD.s19` byte-for-byte.

## ⚠ Naming convention — needs fixing (not renamed yet)

Two different codebases are both called "4.33":

| Location | What it is |
|---|---|
| `C:\Temp\liquid_coater_old_code_reference\Rev 4.33` (this folder) | Production 4.33, the code that shipped |
| `C:\CRTS_Refactored\CNT-014-LIQUID_COATER\12_48_Coater\Rev4.33` | A later, **unreleased** revision, not yet greenlit for production |

Why this needs fixing:

- **Same identity.** Both define `Revision "4.33"` and carry the same revision
  history, so a unit running the refactored build reports itself exactly like
  production. None of the refactored changes are recorded in its history.
- **Different code.** The refactored copy adds `Packets.c` / `MenuSerialize.c`
  and an 8-byte RPDO1, `PurgeRetractWaitErrorState` (110), the menu-var helper
  refactor, and changes to the coating sequence, cameras and `throwGhost()`.
- **It carries a regression production doesn't have.** The refactored copy
  turns the head and pump ON with `update_menu_var_by_str(&X, "ON")`. That never
  matches the `" ON"` enum token, so they stay OFF. Production uses
  `strncpy(" ON")` + `getvalue()` and works. See `tests/README.md`.
- **Folder spelling differs.** "Rev 4.33" here vs "Rev4.33" there.

The fix is to give the refactored copy its own revision number and folder
name, and record its changes in the revision history. Nothing has been renamed
yet.

## Host harness (added 2026-09-21)

- **`pc_side/`**: runs the real `doevents()` loop on a PC over a UDP CAN bus,
  with the RTI simulation thread and the stall detector. See `pc_side/README.md`.
- **`tests/`**: Unity suites against the real sources; 96 tests. See
  `tests/README.md`.
- **VS Code tasks:** `Test: All Tests`, `PC_SIDE: Build & Run`, and
  `PC_SIDE: Tests, Build & Run` (starts the host only if every test passes).

The only source changes are inert `PC_SIDE` guards:

- `mc9s12a128.h`: `_REG_BASE` points at a host array.
- `Interrupts.h`: the `interrupt_handler` pragmas are skipped on the host.
- `vectors.h`: the absolute vector table is skipped on the host.
- `Subroutines1.c`: the three cursor-flag release checks compare `(char)-1`
  on the host only; the target line is untouched.

The edited sources still build to the shipped `HEAD.s19`, byte-identical.
