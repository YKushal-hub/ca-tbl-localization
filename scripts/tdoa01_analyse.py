#!/usr/bin/env python3
"""TDOA-01 analysis (report/PREREG_TDOA01.md). Reads <outdir>/<config>/s<seed>/tdoa_*.csv and
writes <outdir>/TDOA01.md and <outdir>/tdoa01_solves.csv. Applied unchanged after the runs.

usage: tdoa01_analyse.py <outdir>
"""
import csv
import glob
import math
import os
import sys

for _v in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")
import numpy as np  # noqa: E402

CONFIGS = ("p8", "p3", "end")
SEEDS = list(range(1, 21))
CHI2_95 = 5.991
NBOOT = 10000
RNG = np.random.default_rng(20261006)
VERDICTS = ("OK", "TOO_FEW", "COLLINEAR", "SINGULAR", "NO_CONVERGE", "BAD_INPUT", "NO_ROOT", "AMBIGUOUS")


def ok(d):
    p = os.path.join(d, "rc")
    return os.path.exists(p) and open(p).read().strip() == "0"


def nees(r):
    ex = float(r["tdoaX"]) - float(r["trueX"])
    ey = float(r["tdoaY"]) - float(r["trueY"])
    a, b, d = float(r["rxx"]), float(r["rxy"]), float(r["ryy"])
    det = a * d - b * b
    if not (det > 0 and a > 0 and d > 0):
        return float("nan")
    return (d * ex * ex - 2 * b * ex * ey + a * ey * ey) / det


def c95_ci(per_seed):
    """per_seed: list of (inside, total). Bootstrap over seeds of the pooled fraction."""
    v = [(i, t) for i, t in per_seed if t > 0]
    if not v:
        return float("nan"), float("nan"), float("nan")
    ins = np.array([i for i, _ in v], float)
    tot = np.array([t for _, t in v], float)
    point = ins.sum() / tot.sum()
    if len(v) < 2:
        return point, float("nan"), float("nan")
    idx = RNG.integers(0, len(v), size=(NBOOT, len(v)))
    b = ins[idx].sum(axis=1) / tot[idx].sum(axis=1)
    return float(point), float(np.quantile(b, 0.025)), float(np.quantile(b, 0.975))


def f3(t):
    m, lo, hi = t
    return "n/a" if math.isnan(m) else (f"{m:.3f}" if math.isnan(lo) else f"{m:.3f} [{lo:.3f}, {hi:.3f}]")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    solves = []
    att = {c: {v: 0 for v in VERDICTS} for c in CONFIGS}
    att3 = {c: {v: 0 for v in VERDICTS} for c in CONFIGS}
    for c in CONFIGS:
        for s in SEEDS:
            d = os.path.join(out, c, f"s{s}")
            if not ok(d):
                problems.append(f"{c}/s{s} missing or non-zero exit")
                continue
            fs = glob.glob(os.path.join(d, "tdoa_*.csv"))
            if len(fs) != 2:
                problems.append(f"{c}/s{s}: {len(fs)} tdoa files (need 2)")
            for f in fs:
                arm = os.path.basename(f)[len("tdoa_"):].rsplit("_seed", 1)[0]
                for r in csv.DictReader(open(f)):
                    v = r["verdict"]
                    att[c][v] = att[c].get(v, 0) + 1
                    if r["nStamps"] == "3":
                        att3[c][v] = att3[c].get(v, 0) + 1
                    if v == "OK":
                        solves.append(dict(config=c, seed=s, arm=arm, t=r["t"], nStamps=int(r["nStamps"]),
                                           seededBy=r["seededBy"], nees=nees(r), errM=float(r["tdoaErrM"])))
    with open(os.path.join(out, "tdoa01_solves.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["config", "seed", "arm", "t", "nStamps", "seededBy", "nees", "errM"],
                           lineterminator="\n")
        w.writeheader()
        w.writerows(solves)

    def cov(sel):
        per = {}
        for x in sel:
            if math.isnan(x["nees"]):
                continue
            i, t = per.get(x["seed"], (0, 0))
            per[x["seed"]] = (i + (x["nees"] <= CHI2_95), t + 1)
        return c95_ci([per[s] for s in sorted(per)])

    md = ["# TDOA-01 -- the registered analysis, applied unchanged", "",
          "NEES = e' R^-1 e over OK solves (chi^2_2 if R is right; median 1.386, mean 2). C95 = share with NEES <= 5.991, "
          "seed-bootstrap 95 % interval (10 000 resamples, seed 20261006).", ""]
    if problems:
        md += ["**Problems:**", ""] + [f"- {p}" for p in problems] + [""]
    md += ["| config | OK solves | NEES median / mean | C95 | C95, exactly 3 stamps | C95, 4+ stamps | share of OK with 3 stamps | (AMBIGUOUS + NO_ROOT) / 3-stamp attempts |",
           "|---|---|---|---|---|---|---|---|"]
    res = {}
    for c in CONFIGS:
        sc = [x for x in solves if x["config"] == c]
        ne = [x["nees"] for x in sc if not math.isnan(x["nees"])]
        c_all, c_3, c_4 = cov(sc), cov([x for x in sc if x["nStamps"] == 3]), cov([x for x in sc if x["nStamps"] >= 4])
        share3 = (sum(1 for x in sc if x["nStamps"] == 3) / len(sc)) if sc else float("nan")
        a3 = sum(att3[c].values())
        refuse = ((att3[c].get("AMBIGUOUS", 0) + att3[c].get("NO_ROOT", 0)) / a3) if a3 else float("nan")
        res[c] = dict(n=len(sc), c_all=c_all, c_3=c_3, c_4=c_4, share3=share3, refuse=refuse)
        md.append(f"| {c} | {len(sc)}{' (under-populated)' if len(sc) < 100 else ''} | "
                  f"{(np.median(ne) if ne else float('nan')):.3f} / {(np.mean(ne) if ne else float('nan')):.3f} | "
                  f"{f3(c_all)} | {f3(c_3)} | {f3(c_4)} | {share3:.3f} | {refuse:.3f} ({a3} attempts) |")
    md += ["", "Attempts by verdict:", "", "| config | " + " | ".join(VERDICTS) + " |", "|---|" + "---|" * len(VERDICTS)]
    for c in CONFIGS:
        md.append(f"| {c} | " + " | ".join(str(att[c].get(v, 0)) for v in VERDICTS) + " |")
    pooled3 = cov([x for x in solves if x["nStamps"] == 3])
    pooled4 = cov([x for x in solves if x["nStamps"] >= 4])
    md += ["", f"Pooled over all configurations: C95 exactly 3 stamps {f3(pooled3)}, 4+ stamps {f3(pooled4)}.", "",
           "## Predictions (report/PREREG_TDOA01.md sect 3)", ""]
    p1 = (not math.isnan(res["p8"]["c_all"][1])) and 0.90 <= res["p8"]["c_all"][1] and res["p8"]["c_all"][2] <= 0.99
    p2 = res["p3"]["share3"] >= 0.80
    p3 = res["p3"]["refuse"] >= 0.10
    p4 = pooled3[0] < pooled4[0]
    md += [f"- P1 (p8: the C95 interval inside [0.90, 0.99]): {'HELD' if p1 else 'FAILED'}",
           f"- P2 (p3: >= 80 % of OK solves with exactly 3 stamps): {'HELD' if p2 else 'FAILED'} ({res['p3']['share3']:.3f})",
           f"- P3 (p3: AMBIGUOUS + NO_ROOT >= 10 % of 3-stamp attempts): {'HELD' if p3 else 'FAILED'} ({res['p3']['refuse']:.3f})",
           f"- P4 (pooled: C95 of 3-stamp solves below 4+): {'HELD' if p4 else 'FAILED'}", ""]
    open(os.path.join(out, "TDOA01.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
