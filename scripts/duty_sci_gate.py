#!/usr/bin/env python3
"""G185 (--sharedDuty868, DD 56) and G186 (--peerSplitCi, DD 57). usage: duty_sci_gate.py <dir of duty_sci_gate.sh>

  RUNS     every run exits 0 (fatal 1)      OFFSAME  explicit defaults reproduce the run without the flags, byte for byte
  REPORT   --dutyReport868 changes no CSV but prints one line per device
  OVER     base: at least one device's trailing-hour peak passes 2.5 % (the defect)
  LEGAL    duty: every device's mean and trailing-hour peak is at most 2.5 %
  AUDIT    the audited peak (sampled after every ranging exchange, carried byte and MAC frame) is at most 2.5 % and never below LEGAL's
  LINES    the DD56/DD57 announcements appear in their own runs only
  SCI      sci: crew coverage at least 0.05 above base's      FATAL  --peerSplitCi without ekf5 is refused
"""
import csv
import filecmp
import glob
import math
import os
import re
import sys

D = sys.argv[1]
bad = []


def check(g, name, ok, text):
    print(f"  {g} {'ok  ' if ok else 'FAIL'}  {name:8s} {text}")
    if not ok:
        bad.append(name)


def rd(n, ext):
    p = os.path.join(D, n + ext)
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def same(x, y):
    a, b = ({os.path.basename(p) for p in glob.glob(os.path.join(D, n, "*.csv"))} for n in (x, y))
    return len(a), [f for f in sorted(a & b) if not filecmp.cmp(os.path.join(D, x, f), os.path.join(D, y, f), False)] + sorted(a ^ b)


def cover(n):
    k = t = 0
    for f in glob.glob(os.path.join(D, n, "subjtrace_ca_tbl_full_seed722_subj*.csv")):
        for r in csv.DictReader(open(f)):
            t += 1
            k += float(r["errM"]) <= 1.73 * math.sqrt(max(float(r["traceSigma"]), 1e-9))
    return k / max(t, 1), t


def duty(n):
    return [(float(a), float(b)) for a, b in re.findall(r"DD56 device \d+ \S+: .* of [0-9.]+ s: ([0-9.e+-]+), trailing-hour peak ([0-9.e+-]+)", rd(n, ".log"))]


def audit(n):
    return [float(a) for a in re.findall(r"DD56 device \d+ \S+: .*audited peak ([0-9.e+-]+)", rd(n, ".log"))]


check("G185", "RUNS", all(rd(n, ".rc").strip() == "0" for n in ("off", "false", "base", "duty", "sci")), "five runs exit 0")
n, d = same("off", "false")
check("G185", "OFFSAME", n > 0 and not d, f"{n} files, differing {d[:3]}")
n, d = same("off", "base")
check("G185", "REPORT", n > 0 and not d and len(duty("base")) == 5, f"{n} files, differing {d[:3]}, device lines {len(duty('base'))}")
b, q = duty("base"), duty("duty")
check("G185", "OVER", bool(b) and max(x[1] for x in b) > 0.025, f"base trailing-hour peaks {[round(x[1], 4) for x in b]}")
check("G185", "LEGAL", len(q) == 5 and max(max(x) for x in q) <= 0.025, f"duty mean/peak max {max([max(x) for x in q] or [9]):.5f}")
a, o = audit("duty"), audit("base")
check("G185", "AUDIT", len(a) == 5 and max(a) <= 0.025 and all(u > 0 and u >= v[1] - 1e-6 for u, v in zip(a, q)) and max(o or [0]) > 0.025,
      f"audited peaks (after every exchange, carried byte and MAC frame) duty max {max(a or [9]):.5f}, base max {max(o or [0]):.5f}")
check("G185", "LINES", "DD56 sharedDuty868" in rd("duty", ".log") and "DD56 sharedDuty868" not in rd("off", ".log") and "DD57 peerSplitCi" in rd("sci", ".log") and "DD57" not in rd("off", ".log"), "announced in their own runs only")
check("G186", "FATAL", rd("fatal", ".rc").strip() == "1" and "peerSplitCi needs" in rd("fatal", ".log"), f"rc {rd('fatal', '.rc').strip()}")
(co, t0), (cs, t1) = cover("off"), cover("sci")
check("G186", "SCI", t0 > 0 and t1 == t0 and cs - co >= 0.05, f"coverage {co:.3f} -> {cs:.3f}")
print(f"  G185/G186 {len(bad)} failure(s)")
sys.exit(1 if bad else 0)
