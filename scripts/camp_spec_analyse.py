#!/usr/bin/env python3
"""CAMP-SPEC analysis (report/PREREG_CAMP_SPEC.md). Reads <outdir>/<cell>/s<seed>/ and writes
<outdir>/CAMP_SPEC.md and camp_spec_cells.csv.

usage: camp_spec_analyse.py <outdir>
"""
import csv
import glob
import math
import os
import re
import sys

for _v in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")
import numpy as np  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cal01_loocv import spearman  # noqa: E402

DUR = 1800.0
FRAC = 0.25
NBOOT = 10000
RNG = np.random.default_rng(20261004)
ARMS6 = ["ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold", "static_868", "static_2p4", "static_uwb"]


def seeds(out, cell):
    return sorted(glob.glob(os.path.join(out, cell, "s*")), key=lambda p: int(p.rsplit("s", 1)[1]))


def ok(d):
    p = os.path.join(d, "rc")
    return os.path.exists(p) and open(p).read().strip() == "0"


def arm_row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r
    return None


def subj_rows(d, arm):
    p = os.path.join(d, "subject_summary.csv")
    return [r for r in csv.DictReader(open(p)) if r["arm"] == arm] if os.path.exists(p) else []


def trace(d, arm):
    seed = int(d.rsplit("s", 1)[1])
    p = os.path.join(d, f"trace_{arm}_seed{seed}.csv")
    return list(csv.DictReader(open(p))) if os.path.exists(p) else []


def ci(v):
    v = np.asarray([x for x in v if not math.isnan(x)])
    if len(v) == 0:
        return float("nan"), float("nan"), float("nan")
    if len(v) == 1:
        return float(v[0]), float("nan"), float("nan")
    m = v[RNG.integers(0, len(v), size=(NBOOT, len(v)))].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def fmt(m, lo, hi, p=3):
    if math.isnan(m):
        return "n/a"
    return f"{m:.{p}f}" if math.isnan(lo) else f"{m:.{p}f} [{lo:.{p}f}, {hi:.{p}f}]"


def matrix(out, cell, arms, md, problems):
    md += [f"## {cell}", "", "| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |",
           "|---|---|---|---|---|---|---|"]
    res = {}
    for arm in arms:
        av, c50, c95, s95, tff, occ, J = [], [], [], [], [], [], []
        for d in seeds(out, cell):
            if not ok(d):
                problems.append(f"{cell}/{os.path.basename(d)} exit"); continue
            a = arm_row(d, arm)
            ss = subj_rows(d, arm)
            if a is None:
                continue
            st = float(a["steps"])
            av.append(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss]) if ss else float(a["fixCycles"]) / st)
            c50.append(float(a["cep50M"])); c95.append(float(a["cep95M"]))
            s95.append(np.mean([float(r["cep95M"]) for r in ss]) if ss else float("nan"))
            t = float(a["tFirstFixS"]); tff.append(t if t >= 0 else DUR)
            occ.append([float(a[k]) / st for k in ("cycles_868", "cycles_2p4", "cycles_uwb")])
            J.append(np.mean([float(r["totalJ"]) for r in ss]) if ss else float(a["totalJ"]))
        if not av:
            continue
        o = np.mean(occ, axis=0)
        res[arm] = dict(av=av, c95=c95, tff=tff)
        md.append(f"| {arm} | {fmt(*ci(av))} | {np.mean(c50):.1f} / {np.mean(c95):.1f} | {np.nanmean(s95):.1f} | {np.median(tff):.0f} | "
                  f"{o[0]:.3f} / {o[1]:.3f} / {o[2]:.3f} | {np.mean(J):.2f} |")
    # PCS against the trace control, per seed, on the confidence-scored arms
    md += ["", "PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):", ""]
    for arm in ("ca_tbl_full", "ca_tbl_no_integrity"):
        A = []
        for d in seeds(out, cell):
            rows = [r for r in trace(d, arm) if r["hadFix"] == "1" and float(r["gdop"]) != 0.0]
            if len(rows) < 30:
                continue
            err = [float(r["errM"]) for r in rows]
            rp = spearman([float(r["pcs"]) for r in rows], err)
            rc = spearman([-float(r["traceSigma"]) for r in rows], err)
            A.append(rc - rp)
        if A:
            m, lo, hi = ci(A)
            v = "BETTER" if lo > 0 else ("WORSE" if hi < 0 else "INDISTINGUISHABLE")
            md.append(f"- {arm}: A = {fmt(m, lo, hi, 4)} over {len(A)} seeds -> {v}")
    # sessions: time to first coarse / precise and the batch spread
    tc, tp, spread = [], [], []
    for d in seeds(out, cell):
        log = open(os.path.join(d, "run.log")).read()
        m = re.search(r"TA-1 base-station session \(ca_tbl_full\)[^\n]*first coarse/precise \(s\): ([^;]*); batch precision spread (\S+)", log)
        if not m:
            continue
        for a, b in re.findall(r"m\d+ ([-\d.]+)/([-\d.]+)", m.group(1)):
            tc.append(float(a) if float(a) >= 0 else DUR); tp.append(float(b) if float(b) >= 0 else DUR)
        spread.append(m.group(2))
    if tc:
        und = sum(1 for s in spread if s == "UNDEFINED")
        md += ["", f"Base-station batch session: time to first coarse median {np.median(tc):.0f} s, to precise median {np.median(tp):.0f} s "
               f"(never = {DUR:.0f}); precision spread UNDEFINED on {und} of {len(spread)} seeds.", ""]
    return res


def routing(out, md):
    md += ["## D: the NW-2 routing-weight sweep (endgame, ca_tbl_full)", "",
           "| weights a/b/g/slack | delivered / originated (subjects) | diverged hops | seeds |", "|---|---|---|---|"]
    best = None
    for cell in sorted(glob.glob(os.path.join(out, "D_route_*"))):
        name = os.path.basename(cell)[len("D_route_"):]
        ratios, div, n = [], 0, 0
        for d in seeds(out, os.path.basename(cell)):
            if not ok(d):
                continue
            orig = dlv = 0
            for f in glob.glob(os.path.join(d, "fix_routing_*_tier*.csv")):
                for r in csv.DictReader(open(f)):
                    if 1 <= int(r["originId"]) <= 5:
                        orig += r["hop"] == "0"
                        dlv += r["outcome"] == "delivered"
                        div += r["diverged"] == "1"
            if orig:
                ratios.append(dlv / orig); n += 1
        if ratios:
            m = float(np.mean(ratios))
            md.append(f"| {name} | {m:.3f} | {div} | {n} |")
            if best is None or m > best[1] + 0.01 or (abs(m - best[1]) <= 0.01 and name == "a1_b1_g1_s1"):
                best = (name, m)
    if best:
        md += ["", f"Registered criterion (highest delivery ratio; within 0.01 the default wins): **{best[0]}** ({best[1]:.3f}).", ""]


def faults(out, md):
    md += ["## E: the SY-5 fault matrix (tag 0; label = the fault window)", "",
           "| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |",
           "|---|---|---|---|---|---|---|---|"]
    for cell in sorted(glob.glob(os.path.join(out, "E_*"))):
        name = os.path.basename(cell)[2:]
        fault, rec = name.rsplit("_", 1)
        for arm in ("ca_tbl_full", "ca_tbl_no_integrity"):
            tp = fp = fn = 0
            ein, eout, n = [], [], 0
            for d in seeds(out, os.path.basename(cell)):
                if not ok(d):
                    continue
                seed = int(d.rsplit("s", 1)[1])
                w0 = DUR * (0.25 + 0.5 * (seed % 7) / 7.0)
                w1 = w0 + FRAC * DUR
                n += 1
                for r in trace(d, arm):
                    if r["hadFix"] != "1":
                        continue
                    t = float(r["t"]); inw = w0 <= t < w1; sus = r["suspect"] == "1"
                    tp += inw and sus; fp += (not inw) and sus; fn += inw and not sus
                    (ein if inw else eout).append(float(r["errM"]))
            prec = tp / (tp + fp) if tp + fp else float("nan")
            rcl = tp / (tp + fn) if tp + fn else float("nan")
            q = lambda v: float(np.quantile(v, 0.95)) if v else float("nan")
            md.append(f"| {fault} | {rec} | {arm} | {prec:.3f} | {rcl:.3f} | {q(ein):.1f} | {q(eout):.1f} | {n} |")
    md.append("")


def single_hop(out, md):
    md += ["## G: the single-hop confidence baseline (LoRaWAPS-style) against the preset (ca_tbl_full, tag 0)", "",
           "| configuration | hop depth of the fused set | fix cycles | median error (m) | rho(PCS, error) |", "|---|---|---|---|---|"]
    for label, cell, base in (("preset 5 km", "A_main", False), ("preset endgame", "B_end", False),
                              ("single-hop 5 km", "G_single", True), ("single-hop endgame", "G_single_end", True)):
        by = {}
        for d in seeds(out, cell):
            if not ok(d):
                continue
            seed = int(d.rsplit("s", 1)[1])
            tr = {r["t"]: r for r in trace(d, "ca_tbl_full") if r["hadFix"] == "1"}
            hp = os.path.join(d, f"hops_ca_tbl_full_seed{seed}.csv")
            hops = {r["t"]: int(r["maxHop"]) for r in csv.DictReader(open(hp))} if os.path.exists(hp) else {}
            for t, r in tr.items():
                by.setdefault(0 if base else hops.get(t, 0), []).append((float(r["errM"]), float(r["pcs"])))
        for h in sorted(by):
            v = by[h]
            md.append(f"| {label} | {h} | {len(v)} | {np.median([e for e, _ in v]):.1f} | {spearman([p for _, p in v], [e for e, _ in v]):.3f} |")
    md.append("")


def main():
    out = sys.argv[1]
    md, problems = ["# CAMP-SPEC — generated tables (scripts/camp_spec_analyse.py)", ""], []
    A = matrix(out, "A_main", ARMS6, md, problems)
    B = matrix(out, "B_end", ARMS6, md, problems)
    C = matrix(out, "C_runnerup", ["ca_tbl_full", "static_868"], md, problems)
    if "ca_tbl_full" in A and "ca_tbl_full" in C:
        md += ["GEO-01 stage 3: preset geometry (A_main) against the runner-up (C_runnerup), ca_tbl_full: availability "
               f"{np.mean(A['ca_tbl_full']['av']):.3f} vs {np.mean(C['ca_tbl_full']['av']):.3f}, CEP95 "
               f"{np.mean(A['ca_tbl_full']['c95']):.1f} vs {np.mean(C['ca_tbl_full']['c95']):.1f} m.", ""]
    routing(out, md)
    faults(out, md)
    single_hop(out, md)
    md.append(f"Problems: {len(problems)}" + (f": {problems[:10]}" if problems else " (none)"))
    open(os.path.join(out, "CAMP_SPEC.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
