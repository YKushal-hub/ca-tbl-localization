#!/usr/bin/env python3
"""G179 checker (DD 39): --collapse through the full pipeline, 20 workers and 1."""
import csv
import os
import sys

FOOT, PILE_H = 30.0, 3.66


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def first_true(path):
    r = next(csv.DictReader(open(path)))
    return float(r["trueX"]), float(r["trueY"])


def main():
    d = sys.argv[1]
    fails = []
    for n in ("n20", "n1"):
        if rc(d, n) != 0:
            fails.append(f"{n}: exit {rc(d, n)}")
    for n in ("r_missing", "r_depth", "r_bracket"):
        if rc(d, n) in (0, None) or "FATAL" not in open(os.path.join(d, n + ".log")).read():
            fails.append(f"{n}: not refused with a FATAL message")
    if not fails:
        rows = list(csv.DictReader(open(os.path.join(d, "n20", "subject_summary.csv"))))
        for arm in ("ca_tbl_full", "static_868", "static_2p4"):
            ids = sorted(int(r["subjectId"]) for r in rows if r["arm"] == arm)
            if ids != list(range(1, 21)):
                fails.append(f"n20 {arm}: subjects {ids[:5]}... not 1..20")
        # every worker is still and inside the footprint for the whole run
        for sid in range(2, 21):
            p = os.path.join(d, "n20", f"subjtrace_ca_tbl_full_seed5_subj{sid}.csv")
            pts = {(r["trueX"], r["trueY"]) for r in csv.DictReader(open(p))}
            if len(pts) != 1:
                fails.append(f"n20 subject {sid} moved ({len(pts)} distinct true positions)")
                continue
            x, y = (float(v) for v in next(iter(pts)))
            if abs(x) > FOOT / 2 or abs(y) > FOOT / 2:
                fails.append(f"n20 subject {sid} at ({x:.1f}, {y:.1f}) outside the footprint")
        a = first_true(os.path.join(d, "n20", "trace_ca_tbl_full_seed5.csv"))
        b = first_true(os.path.join(d, "n1", "trace_ca_tbl_full_seed5.csv"))
        if a != b:
            fails.append(f"worker 1 at {a} with 20 workers but {b} alone")
        one = list(csv.DictReader(open(os.path.join(d, "n1", "arm_summary.csv"))))
        if len(one) != 3:
            fails.append(f"n1: {len(one)} arm rows, expected 3")
        full = [r for r in one if r["arm"] == "ca_tbl_full"]
        if not full or int(full[0]["fixCycles"]) == 0:
            fails.append("n1 ca_tbl_full: no fix at all")
        # the stations surround the pile
        import json
        dep = json.load(open(os.path.join(d, "n20", "deployment.json")))
        gws = dep["anchors"]["0"]
        if len(gws) != 4 or max(abs(g["x"]) + abs(g["y"]) for g in gws) > 31.0:
            fails.append("stations not the 4-gateway ring at 30 m around the pile")
    if fails:
        print("G179 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print("G179 PASS (20 buried workers through the full pipeline, 1-worker run, worker 1 identical, 3 refusals)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
