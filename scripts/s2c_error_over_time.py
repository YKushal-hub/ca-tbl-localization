#!/usr/bin/env python3
"""S2-C error over time (post-hoc, descriptive, not registered).

usage: s2c_error_over_time.py <outdir of scripts/s2c_run.sh> [<ACC-A outdir> <ACC-A-OPPOINT outdir>]

Five-node median error (m) and within-25 m share at t = 30, 120, 300, 600, 900, 1500, 1795 s for each
arm under unknown start (U cells), and, with the control directories given, under the known start
(ACC-A M_k10 and ACC-A-OPPOINT O_T12; byte-identical to S2-C's C cells on seeds 1-3).
"""
import csv
import glob
import os
import sys

import numpy as np

TS = (30, 120, 300, 600, 900, 1500, 1795)
ARMS = ("ca_tbl_full", "fixed_threshold", "static_2p4")


def at(rows, T):
    for r in rows:
        if float(r["t"]) >= T:
            return float(r["errM"])
    return float("nan")


def table(root, cell, arm):
    errs = {T: [] for T in TS}
    for d in sorted(glob.glob(os.path.join(root, cell, "s*"))):
        s = int(d.rsplit("s", 1)[1])
        fs = [os.path.join(d, f"trace_{arm}_seed{s}.csv")] + \
             [os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv") for k in range(2, 6)]
        if not all(os.path.exists(f) for f in fs):
            continue
        for f in fs:
            rows = list(csv.DictReader(open(f)))
            for T in TS:
                errs[T].append(at(rows, T))
    return errs


def main():
    out = sys.argv[1]
    ctl = sys.argv[2:4] if len(sys.argv) >= 4 else None
    for cfg in ("A", "B"):
        print(f"## {cfg}: five-node median error (m) / share within 25 m, by time")
        print("| arm | start | " + " | ".join(f"t = {T} s" for T in TS) + " |")
        print("|---|---|" + "---|" * len(TS))
        for arm in ARMS:
            rows = [("unknown", table(out, f"U_{cfg}", arm))]
            if ctl:
                root, cell = (ctl[1], f"O_{cfg}_T12") if arm == "fixed_threshold" else (ctl[0], f"M_{cfg}_k10")
                rows.append(("known", table(root, cell, arm)))
            for lab, e in rows:
                n = len(e[TS[0]])
                print(f"| {arm} | {lab} (n {n}) | " + " | ".join(
                    f"{np.nanmedian(e[T]):.0f} / {np.nanmean(np.array(e[T]) <= 25):.2f}" for T in TS) + " |")
        print()


if __name__ == "__main__":
    main()
