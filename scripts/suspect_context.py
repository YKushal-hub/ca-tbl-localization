#!/usr/bin/env python3
"""Phase B: what is the integrity check actually firing on?

    python3 scripts/suspect_context.py results/trace3000/seed_*/trace_ca_tbl_full_seed*.csv

The measured effect of the check at boxHalf 3000 is +5.239 m [-5.242, +15.720] --
adverse in the mean, 11/20 seeds hurt, worst +70.4 m. The leading hypothesis is that
this is not an integrity defect but a Q defect: the EKF's process noise does not grow
with coasting time (v2 section 7.2), so after an outage the prior covariance is far
too small for the drift actually accumulated, which inflates the Mahalanobis distance
of a perfectly good measurement.

That hypothesis makes a specific, falsifiable prediction: firings should cluster
immediately after coverage gaps. This script measures the gap distribution and
compares it against the base rate over all fix cycles, so "clusters post-outage" is a
comparison rather than an impression.

It also scores each firing against ground truth. A firing is CORRECT if the fix the
check rejected would have been worse than the prior it fell back to, and INCORRECT if
the rejected fix was the better of the two. errM on the firing row is post-revert, so
the retained-prior error is read directly; the rejected measurement's quality is
approximated by the next cycle that does produce an accepted fix.
"""

import csv
import glob
import statistics
import sys


def load(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def analyse(rows):
    """Per-firing context. Returns a list of dicts."""
    out = []
    last_fix = None          # index of the most recent accepted fix
    for i, r in enumerate(rows):
        if int(r["suspect"]) == 1:
            gap = (i - last_fix) if last_fix is not None else None
            # how many of the preceding cycles were blind
            lo = max(0, i - 10)
            blind10 = sum(int(rows[k]["blind"]) for k in range(lo, i))
            # next accepted fix after the revert, as a proxy for what was rejected
            nxt = None
            for k in range(i + 1, min(i + 6, len(rows))):
                if int(rows[k]["hadFix"]) == 1 and int(rows[k]["suspect"]) == 0:
                    nxt = float(rows[k]["errM"])
                    break
            out.append({
                "i": i,
                "gap": gap,
                "blind10": blind10,
                "mahal": float(r["mahalanobis"]),
                "traceSigma": float(r["traceSigma"]),
                "errM": float(r["errM"]),
                "rejected": int(r["rejected"]),
                "nNlos": int(r["nNlos"]),
                "next_err": nxt,
            })
        if int(r["hadFix"]) == 1:
            last_fix = i
    return out


def gaps_all_fixes(rows):
    """Base-rate gap distribution: cycles since previous fix, over every fix."""
    out = []
    last = None
    for i, r in enumerate(rows):
        if int(r["hadFix"]) == 1:
            if last is not None:
                out.append(i - last)
            last = i
    return out


def main():
    paths = sorted(sys.argv[1:])
    if not paths:
        print("no trace files given")
        return

    allfire, allbase, nseeds, ncycles = [], [], 0, 0
    for p in paths:
        rows = load(p)
        if not rows:
            continue
        nseeds += 1
        ncycles += len(rows)
        allfire.extend(analyse(rows))
        allbase.extend(gaps_all_fixes(rows))

    print(f"traces: {nseeds}   cycles: {ncycles}   firings: {len(allfire)}")
    if not allfire:
        print("no suspect firings in these traces")
        return

    gaps = [f["gap"] for f in allfire if f["gap"] is not None]
    print("\n=== THE TEST: do firings cluster after a coverage gap? ===")
    print(f"  cycles-since-last-fix AT A FIRING   n={len(gaps):<5} "
          f"median={statistics.median(gaps):.1f}  mean={statistics.mean(gaps):.2f}  max={max(gaps)}")
    print(f"  cycles-since-last-fix, BASE RATE    n={len(allbase):<5} "
          f"median={statistics.median(allbase):.1f}  mean={statistics.mean(allbase):.2f}  max={max(allbase)}")
    imm = sum(1 for g in gaps if g == 1)
    post = sum(1 for g in gaps if g > 1)
    bimm = sum(1 for g in allbase if g == 1)
    print(f"  firings on a cycle immediately after a fix (gap==1): {imm}/{len(gaps)} = {imm/len(gaps):.1%}")
    print(f"  base rate of gap==1 over all fixes:                  {bimm}/{len(allbase)} = {bimm/len(allbase):.1%}")
    print(f"  firings following a REAL gap (>1 cycle):             {post}/{len(gaps)} = {post/len(gaps):.1%}")
    if post / len(gaps) > (1 - bimm / len(allbase)) * 1.5:
        print("  -> VERDICT: firings are strongly enriched post-outage. Consistent with a Q defect.")
    elif post / len(gaps) > (1 - bimm / len(allbase)):
        print("  -> VERDICT: firings are mildly enriched post-outage.")
    else:
        print("  -> VERDICT: NOT enriched post-outage. The check itself is miscalibrated.")

    print("\n=== blind cycles in the 10 cycles preceding a firing ===")
    b = [f["blind10"] for f in allfire]
    print(f"  median={statistics.median(b):.1f}  mean={statistics.mean(b):.2f}  "
          f"firings with >=5 blind of last 10: {sum(1 for x in b if x >= 5)}/{len(b)}")

    print("\n=== was the check RIGHT? (retained prior err vs next accepted fix err) ===")
    scored = [f for f in allfire if f["next_err"] is not None]
    good = sum(1 for f in scored if f["errM"] <= f["next_err"])
    print(f"  scoreable firings: {len(scored)}/{len(allfire)}")
    if scored:
        print(f"  retained prior was better or equal: {good}/{len(scored)} = {good/len(scored):.1%}")
        print(f"  median err at firing (post-revert):  {statistics.median([f['errM'] for f in scored]):.3f} m")
        print(f"  median err at next accepted fix:     {statistics.median([f['next_err'] for f in scored]):.3f} m")

    print("\n=== the strict all-LOS condition ===")
    print(f"  firings with nNlos==0: {sum(1 for f in allfire if f['nNlos']==0)}/{len(allfire)}"
          f"   (suspect requires it, so this must be 100%)")

    print("\n=== mahalanobis / traceSigma at firing ===")
    for k in ("mahal", "traceSigma", "errM"):
        v = [f[k] for f in allfire]
        print(f"  {k:<11} median={statistics.median(v):9.3f}  min={min(v):9.3f}  max={max(v):11.3f}")

    print("\n=== worst 8 firings by error after the revert ===")
    print(f"  {'gap':>5} {'blind10':>8} {'mahal':>9} {'traceSigma':>11} {'errM':>10} {'next_err':>10}")
    for f in sorted(allfire, key=lambda x: -x["errM"])[:8]:
        ne = f"{f['next_err']:10.3f}" if f["next_err"] is not None else f"{'--':>10}"
        g = f"{f['gap']:5}" if f["gap"] is not None else f"{'--':>5}"
        print(f"  {g} {f['blind10']:8} {f['mahal']:9.3f} {f['traceSigma']:11.3f} {f['errM']:10.3f} {ne}")


if __name__ == "__main__":
    main()
