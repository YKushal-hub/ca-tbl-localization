#!/usr/bin/env python3
"""G113: --unknownStartSigmaM (docs/TECH_DEBT.md N.39).

Every filter starts at its usual seed point plus an N(0, s^2) offset per axis, with a prior
std of at least s, drawn from its own generator keyed on (seed, node).

Reads a directory written by scripts/unknown_start_gate.sh (base, off, on). Every check runs
and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  RC      a run exited non-zero.
  NULL    --unknownStartSigmaM=0 is the flag absent: every output file byte-identical except
          run_id.txt (its invocation line differs).
  OFFSET  at s = 100 the subjects' first-row error, on rows with no range (nAnchors 0, so the
          estimate is the seed point carried one prediction), has a median above 20 m;
          flags absent it is below 1 m (seeded at truth). A fire is the offset not applied,
          or applied without the flag.
  PRIOR   at s = 100 those rows carry traceSigma >= 2 * 100^2 (the prior is at least s per
          axis). A fire is an offset under a narrower prior than s, which would be
          overconfident by construction.
  ARMS    a subject with no range on its first row in both arms starts with the same error in
          both: the offset is keyed on (seed, node), not on the arm (rule 8 pairing).
  STREAM  the true trajectories (trueX, trueY of tag 0 and every subject) are identical with
          the flag on and off: the offset drew from no ns-3 stream. A fire is any moved row.
"""
import csv
import filecmp
import glob
import os
import statistics
import sys

ARMS = ["ca_tbl_full", "static_2p4"]


def rows(p):
    with open(p) as h:
        return list(csv.DictReader(h))


def subj_files(d, n, arm):
    return sorted(glob.glob(os.path.join(d, n, f"subjtrace_{arm}_seed1_subj*.csv")))


def main():
    if len(sys.argv) != 2:
        print("usage: unknown_start_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    names = ["base", "off", "on"]
    for n in names:
        if not os.path.isfile(os.path.join(d, n + ".rc")) or not os.path.isdir(os.path.join(d, n)):
            print(f"  G113 MISSING input {n}")
            return 2
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    bad = [n for n in names if open(os.path.join(d, n + ".rc")).read().strip() != "0"]
    check("RC", not bad, f"non-zero exit: {bad}")

    fb = sorted(os.listdir(os.path.join(d, "base")))
    fo = sorted(os.listdir(os.path.join(d, "off")))
    diff = [f for f in fb if f != "run_id.txt" and (f not in fo or not filecmp.cmp(
        os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False))]
    diff += [f for f in fo if f not in fb]
    check("NULL", fb and not diff, f"{len(fb)} files, differing: {diff[:5]}")

    def first(n, arm):
        out = {}
        for f in subj_files(d, n, arm):
            r = rows(f)[0]
            out[os.path.basename(f).rsplit("_", 1)[1]] = r
        return out

    on0 = [r for arm in ARMS for r in first("on", arm).values() if r["nAnchors"] == "0"]
    base0 = [r for arm in ARMS for r in first("base", arm).values() if r["nAnchors"] == "0"]
    mon = statistics.median(float(r["errM"]) for r in on0) if on0 else -1.0
    mbase = statistics.median(float(r["errM"]) for r in base0) if base0 else -1.0
    check("OFFSET", on0 and base0 and mon > 20.0 and 0.0 <= mbase < 1.0,
          f"median first-row error, no-range rows: on {mon:.2f} m (n {len(on0)}), absent {mbase:.2f} m (n {len(base0)})")

    tmin = min((float(r["traceSigma"]) for r in on0), default=-1.0)
    check("PRIOR", on0 and tmin >= 2.0 * 100.0 ** 2, f"min first-row traceSigma at s = 100: {tmin:.1f}")

    fa, fs = first("on", ARMS[0]), first("on", ARMS[1])
    both = [k for k in fa if k in fs and fa[k]["nAnchors"] == "0" and fs[k]["nAnchors"] == "0"]
    mism = [k for k in both if fa[k]["errM"] != fs[k]["errM"]]
    check("ARMS", both and not mism, f"subjects with no first-row range in both arms {len(both)}, differing {mism}")

    moved = 0
    total = 0
    for arm in ARMS:
        for f in [f"trace_{arm}_seed1.csv"] + [os.path.basename(p) for p in subj_files(d, "base", arm)]:
            a, b = rows(os.path.join(d, "base", f)), rows(os.path.join(d, "on", f))
            total += len(a)
            moved += len(a) != len(b) or sum(1 for x, y in zip(a, b) if (x["trueX"], x["trueY"]) != (y["trueX"], y["trueY"]))
    check("STREAM", total > 0 and moved == 0, f"true-position rows compared {total}, moved {moved}")

    nbad = 0
    for name, ok, why in res:
        print(f"  G113 {'ok  ' if ok else 'FAIL'} {name:8s} {why}")
        nbad += 0 if ok else 1
    print(f"  G113 {len(res) - nbad}/{len(res)} checks pass")
    return 1 if nbad else 0


if __name__ == "__main__":
    sys.exit(main())
