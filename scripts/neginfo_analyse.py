#!/usr/bin/env python3
"""N.57 / N.51: does negative information separate subjects whose estimate is wrong from those whose estimate is right?

usage: neginfo_analyse.py <dir of runs with --traceNegInfo, one s<seed> subdirectory each>

Subject side (neginfo_*.csv): a subject-cycle FIRES when a finder its own table places within D of its estimate has
had no range for at least T seconds. Finder side (tasking_*.csv + nodes_*.csv): a roundrobin finder FIRES when it has
been within 20 m of the entry it steers on for 20 s and its target is not within 30 m of it -- "searched here, not
found", a 2.4 GHz/UWB contact range. Truth (subjtrace errM) classifies each cycle: wrong > 50 m, right <= 25 m.
"""
import collections
import csv
import glob
import math
import os
import sys


def main():
    root = sys.argv[1]
    rows, err, fres = [], {}, collections.Counter()
    for d in sorted(glob.glob(os.path.join(root, "s*"))):
        seed = d.rsplit("s", 1)[1]
        for f in glob.glob(os.path.join(d, f"subjtrace_ca_tbl_full_seed{seed}_subj*.csv")):
            for r in csv.DictReader(open(f)):
                err[(seed, int(r["subjectId"]), round(float(r["t"]), 2))] = float(r["errM"])
        for f in glob.glob(os.path.join(d, "neginfo_*.csv")):
            for r in csv.DictReader(open(f)):
                rows.append((seed, int(r["subjectId"]), round(float(r["t"]), 2), float(r["dEstM"]), float(r["silenceS"]),
                             float(r["entryAgeS"])))
        pos = {}
        for r in csv.DictReader(open(os.path.join(d, f"nodes_ca_tbl_full_seed{seed}.csv"))):
            pos[(round(float(r["t"]), 2), int(r["nodeId"]))] = (float(r["x"]), float(r["y"]))
        at = collections.defaultdict(float)
        for r in csv.DictReader(open(os.path.join(d, f"tasking_ca_tbl_full_seed{seed}.csv"))):
            if r["phase"] != "converge" or not r["entryX"]:
                continue
            t, fi, k = round(float(r["t"]), 2), int(r["finderId"]), int(r["targetId"])
            fp, sp, e = pos.get((t, fi)), pos.get((t, k)), err.get((seed, k, t))
            if fp is None or sp is None or e is None:
                continue
            at[fi] = at[fi] + 2.5 if math.hypot(fp[0] - float(r["entryX"]), fp[1] - float(r["entryY"])) <= 20 else 0.0
            fire = at[fi] >= 20 and math.hypot(fp[0] - sp[0], fp[1] - sp[1]) > 30
            fres[("wrong" if e > 50 else "right" if e <= 25 else "mid", fire)] += 1
    wrong = sum(1 for v in err.values() if v > 50)
    right = sum(1 for v in err.values() if v <= 25)
    print(f"subject-cycles: {len(err)}; wrong (> 50 m) {wrong}; right (<= 25 m) {right}")
    print("SUBJECT SIDE (a finder believed within D, unranged for T s)")
    for D in (30, 60, 100):
        for T in (10, 30, 60):
            fired = {}
            for (seed, k, t, de, si, age) in rows:
                if de <= D and si >= T and age <= 60:
                    fired[(seed, k, t)] = True
            tp = sum(1 for key in fired if err.get(key, 0) > 50)
            fp = sum(1 for key in fired if err.get(key, 1e9) <= 25)
            print(f"  D={D:3d} m T={T:2d} s: fires on {tp}/{wrong} wrong, {fp}/{right} right; "
                  f"precision {tp / max(1, len(fired)):.2f}")
    tpf, fpf, mpf = fres[("wrong", True)], fres[("right", True)], fres[("mid", True)]
    wf = tpf + fres[("wrong", False)]
    print("FINDER SIDE (at the entry 20 s, target not within 30 m)")
    print(f"  fires on {tpf}/{wf} wrong finder-cycles, {fpf} right, {mpf} mid; precision {tpf / max(1, tpf + fpf + mpf):.2f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
