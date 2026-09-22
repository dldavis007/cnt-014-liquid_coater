# Coating Sequence State Machine — Failure Analysis

**Source files:** `Subroutines1.c`, `Subroutines.c`  
**State machine location:** `doevents()` — `Subroutines1.c:1202`  
**State definitions:** `Subroutines.h:331–354`

---

## State Flow Overview

```
TrigState (1)
    └─ [LA still moving > 5s]      ──► ErrorState (99)
    └─ [LA not moving]             ──► CkHeadRotation (2)

CkHeadRotation (2)
    └─ [StateTime > 5s]            ──► HeadErrorState (100)
    └─ [HeadSpeed > 5000 RPM]      ──► PurgeRetract (3)

PurgeRetract (3)
    └─ [after 2s, purge retracted] ──► InitLAMove (5)
    └─ [after 2s, still moving]    ──► PurgeRetractWait (4)

PurgeRetractWait (4)
    └─ [StateTime > 20s]           ──► ErrorState (99)
    └─ [purge retracted]           ──► InitLAMove (5)

InitLAMove (5)
    └─ [LAPos != MaxLADist]        ──► HomeState (12)   [recovery]
    └─ [no strokes configured]     ──► ErrorState (99)  [config error]
    └─ [FirstStrokes > 0]          ──► StartCoatState (6)
    └─ [SecondStrokes > 0]         ──► StartSecondCoatState (8)
    └─ [CleanOutStrokes > 0]       ──► StartCleanOutState (10)

StartCoatState (6)
    └─ [!LAMovingTimer — BUG]      ──► ErrorState (99)
    └─ [LA not moving]             ──► FirstCoatState (7)

FirstCoatState (7)
    └─ [LAError]                   ──► ErrorState (99)
    └─ [HeadSpeed < 5000]          ──► HeadErrorState (100)
    └─ [FirstCoatSeq done]         ──► StartSecondCoatState (8)

StartSecondCoatState (8)
    └─ [LA not moving]             ──► SecondCoatState (9)
                                       or StartCleanOutState (10)

SecondCoatState (9)
    └─ [LAError]                   ──► ErrorState (99)
    └─ [HeadSpeed < 5000]          ──► HeadErrorState (100)
    └─ [SecondCoatSeq done]        ──► StartCleanOutState (10)

StartCleanOutState (10)
    └─ [LA not moving]             ──► CleanOutState (11)

CleanOutState (11)
    └─ [LAError]                   ──► ErrorState (99)
    └─ [CleanCoatSeq done]         ──► HomeState (12)

HomeState (12)
    └─ [LA not moving]             ──► StopState (13)  [both branches — see §3.1]

StopState (13)
    └─ [!LAMovingTimer — BUG]      ──► ErrorState (99)
    └─ [LA not moving]             ──► CoatingComplete (14)

CoatingComplete (14)
    └─ [after 1s]                  ──► StopWaitState (15)

StopWaitState (15)
    └─ [Cycle_Complete — always 1] ──► FinishState (16)
    └─ [StateTime > 15s — dead]    ──► default → FinishState (16)

FinishState (16)
    └─ stays here until next trigger

ErrorState (99)
    └─ turns off head + pump       ──► FinishState (16)

HeadErrorState (100)
    └─ turns off head + pump       ──► HdErrHomeState (101)

HdErrHomeState (101)
    └─ [LA not moving]             ──► HdErrStopState (102)  [both branches — see §3.2]

HdErrStopState (102)
    └─ [LA not moving]             ──► default → FinishState (16)
```

---

## Section 1 — Transitions to `ErrorState` (99): `"Warn:TIMEOUT ERROR"`

### 1.1 Actuator Still Moving at Trigger — `TrigState` (line 1226)

```c
else if ( StateTime > RTI_One_Sec * 5 )
    State = ErrorState;
```

If the LA is still moving when the PLC trigger fires, it gets 5 seconds to finish. If it doesn't stop within that window, the sequence errors before it begins.

---

### 1.2 No Stroke Sequences Configured — `InitLAMove` (line 1281)

```c
else
    State = ErrorState;
```

If `FirstStrokes`, `SecondStrokes`, and `CleanOutStrokes` are all zero, the state machine has nowhere to go and immediately transitions to `ErrorState`. This is a configuration error.

---

### 1.3 `LAMovingTimer` Check Outside the Guard Block — `StartCoatState` (line 1303)

```c
if ( !gProcImg[OUT_digi_7] )  // guard block
{
    FirstCoatSeq( 1 );        // sets LAMovingTimer inside
    State++;
}
if (!LAMovingTimer)           // ← runs unconditionally, even while LA is moving
    State = ErrorState;
```

This check runs on **every pass** through `StartCoatState`, not just when the LA is idle. If `LAMovingTimer` happened to expire before this state was entered (e.g., the LA was slow to stop from a prior move), the sequence errors out immediately with no display message, before any coating has started.

> **This is the most common source of a phantom timeout that appears unrelated to coating.**

---

### 1.4 Purge Unit Didn't Retract — `PurgeRetractWait` (line 1256)

```c
if ( StateTime > RTI_One_Sec * 20 )
    State = ErrorState;
```

20-second timeout waiting for `IN_digi_31` to go low. Physical jam or CAN bus failure causes `ErrorState`.

---

### 1.5 `LAMovingTimer` Check Outside the Guard Block — `StopState` (line 1411)

```c
if ( !gProcImg[OUT_digi_7] )
{
    MoveLA ( MaxLADist.value, 100, LACurrent );  // sets LAMovingTimer
    State = CoatingComplete;
}
if (!LAMovingTimer)                              // ← unconditional, same bug as §1.3
    State = ErrorState;
```

Identical structural bug to `StartCoatState`. If `LAMovingTimer` expired before entering this state, the sequence errors instead of extending to max and completing.

---

### 1.6 `LAError` Set in Coating Sequence Functions

`LAError = 1` is raised inside `FirstCoatSeq`, `SecondCoatSeq`, and `CleanCoatSeq`. `FirstCoatState`, `SecondCoatState`, and `CleanOutState` each check it and transition to `ErrorState`.

Two conditions set `LAError`:

**LA didn't start moving within 1 second** (`LAMoveTimer = 1s` set in `MoveLA()`):
```c
else if ( MoveCmdXmtd )
{
    if (!LAMoveTimer)
    {
        LAError = 1;
        Display ("Proc:Not Moving");
    }
    return 0;
}
```

**LA kept moving past its computed timeout** (`LAMovingTimer` based on distance + speed):
```c
if (!Temp_LAMovingTimer)
{
    LAError = 1;
    Display ("Proc:Moving Too Long");
}
```

Both use interrupt-protected reads of `LAMovingTimer` via `Temp_LAMovingTimer` in the sequence functions. The unprotected reads in `StartCoatState` and `StopState` (§1.3, §1.5) are the risky ones.

---

## Section 2 — Transitions to `HeadErrorState` (100): `"Warn:HEAD ERROR"`

### 2.1 Head Didn't Reach Speed — `CkHeadRotation` (line 1233)

```c
if ( StateTime > RTI_One_Sec * 5 )
    State = HeadErrorState;     // ← evaluated first
if ( HeadSpeed > 5000 )
    State++;
```

`HeadSpeed` is reset to `0` in `TrigState` and then updated from tach input captures in `Interrupts.c`. The 5-second timeout is checked **before** the speed check in execution order, so if both conditions become true on the same tick, `HeadErrorState` wins even if the head is spinning.

---

### 2.2 Head Slowed During Coating — `FirstCoatState` / `SecondCoatState` (lines 1311, 1338)

```c
if ( HeadSpeed < 5000 )
    State = HeadErrorState;
```

`HeadSpeed` is set to `0` in `Interrupts.c` when the tach input times out. Any loss of the tach signal — a dirty sensor, brief disconnection, or ECU reset — sets `HeadSpeed = 0` and immediately fires `HeadErrorState` mid-coat.

> **Note:** `CleanOutState` does **not** have this check. The head can stop during cleanout without triggering an error.

---

### 2.3 Head Error Recovery Path

`HeadErrorState` → turns off head and pump → `HdErrHomeState` → `HdErrStopState` → extends LA to max → `FinishState`. See §3.2 for bugs in this path.

---

## Section 3 — Logic Bugs That Cause Incorrect Behavior

### 3.1 `HomeState`: Both Branches Lead to the Same State (line 1388)

```c
case HomeState:
    if ( !gProcImg[OUT_digi_7] )
    {
        if (gProcImg[IN_digi_12] != 0)
            State++;          // 12 + 1 = 13 = StopState
        else
            State = StopState; // also 13
    }
```

The position check is meaningless — `State++` from `HomeState (12)` equals `StopState (13)`, the same as the explicit assignment. The branch that was presumably meant to send the LA home first does nothing different. Both paths immediately fall into `StopState`, which extends the LA to max.

Additionally, `IN_digi_12` is read regardless of `LA_TYPE`. For `LA_TYPE == 1` (Cleaner), position lives at `IN_digi_32`. Since both branches lead to the same state, this doesn't change the outcome here, but is consistent with the wrong-register issue in §3.2.

---

### 3.2 `HdErrStopState`: Unit Mismatch on Position Check (line 1490)

```c
if (gProcImg[IN_digi_12] != MaxLADist.value)
    State++;                  // 102 + 1 = 103 → default → FinishState
else
    State = FinishState;      // dead branch
```

`IN_digi_12` stores `Pos × 10` (set by `MoveLA()`: `gProcImg[IN_digi_12] = Pos*10`), so for a 12" machine it holds `120`. `MaxLADist.value` is in whole inches (`12`). The comparison `120 != 12` is **always true**, so the `State = FinishState` branch is unreachable. Both paths lead to `FinishState` through different routes, so the outcome is the same, but the correct state is never taken by intent.

---

### 3.3 `CleanCoatSeq`: Array Out-of-Bounds When Both Stroke Counts Are Zero (line 462)

```c
else  // SecondStrokes == 0
{
    Len = FirstStrokeLen[(int)FirstStrokes.value - 1].value;  // index = -1 !
    ...
}
```

If `SecondStrokes = 0` **and** `FirstStrokes = 0`, the array index is `-1`. This produces undefined behavior (reads memory before the array). The only way to reach `CleanOutState` with both counts at zero is if `CleanOutStrokes > 0` while the other two are zero — a configuration that `InitLAMove` allows through (`§1.2` only errors when all three are zero).

---

### 3.4 Operator Precedence Bug in `MoveLA()` (lines 267, 285)

```c
// As written:
if ( gProcImg[IN_digi_12+2] & ~0x01 != Current & ~0x01 )

// How C evaluates it (& has lower precedence than !=):
if ( gProcImg[IN_digi_12+2] & (~0x01 != Current) & ~0x01 )
```

`~0x01 != Current` evaluates to `0` or `1` before the `&` operators run. ANDed with `~0x01 = 0xFE`, the result is always `0`. The current-limit byte check **never fires**. The intended expression was:

```c
if ( (gProcImg[IN_digi_12+2] & ~0x01) != (Current & ~0x01) )
```

The current limit is written unconditionally without validation every time `MoveLA()` is called.

---

### 3.5 `StopWaitState`: Timeout and `Cycle_Complete` Both Effectively Dead (line 1426)

```c
// Three lines before the switch statement:
if ( 1 )
    Cycle_Complete = 1;

// Inside the switch:
case StopWaitState:
    if ( StateTime > 15 * RTI_One_Sec )
        State = 99;           // hits default → FinishState (not ErrorState)
    if ( Cycle_Complete )
        State++;              // ← always fires immediately
```

`Cycle_Complete` is set to `1` unconditionally before the switch every time `doevents()` runs (except when `TrigState` resets it). By the time execution reaches `StopWaitState`, `Cycle_Complete` is always `1`, so the state always advances on the first tick. The 15-second timeout never fires. Additionally, `State = 99` hits the `default:` case which resolves to `FinishState`, not `ErrorState`, so it would not produce a timeout warning even if it did fire.

---

### 3.6 `update_menu_var_by_value()` Silently Fails on Out-of-Range Values (line 2037)

```c
if (new_value < var->min || new_value > var->max)
    return -1;  // silent failure — no caller checks this
```

Called at the end of `FirstCoatSeq` and `SecondCoatSeq` to restore `PumpSpd` to `OldPumpSpeed`. If `OldPumpSpeed` was captured with a value outside the menu variable's configured range, the pump speed is silently left at whatever it was during the last stroke rather than being restored. No caller checks the return value.

---

## Section 4 — Summary Table

| # | State | Failure / Bug | Outcome | Display |
|---|---|---|---|---|
| 1.1 | `TrigState` | LA still moving at trigger, > 5s timeout | `ErrorState` | `"Warn:TIMEOUT ERROR"` |
| 1.2 | `InitLAMove` | All stroke counts are zero (config error) | `ErrorState` | `"Warn:TIMEOUT ERROR"` |
| 1.3 | `StartCoatState` | `!LAMovingTimer` check is outside `if(!moving)` guard — phantom timeout | `ErrorState` | `"Warn:TIMEOUT ERROR"` |
| 1.4 | `PurgeRetractWait` | Purge unit fails to retract within 20s | `ErrorState` | `"Warn:TIMEOUT ERROR"` |
| 1.5 | `StopState` | Same unconditional `!LAMovingTimer` bug as 1.3 | `ErrorState` | `"Warn:TIMEOUT ERROR"` |
| 1.6a | `First/Second/CleanCoatSeq` | LA doesn't begin moving within 1s of command | `ErrorState` (next tick) | `"Proc:Not Moving"` |
| 1.6b | `First/Second/CleanCoatSeq` | LA exceeds computed travel time | `ErrorState` (next tick) | `"Proc:Moving Too Long"` |
| 2.1 | `CkHeadRotation` | Head doesn't exceed 5000 RPM within 5s | `HeadErrorState` | `"Warn:HEAD ERROR"` |
| 2.2 | `FirstCoatState` / `SecondCoatState` | Head drops below 5000 RPM mid-coat | `HeadErrorState` | `"Warn:HEAD ERROR"` |
| 3.1 | `HomeState` | Both position branches lead to same state — position check is dead code | Wrong branch label, same outcome | — |
| 3.2 | `HdErrStopState` | `IN_digi_12` (Pos×10) compared against `MaxLADist` (whole inches) — always unequal | Correct branch never taken; same final outcome | — |
| 3.3 | `CleanCoatSeq` | Array index `FirstStrokeLen[-1]` when `FirstStrokes=0` and `SecondStrokes=0` | Undefined behavior / memory read | — |
| 3.4 | `MoveLA()` | Operator precedence bug — current-limit check always evaluates false | Current byte written without validation | — |
| 3.5 | `StopWaitState` | `Cycle_Complete` always `1`; 15s timeout resolves to `FinishState` not `ErrorState` | Both paths are dead code | — |
| 3.6 | `First/SecondCoatSeq` | `update_menu_var_by_value()` silently rejects out-of-range `OldPumpSpeed` | Pump speed not restored after coating | — |
