#!/usr/bin/env python3
"""S2-C NLOS audit (post-hoc, exploratory; reads ACC-A's link logs, runs nothing).

usage: s2c_nlos_audit.py <ACC-A outdir>

For ca_tbl_full, tag 0, cells M_A_k10 and M_B_k10, seeds 1-20, per tier:
  - the detector's confusion as the engine counted it (nlos_summary 'confusion' rows), including
    the links that were never scored and those lost before a score;
  - precision, recall, false-positive rate, and the ROC AUC of the score on scored links
    (nlos_links: truthNlos against score);
  - the measured range residual by truth class where the link log carries a range
    (2.4 GHz: tofRangeM - distM), which says whether NLOS biases the simulated range.
"""
import csv
import glob
import os
import sys
from collections import defaultdict

import numpy as np


def auc(pos, neg):
    if not pos or not neg:
        return float("nan")
    allv = np.array(pos + neg)
    ranks = allv.argsort().argsort().astype(float) + 1
    rp = ranks[: len(pos)].sum()
    return (rp - len(pos) * (len(pos) + 1) / 2) / (len(pos) * len(neg))


def main():
    out = sys.argv[1]
    for cell in ("M_A_k10", "M_B_k10"):
        conf = defaultdict(lambda: defaultdict(float))
        sc = defaultdict(lambda: {"pos": [], "neg": []})
        res = defaultdict(lambda: {0: [], 1: []})
        for s in range(1, 21):
            d = os.path.join(out, cell, f"s{s}")
            for r in csv.DictReader(open(os.path.join(d, f"nlos_summary_ca_tbl_full_seed{s}.csv"))):
                if r["kind"] != "confusion":
                    continue
                c = conf[(r["tier"], r["detector"])]
                for k in ("tp", "fp", "fn", "tn", "unscoredLos", "unscoredNlos", "lostLos", "lostNlos"):
                    c[k] += float(r[k] or 0)
            for r in csv.DictReader(open(os.path.join(d, f"nlos_links_ca_tbl_full_seed{s}.csv"))):
                t = r["tier"]
                truth = int(r["truthNlos"])
                if r["score"] not in ("", "-1"):
                    sc[t]["pos" if truth else "neg"].append(float(r["score"]))
                if r["tofRangeM"] not in ("",):
                    res[t][truth].append(float(r["tofRangeM"]) - float(r["distM"]))
        print(f"## {cell} (ca_tbl_full, tag 0, seeds 1-20)")
        print("| tier | detector | TP | FP | FN | TN | precision | recall | FPR | unscored LOS/NLOS | lost LOS/NLOS | AUC (scored) |")
        print("|---|---|---|---|---|---|---|---|---|---|---|---|")
        for (tier, det), c in sorted(conf.items()):
            tp, fp, fn, tn = c["tp"], c["fp"], c["fn"], c["tn"]
            prec = tp / (tp + fp) if tp + fp else float("nan")
            rec = tp / (tp + fn) if tp + fn else float("nan")
            fpr = fp / (fp + tn) if fp + tn else float("nan")
            a = auc(sc[tier]["pos"], sc[tier]["neg"])
            print(f"| {tier} | {det} | {tp:.0f} | {fp:.0f} | {fn:.0f} | {tn:.0f} | {prec:.3f} | {rec:.3f} | {fpr:.3f} | "
                  f"{c['unscoredLos']:.0f}/{c['unscoredNlos']:.0f} | {c['lostLos']:.0f}/{c['lostNlos']:.0f} | {a:.3f} |")
        for tier, v in sorted(res.items()):
            if v[0] or v[1]:
                f = lambda x: (f"n {len(x)}, mean {np.mean(x):+.2f} m, SD {np.std(x):.2f} m" if x else "n 0")
                print(f"- {tier} range residual (range - true distance): LOS {f(v[0])}; NLOS {f(v[1])}")
        print()


if __name__ == "__main__":
    main()
