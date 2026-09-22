#!/usr/bin/env python3
"""
run_all.py - run every built Unity suite and print an AGGREGATE summary
(total tests / passed / failed / ignored, plus a list of the failures).

The Makefile `summary` target builds all test_*.exe first, then invokes this;
so this script only runs the already-built executables and tallies their
Unity output. Used by the "Test: All Tests" VS Code task.
"""

import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

suites = sorted(
    os.path.splitext(os.path.basename(f))[0]
    for f in glob.glob(os.path.join(HERE, "test_*.c"))
)

# Unity per-suite footer, e.g. "52 Tests 1 Failures 0 Ignored"
FOOTER_RE = re.compile(r"(\d+)\s+Tests\s+(\d+)\s+Failures\s+(\d+)\s+Ignored")

tot_tests = tot_fail = tot_ign = 0
failures = []          # (suite, "file:line:test:FAIL: message")
not_completed = []     # suites that crashed / produced no footer

print("=" * 72)
for s in suites:
    exe = os.path.join(HERE, s + ".exe")
    if not os.path.exists(exe):
        not_completed.append(s + " (not built)")
        continue

    try:
        r = subprocess.run([exe], capture_output=True, text=True, timeout=120)
        out = r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        not_completed.append(s + " (TIMED OUT)")
        continue

    print(f"\n----- {s} -----")
    print(out.rstrip())

    m = FOOTER_RE.search(out)
    if not m:
        not_completed.append(s + " (no result line / crashed)")
        continue

    n, f, i = int(m.group(1)), int(m.group(2)), int(m.group(3))
    tot_tests += n
    tot_fail += f
    tot_ign += i
    for ln in out.splitlines():
        if ":FAIL" in ln:                      # e.g. file.c:129:test_x:FAIL: msg
            failures.append((s, ln.strip()))

# ---- aggregate summary ----
print("\n" + "=" * 72)
print("SUMMARY")
print(f"  Suites  : {len(suites)}")
print(f"  Tests   : {tot_tests}")
print(f"  Passed  : {tot_tests - tot_fail - tot_ign}")
print(f"  Failed  : {tot_fail}")
print(f"  Ignored : {tot_ign}")

if failures:
    print(f"\nFAILURES ({len(failures)}):")
    for suite, ln in failures:
        print(f"  [{suite}] {ln}")

if not_completed:
    print(f"\nSUITES NOT COMPLETED ({len(not_completed)}):")
    for c in not_completed:
        print(f"  {c}")

if tot_fail or not_completed:
    print("\nRESULT: FAIL")
    sys.exit(1)

print("\nRESULT: OK  (all tests passed)")
sys.exit(0)
