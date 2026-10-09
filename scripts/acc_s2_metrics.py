#!/usr/bin/env python3
"""S2 cloud branch: per-seed, per-arm scoreboard metrics from a campaign's run directories.

usage: acc_s2_metrics.py <cell_dir> [<cell_dir> ...] > metrics.csv
  Each <cell_dir> holds s<seed>/ run directories (traces present). One row per (cell, seed, arm):
  fix   five-node fix availability, mean over tag 0 and subjects of fixCycles / steps
        (subject_summary.csv);
  w25   five-node share of cycles with error <= 25 m (tag 0 trace and the four subject traces);
  t0w25 tag 0's share within 25 m;
  c50, c95  tag 0's median and 95th-percentile error over the run's cycles, m;
  J     mean total energy per node, J (subject_summary.csv).
Denominator: every cycle of every node's trace (720 per node at 1800 s); no cycle is dropped.
"""
import csv
import glob
import os
import sys

import numpy as np
import pandas as pd


def main():
    w = csv.writer(sys.stdout, lineterminator="\n")
    w.writerow(["cell", "seed", "arm", "fix", "w25", "t0w25", "c50", "c95", "J"])
    for cell_dir in sys.argv[1:]:
        cell = os.path.basename(os.path.normpath(cell_dir))
        for d in sorted(glob.glob(os.path.join(cell_dir, "s*")), key=lambda p: int(p.rsplit("s", 1)[1])):
            seed = int(d.rsplit("s", 1)[1])
            ss = pd.read_csv(os.path.join(d, "subject_summary.csv"))
            for arm in sorted(ss.arm.unique()):
                a = ss[ss.arm == arm]
                t0 = pd.read_csv(os.path.join(d, f"trace_{arm}_seed{seed}.csv"))
                shares = [(t0.errM <= 25).mean()]
                for k in range(2, 6):
                    f = os.path.join(d, f"subjtrace_{arm}_seed{seed}_subj{k}.csv")
                    if os.path.exists(f):
                        shares.append((pd.read_csv(f).errM <= 25).mean())
                w.writerow([cell, seed, arm, f"{(a.fixCycles / a.steps).mean():.6f}", f"{np.mean(shares):.6f}",
                            f"{shares[0]:.6f}", f"{np.median(t0.errM):.4f}", f"{np.quantile(t0.errM, 0.95):.4f}",
                            f"{a.totalJ.mean():.4f}"])


if __name__ == "__main__":
    main()
