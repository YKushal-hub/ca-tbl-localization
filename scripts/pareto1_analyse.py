#!/usr/bin/env python3
"""PARETO-1 (report/PREREG_PARETO1.md): the predicted-benefit hand-off rule (TII-1) against every simple rule, on the
accuracy-energy plane. Seeds 401-420; configurations A (5 km, full site) and B (endgame); the preset's 12 gateways.
Committed with the registration, before any cell.

Cells (directory <G>_<cell>/s<seed>; one arm each unless BASE):
  BASE      ca_tbl_full, fixed_threshold (12 m^2), static_868, static_2p4, lora_fusion, poucet_fusion
  FT<T>     fixed_threshold, --fixedVarThreshold=T                       (the flat-threshold family)
  OPP       ca_tbl_full, --c2bEscalation=true                           (opportunistic: escalate when in reach)
  D868      fixed_threshold, --dualBase868OnNeed=true                   (skip 868 MHz when 2.4 GHz solves)
  BEN<L>    fixed_threshold, --benefitLambda=L --benefitSkip868=true    (the predicted-benefit rule, TII-1)
  BNS<L>    fixed_threshold, --benefitLambda=L                          (ablation: rule without the 868 MHz skip)

Decision rule (P1, the go/no-go): S = the simple rules (BASE ca_tbl_full, fixed_threshold, static_2p4, lora_fusion; every
FT, OPP, D868). FEASIBLE = mean five-worker A(25 m) >= 0.95 in A and in B. E* = the feasible simple rule with the lowest
mean subject energy in A. GO if some BEN<L> is feasible and, against E* in A, its paired subject-energy difference has
an upper 95 % bound below 0 with mean energy <= 0.80 x E*'s, and its paired five-worker A(25 m) difference has a lower
bound >= -0.02 in A and in B. If no simple rule is feasible, GO needs a feasible BEN<L>.

usage: pareto1_analyse.py <outdir>
"""
import csv
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell as goal_cell  # noqa: E402

SEEDS = list(range(401, 421))
GEOS = ["A", "B"]
FT = [3, 30, 100, 300]
LAMBDAS = ["0.01", "0.1", "1", "10", "100", "1000"]
BNS = ["100"]
BASE_ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "lora_fusion", "poucet_fusion"]
# (cell directory, arm, label)
ITEMS = [("BASE", a, a if a != "fixed_threshold" else "FT12") for a in BASE_ARMS]
ITEMS += [(f"FT{t}", "fixed_threshold", f"FT{t}") for t in FT]
ITEMS += [("OPP", "ca_tbl_full", "OPP"), ("D868", "fixed_threshold", "D868")]
ITEMS += [(f"BEN{l}", "fixed_threshold", f"BEN{l}") for l in LAMBDAS]
ITEMS += [(f"BNS{l}", "fixed_threshold", f"BNS{l}") for l in BNS]
SIMPLE = ["ca_tbl_full", "FT12", "static_2p4", "lora_fusion"] + [f"FT{t}" for t in FT] + ["OPP", "D868"]
CELLS = sorted({c for c, _, _ in ITEMS})


def t25(d, arm):
    """Seconds to the lost worker's first cycle within 25 m (NaN if never)."""
    seed = re.search(r"s(\d+)$", d).group(1)
    p = os.path.join(d, f"trace_{arm}_seed{seed}.csv")
    if not os.path.exists(p):
        return float("nan")
    for i, r in enumerate(csv.DictReader(open(p))):
        if float(r["errM"]) <= 25.0:
            return 2.5 * (i + 1)
    return float("nan")


def row(d, arm):
    """(tag-0 A25, five A25, CEP95, subject J, t25 s, tFirstFix s) or None."""
    r = goal_cell(d, arm)
    if not r:
        return None
    ttff = float("nan")
    for a in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if a["arm"] == arm:
            ttff = float(a.get("tFirstFixS", "nan"))
    return tuple(r) + (t25(d, arm), ttff)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    D = {}
    for g in GEOS:
        for c in CELLS:
            for s in SEEDS:
                d = os.path.join(out, f"{g}_{c}", f"s{s}")
                if not cs.ok(d):
                    problems.append(f"{d}: missing or rc not 0")
                    continue
                m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
                if not m or m.group(2) != "0":
                    problems.append(f"{d}: run_id.txt not dirty 0")
                else:
                    sources.add(m.group(1))
                lg = open(os.path.join(d, "run.log"), errors="replace").read()
                if c.startswith("BEN") and "TII-1 benefitSkip868 (fixed_threshold" not in lg:
                    problems.append(f"{d}: TII-1 line missing")
        for c, a, lab in ITEMS:
            D[(g, lab)] = {}
            for s in SEEDS:
                d = os.path.join(out, f"{g}_{c}", f"s{s}")
                if cs.ok(d):
                    r = row(d, a)
                    if r:
                        D[(g, lab)][s] = r
            if len(D[(g, lab)]) != len(SEEDS):
                problems.append(f"{(g, lab)}: {len(D[(g, lab)])} seeds, not {len(SEEDS)}")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")

    def mean(g, lab, k):
        x = D.get((g, lab), {})
        return float(np.nanmean([r[k] for r in x.values()])) if x else float("nan")

    def pr(g, a, b, k):
        x, y = D.get((g, a), {}), D.get((g, b), {})
        s = sorted(set(x) & set(y))
        return cs.ci([x[i][k] - y[i][k] for i in s]) if s else (float("nan"),) * 3

    labels = [lab for _, _, lab in ITEMS]
    md = ["# PARETO-1 — seeds 401-420 (scripts/pareto1_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for g in GEOS:
        md += [f"## {g} ({'5 km' if g == 'A' else 'endgame'})", "",
               "| rule | A(25 m) five | A(25 m) lost | CEP95 lost (m) | subject J | t to 25 m (s) | TTFF (s) |",
               "|---|---|---|---|---|---|---|"]
        for lab in labels:
            md.append(f"| {lab} | {mean(g, lab, 1):.3f} | {mean(g, lab, 0):.3f} | {mean(g, lab, 2):.1f} | {mean(g, lab, 3):.2f} | "
                      f"{mean(g, lab, 4):.0f} | {mean(g, lab, 5):.0f} |")
        md.append("")
    feas = {lab: all(mean(g, lab, 1) >= 0.95 for g in GEOS) for lab in labels}
    simple_feas = [lab for lab in SIMPLE if feas[lab]]
    estar = min(simple_feas, key=lambda lab: mean("A", lab, 3)) if simple_feas else None
    md += ["## Decision (P1)", "", f"Feasible simple rules: {simple_feas or 'none'}; E* = {estar}"
           + (f" ({mean('A', estar, 3):.2f} J in A)" if estar else ""), ""]
    md += ["| BEN | feasible | J ratio to E* (A) | ΔJ vs E* (A) | ΔA five vs E* (A) | ΔA five vs E* (B) | passes |", "|---|---|---|---|---|---|---|"]
    go = False
    for l in LAMBDAS:
        lab = f"BEN{l}"
        if estar:
            ratio = mean("A", lab, 3) / mean("A", estar, 3)
            dj = pr("A", lab, estar, 3)
            da = {g: pr(g, lab, estar, 1) for g in GEOS}
            ok = feas[lab] and dj[2] < 0 and ratio <= 0.80 and all(da[g][1] >= -0.02 for g in GEOS)
            md.append(f"| {lab} | {feas[lab]} | {ratio:.3f} | {cs.fmt(*dj, p=2)} | {cs.fmt(*da['A'])} | {cs.fmt(*da['B'])} | {ok} |")
        else:
            ok = feas[lab]
            md.append(f"| {lab} | {feas[lab]} | n/a | n/a | n/a | n/a | {ok} |")
        go = go or ok
    md += ["", f"**P1: {'GO' if go else 'NO-GO'}**", ""]
    md += ["## Paired contrasts against FT12 (the flat threshold) and ca_tbl_full", "",
           "| geometry | rule | ΔA five vs FT12 | ΔJ vs FT12 | ΔA five vs ca_tbl_full | ΔJ vs ca_tbl_full |", "|---|---|---|---|---|---|"]
    for g in GEOS:
        for lab in labels:
            if lab in ("FT12",):
                continue
            md.append(f"| {g} | {lab} | {cs.fmt(*pr(g, lab, 'FT12', 1))} | {cs.fmt(*pr(g, lab, 'FT12', 3), p=2)} | "
                      f"{cs.fmt(*pr(g, lab, 'ca_tbl_full', 1))} | {cs.fmt(*pr(g, lab, 'ca_tbl_full', 3), p=2)} |")
    md += ["", f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "PARETO1.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
