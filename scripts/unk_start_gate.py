#!/usr/bin/env python3
"""G130: --unknownStart, --traceSubjectConsistency and --measVarScale (S2-C).

usage: unk_start_gate.py <dir written by scripts/unk_start_gate.sh>

What a firing looks like on disk, per check:
  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  the three flags at their defaults, given explicitly, change an output (run_id.txt
           excepted): the null control.
  CONS     --traceSubjectConsistency changes a file other than the subject traces, or a subject
           trace is not the control's row for row with exactly pxx,pxy,pyy,mahalanobis,meanSigmaM
           appended.
  UNK      under --unknownStart a node's first row, when it fused no range (nAnchors 0), is not at
           the search box's centre (within 60 m: one cycle of dead reckoning); or any first row
           sits within 1 m of its truth (a truth-seeded filter; under a 433 m prior that is
           improbable by chance); or no node starts more than 100 m off (the check would then
           prove nothing); or the control's subjects do not start at their truth (under 5 m), so
           the flag would not be what changes the start. A first row that fused ranges is not
           held to the centre: the preset's Case D fuses a single range as a scalar update, which
           moves a wide prior a long way (seed 7: 126.6 m along the range, still 222 m off).
  MVS      at --measVarScale=4 the RMS sigma of the first cycle that fuses ranges is not exactly 2x
           the control's (sqrt 4), on the same cycle before the runs can diverge.
"""
import csv
import filecmp
import glob
import math
import os
import sys

EXTRA = ["pxx", "pxy", "pyy", "mahalanobis", "meanSigmaM"]


def rows(p):
    with open(p) as fh:
        return list(csv.DictReader(fh))


def main():
    d = sys.argv[1]
    res = []

    def rec(label, ok, why):
        res.append((label, ok, why))

    runs = ["base", "dflt", "cons", "unk", "mvs"]
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r))
           or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    rec("RUNS", not bad, f"runs not exiting 0 or missing: {bad}" if bad else "5 runs exit 0")
    if bad:
        return report(res)

    def files(r):
        return sorted(os.path.relpath(p, os.path.join(d, r))
                      for p in glob.glob(os.path.join(d, r, "**", "*"), recursive=True)
                      if os.path.isfile(p) and not p.endswith("run_id.txt"))

    fb = files("base")
    diff = [f for f in fb if not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "dflt", f), shallow=False)]
    rec("DEFAULT", fb == files("dflt") and not diff,
        f"defaults given explicitly: {len(fb)} files, {len(diff)} differ {diff[:3]}")

    other = [f for f in fb if not os.path.basename(f).startswith("subjtrace_")
             and not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "cons", f), shallow=False)]
    subj = [f for f in fb if os.path.basename(f).startswith("subjtrace_")]
    okc = bool(subj) and not other and fb == files("cons")
    for f in subj:
        a = open(os.path.join(d, "base", f)).read().splitlines()
        b = open(os.path.join(d, "cons", f)).read().splitlines()
        if len(a) != len(b) or b[0] != a[0] + "," + ",".join(EXTRA):
            okc = False
            continue
        for x, y in zip(a[1:], b[1:]):
            if not y.startswith(x + ",") or len(y.split(",")) != len(x.split(",")) + len(EXTRA):
                okc = False
                break
    rec("CONS", okc, f"{len(subj)} subject traces, other files differing: {other[:3]}")

    firsts, far, ctrl, near = [], 0, [], 0
    for p in sorted(glob.glob(os.path.join(d, "unk", "*trace_ca_tbl_full_seed7*.csv"))):
        r0 = rows(p)[0]
        if float(r0["errM"]) > 100.0:
            far += 1
        if float(r0["errM"]) < 1.0:
            near += 1
        if r0["nAnchors"] == "0":
            firsts.append(math.hypot(float(r0["estX"]), float(r0["estY"])))
    for p in sorted(glob.glob(os.path.join(d, "base", "subjtrace_ca_tbl_full_seed7*.csv"))):
        ctrl.append(float(rows(p)[0]["errM"]))
    oku = bool(firsts) and max(firsts) < 60.0 and far >= 1 and near == 0 and bool(ctrl) and max(ctrl) < 5.0
    rec("UNK", oku, f"range-free first rows {len(firsts)}, max distance from centre "
        f"{max(firsts) if firsts else float('nan'):.1f} m; nodes starting > 100 m off {far}, within 1 m {near}; "
        f"control subjects' start error max {max(ctrl) if ctrl else float('nan'):.2f} m")

    def first_sigma(r):
        for x in rows(os.path.join(d, r, "trace_ca_tbl_full_seed7.csv")):
            if float(x["meanSigmaM"]) > 0.0:
                return x["t"], float(x["meanSigmaM"])
        return None, float("nan")

    tc, sc = first_sigma("cons")
    tm, sm = first_sigma("mvs")
    ratio = sm / sc if sc > 0 else float("nan")
    rec("MVS", tc is not None and tc == tm and abs(ratio - 2.0) < 1e-3,
        f"first fused cycle t {tc} / {tm}, RMS sigma ratio {ratio:.6f} (expected 2)")
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G130 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
