#!/usr/bin/env python3
"""G178 checker for scripts/collapse_world_gate.sh (DD 37). Exit 0 only if every check passes."""
import csv
import glob
import hashlib
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def sha(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def main():
    d = sys.argv[1]
    fails = []
    for n in ("det1", "det2", "keyed", "high"):
        if rc(d, n) != 0:
            fails.append(f"{n}: exit {rc(d, n)}")
    for n in ("r_missing", "r_depth", "r_bracket", "r_station"):
        if rc(d, n) in (0, None):
            fails.append(f"{n}: not refused (exit {rc(d, n)})")
        elif "FATAL" not in open(os.path.join(d, n + ".log")).read():
            fails.append(f"{n}: refused without a FATAL message")
    if not fails:
        files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(d, "det1", "*.csv")))
        need = {"obs_links.csv", "obs_anchors.csv", "truth_victims.csv", "truth_anchors.csv", "truth_links.csv"}
        if not need.issubset(files):
            fails.append(f"det: missing outputs {need - set(files)}")
        for f in files:
            if sha(os.path.join(d, "det1", f)) != sha(os.path.join(d, "det2", f)):
                fails.append(f"det: {f} differs between identical runs")
        full = {}
        for r in csv.DictReader(open(os.path.join(d, "det1", "obs_links.csv"))):
            if int(r["rx"]) >= 10000:
                full[(r["slot"], r["band"], r["tx"], r["rx"])] = r
        nk = 0
        for r in csv.DictReader(open(os.path.join(d, "keyed", "obs_links.csv"))):
            k = (r["slot"], r["band"], r["tx"], r["rx"])
            if int(r["rx"]) < 10000:
                fails.append("keyed: worker-worker row present with --victimLinks=false")
                break
            if full.get(k) != r:
                fails.append(f"keyed: row {k} differs from the full run")
                break
            nk += 1
        if nk != len(full):
            fails.append(f"keyed: {nk} rows vs {len(full)} in the full run")
        vic = list(csv.DictReader(open(os.path.join(d, "det1", "truth_victims.csv"))))
        if len(vic) != 8 or any(not (0.5 - 1e-6 <= float(v["depth_m"]) <= 3.0 + 1e-6) for v in vic):
            fails.append("truth: victim count or depth outside [depthMin, depthMax]")
        anch = list(csv.DictReader(open(os.path.join(d, "det1", "obs_anchors.csv"))))
        if any(set(a) & {"x", "y", "z"} for a in anch[:1]):
            fails.append("obs_anchors carries true coordinates")
        # the high bracket must not receive more 2.4 GHz worker-point links than central
        def rx24(run):
            n = ok = 0
            for r in csv.DictReader(open(os.path.join(d, run, "obs_links.csv"))):
                if r["band"] == "2400" and 10000 <= int(r["rx"]) < 20000:
                    n += 1
                    ok += int(r["received"])
            return ok / max(n, 1)
        if rx24("high") > rx24("det1"):
            fails.append("bracket: high rubble receives more 2.4 GHz links than central")
    # leak: system-side scripts must not read World truth
    for f in glob.glob(os.path.join(ROOT, "scripts", "collapse", "*.py")):
        b = os.path.basename(f)
        if b.startswith("score_") or b in ("nist6055_slopes.py", "eta_uwb_brackets.py"):
            continue
        if re.search(r"truth_", open(f, encoding="utf-8").read()):
            fails.append(f"leak: scripts/collapse/{b} mentions truth_ files")
    if fails:
        print("G178 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print("G178 PASS (determinism, keyed independence, 4 refusals, truth sanity, bracket order, no-leak)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
