#!/usr/bin/env python3
"""LIMITS (report/PREREG_LIMITS_2026-11-14.md): L1 tier-1 reach sensitivity and L2 the two-tier arm. Seeds 361-380.
Primary metric A(25 m) (scripts/goal_report.py), five-node and tag 0; secondary CEP95 tag 0 and subject energy.
Paired 95 % bootstrap intervals over the 20 seeds (scripts/camp_spec_analyse.ci). Written to <L1 dir>/LIMITS_L1.md and
<L2 dir>/LIMITS_L2.md. Committed with the registration, before any cell.

usage: limits_analyse.py <L1 outdir> <L2 outdir> [<sweep dir, default ~/ns/catbl-work/envsweep_2026-11-14>]
Either outdir may be '-' to skip that study.
"""
import csv
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

SEEDS = range(361, 381)
OFFS = [0, 15, 30]
L1ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4"]
F = "ca_tbl_full"


def load(dirpath, arm, seeds=SEEDS):
    out = {}
    for s in seeds:
        d = os.path.join(dirpath, f"s{s}")
        if os.path.isdir(d) and cs.ok(d):
            r = cell(d, arm)
            if r:
                out[s] = r
    return out


def pd(a, b, k):
    s = sorted(set(a) & set(b))
    return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3


def mean(x, k):
    return float(np.mean([r[k] for r in x.values()])) if x else float("nan")


def sidecars(top, problems, pat):
    srcs = set()
    for d in sorted(glob.glob(os.path.join(top, pat))):
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            srcs.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        if "UWB mode 850k:" not in lg or "ekf5 window smoother:" not in lg:
            problems.append(f"{d}: preset lines missing")
        if any(int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)):
            problems.append(f"{d}: window solver failures")
    if len(srcs) > 1:
        problems.append(f"more than one source: {sorted(srcs)}")
    return srcs


def l1(out, sweep):
    problems = []
    srcs = sidecars(out, problems, "[AB]_O*/s*")
    cells = sorted(glob.glob(os.path.join(out, "[AB]_O*", "s*")))
    if len(cells) != 80:
        problems.append(f"{len(cells)} cells, not 80")
    D = {}
    for c in "AB":
        for o in OFFS:
            for a in L1ARMS:
                p = os.path.join(sweep, f"{c}_E1") if o == 0 else os.path.join(out, f"{c}_O{o}")
                D[(c, o, a)] = load(p, a)
                if len(D[(c, o, a)]) != 20:
                    problems.append(f"{(c, o, a)}: {len(D[(c, o, a)])} seeds, not 20")
    md = ["# LIMITS L1 -- tier-1 reach sensitivity (scripts/limits_analyse.py)", "",
          f"Sources of the new cells: {', '.join(sorted(srcs)) or 'none'}. Offset 0 dB = the ENV-SWEEP's E1 cells "
          f"({sweep}), reused after the flag-off byte-identity check (VERIFY cell). A(25 m) = share of ALL cycles with "
          f"error <= 25 m, window 1800 s / 720 cycles per node, five-node pools tag 0 and four subjects.", "",
          "| config | offset dB | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 | CEP95 tag 0 (m) | subject J |",
          "|---|---|---|---|---|---|---|---|"]
    for c in "AB":
        for o in OFFS:
            for a in L1ARMS:
                x = D[(c, o, a)]
                if x:
                    arr = np.array(list(x.values()))
                    md.append(f"| {c} | {o} | {a} | {arr[:, 0].mean():.3f} | {arr[:, 1].mean():.3f} | "
                              f"{(arr[:, 1] >= 0.95).mean():.2f} | {np.nanmean(arr[:, 2]):.1f} | {np.nanmean(arr[:, 3]):.2f} |")
    md += ["", "## CA-TBL minus comparator, per offset", "",
           "| config | offset | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|---|"]
    for c in "AB":
        for o in OFFS:
            for comp in L1ARMS[1:]:
                T, O = D[(c, o, F)], D[(c, o, comp)]
                md.append(f"| {c} | {o} | {comp} | {cs.fmt(*pd(T, O, 1))} | {cs.fmt(*pd(T, O, 0))} | "
                          f"{cs.fmt(*pd(T, O, 2), p=1)} | {cs.fmt(*pd(T, O, 3), p=2)} |")
    md += ["", "## Each arm, 0 dB minus +30 dB (positive = the arm loses when tier-1 reach shrinks)", "",
           "| config | arm | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in "AB":
        for a in L1ARMS:
            md.append(f"| {c} | {a} | {cs.fmt(*pd(D[(c, 0, a)], D[(c, 30, a)], 1))} | {cs.fmt(*pd(D[(c, 0, a)], D[(c, 30, a)], 0))} | "
                      f"{cs.fmt(*pd(D[(c, 0, a)], D[(c, 30, a)], 2), p=1)} | {cs.fmt(*pd(D[(c, 0, a)], D[(c, 30, a)], 3), p=2)} |")
    p2 = {c: pd(D[(c, 0, "static_868")], D[(c, 30, "static_868")], 1) for c in "AB"}
    p3 = mean(D[("B", 30, F)], 1)
    p4 = mean(D[("A", 30, F)], 1)
    p5 = {(c, o): pd(D[(c, o, F)], D[(c, o, "static_868")], 1) for c in "AB" for o in OFFS}
    chain = [mean(D[("A", o, F)], 1) for o in OFFS]
    p7 = pd(D[("A", 0, "static_2p4")], D[("A", 30, "static_2p4")], 1)
    j0, j30 = mean(D[("A", 0, F)], 3), mean(D[("A", 30, F)], 3)
    v = lambda b: "HOLDS" if b else "FAILS"
    md += ["", "## Registered predictions (L1)", "",
           "- P1 probe r80 at +30 dB in [200, 320] m (seeds 362-364): read from the probe runs, see report/LIMITS_RESULTS_2026-11-14.md",
           f"- P2 static_868 five-node A(25 m) falls 0 -> +30 dB in A and B (paired interval above 0 in both): "
           f"{v(all(p2[c][1] > 0 for c in 'AB'))} (A {cs.fmt(*p2['A'])}, B {cs.fmt(*p2['B'])})",
           f"- P3 CA-TBL five-node A(25 m) in B at +30 dB >= 0.95: {v(p3 >= 0.95)} ({p3:.3f})",
           f"- P4 CA-TBL five-node A(25 m) in A at +30 dB < 0.95: {v(p4 < 0.95)} ({p4:.3f})",
           f"- P5 CA-TBL beats static_868 on five-node A(25 m) at every offset and config (interval above 0): "
           f"{v(all(x[1] > 0 for x in p5.values()))} (lowest lower bound {min(x[1] for x in p5.values()):.3f})",
           f"- P6 CA-TBL five-node A(25 m) in A non-increasing with offset: {v(chain[0] >= chain[1] >= chain[2])} "
           f"({' >= '.join(f'{x:.3f}' for x in chain)})",
           f"- P7 static_2p4 five-node A(25 m) in A also falls 0 -> +30 dB (interval above 0): {v(p7[1] > 0)} ({cs.fmt(*p7)})",
           f"- P8 CA-TBL subject J in A moves by less than 25 % of its 0 dB value at +30 dB: {v(abs(j30 - j0) < 0.25 * j0)} "
           f"({j0:.2f} -> {j30:.2f} J)", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "LIMITS_L1.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


def uwb_mech(d, arm):
    """tag-0 cycles on UWB: (n, with no UWB fix, with visUwb == 0), and cycles with a UWB fix, all cycles."""
    seed = int(d.rsplit("s", 1)[1])
    p = os.path.join(d, f"trace_{arm}_seed{seed}.csv")
    rs = list(csv.DictReader(open(p)))
    u = [r for r in rs if r["activeTier"] == "UWB"]
    return (len(u), sum(1 for r in u if r["fixTier"] != "UWB"), sum(1 for r in u if r["visUwb"] == "0"),
            sum(1 for r in rs if r["fixTier"] == "UWB"), len(rs))


def l2(out, sweep):
    problems = []
    srcs = sidecars(out, problems, "[AB]_E[0-3]/s*")
    cells = sorted(glob.glob(os.path.join(out, "[AB]_E[0-3]", "s*")))
    if len(cells) != 160:
        problems.append(f"{len(cells)} cells, not 160")
    T = "ca_tbl_two_tier"
    arms = [F, "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
    D = {}
    for c in "AB":
        for e in range(4):
            D[(c, e, T)] = load(os.path.join(out, f"{c}_E{e}"), T)
            if len(D[(c, e, T)]) != 20:
                problems.append(f"{(c, e, T)}: {len(D[(c, e, T)])} seeds, not 20")
            for a in arms:
                D[(c, e, a)] = load(os.path.join(sweep, f"{c}_E{e}"), a)
    md = ["# LIMITS L2 -- the two-tier arm (scripts/limits_analyse.py)", "",
          f"Sources of the two-tier cells: {', '.join(sorted(srcs)) or 'none'}. Comparator arms are the ENV-SWEEP's per-arm results "
          f"({sweep}), reused after the per-arm independence check (VERIFY cell). Window 1800 s / 720 cycles per node.", "",
          "| config | env | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 | CEP95 tag 0 (m) | subject J |", "|---|---|---|---|---|---|---|---|"]
    for c in "AB":
        for e in range(4):
            for a in [T] + arms:
                x = D[(c, e, a)]
                if x:
                    arr = np.array(list(x.values()))
                    md.append(f"| {c} | E{e} | {a} | {arr[:, 0].mean():.3f} | {arr[:, 1].mean():.3f} | "
                              f"{(arr[:, 1] >= 0.95).mean():.2f} | {np.nanmean(arr[:, 2]):.1f} | {np.nanmean(arr[:, 3]):.2f} |")
    md += ["", "## CA-TBL (ca_tbl_full) minus ca_tbl_two_tier, paired over seeds", "",
           "| config | env | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    dd = {}
    for c in "AB":
        for e in range(4):
            dd[(c, e)] = {k: pd(D[(c, e, F)], D[(c, e, T)], k) for k in range(4)}
            r = dd[(c, e)]
            md.append(f"| {c} | E{e} | {cs.fmt(*r[1])} | {cs.fmt(*r[0])} | {cs.fmt(*r[2], p=1)} | {cs.fmt(*r[3], p=2)} |")
    md += ["", "## Two-tier minus static_868, paired (is the UWB rung worth anything without tier 2?)", "",
           "| config | env | Δ A(25 m) five-node | Δ CEP95 tag 0 (m) |", "|---|---|---|---|"]
    for c in "AB":
        for e in range(4):
            md.append(f"| {c} | E{e} | {cs.fmt(*pd(D[(c, e, T)], D[(c, e, 'static_868')], 1))} | "
                      f"{cs.fmt(*pd(D[(c, e, T)], D[(c, e, 'static_868')], 2), p=1)} |")
    md += ["", "## Mechanism, tag 0 (both arms; trace_<arm>_seed<N>.csv, all 720 cycles per seed)", "",
           "UWB cycle = activeTier UWB. 'no UWB fix' = fixTier != UWB on it (the node fell back to the floor). "
           "'no anchor in reach' = visUwb == 0, truth-based count of UWB anchors within the deployment's nominal UWB range.", "",
           "| config | env | arm | share of cycles with a UWB fix | UWB cycles (pooled) | of which no UWB fix | of which no anchor in reach |",
           "|---|---|---|---|---|---|---|"]
    mech = {}
    for c in "AB":
        for e in range(4):
            for a, base in ((T, os.path.join(out, f"{c}_E{e}")), (F, os.path.join(sweep, f"{c}_E{e}"))):
                tot = np.zeros(5)
                ok = 0
                for s in SEEDS:
                    d = os.path.join(base, f"s{s}")
                    if os.path.isdir(d) and cs.ok(d) and os.path.exists(os.path.join(d, f"trace_{a}_seed{s}.csv")):
                        tot += np.array(uwb_mech(d, a))
                        ok += 1
                mech[(c, e, a)] = tot
                if ok:
                    n, nof, nvis, fixu, allc = tot
                    md.append(f"| {c} | E{e} | {a} | {fixu / allc:.3f} | {int(n)} | "
                              f"{(f'{nof / n:.3f}' if n else 'n/a (0 UWB cycles)')} | {(f'{nvis / n:.3f}' if n else 'n/a')} |")
    v = lambda b: "HOLDS" if b else "FAILS"
    a1 = dd[("A", 1)]
    q2 = [e for e in range(4) if dd[("A", e)][1][1] > 0]
    mA, mB = dd[("A", 1)][1][0], dd[("B", 1)][1][0]
    n, nof = mech[("A", 1, T)][0], mech[("A", 1, T)][1]
    md += ["", "## Registered predictions (L2)", "",
           f"- Q1 full - two-tier five-node A(25 m), A, E1 (forest): interval above 0: {v(a1[1][1] > 0)} ({cs.fmt(*a1[1])})",
           f"- Q2 the same in at least 3 of 4 environments in A: {v(len(q2) >= 3)} (environments with the interval above 0: "
           f"{', '.join(f'E{e}' for e in q2) or 'none'})",
           f"- Q3 the E1 difference is smaller in B than in A: {v(mB < mA)} (A {mA:.3f}, B {mB:.3f})",
           f"- Q4 CEP95 tag 0, A, E1: full better than two-tier (interval of full - two-tier below 0): {v(a1[2][2] < 0)} ({cs.fmt(*a1[2], p=1)} m)",
           f"- Q5 subject J, A, E1: two-tier cheaper than full (interval of full - two-tier above 0): {v(a1[3][1] > 0)} ({cs.fmt(*a1[3], p=2)} J)",
           f"- Q6 mechanism: at least half of two-tier's tag-0 UWB cycles in A, E1 have no UWB fix: "
           + (f"{v(nof / n >= 0.5)} ({int(nof)}/{int(n)} = {nof / n:.3f})" if n >= 100 else f"NOT TESTABLE ({int(n)} UWB cycles pooled, fewer than 100)"), ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "LIMITS_L2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    sweep = sys.argv[3] if len(sys.argv) > 3 else os.path.expanduser("~/ns/catbl-work/envsweep_2026-11-14")
    if sys.argv[1] != "-":
        l1(sys.argv[1], sweep)
    if sys.argv[2] != "-":
        l2(sys.argv[2], sweep)
    return 0


if __name__ == "__main__":
    sys.exit(main())
