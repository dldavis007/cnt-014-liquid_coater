#!/usr/bin/env python3
"""
compare_coatseq.py - diff the coating-sequence switch between two revisions.

The 4.33 -> 4.34 work was a refactor: the state machine moved from
Subroutines1.c to unit.c and the CANopen process image went from flat
gProcImg[] indexing to named pointers. That renaming swamps a raw diff, so this
normalises the accessors (and strips comments/whitespace) and diffs what is
left - i.e. the actual behaviour.

    python compare_coatseq.py

Expected output for 4.33 vs 4.34 is TWO deltas; anything else means the
sequence changed and the unit tests in this directory should be re-checked
against it. See REGRESSION.md for the recorded baseline.
"""

import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
COATER = os.path.abspath(os.path.join(HERE, "..", ".."))

# (label, path, first line of `case TrigState:`, last line of the switch body)
OLD = ("Rev4.33", os.path.join(COATER, "Rev4.33", "SourceFiles", "Subroutines1.c"), 1204, 1495)
NEW = ("Rev4.34", os.path.join(COATER, "Rev4.34", "SourceFiles", "unit.c"),          937, 1228)

# Accessors that were renamed by the refactor but denote the same byte.
ALIASES = [
    ("gProcImg[OUT_digi_7]",    "ACT_MOVING"),
    ("*rpdo4_actuator_moving",  "ACT_MOVING"),
    ("gProcImg[IN_digi_31]",    "PURGE_MOVING"),
    ("*rpdo7_purge_moving",     "PURGE_MOVING"),
    ("gProcImg[IN_digi_12]",    "LA_POS"),
    ("tpdo3_actuator_1[0]",     "LA_POS"),
]


def normalise(path, start, end):
    with open(path, encoding="utf-8", errors="replace") as fh:
        lines = fh.read().splitlines()[start - 1:end]
    out = []
    for line in lines:
        line = re.sub(r"//.*$", "", line)          # trailing comments
        for old, new in ALIASES:
            line = line.replace(old, new)
        line = re.sub(r"\s+", " ", line).strip()   # indentation / spacing
        if line:
            out.append(line)
    return out


def main():
    for label, path, _, _ in (OLD, NEW):
        if not os.path.exists(path):
            print(f"missing {label}: {path}")
            return 2

    a = normalise(*OLD[1:])
    b = normalise(*NEW[1:])
    diff = list(difflib.unified_diff(a, b, OLD[0], NEW[0], lineterm="", n=1))

    if not diff:
        print("coating sequence is IDENTICAL after normalising accessors")
        return 0

    print("\n".join(diff))
    changes = sum(1 for d in diff if d[:1] in "+-" and d[:3] not in ("---", "+++"))
    print(f"\n{changes} changed line(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
