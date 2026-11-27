#!/usr/bin/env python3
"""G190 (--sigmaShadow, DD 75). Checks, each printed as "G190 PASS|FAIL <name> ...":
  RUNS     every run rc 0
  OFFSAME  the explicit default equals the default, every csv byte for byte
  LINES    the DD75 line in "on" only
  ROWS     tag 0's trace and the 19 subject traces present in off and on, same row counts
  ESTSAME  the estimate (estX, estY) and the error identical in every row of off and on
  NOLOWER  on's traceSigma >= off's in every row
  EFFECT   on raises traceSigma in at least 1 % of rows, and coverage (errM <= 1.73 sqrt(traceSigma)) does not fall
usage: sigma_shadow_gate.py <dir>"""
import csv, filecmp, glob, os, sys


def files(d):
    return sorted(os.path.basename(f) for f in glob.glob(os.path.join(d, "*.csv")))


def main():
    d = sys.argv[1]
    bad = []

    def rep(name, ok, msg=""):
        print(f"G190 {'PASS' if ok else 'FAIL'} {name} {msg}")
        if not ok:
            bad.append(name)

    rcs = {n: (open(os.path.join(d, n + ".rc")).read().strip() if os.path.exists(os.path.join(d, n + ".rc")) else "missing") for n in ("off", "false", "on")}
    rep("RUNS", all(v == "0" for v in rcs.values()), str(rcs))
    fo, ff = files(os.path.join(d, "off")), files(os.path.join(d, "false"))
    same = fo == ff and len(fo) > 0 and all(filecmp.cmp(os.path.join(d, "off", f), os.path.join(d, "false", f), shallow=False) for f in fo)
    rep("OFFSAME", same, f"{len(fo)} csv")
    lg = {n: open(os.path.join(d, n + ".log"), errors="replace").read() if os.path.exists(os.path.join(d, n + ".log")) else "" for n in ("off", "false", "on")}
    rep("LINES", "DD75 sigmaShadow" in lg["on"] and "DD75" not in lg["off"] and "DD75" not in lg["false"])
    pat = ["trace_ca_tbl_full_seed731.csv"] + [f"subjtrace_ca_tbl_full_seed731_subj{k}.csv" for k in range(2, 21)]
    rows_ok, pairs = True, []
    for p in pat:
        a, b = os.path.join(d, "off", p), os.path.join(d, "on", p)
        if not (os.path.exists(a) and os.path.exists(b)):
            rows_ok = False; continue
        ra, rb = list(csv.DictReader(open(a))), list(csv.DictReader(open(b)))
        if len(ra) != len(rb) or not ra:
            rows_ok = False; continue
        pairs += list(zip(ra, rb))
    rep("ROWS", rows_ok and len(pairs) > 0, f"{len(pairs)} row pairs")
    est = len(pairs) > 0 and all(x["estX"] == y["estX"] and x["estY"] == y["estY"] and x["errM"] == y["errM"] for x, y in pairs)
    rep("ESTSAME", est)
    lower = [1 for x, y in pairs if float(y["traceSigma"]) < float(x["traceSigma"])]
    rep("NOLOWER", len(pairs) > 0 and not lower, f"{len(lower)} rows lower")
    raised = sum(1 for x, y in pairs if float(y["traceSigma"]) > float(x["traceSigma"]))
    cov = lambda r: float(r["errM"]) <= 1.73 * max(float(r["traceSigma"]), 0.0) ** 0.5
    co, cn = (sum(cov(x) for x, _ in pairs), sum(cov(y) for _, y in pairs)) if pairs else (0, 0)
    rep("EFFECT", len(pairs) > 0 and raised >= 0.01 * len(pairs) and cn >= co, f"raised {raised}/{len(pairs)}, covered {co} -> {cn}")
    print(f"G190 {'PASS' if not bad else 'FAIL'} overall {' '.join(bad)}")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
