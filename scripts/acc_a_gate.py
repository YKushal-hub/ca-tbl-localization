#!/usr/bin/env python3
"""G100: --imuQuScale, the multiplier on the IMU increment covariance (Part A, A1).

usage: acc_a_gate.py <dir written by scripts/acc_a_gate.sh>

On a cycle without a position fix, the growth of trace(P) from the previous cycle is dominated by
the prediction, i.e. by the IMU increment covariance the filter was handed (the at most two ranges
such a cycle may fuse shrink P in one direction only). A multiplier on it shows up there first.
"Pure prediction" (no fix AND no range) was the first predicate; at 5 km tag 0 has a range on
almost every cycle (2-3 such cycles in 600 s), and REACH fired on it, so the predicate is no fix.

Checks (each prints "G100 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     the three runs exit 0.
  DEFAULT  --imuQuScale=1 and no flag: every output file except run_id.txt byte-identical.
  TAG0     tag 0's trace: the median growth of traceSigma over cycles without a fix is at
           least 5x larger at 10 than at 1 (tag 0's call site).
  SUBJ     the same over every subject trace (the subjects' call site).
  REACH    rule 4: each median rests on at least 20 cycles in both runs.
"""
import csv
import filecmp
import glob
import os
import statistics
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G100 {'ok  ' if ok else 'FAIL'}    {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def files(d):
    out = set()
    for root, _, fs in os.walk(d):
        for f in fs:
            out.add(os.path.relpath(os.path.join(root, f), d))
    out.discard("run_id.txt")
    return out


def growth(paths):
    """traceSigma(t) - traceSigma(t-1) on rows with hadFix 0."""
    g = []
    for p in paths:
        prev = None
        for r in csv.DictReader(open(p)):
            ts = float(r["traceSigma"])
            if prev is not None and r["hadFix"] == "0":
                g.append(ts - prev)
            prev = ts
    return g


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("base", "one", "ten") if rc(d, n) != 0]
    report(not bad, "RUNS", "3 runs exited 0" if not bad else f"non-zero exit: {bad}")
    b, o = os.path.join(d, "base"), os.path.join(d, "one")
    fb, fo = files(b), files(o)
    diff = sorted(f for f in fb & fo if not filecmp.cmp(os.path.join(b, f), os.path.join(o, f), shallow=False))
    report(fb == fo and not diff and len(fb) > 2, "DEFAULT",
           f"{len(fb)} files; differing {diff[:3]}; only one side {sorted(fb ^ fo)[:3]}")
    reach = []
    for label, pat in (("TAG0", "trace_ca_tbl_full_seed*.csv"),
                       ("SUBJ", "subjtrace_ca_tbl_full_seed*_subj*.csv")):
        g1 = growth(sorted(glob.glob(os.path.join(d, "one", pat))))
        g10 = growth(sorted(glob.glob(os.path.join(d, "ten", pat))))
        if not g1 or not g10:
            global nmiss
            nmiss += 1
            print(f"  G100 MISSING {label:<8}no cycles without a fix ({len(g1)}, {len(g10)})")
            continue
        m1, m10 = statistics.median(g1), statistics.median(g10)
        report(m1 > 0 and m10 >= 5.0 * m1, label,
               f"median trace growth per cycle without a fix {m1:.4g} m^2 at 1, {m10:.4g} at 10 "
               f"(ratio {m10 / m1 if m1 > 0 else float('nan'):.2f}, need >= 5)")
        reach.append((label, len(g1), len(g10)))
    report(len(reach) == 2 and all(a >= 20 and c >= 20 for _, a, c in reach), "REACH",
           f"cycles without a fix (at 1, at 10): {reach} (need >= 20 each)")
    print(f"  G100: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
