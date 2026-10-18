#!/usr/bin/env python3
"""G133: --acquireReinit (R1, TECH_DEBT N.54), under --unknownStart.

usage: acqreinit_gate.py <dir written by scripts/acqreinit_gate.sh>

What a firing looks like on disk, per check:
  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  --acquireReinit=false given explicitly changes an output (run_id.txt excepted).
  FIRES    with the flag on, no ACQREINIT line is printed, or more lines than field-node filters
           (one firing per filter at most); or the control prints any.
  CHANGES  with the flag on, tag 0's trace is byte-identical to the control's (the mechanism fired
           but changed nothing it should change).
"""
import filecmp
import glob
import os
import sys

NODES_MAX = 64  # generous upper bound on field-node filters in a preset run


def main():
    d = sys.argv[1]
    res = []
    runs = ["base", "off", "on"]
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
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False)]
    res.append(("DEFAULT", fb == files("off") and not diff, f"{len(fb)} files, {len(diff)} differ {diff[:3]}"))

    def nfire(r):
        with open(os.path.join(d, r + ".log"), errors="replace") as fh:
            return sum(1 for ln in fh if ln.startswith("ACQREINIT "))

    n_on, n_base = nfire("on"), nfire("base")
    res.append(("FIRES", 1 <= n_on <= NODES_MAX and n_base == 0,
                f"ACQREINIT lines: flag on {n_on}, control {n_base}"))
    t_on = sorted(glob.glob(os.path.join(d, "on", "trace_ca_tbl_full_seed7.csv")))
    t_b = sorted(glob.glob(os.path.join(d, "base", "trace_ca_tbl_full_seed7.csv")))
    same = bool(t_on and t_b) and filecmp.cmp(t_on[0], t_b[0], shallow=False)
    res.append(("CHANGES", bool(t_on and t_b) and not same,
                "tag 0 trace differs from the control" if (t_on and t_b and not same) else "tag 0 trace identical or missing"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G133 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
