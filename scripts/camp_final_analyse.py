#!/usr/bin/env python3
"""CAMP-FINAL analysis (report/PREREG_CAMP_FINAL.md). Reads <outdir>/<cell>/s<seed>/ and writes
<outdir>/CAMP_FINAL.md.

The tables CAMP-SPEC defined (the arm matrix with PCS against the trace control and the
sessions, the fault matrix, the single-hop baseline) are scripts/camp_spec_analyse.py's
functions, imported and applied unchanged; poucet_fusion is a seventh arm in the matrix. Added
here: the N8_ring sensitivity cell, the airtime numbers (the digest against full pushes, the
reverse path against flooding), the batch precision spread, and the registered predictions.

usage: camp_final_analyse.py <outdir>
"""
import csv
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402 -- CAMP-SPEC's tables, reused unchanged
import numpy as np  # noqa: E402

ARMS7 = cs.ARMS6 + ["poucet_fusion"]
DUR = cs.DUR


def uwb_fixes(d, seed, arm="ca_tbl_full"):
    """UWB-delivered fixes of <arm> over tag 0 and the subjects (trace fixTier UWB)."""
    n = 0
    fs = [os.path.join(d, f"trace_{arm}_seed{seed}.csv")] + \
        glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{seed}_subj*.csv"))
    for f in fs:
        if os.path.exists(f):
            n += sum(1 for r in csv.DictReader(open(f)) if r["hadFix"] == "1" and r["fixTier"] == "UWB")
    return n


def spreads(out, cell):
    """The base station's batch precision spread per seed: a number, or None when UNDEFINED."""
    v = []
    for d in cs.seeds(out, cell):
        if not cs.ok(d):
            continue
        m = re.search(r"TA-1 base-station session \(ca_tbl_full\)[^\n]*batch precision spread (\S+)",
                      open(os.path.join(d, "run.log")).read())
        if m:
            v.append(None if m.group(1) == "UNDEFINED" else float(m.group(1).rstrip("s")))
    return v


def airtime_rows(d, seed, arm="ca_tbl_full"):
    p = os.path.join(d, f"airtime_{arm}_seed{seed}.csv")
    if not os.path.exists(p):
        return None
    a = {}
    for r in csv.DictReader(open(p)):
        a[(int(r["tier"]), r["msgType"])] = float(r["airtimeS"])
    return a


def answers(d):
    m = re.search(r"TA-1 base-station session \(ca_tbl_full\): answers delivered (\d+)",
                  open(os.path.join(d, "run.log")).read())
    return int(m.group(1)) if m else None


def airtime(out, md, problems):
    md += ["## Airtime (ca_tbl_full; --reportAirtime)", "",
           "The digest against full pushes, per tier: saving = FULL_PUSH_CF - (DIGEST + ENTRY_PUSH), "
           "per seed, so > 0 means the digest exchange took less airtime than pushing the whole "
           "table at every digest would have.", "",
           "| cell | tier | DIGEST + ENTRY_PUSH (s) | full-push counterfactual (s) | saving (s), mean [95 %] | seeds |",
           "|---|---|---|---|---|---|"]
    res = {}
    for cell in ("A_main", "B_end"):
        for t in (0, 1):
            act, cf, sv = [], [], []
            for d in cs.seeds(out, cell):
                if not cs.ok(d):
                    continue
                a = airtime_rows(d, int(d.rsplit("s", 1)[1]))
                if a is None:
                    problems.append(f"{cell}/{os.path.basename(d)} no airtime file"); continue
                x = a.get((t, "DIGEST"), 0.0) + a.get((t, "ENTRY_PUSH"), 0.0)
                c = a.get((t, "FULL_PUSH_CF"), 0.0)
                act.append(x); cf.append(c); sv.append(c - x)
            if sv:
                m = cs.ci(sv)
                res[(cell, t)] = m
                md.append(f"| {cell} | {t} | {np.mean(act):.2f} | {np.mean(cf):.2f} | {cs.fmt(*m, 2)} | {len(sv)} |")
    md += ["", "The reverse path against flooding (5 km): A_main's ca_tbl_full against H_flood (the same preset "
           "with --reversePath=false), paired by seed. RESPONSE airtime summed over the tiers; per answer is that "
           "over the base station's answers delivered.", "",
           "| quantity | reverse path (A) | flooding (H) | A - H, mean [95 %] | seeds |", "|---|---|---|---|---|"]
    ra, rh, pa, ph, d1, d2 = [], [], [], [], [], []
    for d in cs.seeds(out, "A_main"):
        s = d.rsplit("s", 1)[1]
        h = os.path.join(out, "H_flood", f"s{s}")
        if not (cs.ok(d) and cs.ok(h)):
            continue
        aa, ah = airtime_rows(d, int(s)), airtime_rows(h, int(s))
        na, nh = answers(d), answers(h)
        if None in (aa, ah, na, nh):
            problems.append(f"airtime pair s{s} incomplete"); continue
        xa = sum(aa.get((t, "RESPONSE"), 0.0) for t in range(3))
        xh = sum(ah.get((t, "RESPONSE"), 0.0) for t in range(3))
        ra.append(xa); rh.append(xh); d1.append(xa - xh)
        if na > 0 and nh > 0:
            pa.append(xa / na); ph.append(xh / nh); d2.append(xa / na - xh / nh)
    if d1:
        res["resp"] = cs.ci(d1)
        md.append(f"| RESPONSE airtime (s) | {np.mean(ra):.2f} | {np.mean(rh):.2f} | {cs.fmt(*res['resp'], 2)} | {len(d1)} |")
    if d2:
        res["per"] = cs.ci(d2)
        md.append(f"| RESPONSE airtime per answer delivered (s) | {np.mean(pa):.3f} | {np.mean(ph):.3f} | "
                  f"{cs.fmt(*res['per'], 3)} | {len(d2)} |")
    md.append("")
    return res


def main():
    out = sys.argv[1]
    md, problems = ["# CAMP-FINAL — generated tables (scripts/camp_final_analyse.py)", ""], []
    A = cs.matrix(out, "A_main", ARMS7, md, problems)
    B = cs.matrix(out, "B_end", ARMS7, md, problems)
    C = cs.matrix(out, "C_n8", ["ca_tbl_full", "static_868"], md, problems)
    if "ca_tbl_full" in A and "ca_tbl_full" in C:
        md += ["Sensitivity geometry: the spec's three gateways (A_main) against N8_ring (C_n8), ca_tbl_full: "
               f"availability {np.mean(A['ca_tbl_full']['av']):.3f} vs {np.mean(C['ca_tbl_full']['av']):.3f}, CEP95 "
               f"{np.mean(A['ca_tbl_full']['c95']):.1f} vs {np.mean(C['ca_tbl_full']['c95']):.1f} m.", ""]
    for cell in ("A_main", "B_end"):
        sp = spreads(out, cell)
        dv = [x for x in sp if x is not None]
        md += [f"{cell}: batch precision spread defined on {len(dv)} of {len(sp)} seeds"
               + (f", median {np.median(dv):.0f} s" if dv else "") + ".", ""]
    uw = {}
    for cell in ("A_main", "B_end"):
        uw[cell] = [uwb_fixes(d, int(d.rsplit("s", 1)[1])) for d in cs.seeds(out, cell) if cs.ok(d)]
        md += [f"{cell}: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) {sum(uw[cell])}, on "
               f"{sum(1 for x in uw[cell] if x > 0)} of {len(uw[cell])} seeds.", ""]
    cs.faults(out, md)
    cs.single_hop(out, md)
    air = airtime(out, md, problems)
    toa = os.path.join(out, "T_toa", "s1", "toa_table.csv")
    md += [f"Time-on-air table: {toa if os.path.exists(toa) else 'MISSING'}"
           + (f" ({sum(1 for _ in open(toa)) - 1} rows)" if os.path.exists(toa) else ""), ""]
    md.append(f"Problems: {len(problems)}" + (f": {problems[:10]}" if problems else " (none)"))
    open(os.path.join(out, "CAMP_FINAL.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
