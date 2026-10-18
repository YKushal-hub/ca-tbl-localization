#!/usr/bin/env python3
"""G132: --acquireMinAnchors (S2-C), under --unknownStart.

usage: acq_gate.py <dir written by scripts/acq_gate.sh>

What a firing looks like on disk, per check (tag 0's trace and the subjects' traces):
  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  --acquireMinAnchors=0 given explicitly changes an output (run_id.txt excepted).
  ACQ      at 3, a node has a fix row with nAnchors < 3 before its first fix row with nAnchors >= 3;
           or the control (flag off) has no such early sparse fix on any node, so the check could
           not fire.
"""
import csv
import filecmp
import glob
import os
import sys


def early_sparse(path):
    """Fix rows with nAnchors < 3 before the first fix row with nAnchors >= 3."""
    n = 0
    with open(path) as fh:
        for r in csv.DictReader(fh):
            if r["hadFix"] != "1":
                continue
            if int(r["nAnchors"]) >= 3:
                break
            n += 1
    return n


def main():
    d = sys.argv[1]
    res = []
    runs = ["base", "zero", "acq3"]
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r))
           or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"runs not exiting 0 or missing: {bad}" if bad else "3 runs exit 0"))
    if bad:
        return report(res)

    def files(r):
        return sorted(os.path.relpath(p, os.path.join(d, r))
                      for p in glob.glob(os.path.join(d, r, "**", "*"), recursive=True)
                      if os.path.isfile(p) and not p.endswith("run_id.txt"))

    fb = files("base")
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "zero", f), shallow=False)]
    res.append(("DEFAULT", fb == files("zero") and not diff, f"{len(fb)} files, {len(diff)} differ {diff[:3]}"))

    def per_node(r):
        return {os.path.basename(p): early_sparse(p)
                for p in sorted(glob.glob(os.path.join(d, r, "*trace_ca_tbl_full_seed7*.csv")))}

    a3, a0 = per_node("acq3"), per_node("base")
    res.append(("ACQ", bool(a3) and sum(a3.values()) == 0 and sum(a0.values()) > 0,
                f"early sparse fixes before the first >= 3-range fix: at 3 {sum(a3.values())} over {len(a3)} nodes; "
                f"flag off {sum(a0.values())}"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G132 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
