#!/usr/bin/env python3
"""G184: --sparseRangeGeometry (DD 54). usage: sparse_geometry_gate.py <dir of sparse_geometry_gate.sh>

  RUNS     the three runs exit 0
  OFFSAME  the explicit default reproduces the run without the flag, every CSV byte for byte
  LINE     the flag announces itself in the on run only
  FATAL    the flag without --sparseInStep is refused (rc 1, the message)
  JUMP     crew error rising > 50 m in one cycle at a claimed sqrt(tr P) < 30 m: at least 5 off (the defect), at most a quarter on
  COVER    crew cycles inside 1.73 sqrt(tr P): at least 0.10 more on than off
"""
import csv
import filecmp
import glob
import math
import os
import sys

D = sys.argv[1]
bad = []


def check(name, ok, text):
    print(f"  G184 {'ok  ' if ok else 'FAIL'}  {name:8s} {text}")
    if not ok:
        bad.append(name)


def rd(n, ext):
    p = os.path.join(D, n + ext)
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def stats(n):
    jumps = inside = tot = 0
    for f in sorted(glob.glob(os.path.join(D, n, "subjtrace_ca_tbl_full_seed701_subj*.csv"))):
        prev = None
        for r in csv.DictReader(open(f)):
            e, s = float(r["errM"]), math.sqrt(max(float(r["traceSigma"]), 1e-9))
            tot += 1
            inside += e <= 1.73 * s
            if prev and float(r["t"]) > 300 and prev[1] < 30 and e - prev[0] > 50:
                jumps += 1
            prev = (e, s)
    return jumps, inside / max(tot, 1), tot


check("RUNS", all(rd(n, ".rc").strip() == "0" for n in ("off", "false", "on")), str([rd(n, ".rc").strip() for n in ("off", "false", "on")]))
a, b = ({os.path.basename(p) for p in glob.glob(os.path.join(D, n, "*.csv"))} for n in ("off", "false"))
diff = [x for x in sorted(a & b) if not filecmp.cmp(os.path.join(D, "off", x), os.path.join(D, "false", x), False)] + sorted(a ^ b)
check("OFFSAME", bool(a) and not diff, f"{len(a)} files, differing {diff[:3]}")
check("LINE", "DD54" in rd("on", ".log") and "DD54" not in rd("off", ".log"), "announced on only")
check("FATAL", rd("fatal", ".rc").strip() == "1" and "needs --sparseInStep" in rd("fatal", ".log"), f"rc {rd('fatal', '.rc').strip()}")
(jo, co, to), (jn, cn, tn) = stats("off"), stats("on")
check("JUMP", to > 0 and tn == to and jo >= 5 and jn <= jo / 4, f"off {jo}, on {jn} (cycles {to}, {tn})")
check("COVER", to > 0 and cn - co >= 0.10, f"off {co:.3f}, on {cn:.3f}")
print(f"  G184 {len(bad)} failure(s)")
sys.exit(1 if bad else 0)
