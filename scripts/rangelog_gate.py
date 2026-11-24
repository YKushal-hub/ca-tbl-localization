#!/usr/bin/env python3
"""G137: B1 --traceRanges (tag 0) and --traceSubjectRanges (measured subjects).
  RUNS     a run's rc is not 0, or a directory is missing.
  DEFAULT  false given explicitly changes an output (run_id.txt excepted).
  INERT    with --traceRanges on, any output differs from the control or the new files are not exactly ranges_*
           (the log must observe, not act).
  SUBJINERT the same for --traceSubjectRanges and subjranges_*.
  LOG      the ranges file is missing or empty, or some trace row with nRangesEkf > 0 has no in-set
           ranges logged at its t.
  SUBJLOG  the subjranges file is missing or empty, or some subject trace row with nAnchors > 0
           has no range logged for that subject at its t.
"""
import csv, filecmp, glob, os, sys


def main():
    d = sys.argv[1]; res = []
    bad = [r for r in ("base", "off", "on", "subj") if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad: {bad}" if bad else "4 runs exit 0"))
    if bad:
        return report(res)
    def files(r):
        return sorted(f for f in os.listdir(os.path.join(d, r)) if f != "run_id.txt")
    fb = files("base")
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False)]
    res.append(("DEFAULT", fb == files("off") and not diff, f"{len(diff)} differ"))
    on = files("on"); extra = [f for f in on if f not in fb]
    diff2 = [f for f in fb if f not in on or not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "on", f), shallow=False)]
    res.append(("INERT", not diff2 and extra == ["ranges_ca_tbl_full_seed7.csv"], f"differ {diff2[:3]}, new files {extra}"))
    sub = files("subj"); extra = [f for f in sub if f not in fb]
    diff3 = [f for f in fb if f not in sub or not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "subj", f), shallow=False)]
    res.append(("SUBJINERT", not diff3 and extra == ["subjranges_ca_tbl_full_seed7.csv"], f"differ {diff3[:3]}, new files {extra}"))
    p = os.path.join(d, "on", "ranges_ca_tbl_full_seed7.csv")
    if not os.path.exists(p):
        res.append(("LOG", False, "no ranges file")); return report(res)
    rows = list(csv.DictReader(open(p)))
    ts = {r["t"] for r in rows if r["inSet"] == "1"}
    tr = [r for r in csv.DictReader(open(os.path.join(d, "on", "trace_ca_tbl_full_seed7.csv"))) if int(r["nRangesEkf"]) > 0]
    miss = [r["t"] for r in tr if r["t"] not in ts]
    res.append(("LOG", bool(rows) and not miss, f"{len(rows)} range rows; trace rows with ranges but none logged: {len(miss)}"))
    p = os.path.join(d, "subj", "subjranges_ca_tbl_full_seed7.csv")
    if not os.path.exists(p):
        res.append(("SUBJLOG", False, "no subjranges file")); return report(res)
    rows = list(csv.DictReader(open(p)))
    have = {(r["subj"], float(r["t"])) for r in rows}
    traces = sorted(glob.glob(os.path.join(d, "subj", "subjtrace_ca_tbl_full_seed7_subj*.csv")))
    need = [(f.rsplit("_subj", 1)[1][:-4], float(r["t"])) for f in traces
            for r in csv.DictReader(open(f)) if int(r["nAnchors"]) > 0]
    miss = [k for k in need if k not in have]
    res.append(("SUBJLOG", bool(rows) and bool(need) and not miss,
                f"{len(rows)} rows; {len(need)} subject cycles with ranges, not logged: {len(miss)} {miss[:2]}"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G137 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
