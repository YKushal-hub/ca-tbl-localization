#!/usr/bin/env python3
"""G30: the integrity check's eligibility is read from the NLOS score, as the spec says
(docs/SPEC_CONFORMANCE_PLAN.md DC-8).

The spec's condition for calling a fix SUSPECT is Mahalanobis disagreement "AND NLOS_score
is LOW" (Physical 2.6). The default reads "every link tagged LOS" instead, and under the
default tag that set is empty: the control run below evaluates ~300 fixes and finds 0
all-LOS (TECH_DEBT I.1). Under --integrityNlosFromScore a fix is eligible when the mean
NLOS_score over its scored links is below --nlosScoreThreshold; with none scored the tag
decides.

Reads a directory written by scripts/integrity_score_gate.sh: one sub-directory per run,
<name>.log and <name>.rc beside it. Every check runs and is reported; exit 1 if any
failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  OFF    the control (classifier on, flag off) writes no integrity_* file and no DC-8
         line, and has no all-LOS evaluated fix -- the structural zero the flag exists for.
  JOIN   INDEPENDENT RECOUNT (rule 3), per row of integrity_ca_tbl_full_seed7.csv: the
         nlos_links_* rows at the same t on the trace's fixTier are the fix set --
         nRanges of them -- and nScored of them carry a score, whose mean of min(1, s) is
         meanScore to 1e-6. The scorer writes those rows; the policy never reads them.
  RULE   eligible == (basis 1 ? meanScore < thr : nNlosTag == 0) on every row, and
         suspect == eligible AND mahalanobis > 9.21 -- at the threshold each run was given
         (0.5 and 0.1), so a threshold that did not reach the policy fails at 0.1, where
         85 rows sit in [0.1, 0.5) at seed 7.
  TRACE  every evaluated row is a trace row with the same nNlos and mahalanobis; the
         trace's suspect rows are exactly the file's; arm_summary's suspects agree.
  COUNT  the stdout line's eligible / not / fell-back / suspects equal the file's.
  POP    the population the flag exists for: eligible fixes > 0 and suspects > 0.
  ARM    only the arm that runs the check writes the file: none for ca_tbl_no_integrity.
  R      the flag without --nlosClassifier is refused, exit 1, for that reason.
"""
import csv
import os
import sys
from collections import defaultdict

MAHAL = 9.21  # chi-square(2) at 0.99, the policy's mahalanobisThreshold
TIER = {"868MHz": "868", "2.4GHz": "2.4", "UWB": "UWB"}


def rows(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def key(t):
    return round(float(t), 4)


def main():
    if len(sys.argv) != 2:
        print("usage: integrity_score_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def p(run, f):
        return os.path.join(d, run, f)

    INT = "integrity_ca_tbl_full_seed7.csv"
    TRACE = "trace_ca_tbl_full_seed7.csv"
    LINKS = "nlos_links_ca_tbl_full_seed7.csv"
    runs = {"score": 0.5, "score_t01": 0.1}

    def c_runs():
        bad = [n for n in ["tag", "score", "score_t01"] if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_off():
        files = [f for f in os.listdir(os.path.join(d, "tag")) if f.startswith("integrity_")]
        line = "DC-8" in log(d, "tag")
        t = rows(p("tag", TRACE))
        ev = [r for r in t if float(r["mahalanobis"]) > 0.0]
        allLos = sum(r["nNlos"] == "0" for r in ev)
        check("OFF", not files and not line and len(ev) > 0 and allLos == 0,
              f"control: {len(files)} integrity_* files and {'a' if line else 'no'} DC-8 line "
              f"(need none); {allLos} all-LOS among {len(ev)} evaluated fixes (need 0 of > 0: "
              "the structural zero)")
    safe("OFF", c_off)

    for run, thr in runs.items():
        def c_join(run=run):
            L = defaultdict(list)
            for r in rows(p(run, LINKS)):
                L[(key(r["t"]), r["tier"])].append(r)
            bad_n = bad_s = bad_m = 0
            for r in rows(p(run, INT)):
                links = L.get((key(r["t"]), TIER[r["fixTier"]]), [])
                sc = [min(1.0, float(x["score"])) for x in links if float(x["score"]) >= 0.0]
                bad_n += len(links) != int(r["nRanges"])
                bad_s += len(sc) != int(r["nScored"])
                if r["basis"] == "1":
                    bad_m += (not sc) or abs(sum(sc) / len(sc) - float(r["meanScore"])) > 1e-6
                elif r["basis"] == "2":
                    bad_m += bool(sc) or float(r["meanScore"]) != -1.0
                else:
                    bad_m += 1
            check("JOIN", bad_n == 0 and bad_s == 0 and bad_m == 0,
                  f"{run}: recounted from nlos_links -- {bad_n} rows whose fix set size differs, "
                  f"{bad_s} whose scored count differs, {bad_m} whose mean or basis differs (need 0)")
        safe("JOIN", c_join)

        def c_rule(run=run, thr=thr):
            R = rows(p(run, INT))
            bad_e = bad_s = edge = band = 0
            for r in R:
                m = float(r["meanScore"])
                elig = (m < thr) if r["basis"] == "1" else (r["nNlosTag"] == "0")
                bad_e += elig != (r["eligible"] == "1")
                mh = float(r["mahalanobis"])
                edge += abs(mh - MAHAL) < 1e-4
                bad_s += (r["suspect"] == "1") != (elig and mh > MAHAL)
                band += r["basis"] == "1" and thr <= m < 0.5
            ok = bad_e == 0 and bad_s == 0 and edge == 0 and len(R) > 0
            if thr < 0.5:
                ok = ok and band > 0
            check("RULE", ok,
                  f"{run} (threshold {thr}): {bad_e} rows whose eligibility breaks the rule, "
                  f"{bad_s} whose suspect verdict does (need 0 of {len(R)}; {edge} within "
                  f"rounding of 9.21)" + (f"; {band} rows in [{thr}, 0.5), where a threshold "
                                           "that did not arrive would decide differently"
                                           if thr < 0.5 else ""))
        safe("RULE", c_rule)

        def c_trace(run=run):
            R = rows(p(run, INT))
            T = {key(r["t"]): r for r in rows(p(run, TRACE))}
            miss = sum(1 for r in R if key(r["t"]) not in T)
            diff = sum(1 for r in R if key(r["t"]) in T and (
                T[key(r["t"])]["nNlos"] != r["nNlosTag"] or
                abs(float(T[key(r["t"])]["mahalanobis"]) - float(r["mahalanobis"])) > 1e-4))
            fs = {key(r["t"]) for r in R if r["suspect"] == "1"}
            ts = {k for k, r in T.items() if r["suspect"] == "1"}
            sm = [r for r in rows(p(run, "arm_summary.csv")) if r["arm"] == "ca_tbl_full"]
            ns = int(sm[0]["suspects"]) if len(sm) == 1 else -1
            check("TRACE", miss == 0 and diff == 0 and fs == ts and ns == len(ts),
                  f"{run}: {miss} file rows missing from the trace, {diff} disagreeing on nNlos or "
                  f"mahalanobis (need 0); suspect cycles file {len(fs)}, trace {len(ts)}, "
                  f"the same set: {fs == ts}; arm_summary suspects {ns}")
        safe("TRACE", c_trace)

        def c_count(run=run):
            R = rows(p(run, INT))
            e = sum(r["basis"] == "1" and r["eligible"] == "1" for r in R)
            ne = sum(r["basis"] == "1" and r["eligible"] == "0" for r in R)
            fb = sum(r["basis"] == "2" for r in R)
            su = sum(r["suspect"] == "1" for r in R)
            lines = [l for l in log(d, run).splitlines() if "DC-8 integrity eligibility" in l]
            want = (f"DC-8 integrity eligibility from NLOS_score: {e} eligible and {ne} not, of fix "
                    f"cycles with a scored link; {fb} with none fell back to the tag; {su} suspects")
            ok = len(lines) == 1 and lines[0].strip() == want
            check("COUNT", ok, f"{run}: stdout {'matches' if ok else 'DIFFERS from'} the file's "
                               f"{e} / {ne} / {fb} / {su}")
            if run == "score":
                check("POP", e > 0 and su > 0,
                      f"{run}: {e} eligible fixes and {su} suspects (need both > 0)")
        safe("COUNT", c_count)

    def c_arm():
        other = [f for f in os.listdir(os.path.join(d, "score")) if f.startswith("integrity_")
                 and "ca_tbl_no_integrity" in f]
        tr = os.path.exists(p("score", "trace_ca_tbl_no_integrity_seed7.csv"))
        check("ARM", tr and not other,
              f"ca_tbl_no_integrity ran ({tr}) and wrote {len(other)} integrity_* files (need 0)")
    safe("ARM", c_arm)

    def c_ref():
        got = rc(d, "r_noclf")
        has = "--integrityNlosFromScore needs --nlosClassifier=true" in log(d, "r_noclf")
        check("R", got == 1 and has, f"r_noclf: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G30 {tag:7s} {name:5s} {why}")
    print(f"  G30: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
