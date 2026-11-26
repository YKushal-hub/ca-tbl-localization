#!/usr/bin/env python3
"""G180 checker (DD 41): with --interferenceFrameKey the copies of one broadcast stop colliding."""
import csv
import glob
import os
import sys


def zero_frac(run):
    z = t = 0
    for f in glob.glob(os.path.join(run, "subjtrace_ca_tbl_full_seed5_subj*.csv")):
        rows = list(csv.DictReader(open(f)))
        z += sum(r["nAnchors"] == "0" for r in rows)
        t += len(rows)
    return z / t if t else float("nan"), t


def main():
    d = sys.argv[1]
    fails = []
    for n in ("off", "on"):
        try:
            rc = int(open(os.path.join(d, n + ".rc")).read())
        except (OSError, ValueError):
            rc = None
        if rc != 0:
            fails.append(f"{n}: exit {rc}")
    if not fails:
        zoff, toff = zero_frac(os.path.join(d, "off"))
        zon, ton = zero_frac(os.path.join(d, "on"))
        if not (toff == ton and toff >= 4 * 40):
            fails.append(f"subject trace rows {toff} / {ton}, expected equal and >= 160")
        if not zoff >= 0.3:
            fails.append(f"off: zero-anchor fraction {zoff:.3f}; the reference defect no longer shows (< 0.3)")
        if not zon <= 0.05:
            fails.append(f"on: zero-anchor fraction {zon:.3f} > 0.05; subjects still starve")
        t0 = [list(csv.DictReader(open(os.path.join(d, n, "trace_ca_tbl_full_seed5.csv")))) for n in ("off", "on")]
        if not all(sum(r["nAnchors"] == "0" for r in rows) == 0 for rows in t0):
            fails.append("tag 0 lost its anchors")
    if fails:
        print("G180 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print(f"G180 PASS (subjects 2..5 with no anchor: off {zoff:.3f}, on {zon:.3f})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
