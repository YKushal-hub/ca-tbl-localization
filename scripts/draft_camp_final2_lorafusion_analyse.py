#!/usr/bin/env python3
"""CAMP-FINAL-2 analysis (report/PREREG_CAMP_FINAL2.md). Reads <outdir>/<cell>/s<seed>/.

1. CAMP-FINAL's tables (scripts/camp_final_analyse.py, applied unchanged) with dual-band LoRa
   fusion (lora_fusion) as an eighth arm in A_main and B_end -> <outdir>/CAMP_FINAL.md.
2. The comparison table: ca_tbl_full minus each comparator, paired by seed, mean [95 % seed
   bootstrap], on availability (the five subjects), tag 0 CEP95 and subject energy, in A_main
   and B_end -> <outdir>/PAIRED.md and paired.csv.

usage: camp_final2_analyse.py <outdir>
"""
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_final_analyse as cfa  # noqa: E402 -- CAMP-FINAL's tables, reused unchanged
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402

COMPARATORS = ["poucet_fusion", "lora_fusion", "fixed_threshold", "static_868", "static_2p4", "static_uwb",
               "ca_tbl_no_integrity"]


def per_seed(out, cell, arm):
    v = {}
    for d in cs.seeds(out, cell):
        if not cs.ok(d):
            continue
        a = cs.arm_row(d, arm)
        ss = cs.subj_rows(d, arm)
        if a is None:
            continue
        seed = int(d.rsplit("s", 1)[1])
        v[seed] = dict(av=np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss]) if ss
                       else float(a["fixCycles"]) / float(a["steps"]),
                       c95=float(a["cep95M"]),
                       J=np.mean([float(r["totalJ"]) for r in ss]) if ss else float(a["totalJ"]))
    return v


def main():
    out = sys.argv[1]
    cfa.ARMS7 = cs.ARMS6 + ["poucet_fusion", "lora_fusion"]
    rc = cfa.main()
    md = ["# CAMP-FINAL-2 — ca_tbl_full against each comparator, paired by seed", "",
          "Mean of the per-seed difference ca_tbl_full minus comparator [95 % seed bootstrap]. Availability higher is "
          "better; CEP95 and energy lower are better.", ""]
    rows = []
    for cell, title in (("A_main", "5 km, the spec's three gateways"), ("B_end", "the endgame")):
        ref = per_seed(out, cell, "ca_tbl_full")
        md += [f"## {title} ({cell})", "", "| comparator | seeds | Δ availability | Δ tag 0 CEP95 (m) | Δ subject J |",
               "|---|---|---|---|---|"]
        for comp in COMPARATORS:
            v = per_seed(out, cell, comp)
            s = sorted(set(v) & set(ref))
            if not s:
                continue
            d = {k: cs.ci([ref[i][k] - v[i][k] for i in s]) for k in ("av", "c95", "J")}
            md.append(f"| {comp} | {len(s)} | {cs.fmt(*d['av'])} | {cs.fmt(*d['c95'], p=1)} | {cs.fmt(*d['J'], p=2)} |")
            rows.append(dict(cell=cell, comparator=comp, seeds=len(s),
                             **{f"{k}_{q}": f"{d[k][j]:.4f}" for k in ("av", "c95", "J") for j, q in enumerate(("mean", "lo", "hi"))}))
        md.append("")
    open(os.path.join(out, "PAIRED.md"), "w").write("\n".join(md) + "\n")
    if rows:
        with open(os.path.join(out, "paired.csv"), "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()), lineterminator="\n")
            w.writeheader()
            w.writerows(rows)
    print("\n".join(md))
    return rc


if __name__ == "__main__":
    sys.exit(main())
