#!/usr/bin/env python3
"""
pareto_sweep.py -- operating-point sweep for the decision-layer comparison.

WHY A SWEEP AND NOT A COMPARISON OF TWO POINTS
----------------------------------------------
`fixedVarThresholdM2 = 100.0` means the literature baseline escalates at
sigma = 10 m. The pre-registered v2 threshold escalates at sigma_eff = 3.162 m.
Those are DIFFERENT OPERATING POINTS on the accuracy/energy curve, so a
single-point comparison is unfair in one direction or the other regardless of
which wins -- and which direction depends only on which threshold happens to sit
nearer the knee. Reporting "v2 beats fixed_threshold" or the reverse from one
pair of points would be an artefact of two constants chosen for unrelated
reasons.

"Is the confidence model better than a flat threshold" is only answerable as
"does one Pareto front dominate the other". So both are swept across a range of
operating points and the resulting fronts in (median error, joules) are
compared. If the fronts overlap, the honest conclusion is that the decision
layer is EQUIVALENT to a flat threshold at matched operating points -- a cleaner
and more publishable negative than a single-point loss.

WHAT IS SWEPT
-------------
  fixed_threshold : --fixedVarThreshold over a decade-spaced grid in sigma
  ca_tbl v2       : --pcsSigmaTargetM over the SAME grid in sigma

Both grids are stated in metres of position sigma so the two arms are swept over
the same physical quantity. That is the whole point: an operating point is "how
much uncertainty do you tolerate before spending energy", and both rules answer
it, one via the EKF posterior alone and one via the confidence score.

    python3 scratch/python_handoff/pareto_sweep.py --plan     # print the runs
    python3 scratch/python_handoff/pareto_sweep.py --analyse results/pareto
"""

import sys
import os
import csv
import glob
import math
import argparse


# Operating points, in metres of position sigma. Decade-spaced with a half-decade
# step, spanning from well inside UWB's regime (1 m) to well past tier 1's
# (100 m), so both fronts are traced across their whole usable range rather than
# sampled near one arm's default.
#
# FIXED IN ADVANCE, and deliberately symmetric: the same grid for both arms. A
# grid chosen per-arm would let the sweep itself encode a preference.
SIGMA_GRID_M = [1.0, 1.78, 3.162, 5.62, 10.0, 17.8, 31.6, 56.2, 100.0]


def plan(seeds, duration, outroot):
    """Emit the run commands. Printed rather than executed so the caller controls
    scheduling, and so the exact invocations land in the report."""
    lines = []
    for s in SIGMA_GRID_M:
        # fixed_threshold's knob is a VARIANCE in m^2; the grid is in sigma.
        var = s * s
        lines.append(
            f'ca_tbl_causal_sim --nlosSource=propagation --pcsForm=legacy '
            f'--fixedVarThreshold={var:.4f} '
            f'--seedStart=1 --seedCount={seeds} --duration={duration} '
            f'--trace=false --outdir={outroot}/ft_sigma{s:g}')
    for s in SIGMA_GRID_M:
        lines.append(
            f'ca_tbl_causal_sim --nlosSource=propagation --pcsForm=v2 '
            f'--pcsSigmaTargetM={s:.4f} '
            f'--seedStart=1 --seedCount={seeds} --duration={duration} '
            f'--trace=false --outdir={outroot}/v2_sigma{s:g}')
    return lines


# ------------------------------------------------------------------ analysis --

def collect(outroot):
    """{family: [(sigma, arm, cep50, totalJ, escPerFix, n_seeds), ...]}"""
    out = {}
    for d in sorted(glob.glob(os.path.join(outroot, "*_sigma*"))):
        base = os.path.basename(d)
        fam, _, sig = base.partition("_sigma")
        p = os.path.join(d, "arm_summary.csv")
        if not os.path.exists(p):
            continue
        with open(p, newline="") as f:
            rows = list(csv.DictReader(f))
        # The arm of interest differs per family: the flat threshold lives in
        # fixed_threshold, the confidence model in ca_tbl_no_integrity (chosen as
        # the primary PCS arm so the integrity gate's escalation side effect is
        # not confounded into the comparison).
        want = "fixed_threshold" if fam == "ft" else "ca_tbl_no_integrity"
        sel = [r for r in rows if r["arm"] == want]
        if not sel:
            continue
        agg = []
        for r in sel:
            fixc = max(int(r["steps"]) - int(r["noFixCycles"]), 1)
            agg.append((float(r["cep50M"]), float(r["totalJ"]),
                        int(r["escalations"]) / fixc))
        n = len(agg)
        out.setdefault(fam, []).append((
            float(sig),
            want,
            sum(a[0] for a in agg) / n,
            sum(a[1] for a in agg) / n,
            sum(a[2] for a in agg) / n,
            n))
    for fam in out:
        out[fam].sort()
    return out


def pareto_front(points):
    """Minimise both error and joules. Returns the non-dominated subset.

    A point is dominated if another is no worse on BOTH axes and strictly better
    on at least one.
    """
    front = []
    for p in points:
        e, j = p[2], p[3]
        dominated = any((q[2] <= e and q[3] <= j) and (q[2] < e or q[3] < j)
                        for q in points if q is not p)
        if not dominated:
            front.append(p)
    return front


def analyse(outroot):
    data = collect(outroot)
    if not data:
        print(f"no sweep directories under {outroot}")
        return

    label = {"ft": "fixed_threshold (flat variance)",
             "v2": "ca_tbl_no_integrity (PCS v2)"}
    fronts = {}
    for fam in sorted(data):
        pts = data[fam]
        fronts[fam] = pareto_front(pts)
        print(f"\n=== {label.get(fam, fam)} — {len(pts)} operating points, "
              f"{pts[0][5]} seeds each")
        print(f"{'sigma_m':>9} {'cep50_m':>9} {'totalJ':>9} {'esc/fix':>9}  front")
        for p in pts:
            mark = "  *" if p in fronts[fam] else ""
            print(f"{p[0]:9.3g} {p[2]:9.3f} {p[3]:9.2f} {p[4]:9.4f}{mark}")

    if "ft" not in fronts or "v2" not in fronts:
        print("\nboth families needed for a front comparison")
        return

    # Domination test: does either front lie strictly inside the other? Reported
    # as the fraction of each front's points that are dominated by ANY point of
    # the other family -- not by the other FRONT, because a dominating point need
    # not itself be non-dominated within its own family.
    def dominated_frac(a_pts, b_pts):
        dom = sum(1 for a in a_pts
                  if any((b[2] <= a[2] and b[3] <= a[3])
                         and (b[2] < a[2] or b[3] < a[3]) for b in b_pts))
        return dom / max(len(a_pts), 1)

    fv = dominated_frac(fronts["v2"], data["ft"])
    vf = dominated_frac(fronts["ft"], data["v2"])
    print(f"\n--- front comparison")
    print(f"  {fv:.0%} of the PCS-v2 front is dominated by some flat-threshold point")
    print(f"  {vf:.0%} of the flat-threshold front is dominated by some PCS-v2 point")
    if fv == 0 and vf == 0:
        print("  VERDICT: neither front dominates -- at matched operating points the "
              "decision layer is EQUIVALENT to a flat variance threshold.")
    elif fv > vf:
        print("  VERDICT: the flat threshold front dominates. The confidence model "
              "does not pay for itself at any operating point tested.")
    else:
        print("  VERDICT: the PCS-v2 front dominates. Report the margin and the "
              "operating range over which it holds.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--plan", action="store_true")
    ap.add_argument("--analyse", metavar="OUTROOT")
    ap.add_argument("--seeds", type=int, default=20)
    ap.add_argument("--duration", type=int, default=3600)
    ap.add_argument("--outroot", default="results/pareto")
    a = ap.parse_args()
    if a.plan:
        for line in plan(a.seeds, a.duration, a.outroot):
            print(line)
    elif a.analyse:
        analyse(a.analyse)
    else:
        ap.print_help()


if __name__ == "__main__":
    main()
