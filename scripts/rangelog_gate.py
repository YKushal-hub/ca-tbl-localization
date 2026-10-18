#!/usr/bin/env python3
"""G137: B1 --traceRanges.
  RUNS     a run's rc is not 0, or a directory is missing.
  DEFAULT  false given explicitly changes an output (run_id.txt excepted).
  INERT    with the flag on, any output other than the new ranges_* file differs from the control
           (the log must observe, not act).
  LOG      the ranges file is missing or empty, or some trace row with nRangesEkf > 0 has no in-set
           ranges logged at its t.
"""
import csv, filecmp, glob, os, sys


def main():
    d = sys.argv[1]; res = []
    bad = [r for r in ("base", "off", "on") if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad: {bad}" if bad else "3 runs exit 0"))
    if bad:
        return report(res)
    def files(r):
        return sorted(f for f in os.listdir(os.path.join(d, r)) if f != "run_id.txt")
    fb = files("base")
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False)]
    res.append(("DEFAULT", fb == files("off") and not diff, f"{len(diff)} differ"))
    on = files("on"); extra = [f for f in on if f not in fb]
    diff2 = [f for f in fb if f in on and not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "on", f), shallow=False)]
    res.append(("INERT", not diff2 and extra == ["ranges_ca_tbl_full_seed7.csv"], f"differ {diff2[:3]}, new files {extra}"))
    p = os.path.join(d, "on", "ranges_ca_tbl_full_seed7.csv")
    if not os.path.exists(p):
        res.append(("LOG", False, "no ranges file")); return report(res)
    rows = list(csv.DictReader(open(p)))
    ts = {r["t"] for r in rows if r["inSet"] == "1"}
    tr = [r for r in csv.DictReader(open(os.path.join(d, "on", "trace_ca_tbl_full_seed7.csv"))) if int(r["nRangesEkf"]) > 0]
    miss = [r["t"] for r in tr if r["t"] not in ts]
    res.append(("LOG", bool(rows) and not miss, f"{len(rows)} range rows; trace rows with ranges but none logged: {len(miss)}"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G137 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
