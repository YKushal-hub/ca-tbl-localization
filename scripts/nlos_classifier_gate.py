#!/usr/bin/env python3
"""L.2.2 / G20 -- NLOS scores are PRODUCED, CHECKED against the model and FED to PCS, in a run.

WHY THIS EXISTS. The unit suite (ca_tbl_nlos_test) proves the leading-edge test, the
synthesis, the two receiver-side scores and the PCS hand-off on constructed inputs. It
cannot prove that a run scores anything, that the confusion matrix it prints is the one its
rows add up to, or that PCS ever reads the term. This gate reads what a RUN wrote.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). With --nlosClassifier the scenario writes, per
arm, nlos_links_<arm>_seed<N>.csv -- one row per decoded link it ranged, scored or not --
and nlos_summary_<arm>_seed<N>.csv: a confusion row per tier (868, 2.4, UWB) with the
unscored and lost links by modelled truth, and one pcs row describing PCS term 3 over the
run's fix cycles. The defects this guards against have the form of ABSENCE or of a count
that does not add up -- the flag unwired, no link scored, a matrix that is not its rows, a
term that never moves or is never fed -- so it COUNTS rows (rule 12) and RECOUNTS the
matrix and the printed block from the rows (rule 3); it does not scan for one bad value.

WHAT IT CHECKS
  1. Both files exist with the expected headers; the links file has at least one row; the
     summary has exactly the 868, 2.4 and UWB confusion rows and one pcs row.
  2. Every link row honours its detector's contract: a score is -1 (unscored, verdict -1)
     or lies in [0, 1] with verdict = (score >= threshold); a scored UWB row satisfies
     score = 1 - cirALe / cirAPeak (Physical sect 1.3) with the leading edge at or before
     the peak; a scored 868 or 2.4 row carries its z and score = ramp(z) (0 at z <= 1, 1 at
     z >= 3), and an 868 row needs at least 3 readings in its window.
  3. The confusion matrix and the unscored counts in the summary equal the recount of the
     rows, tier by tier.
  4. The L.2.2 done-when: at least one tier's matrix has every cell >= --min-cell (default
     5). A UWB-only run cannot pass this, and that is the recorded finding, not a bug in
     the gate: a decoded NLOS UWB frame does not occur in any regime tried.
  5. The pcs row: fixCycles >= termCycles >= fedCycles; the term was computed on at least
     one cycle, took at least 2 distinct values, changed at least once, was strictly inside
     (0,1) at least once, and stayed in [0, 1]. --require-fed: fedCycles == termCycles > 0
     (the feed is on and every scored cycle used it). --forbid-fed: fedCycles == 0.
  6. --stdout LOG: exactly one "NLOS classifier" block (run a single arm); its per-tier
     counts and its PCS line equal the summary.

usage: nlos_classifier_gate.py [--min-cell N] [--require-fed|--forbid-fed] [--stdout LOG]
                               <nlos_links csv> <nlos_summary csv>
exit 0 PASS, 1 FAIL, 2 usage.
"""
import csv
import math
import re
import sys

LCOLS = ["t", "cycle", "tier", "anchor", "distM", "truthNlos", "nlosDb", "kFactor", "snrDb",
         "score", "verdict", "cirSnrAccDb", "cirDirectPow", "cirLeNs", "cirPeakNs", "cirALe",
         "cirAPeak", "cirNoise", "cirClusters", "cirRays", "tofRangeM", "predRssiDbm",
         "measRssiDbm", "excessDb", "winN", "sRssiDb", "sSnrDb", "refMedian", "refSigma", "z"]
SCOLS = ["kind", "tier", "detector", "threshold", "tp", "fp", "fn", "tn", "unscoredLos",
         "unscoredNlos", "lostLos", "lostNlos", "meanScoreLos", "meanScoreNlos", "fixCycles",
         "termCycles", "fedCycles", "distinct", "changes", "interior", "min", "max", "mean",
         "differsFromTag"]
TIERS = ["868", "2.4", "UWB"]
DETECTORS = {"868": "rssi_snr_variability", "2.4": "tof_vs_rssi", "UWB": "cir_leading_edge"}


def fnum(s):
    try:
        return float(s)
    except (TypeError, ValueError):
        return float("nan")


def ramp(z):
    if not math.isfinite(z) or z <= 1.0:
        return 0.0
    if z >= 3.0:
        return 1.0
    return (z - 1.0) / 2.0


def main(argv):
    args = argv[1:]
    min_cell = 5
    fed_mode = None
    stdout_log = None
    while args and args[0].startswith("--"):
        a = args.pop(0)
        if a == "--min-cell" and args:
            min_cell = int(args.pop(0))
        elif a == "--require-fed":
            fed_mode = "require"
        elif a == "--forbid-fed":
            fed_mode = "forbid"
        elif a == "--stdout" and args:
            stdout_log = args.pop(0)
        else:
            print(__doc__)
            return 2
    if len(args) != 2:
        print(__doc__)
        return 2
    links_path, summary_path = args
    fails = []

    # ---- 1. files, headers, rows -------------------------------------------------
    try:
        with open(links_path, newline="") as f:
            lr = csv.reader(f)
            lhead = next(lr, None)
            links = [r for r in lr]
    except OSError as e:
        print(f"G20 FAIL: cannot read links file: {e}")
        return 1
    try:
        with open(summary_path, newline="") as f:
            sr = csv.reader(f)
            shead = next(sr, None)
            summ = [r for r in sr]
    except OSError as e:
        print(f"G20 FAIL: cannot read summary file: {e}")
        return 1
    if lhead != LCOLS:
        print(f"G20 FAIL: links header differs from the contract: {lhead}")
        return 1
    if shead != SCOLS:
        print(f"G20 FAIL: summary header differs from the contract: {shead}")
        return 1
    if not links:
        print("G20 FAIL: links file holds no rows -- no link was scored or even decoded")
        return 1
    rows = [dict(zip(LCOLS, r)) for r in links]
    if any(len(r) != len(LCOLS) for r in links):
        fails.append("a links row has the wrong number of fields")
    srows = [dict(zip(SCOLS, r)) for r in summ]
    conf = {r["tier"]: r for r in srows if r["kind"] == "confusion"}
    pcs = [r for r in srows if r["kind"] == "pcs"]
    if sorted(conf) != sorted(TIERS) or len(pcs) != 1 or len(srows) != 4:
        print(f"G20 FAIL: summary must hold 3 confusion rows and 1 pcs row, got "
              f"{[(r['kind'], r['tier']) for r in srows]}")
        return 1
    pcs = pcs[0]
    thr = fnum(conf["868"]["threshold"])
    if not (0.0 < thr < 1.0) or any(fnum(conf[t]["threshold"]) != thr for t in TIERS):
        fails.append(f"threshold must be one value in (0,1), got "
                     f"{[conf[t]['threshold'] for t in TIERS]}")

    # ---- 2. per-row contract, and 3. the recount -----------------------------------
    count = {t: {"tp": 0, "fp": 0, "fn": 0, "tn": 0, "unscoredLos": 0, "unscoredNlos": 0}
             for t in TIERS}
    bad = []
    for i, r in enumerate(rows, start=2):
        t = r["tier"]
        if t not in TIERS or r["truthNlos"] not in ("0", "1"):
            bad.append(f"line {i}: tier '{t}' truth '{r['truthNlos']}'")
            continue
        truth = r["truthNlos"] == "1"
        s = fnum(r["score"])
        v = r["verdict"]
        if s == -1.0:
            if v != "-1":
                bad.append(f"line {i}: unscored row with verdict {v}")
            count[t]["unscoredNlos" if truth else "unscoredLos"] += 1
            continue
        if not (0.0 <= s <= 1.0):
            bad.append(f"line {i}: score {r['score']} outside [0,1]")
            continue
        want = "1" if s >= thr else "0"
        if v != want:
            bad.append(f"line {i}: verdict {v} but score {s} against threshold {thr}")
        if t == "UWB":
            ale, apk = fnum(r["cirALe"]), fnum(r["cirAPeak"])
            le, pk = fnum(r["cirLeNs"]), fnum(r["cirPeakNs"])
            if not (apk > 0.0) or abs(s - (1.0 - ale / apk)) > 1e-6 or not (le <= pk):
                bad.append(f"line {i}: UWB score {s} is not 1 - {ale}/{apk}, or edge {le} after "
                           f"peak {pk}")
        else:
            z = fnum(r["z"])
            if not math.isfinite(z) or abs(s - ramp(z)) > 1e-6:
                bad.append(f"line {i}: {t} score {s} is not ramp(z={r['z']})")
            if t == "868" and not (fnum(r["winN"]) >= 3):
                bad.append(f"line {i}: 868 scored with {r['winN']} readings in the window")
        cell = ("tp" if truth else "fp") if want == "1" else ("fn" if truth else "tn")
        count[t][cell] += 1
    if bad:
        fails.append(f"{len(bad)} rows break the detector contract, first: {bad[0]}")
    for t in TIERS:
        for k, n in count[t].items():
            if fnum(conf[t][k]) != n:
                fails.append(f"{t} {k}: summary says {conf[t][k]}, the rows add up to {n}")

    # ---- 4. the done-when ---------------------------------------------------------
    full = [t for t in TIERS
            if min(count[t]["tp"], count[t]["fp"], count[t]["fn"], count[t]["tn"]) >= min_cell]
    if not full:
        fails.append(f"no tier has every confusion cell >= {min_cell}: " + "; ".join(
            f"{t} tp {count[t]['tp']} fp {count[t]['fp']} fn {count[t]['fn']} tn {count[t]['tn']}"
            for t in TIERS))

    # ---- 5. the PCS term ------------------------------------------------------------
    fix, term, fed = (int(fnum(pcs[k])) if math.isfinite(fnum(pcs[k])) else -1
                      for k in ("fixCycles", "termCycles", "fedCycles"))
    distinct, changes, interior = (int(fnum(pcs[k])) if math.isfinite(fnum(pcs[k])) else -1
                                   for k in ("distinct", "changes", "interior"))
    lo, hi = fnum(pcs["min"]), fnum(pcs["max"])
    if not (fix >= term >= fed >= 0):
        fails.append(f"pcs: need fixCycles >= termCycles >= fedCycles >= 0, got {fix}/{term}/{fed}")
    if term < 1 or distinct < 2 or changes < 1 or interior < 1 or not (0.0 <= lo < hi <= 1.0):
        fails.append(f"pcs: the term does not vary -- term cycles {term}, distinct {distinct}, "
                     f"changes {changes}, interior {interior}, range [{lo}, {hi}]")
    if fed_mode == "require" and not (fed == term and fed > 0):
        fails.append(f"pcs: --require-fed but fed on {fed} of {term} term cycles")
    if fed_mode == "forbid" and fed != 0:
        fails.append(f"pcs: --forbid-fed but fed on {fed} cycles")

    # ---- 6. the printed block --------------------------------------------------------
    if stdout_log is not None:
        try:
            text = open(stdout_log, errors="replace").read()
        except OSError as e:
            text = ""
            fails.append(f"cannot read stdout log: {e}")
        nblocks = text.count("NLOS classifier (threshold")
        if nblocks != 1:
            fails.append(f"stdout: expected exactly one NLOS classifier block, found {nblocks}")
        for t in TIERS:
            m = re.search(r"^\s+" + re.escape(t) + " " + DETECTORS[t] +
                          r": tp (\d+) fp (\d+) fn (\d+) tn (\d+); unscored LOS (\d+) NLOS (\d+);"
                          r" lost before any reading LOS (\d+) NLOS (\d+);", text, re.M)
            if not m:
                fails.append(f"stdout: no {t} line")
                continue
            got = [int(x) for x in m.groups()]
            want = [int(fnum(conf[t][k])) for k in ("tp", "fp", "fn", "tn", "unscoredLos",
                                                    "unscoredNlos", "lostLos", "lostNlos")]
            if got != want:
                fails.append(f"stdout: {t} printed {got}, summary {want}")
        m = re.search(r"NLOS PCS term 3 = mean\(1 - NLOS_score\): fix cycles (\d+), with a "
                      r"scored link (\d+), fed to PCS (\d+); distinct values (\d+), changes (\d+)",
                      text)
        if not m:
            fails.append("stdout: no PCS term line")
        elif [int(x) for x in m.groups()] != [fix, term, fed, distinct, changes]:
            fails.append(f"stdout: PCS line {m.groups()} != summary "
                         f"{[fix, term, fed, distinct, changes]}")

    for t in TIERS:
        c = count[t]
        print(f"  {t:>3} {DETECTORS[t]:<21} tp {c['tp']:>5} fp {c['fp']:>5} fn {c['fn']:>5} "
              f"tn {c['tn']:>5}  unscored {c['unscoredLos'] + c['unscoredNlos']:>4}  lost LOS "
              f"{conf[t]['lostLos']} NLOS {conf[t]['lostNlos']}")
    print(f"  PCS term: fix cycles {fix}, term cycles {term}, fed {fed}, distinct {distinct}, "
          f"changes {changes}, interior {interior}, range [{lo}, {hi}]")
    if fails:
        for f_ in fails:
            print(f"G20 FAIL: {f_}")
        return 1
    print(f"G20 PASS: {len(rows)} link rows; full matrix on {', '.join(full)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
