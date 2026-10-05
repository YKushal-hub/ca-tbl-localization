#!/usr/bin/env python3
"""L.1.4 / G16 -- the anchor grant tie-break is reached, and reached by the spec's rule.

WHY THIS EXISTS. CaTblGrantCoordinator and its unit suite existed for weeks with zero
scenario call sites: every check passed and the mechanism never ran. A unit suite
proves the rule is implemented; it cannot prove anything calls it. This gate reads what
a RUN wrote and asks whether the tie-break happened there.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). With --anchorGrantGossipS > 0 the scenario
writes grant_verdicts_<arm>_seed<N>.csv, one row per verdict:

    t,anchorId,targetId,otherAnchorId,source,verdict,action,windowStartS,windowEndS

The done-when of L.1.4 is two anchors reaching OPPOSITE verdicts on one target: a row
where the higher id reports WIN against the lower, and a row where that lower id reports
DEFER against that higher one, for the same target, over overlapping windows. That pair
is what this gate counts. The defect it guards against has the form of ABSENCE -- the
coordinator unwired again, the gossip inaudible, the flag silently inert -- so it is
checked by counting rows in a file a fresh run must produce, never by scanning a file
that might simply be missing.

WHAT IT CHECKS
  1. The file exists and has at least one data row (rows COUNTED, rule 12).
  2. The rule itself, on every row that names an opponent: DEFER only at the LOWER id,
     WIN only at the HIGHER id. An inverted rule still produces opposite verdicts, so
     (3) alone would pass it.
  3. Action agrees with verdict: yield/withhold only on DEFER, keep only on WIN.
  4. At least one mutual opposite-verdict pair, as defined above.

usage: grant_tiebreak_gate.py <grant_verdicts csv>
exit 0 PASS, 1 FAIL, 2 usage/ERROR.
"""
import csv
import sys

COLS = ["t", "anchorId", "targetId", "otherAnchorId", "source", "verdict", "action",
        "windowStartS", "windowEndS"]


def main(argv):
    if len(argv) != 2:
        print("usage: grant_tiebreak_gate.py <grant_verdicts csv>")
        return 2
    path = argv[1]
    try:
        with open(path, newline="") as fh:
            rd = csv.reader(fh)
            header = next(rd, None)
            rows = [r for r in rd if r]
    except OSError as e:
        print(f"G16 FAIL: cannot read {path}: {e} -- the flag-on run wrote no verdict file")
        return 1
    if header != COLS:
        print(f"G16 FAIL: header {header} is not {COLS}")
        return 1
    print(f"G16 rows: {len(rows)} in {path}")
    if not rows:
        print("G16 FAIL: zero verdict rows -- no gateway ever evaluated a grant")
        return 1

    bad = 0
    recs = []
    for n, r in enumerate(rows, start=2):
        if len(r) != len(COLS):
            print(f"G16 FAIL: line {n} has {len(r)} fields: {r}")
            return 1
        a, tgt, other = int(r[1]), int(r[2]), int(r[3])
        verdict, action = r[5], r[6]
        s, e = float(r[7]), float(r[8])
        if verdict == "DEFER" and not (other > a):
            print(f"G16 FAIL: line {n}: DEFER at {a} against {other} -- the LOWER id defers")
            bad += 1
        if verdict == "WIN" and not (0 < other < a):
            print(f"G16 FAIL: line {n}: WIN at {a} against {other} -- only the HIGHER id wins")
            bad += 1
        if action in ("yield", "withhold") and verdict != "DEFER":
            print(f"G16 FAIL: line {n}: action {action} on verdict {verdict}")
            bad += 1
        if action == "keep" and verdict != "WIN":
            print(f"G16 FAIL: line {n}: action keep on verdict {verdict}")
            bad += 1
        recs.append((a, tgt, other, verdict, s, e))
    if bad:
        return 1

    wins = [x for x in recs if x[3] == "WIN"]
    defers = [x for x in recs if x[3] == "DEFER"]
    pairs = 0
    for (hi, tgt, lo, _, s1, e1) in wins:
        for (a2, tgt2, other2, _, s2, e2) in defers:
            if a2 == lo and other2 == hi and tgt2 == tgt and s1 < e2 and s2 < e1:
                pairs += 1
                print(f"G16 pair: target {tgt}: {hi} WIN / {lo} DEFER, "
                      f"windows [{s1}, {e1}) and [{s2}, {e2})")
                break
    print(f"G16 verdicts: WIN {len(wins)}, DEFER {len(defers)}, mutual opposite pairs {pairs}")
    if pairs == 0:
        print("G16 FAIL: no two anchors reached opposite verdicts on one target")
        return 1
    print("G16 PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
