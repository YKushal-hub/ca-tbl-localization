#!/usr/bin/env python3
"""G131: --nlosRSource (S2-C), which NLOS tag keys R and the LOS count in Step().

usage: nlos_r_gate.py <dir written by scripts/nlos_r_gate.sh>

What a firing looks like on disk, per check (tag 0's trace column nNlos is meas.size() - nLos,
the count the flag rekeys):
  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  --nlosRSource=operative given explicitly changes an output (run_id.txt excepted).
  NONE     under 'none' a trace row has nNlos > 0; or the control has no row with nNlos > 0, so
           the check could not fire.
  DET      under 'detector' a trace row has nNlos > 0 on a cycle where tag 0's link log holds no
           NLOS verdict (verdict 1); or no control row with nNlos > 0 falls on such a cycle, so the
           oracle and the detector were never told apart.
"""
import csv
import filecmp
import glob
import os
import sys


def rows(p):
    with open(p) as fh:
        return list(csv.DictReader(fh))


def main():
    d = sys.argv[1]
    res = []
    runs = ["base", "oper", "none", "det"]
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r))
           or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"runs not exiting 0 or missing: {bad}" if bad else "4 runs exit 0"))
    if bad:
        return report(res)

    def files(r):
        return sorted(os.path.relpath(p, os.path.join(d, r))
                      for p in glob.glob(os.path.join(d, r, "**", "*"), recursive=True)
                      if os.path.isfile(p) and not p.endswith("run_id.txt"))

    fb = files("base")
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "oper", f), shallow=False)]
    res.append(("DEFAULT", fb == files("oper") and not diff, f"{len(fb)} files, {len(diff)} differ {diff[:3]}"))

    tr = {r: rows(os.path.join(d, r, "trace_ca_tbl_full_seed7.csv")) for r in ("base", "none", "det")}
    base_n = [x for x in tr["base"] if int(x["nNlos"]) > 0]
    none_n = [x for x in tr["none"] if int(x["nNlos"]) > 0]
    res.append(("NONE", not none_n and bool(base_n),
                f"rows with nNlos > 0: none {len(none_n)}, control {len(base_n)}"))

    def verdict_times(r):
        return {round(float(x["t"]), 3) for x in rows(os.path.join(d, r, "nlos_links_ca_tbl_full_seed7.csv"))
                if x["verdict"] == "1"}

    vt_det = verdict_times("det")
    vt_base = verdict_times("base")
    det_bad = [x["t"] for x in tr["det"] if int(x["nNlos"]) > 0 and round(float(x["t"]), 3) not in vt_det]
    det_n = sum(1 for x in tr["det"] if int(x["nNlos"]) > 0)
    apart = sum(1 for x in base_n if round(float(x["t"]), 3) not in vt_base)
    res.append(("DET", not det_bad and apart > 0,
                f"detector rows with nNlos > 0: {det_n}, of them off a verdict cycle {len(det_bad)} {det_bad[:3]}; "
                f"control rows with nNlos > 0 off a verdict cycle (oracle != detector): {apart}"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G131 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
