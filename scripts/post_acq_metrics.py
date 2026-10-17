#!/usr/bin/env python3
"""B14 (scope 2026-10-17): accuracy after acquisition, the standard companion of arm_summary.csv's CEP95.

Per-run CEP95 over the whole run is dominated by the first minutes (acquisition) and is a noisy tail statistic (V7
sect 2, V8 sect 3). This reports, per arm, tag 0's CEP50 and CEP95 over fix rows (hadFix = 1, the EKF-fix definition the
owner fixed on 2026-10-17) with t >= --after seconds (default 300), from trace_<arm>_seed<N>.csv.

usage: post_acq_metrics.py <run dir> [<run dir> ...] [--after S]
       prints one CSV row per (run dir, arm): dir,arm,n,cep50_post,cep95_post
library: from post_acq_metrics import post_acq ; post_acq(run_dir, arm, after=300.0) -> (n, cep50, cep95)
"""
import csv, glob, os, sys
import numpy as np


def post_acq(run_dir, arm, after=300.0):
    fs = glob.glob(os.path.join(run_dir, f"trace_{arm}_seed*.csv"))
    if not fs:
        return 0, float("nan"), float("nan")
    e = [float(r["errM"]) for r in csv.DictReader(open(fs[0])) if r["hadFix"] == "1" and float(r["t"]) >= after]
    if not e:
        return 0, float("nan"), float("nan")
    return len(e), float(np.percentile(e, 50)), float(np.percentile(e, 95))


def main():
    args = sys.argv[1:]
    after = 300.0
    if "--after" in args:
        k = args.index("--after"); after = float(args[k + 1]); del args[k:k + 2]
    print("dir,arm,n,cep50_post,cep95_post")
    for d in args:
        for f in sorted(glob.glob(os.path.join(d, "trace_*_seed*.csv"))):
            arm = os.path.basename(f)[len("trace_"):].rsplit("_seed", 1)[0]
            n, c50, c95 = post_acq(d, arm, after)
            print(f"{d},{arm},{n},{c50:.3f},{c95:.3f}")


if __name__ == "__main__":
    main()
