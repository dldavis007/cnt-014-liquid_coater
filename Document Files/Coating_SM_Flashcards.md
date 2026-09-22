# Coating Sequence State Machine — Failure Analysis Flashcards

## ErrorState Transitions

What failure in `TrigState` causes `ErrorState`, and after how long? :: The LA is still moving when the PLC trigger fires and does not stop within 5 seconds. Display: `"Warn:TIMEOUT ERROR"`

What failure in `InitLAMove` causes an immediate `ErrorState`? :: All three stroke counts (`FirstStrokes`, `SecondStrokes`, `CleanOutStrokes`) are zero. There is nothing for the state machine to do — configuration error. Display: `"Warn:TIMEOUT ERROR"`

Which state has a `!LAMovingTimer` check placed **outside** the `if(!moving)` guard, and why is that dangerous? :: `StartCoatState`. The check runs on every pass, even while the LA is still moving. If `LAMovingTimer` expired before entering the state, the sequence immediately errors with no display message before coating begins — a phantom timeout.

Which two states share the same `!LAMovingTimer`-outside-guard bug? :: `StartCoatState` (line 1303) and `StopState` (line 1411).

What failure in `PurgeRetractWait` causes `ErrorState`, and after how long? :: The purge unit does not retract (`IN_digi_31` stays high) within 20 seconds. Display: `"Warn:TIMEOUT ERROR"`

What display message does `"Proc:Not Moving"` indicate, and which state does it ultimately lead to? :: The LA did not begin moving within 1 second of a move command being sent (`LAMoveTimer` expired). Sets `LAError = 1` → `ErrorState` on the next tick. Display before error: `"Proc:Not Moving"`

What display message does `"Proc:Moving Too Long"` indicate, and which state does it lead to? :: The LA exceeded its computed travel timeout (`LAMovingTimer` expired while `OUT_digi_7` was still set). Sets `LAError = 1` → `ErrorState` on the next tick. Display before error: `"Proc:Moving Too Long"`

Which three states check `LAError` and transition to `ErrorState`? :: `FirstCoatState`, `SecondCoatState`, and `CleanOutState`.

## HeadErrorState Transitions

What failure in `CkHeadRotation` causes `HeadErrorState`, and after how long? :: `HeadSpeed` does not exceed 5000 RPM within 5 seconds. Note: the timeout check comes first in code order, so it wins even if the head reaches speed on the exact same tick. Display: `"Warn:HEAD ERROR"`

Which two coating states check `HeadSpeed` during the sequence, and what threshold triggers `HeadErrorState`? :: `FirstCoatState` and `SecondCoatState`. If `HeadSpeed` drops below 5000 RPM mid-coat, `HeadErrorState` fires immediately. Display: `"Warn:HEAD ERROR"`

Which coating state does **not** check `HeadSpeed`, meaning the head can stop without causing an error? :: `CleanOutState`. The head can stop spinning during cleanout without triggering `HeadErrorState`.

Why is `HeadSpeed` reset to `0` at the start of every coating cycle, and what is the risk? :: `TrigState` sets `HeadSpeed = 0`. This means the head must be confirmed spinning from scratch via the tach interrupt every cycle. If the tach signal is lost or delayed, `CkHeadRotation` can time out even if the head is physically running.

## Logic Bugs

What is the bug in `HomeState`, and does it affect the final outcome? :: Both the `if (IN_digi_12 != 0)` and `else` branches set `State` to `StopState (13)` — `State++` from `HomeState (12)` equals 13, the same as the explicit assignment. The position check is dead code. The final outcome is the same regardless.

What is the unit mismatch bug in `HdErrStopState`? :: `gProcImg[IN_digi_12]` stores position as `Pos × 10` (e.g., `120` for a 12" machine), but it is compared against `MaxLADist.value` in whole inches (`12`). The comparison `120 != 12` is always true, so the `State = FinishState` branch is unreachable. Both code paths still reach `FinishState`.

What out-of-bounds array access exists in `CleanCoatSeq`, and under what configuration does it occur? :: When `SecondStrokes = 0` and `FirstStrokes = 0`, the index `(int)FirstStrokes.value - 1` evaluates to `-1`, reading memory before `FirstStrokeLen[]`. It can be reached when `CleanOutStrokes > 0` while both other stroke counts are zero.

What is the operator precedence bug in `MoveLA()`, and what is its effect? :: `gProcImg[IN_digi_12+2] & ~0x01 != Current & ~0x01` — `!=` binds tighter than `&`, so the condition always evaluates to `0`. The current-limit byte is never validated and is overwritten unconditionally on every call to `MoveLA()`.

Why is the 15-second timeout in `StopWaitState` effectively dead code? :: `Cycle_Complete` is set to `1` unconditionally three lines before the `switch` statement (`if (1) { Cycle_Complete = 1; }`), so `StopWaitState` always exits via the `Cycle_Complete` branch on the first tick. Additionally, `State = 99` hits the `default:` case which resolves to `FinishState`, not `ErrorState`, so no warning would appear even if the timeout did fire.

What is the silent failure risk in `update_menu_var_by_value()` at the end of coating sequences? :: If `OldPumpSpeed` is outside the valid range of `PumpSpd` (0–100), the function returns `-1` without updating the variable. The pump speed is not restored after coating completes. No caller checks the return value.

## Quick Reference — Outcome by State

What are the two possible error outcomes from the coating state machine? :: `ErrorState (99)` — displays `"Warn:TIMEOUT ERROR"` and goes to `FinishState`. `HeadErrorState (100)` — displays `"Warn:HEAD ERROR"` and runs the recovery path through `HdErrHomeState` → `HdErrStopState` → `FinishState`.

Which states can transition to `ErrorState`? :: `TrigState`, `InitLAMove`, `StartCoatState`, `PurgeRetractWait`, `StopState`, `FirstCoatState` (via `LAError`), `SecondCoatState` (via `LAError`), `CleanOutState` (via `LAError`).

Which states can transition to `HeadErrorState`? :: `CkHeadRotation` (spin-up timeout), `FirstCoatState` (mid-coat slowdown), `SecondCoatState` (mid-coat slowdown).

What display message is shown for all `ErrorState` transitions? :: `"Warn:TIMEOUT ERROR"`

What display message is shown for all `HeadErrorState` transitions? :: `"Warn:HEAD ERROR"`
