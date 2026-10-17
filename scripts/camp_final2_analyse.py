#!/usr/bin/env python3
"""CAMP-FINAL2 analysis (report/PREREG_CAMP_FINAL2.md). usage: camp_final2_analyse.py <outdir>

CAMP-FINAL's tables (scripts/camp_final_analyse.py, which imports CAMP-SPEC's) applied unchanged, plus the post-acquisition
CEP95 (scripts/post_acq_metrics.py, B14), the calibrated-Wolf endgame cell W_end, and the registered predictions Q1..Q10.
Writes <outdir>/CAMP_FINAL2.md.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_final_analyse as cf  # noqa: E402
import camp_spec_analyse as cs  # noqa: E402
from post_acq_metrics import post_acq  # noqa: E402
import numpy as np  # noqa: E402


def col(out, cell, arm, key):
    v = []
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            r = cs.arm_row(d, arm)
            if r is not None:
                v.append(float(r[key]))
    return v


def pacq(out, cell, arm):
    return [post_acq(d, arm)[2] for d in cs.seeds(out, cell) if cs.ok(d)]


def main():
    out = sys.argv[1]
    md, problems = ["# CAMP-FINAL2 — generated tables (scripts/camp_final2_analyse.py)", ""], []
    A = cs.matrix(out, "A_main", cf.ARMS7, md, problems)
    B = cs.matrix(out, "B_end", cf.ARMS7, md, problems)
    C = cs.matrix(out, "C_n8", ["ca_tbl_full", "static_868"], md, problems)
    W = cs.matrix(out, "W_end", ["ca_tbl_full", "static_2p4", "static_868"], md, problems)
    md += ["## CEP95 after acquisition (B14: tag 0 fixes after 300 s; mean over seeds, NaN-aware)", "",
           "| cell | arm | CEP95 (m) | CEP95 after 300 s (m) | tag-0 J |", "|---|---|---|---|---|"]
    PA = {}
    for cell, arms in (("A_main", cf.ARMS7), ("B_end", cf.ARMS7), ("C_n8", ["ca_tbl_full", "static_868"]),
                       ("W_end", ["ca_tbl_full", "static_2p4", "static_868"])):
        for arm in arms:
            c95, pa, j = col(out, cell, arm, "cep95M"), pacq(out, cell, arm), col(out, cell, arm, "totalJ")
            PA[(cell, arm)] = (c95, pa, j)
            if c95:
                md.append(f"| {cell} | {arm} | {np.mean(c95):.1f} | {np.nanmean(pa):.1f} | {np.mean(j):.2f} |")
    md.append("")
    for cell in ("A_main", "B_end"):
        sp = cf.spreads(out, cell)
        dv = [x for x in sp if x is not None]
        md += [f"{cell}: batch precision spread defined on {len(dv)} of {len(sp)} seeds"
               + (f", median {np.median(dv):.0f} s" if dv else "") + ".", ""]
    uw = {c: [cf.uwb_fixes(d, int(d.rsplit("s", 1)[1])) for d in cs.seeds(out, c) if cs.ok(d)] for c in ("A_main", "B_end")}
    for c in uw:
        md += [f"{c}: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) {sum(uw[c])}, on {sum(1 for x in uw[c] if x > 0)} of {len(uw[c])} seeds.", ""]
    cs.faults(out, md)
    cs.single_hop(out, md)
    air = cf.airtime(out, md, problems)
    toa = os.path.join(out, "T_toa", "s1", "toa_table.csv")
    md += [f"Time-on-air table: {toa if os.path.exists(toa) else 'MISSING'}", ""]
    # Registered predictions evaluated mechanically here (Q1, Q6, Q7, Q8, Q9, Q10); Q2-Q5 are read from the tables above.
    q = []
    q.append(("Q1", sum(uw["A_main"]) == 0, f"A UWB-delivered fixes {sum(uw['A_main'])}"))
    if (("A_main", 0) in air):
        m = air[("A_main", 0)]; q.append(("Q6", m[2] < 0, f"A 868 digest saving {m[0]:.2f} [{m[1]:.2f}, {m[2]:.2f}] s"))
    if "per" in air:
        m = air["per"]; q.append(("Q7", m[1] > 0, f"A - H RESPONSE per answer {m[0]:.3f} [{m[1]:.3f}, {m[2]:.3f}] s"))
    fa, fs = A.get("ca_tbl_full", {}).get("av"), A.get("static_868", {}).get("av")  # CAMP-SPEC's availability (subjects' fix share)
    if fa and fs:
        q.append(("Q8", np.mean(fa) - np.mean(fs) > 0.02, f"A availability ca_tbl_full {np.mean(fa):.3f} vs static_868 {np.mean(fs):.3f}"))
    a, s8 = PA[("A_main", "ca_tbl_full")], PA[("A_main", "static_868")]
    if a[1] and s8[1]:
        d = [x - y for x, y in zip(a[1], s8[1]) if np.isfinite(x) and np.isfinite(y)]
        m = cs.ci(d); q.append(("Q9", m[2] < 0, f"A post-acq CEP95 ca_tbl_full - static_868 {m[0]:+.1f} [{m[1]:+.1f}, {m[2]:+.1f}] m"))
    bf, b2 = PA[("B_end", "ca_tbl_full")], PA[("B_end", "static_2p4")]
    if bf[0] and b2[0]:
        ok = np.mean(b2[0]) <= 2 * np.mean(bf[0]) and np.mean(b2[2]) <= 0.5 * np.mean(bf[2])
        q.append(("Q10", ok, f"B static_2p4 CEP95 {np.mean(b2[0]):.1f} vs ca_tbl_full {np.mean(bf[0]):.1f} m; J {np.mean(b2[2]):.2f} vs {np.mean(bf[2]):.2f}"))
    md += ["## Predictions evaluated by this script", ""] + [f"- **{k}**: {'held' if v else 'not held'} -- {t}" for k, v, t in q] + [""]
    md.append(f"Problems: {len(problems)}" + (f": {problems[:10]}" if problems else " (none)"))
    open(os.path.join(out, "CAMP_FINAL2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
