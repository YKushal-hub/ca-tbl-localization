#!/usr/bin/env python3
"""G134: --foliageDepthClamp (B3f, report/FOLIAGE_DEPTH_DEFECT.md).

usage: foliage_gate.py <dir written by scripts/foliage_gate.sh>
Checks:
  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  --foliageDepthClamp=false given explicitly changes an output (run_id.txt excepted), probe or sim.
  FIRES    the flag-on sim run prints no FOLIAGECLAMP line or reports 0 firings; or the control prints one.
  REACH    with the flag on, UWB PDR at <= 10 m is not higher than the control's (the defect's signature),
           or any 868 MHz probe row at >= 500 m has a PDR more than 0.03 from the control's. (Not byte
           identity: the clamp changes which short links are received, which displaces later draws of the
           global random stream, TECH_DEBT 1.1; the 868 rows then move by sampling noise only.)
"""
import csv, filecmp, glob, os, re, sys


def main():
    d = sys.argv[1]
    res = []
    runs = ["pbase", "poff", "pon", "sbase", "soff", "son"]
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad runs: {bad}" if bad else "6 runs exit 0"))
    if bad:
        return report(res)

    def files(r):
        return sorted(os.path.relpath(p, os.path.join(d, r)) for p in glob.glob(os.path.join(d, r, "**", "*"), recursive=True)
                      if os.path.isfile(p) and not p.endswith("run_id.txt"))
    diffs = []
    for a, b in (("pbase", "poff"), ("sbase", "soff")):
        fa = files(a)
        if fa != files(b):
            diffs.append(f"{a}/{b} file sets differ")
        diffs += [f"{b}/{f}" for f in fa if os.path.exists(os.path.join(d, b, f)) and not filecmp.cmp(os.path.join(d, a, f), os.path.join(d, b, f), shallow=False)]
    res.append(("DEFAULT", not diffs, f"{len(diffs)} differences {diffs[:3]}"))

    def firings(r):
        m = re.findall(r"^FOLIAGECLAMP firings=(\d+)", open(os.path.join(d, r + ".log"), errors="replace").read(), re.M)
        return int(m[-1]) if m else None
    fon, fb = firings("son"), firings("sbase")
    res.append(("FIRES", fon is not None and fon > 0 and fb is None, f"flag on: {fon}, control: {fb}"))

    def probe(r):
        return list(csv.DictReader(open(os.path.join(d, r, "probe_range_sweep.csv"))))
    on, base = probe("pon"), probe("pbase")
    def pdr(rows, tier, maxd):
        v = [float(x["pdr"]) for x in rows if x["tier"] == tier and float(x["distance_m"]) <= maxd]
        return sum(v) / len(v) if v else float("nan")
    u_on, u_b = pdr(on, "UWB", 10.0), pdr(base, "UWB", 10.0)
    long_on = [x for x in on if x["tier"] == "868MHz" and float(x["distance_m"]) >= 500]
    long_b = [x for x in base if x["tier"] == "868MHz" and float(x["distance_m"]) >= 500]
    dmax = max((abs(float(a["pdr"]) - float(b["pdr"])) for a, b in zip(long_on, long_b)), default=float("nan"))
    same868 = len(long_b) > 0 and len(long_on) == len(long_b) and dmax <= 0.03
    res.append(("REACH", u_on > u_b and same868, f"UWB PDR <= 10 m: on {u_on:.3f}, control {u_b:.3f}; 868 rows >= 500 m max |dPDR| {dmax:.3f} ({len(long_b)} rows)"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G134 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
