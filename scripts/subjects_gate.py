#!/usr/bin/env python3
"""G46: every field node is a measured subject (docs/SPEC_CONFORMANCE_PLAN.md MS-1,
--measuredSubjects; the plan names it "G22 (subjects)").

The spec localises every field node ("a five-person search naturally produces five competing
escalation requests"; the final audit's deployment Q2 is 3 gateways, 5 field nodes). Until
now only tag 0 ran the pipeline; the other tags were protocol peers or fixed-tier relays, and
PeerRequestDriver sent constant 128/128 requests in their name. Under --measuredSubjects=N,
tags 2..N run the core of tag 0's cycle on their own accounts -- ranging on the active tier,
the tier-1 floor, Step() with its own escalation state machine, the handshake with their own
terms, the downlink, listen/sleep -- each in its own slot of a round before tag 0's instant.
A relay that is a subject is ranged, charged and stepped once, by the subject pipeline.

Reads a directory written by scripts/subjects_gate.sh (five: 5 subjects x 2 arms x 2 seeds;
one: the explicit default; relay: 3 subjects among 5 relays under --relaySlots; r_big, r_zero,
warn). Every check runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --measuredSubjects=1 writes no subject_summary.csv, no subjtrace_ file, no MS-1
          line and digests neither.
  N       the five run IS a five-subject run: its recorded invocation says
          --measuredSubjects=5 and every MS-1 line says 5.
  ROWS    subject_summary.csv has N x arms x seeds rows, one per (arm, seed, subject), and
          the subjects are ids 1..N -- a row count, not a non-empty test (rule 12).
  SUBJ1   subject 1's row is tag 0's arm_summary row, column for column, byte for byte.
  STEPS   every subject has steps == duration/dt, its three tier cycle counts sum to steps,
          and (subjects 2..N) its subjtrace has steps rows: a relay stepped twice in a cycle
          counts two policy cycles for one step.
  RECOUNT fixCycles, cycles_<tier> and escalations recounted from the traces with no help
          from the writer (rule 3): hadFix, activeTier and escalated; and rmseM from errM.
  SIX     sect 6 per subject: a tier with no cycles has tx_ == 0 and j_ == sleep_ -- charged
          for sleeping and nothing else. Non-vacuous: at least one such tier in each arm.
  OWN     subjects send their own handshakes: static_868 subjects request nothing; some
          ca_tbl_full subject 2..N does; each MS-1 line agrees with the summary.
  DIGEST  subject_summary.csv and every subjtrace_ file is in run_id.txt with its sha256.
  R       N > --nTags and N = 0 are refused (exit 1); a tag-0-only mechanism is warned about.
"""
import csv
import hashlib
import math
import os
import re
import sys

TIERS = {"868MHz": "868", "2.4GHz": "2p4", "UWB": "uwb"}
SUBJ = re.compile(r"subj (\d+) steps (\d+) fixes (\d+) esc (\d+) requests (\d+)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def invocation(d, n):
    with open(os.path.join(d, n, "run_id.txt")) as f:
        return f.read()


def digests(d, n):
    out = {}
    for ln in invocation(d, n).splitlines():
        p = ln.split()
        if len(p) == 2 and len(p[0]) == 64:
            out[p[1]] = p[0]
    return out


def steps_expected(d, n):
    m = re.search(r"duration=([0-9.]+)s\s+dt=([0-9.]+)s", log(d, n))
    return int(math.floor(float(m.group(1)) / float(m.group(2))))


def trace_name(run_dir, arm, seed, sid):
    if sid == "1":
        return os.path.join(run_dir, f"trace_{arm}_seed{seed}.csv")
    return os.path.join(run_dir, f"subjtrace_{arm}_seed{seed}_subj{sid}.csv")


def main():
    if len(sys.argv) != 2:
        print("usage: subjects_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError, StopIteration) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    READ = ("five", "one", "relay")

    def c_runs():
        bad = [n for n in READ if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fs = os.listdir(os.path.join(d, "one"))
        extra = [f for f in fs if f == "subject_summary.csv" or f.startswith("subjtrace_")]
        lines = log(d, "one").count("MS-1")
        dg = [f for f in digests(d, "one") if f == "subject_summary.csv" or f.startswith("subjtrace_")]
        check("NULL", not extra and lines == 0 and not dg and "trace_ca_tbl_full_seed7.csv" in fs,
              f"--measuredSubjects=1: subject files {len(extra)}, MS-1 lines {lines}, digested "
              f"subject files {len(dg)} (need 0, 0, 0)")
    safe("NULL", c_null)

    def c_n():
        inv = "--measuredSubjects=5" in invocation(d, "five")
        ns = re.findall(r"MS-1 subjects: (\d+);", log(d, "five"))
        check("N", inv and len(ns) == 4 and set(ns) == {"5"},
              f"five: invocation {'records' if inv else 'does NOT record'} --measuredSubjects=5; "
              f"MS-1 lines {ns} (need four, all 5)")
    safe("N", c_n)

    def c_rows():
        out = []
        for run, n, arms, seeds in (("five", 5, 2, 2), ("relay", 3, 1, 1)):
            s = rows(os.path.join(d, run, "subject_summary.csv"))
            keys = [(r["arm"], r["seed"], r["subjectId"]) for r in s]
            groups = {}
            for a, sd, i in keys:
                groups.setdefault((a, sd), set()).add(i)
            ids_ok = all(g == {str(k) for k in range(1, n + 1)} for g in groups.values())
            ok = len(s) == n * arms * seeds and len(set(keys)) == len(keys) and \
                len(groups) == arms * seeds and ids_ok
            out.append((run, ok, f"{run}: {len(s)} rows (need {n * arms * seeds}), "
                                 f"{len(groups)} (arm, seed) groups, ids 1..{n} "
                                 f"{'each' if ids_ok else 'NOT each'}"))
        check("ROWS", all(o[1] for o in out), "; ".join(o[2] for o in out))
    safe("ROWS", c_rows)

    def c_subj1():
        bad, n = [], 0
        for run in ("five", "relay"):
            arm = {(r["arm"], r["seed"]): r for r in rows(os.path.join(d, run, "arm_summary.csv"))}
            for r in rows(os.path.join(d, run, "subject_summary.csv")):
                if r["subjectId"] != "1":
                    continue
                a = arm[(r["arm"], r["seed"])]
                shared = [k for k in r if k in a]
                n += 1
                bad += [(run, r["arm"], r["seed"], k, r[k], a[k]) for k in shared if r[k] != a[k]]
        check("SUBJ1", n == 5 and not bad,
              f"{n} subject-1 rows (need 5) against arm_summary; differing columns {len(bad)} "
              f"(need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("SUBJ1", c_subj1)

    def c_steps():
        bad, n = [], 0
        for run in ("five", "relay"):
            want = steps_expected(d, run)
            rd = os.path.join(d, run)
            for r in rows(os.path.join(rd, "subject_summary.csv")):
                n += 1
                cyc = sum(int(r["cycles_" + t]) for t in ("868", "2p4", "uwb"))
                if int(r["steps"]) != want or cyc != want:
                    bad.append((run, r["arm"], r["seed"], r["subjectId"], r["steps"], cyc, want))
                if r["subjectId"] != "1":
                    tr = len(rows(trace_name(rd, r["arm"], r["seed"], r["subjectId"])))
                    if tr != want:
                        bad.append((run, r["subjectId"], "trace rows", tr, want))
        check("STEPS", n > 0 and not bad,
              f"{n} subject rows; steps, policy cycles or trace rows != duration/dt: {len(bad)} "
              f"(need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("STEPS", c_steps)

    def c_recount():
        bad, n = [], 0
        for run in ("five", "relay"):
            rd = os.path.join(d, run)
            for r in rows(os.path.join(rd, "subject_summary.csv")):
                tr = rows(trace_name(rd, r["arm"], r["seed"], r["subjectId"]))
                n += 1
                fix = sum(x["hadFix"] == "1" for x in tr)
                esc = sum(x["escalated"] == "1" for x in tr)
                cyc = {t: sum(TIERS[x["activeTier"]] == t for x in tr) for t in ("868", "2p4", "uwb")}
                err = [float(x["errM"]) for x in tr]
                rmse = math.sqrt(sum(e * e for e in err) / len(err))
                got = (int(r["fixCycles"]), int(r["escalations"]),
                       tuple(int(r["cycles_" + t]) for t in ("868", "2p4", "uwb")))
                want = (fix, esc, tuple(cyc[t] for t in ("868", "2p4", "uwb")))
                if got != want or abs(rmse - float(r["rmseM"])) > 1e-3:
                    bad.append((run, r["arm"], r["seed"], r["subjectId"], got, want,
                                r["rmseM"], round(rmse, 4)))
        check("RECOUNT", n > 0 and not bad,
              f"{n} subjects; fixCycles/escalations/cycles/rmseM not as recounted from the "
              f"traces: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("RECOUNT", c_recount)

    def c_six():
        bad, per_arm = [], {}
        for run in ("five", "relay"):
            for r in rows(os.path.join(d, run, "subject_summary.csv")):
                for t in ("868", "2p4", "uwb"):
                    if int(r["cycles_" + t]) != 0:
                        continue
                    per_arm[r["arm"]] = per_arm.get(r["arm"], 0) + 1
                    if float(r["tx_" + t]) != 0.0 or \
                            abs(float(r["j_" + t]) - float(r["sleep_" + t])) > 1e-9:
                        bad.append((run, r["arm"], r["seed"], r["subjectId"], t,
                                    r["tx_" + t], r["j_" + t], r["sleep_" + t]))
        ok = not bad and per_arm.get("static_868", 0) > 0 and per_arm.get("ca_tbl_full", 0) > 0
        check("SIX", ok,
              f"unused (subject, tier) pairs per arm {per_arm} (need > 0 in each); charged for "
              f"more than sleep: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("SIX", c_six)

    def c_own():
        s = rows(os.path.join(d, "five", "subject_summary.csv"))
        st = [r for r in s if r["arm"] == "static_868"]
        cf = [r for r in s if r["arm"] == "ca_tbl_full" and r["subjectId"] != "1"]
        st_bad = [r["subjectId"] for r in st if r["escRequests"] != "0" or r["escalations"] != "0"]
        own = sum(int(r["escRequests"]) > 0 for r in cf)
        want = sorted(sorted((r["subjectId"], r["steps"], r["fixCycles"], r["escalations"],
                              r["escRequests"]) for r in s if r["arm"] == a and r["seed"] == sd
                             and r["subjectId"] != "1")
                      for a, sd in {(r["arm"], r["seed"]) for r in s})
        got = sorted(sorted(SUBJ.findall(ln)) for ln in log(d, "five").splitlines()
                     if "MS-1 subjects:" in ln)
        check("OWN", not st_bad and own > 0 and got == want,
              f"static_868 subjects that requested or escalated {st_bad} (need none); ca_tbl_full "
              f"subjects 2..N with requests {own} (need > 0); MS-1 lines "
              f"{'agree' if got == want else 'DISAGREE'} with subject_summary")
    safe("OWN", c_own)

    def c_digest():
        bad, n = [], 0
        for run in ("five", "relay"):
            dg = digests(d, run)
            for f in os.listdir(os.path.join(d, run)):
                if f != "subject_summary.csv" and not f.startswith("subjtrace_"):
                    continue
                n += 1
                with open(os.path.join(d, run, f), "rb") as fh:
                    h = hashlib.sha256(fh.read()).hexdigest()
                if dg.get(f) != h:
                    bad.append((run, f, "absent" if f not in dg else "differs"))
        check("DIGEST", n == 20 and not bad,
              f"{n} subject files (need 20); not digested as written: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("DIGEST", c_digest)

    def c_ref():
        for n, need in (("r_big", "FATAL: --measuredSubjects=5 must be 1..3"),
                        ("r_zero", "FATAL: --measuredSubjects=0 must be 1..")):
            got = rc(d, n)
            has = need in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
        got = rc(d, "warn")
        has = "WARNING: --measuredSubjects > 1 runs these for subject 1 (tag 0) only: --tier1Tdoa" \
            in log(d, "warn")
        check("R", got == 0 and has, f"warn: exit {got} (need 0), tag-0-only warning "
                                     f"{'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G46 {tag:7s} {name:7s} {why}")
    print(f"  G46: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
