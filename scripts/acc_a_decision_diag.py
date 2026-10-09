#!/usr/bin/env python3
"""Post-hoc decision diagnosis of ACC-A's traces (report/ACC_A_DECISION_DIAGNOSIS.md): what decides
tag 0's tier in ca_tbl_full and fixed_threshold, cycle by cycle. Reads only; exploratory, not
registered.

usage: acc_a_decision_diag.py <outdir written by scripts/acc_a_run.sh>

A decision row is a trace row on which Step() reached the decision: hadFix 1 and nAnchors >= 3.
Two-range rows (Case A/B, solved in the scenario) and coasts take the dropout path, which computes
no PCS and logs 0. An escalation on a decision row whose own rule condition is false, or on any
other row, came from the NW-4 URGENT-query hook: the only other writer of `escalated` in Step() is
suspectForcesEscalation, off in the preset. UWB grants that land later change the tier without
setting `escalated` and are not counted here.
"""
import csv
import os
import sys
from collections import Counter

ARMS = ("ca_tbl_full", "fixed_threshold")
CELLS = ("M_A_k1", "M_A_k10", "M_B_k1", "M_B_k10")
W1, W3, TREF = 2.994675, 0.005325, 10.0  # spec_profile.txt: CAL-03 fit, traceRef default
UP = {"868MHz": 1.64235, "2.4GHz": 1.376873}
DOWN = {"2.4GHz": 1.542098, "UWB": 1.880221}
FIXED_UP, FIXED_HEALTHY = 100.0, 50.0  # CaTblPolicyConfig default; --fixedVarThreshold unset


def trace_equiv(p, t3):
    return TREF / ((p - W3 * t3) / W1) - TREF


def up(arm, r):
    if arm == "fixed_threshold":
        return float(r["traceSigma"]) > FIXED_UP
    return float(r["pcs"]) < UP[r["activeTier"]]


def healthy(arm, r):
    if arm == "fixed_threshold":
        return float(r["traceSigma"]) < FIXED_HEALTHY
    return r["suspect"] == "0" and float(r["pcs"]) > DOWN[r["activeTier"]]


def cell_counts(out, cell, arm):
    c = Counter()
    for s in range(1, 21):
        p = os.path.join(out, cell, f"s{s}", f"trace_{arm}_seed{s}.csv")
        for r in csv.DictReader(open(p)):
            a = r["activeTier"]
            fix = r["hadFix"] == "1"
            dec = fix and int(r["nAnchors"]) >= 3
            c["cycles"] += 1
            c["occ_" + a] += 1
            if fix:
                c["fix_" + a] += 1
                c[f"fix_{a}_on_{r['fixTier']}"] += 1
            if a == "UWB" and fix and r["fixTier"] == "UWB":
                c["uwbfix"] += 1
            if arm == "ca_tbl_full" and dec and r["suspect"] == "0":
                c["pcs_rows"] += 1
                d = float(r["pcs"]) - W1 * TREF / (TREF + float(r["traceSigma"]))
                c["pcs_on_formula"] += -6e-5 <= d <= W3 + 6e-5
            if dec:
                c["dec_" + a] += 1
                if a in UP and up(arm, r):
                    c["want_" + a] += 1
                    c["want_unmet_" + a] += r["escalated"] == "0"
                if a in DOWN:
                    c["healthy_" + a] += healthy(arm, r)
            if r["escalated"] == "1":
                c["esc"] += 1
                c["esc_rule" if (dec and a in UP and up(arm, r)) else "esc_query"] += 1
            if r["deescalated"] == "1":
                c["de"] += 1
                c["de_rule" if (dec and a in DOWN and healthy(arm, r)) else "de_dropout"] += 1
    return c


def pct(n, d):
    return f"{100.0 * n / d:.1f} %" if d else "n/a"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    print("Preset PCS as a trace(P) threshold (m^2), term 3 = 1 .. 0:")
    for k, v in UP.items():
        print(f"  escalate from {k}: trace > {trace_equiv(v, 1):.2f} .. {trace_equiv(v, 0):.2f}")
    for k, v in DOWN.items():
        print(f"  healthy at {k}: trace < {trace_equiv(v, 1):.2f} .. {trace_equiv(v, 0):.2f}")
    print(f"  fixed_threshold: escalate trace > {FIXED_UP:g}, healthy trace < {FIXED_HEALTHY:g}\n")
    for cell in CELLS:
        cc = {arm: cell_counts(out, cell, arm) for arm in ARMS}
        f = cc["ca_tbl_full"]
        print(f"## {cell} (tag 0, seeds 1-20)")
        print(f"PCS on the trace formula: {f['pcs_on_formula']} of {f['pcs_rows']} non-SUSPECT decision rows")
        print("| | ca_tbl_full | fixed_threshold |\n|---|---|---|")

        def row(label, fn):
            print(f"| {label} | {fn(cc['ca_tbl_full'])} | {fn(cc['fixed_threshold'])} |")

        row("occupancy 868 / 2.4 / UWB", lambda c: " / ".join(
            pct(c["occ_" + t], c["cycles"]) for t in ("868MHz", "2.4GHz", "UWB")))
        row("escalation decisions per run", lambda c: f"{c['esc'] / 20:.1f}")
        row("... by the rule / by the URGENT-query hook",
            lambda c: f"{pct(c['esc_rule'], c['esc'])} / {pct(c['esc_query'], c['esc'])}")
        row("demotions per run", lambda c: f"{c['de'] / 20:.1f}")
        row("... healthy rule / dropout path",
            lambda c: f"{pct(c['de_rule'], c['de'])} / {pct(c['de_dropout'], c['de'])}")
        row("rule wants up at 868 (of decision rows); unmet",
            lambda c: f"{pct(c['want_868MHz'], c['dec_868MHz'])}; {pct(c['want_unmet_868MHz'], c['want_868MHz'])}")
        row("rule wants up at 2.4 (of decision rows); unmet",
            lambda c: f"{pct(c['want_2.4GHz'], c['dec_2.4GHz'])}; {pct(c['want_unmet_2.4GHz'], c['want_2.4GHz'])}")
        row("healthy at 2.4 / at UWB (of decision rows)", lambda c: (
            f"{pct(c['healthy_2.4GHz'], c['dec_2.4GHz'])} / {pct(c['healthy_UWB'], c['dec_UWB'])}"))
        row("fixes solved on 2.4 GHz ranges while at 868 / at 2.4", lambda c: (
            f"{pct(c['fix_868MHz_on_2.4GHz'], c['fix_868MHz'])} / {pct(c['fix_2.4GHz_on_2.4GHz'], c['fix_2.4GHz'])}"))
        row("UWB cycles / UWB-solved fixes per run",
            lambda c: f"{c['occ_UWB'] / 20:.1f} / {c['uwbfix'] / 20:.2f}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
