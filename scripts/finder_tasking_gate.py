#!/usr/bin/env python3
"""G47: finders search for specific people (docs/SPEC_CONFORMANCE_PLAN.md MS-1,
--finderTasking=roundrobin).

The spec's rescuers search for identified people (Network sect 1: every node has an
identity). Every finder steered on tag 0's table entry, so with five measured subjects four
were never searched for. Under --finderTasking=roundrobin finder k (0-based, id 501 + k)
steers on subject (k mod N) + 1's freshest entry, N = --measuredSubjects, and the
--finderDispersal spread is over the finders tasked to the same subject: finder k is the
(k div N)-th of the ceil((nFinders - k mod N) / N) of them. The rule is OURS.

Reads a directory written by scripts/finder_tasking_gate.sh (rr2, rr5, sweep; def and tag0;
r_bad, r_one, r_nofind, r_mode0). Every check runs and is reported; exit 1 if any failed, 2
if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --finderTasking=tag0 is the default: every file byte-identical, no tasking_ file,
          no MS-1 tasking line.
  TARGET  every row's targetId is subject (k mod N) + 1 of its finder k.
  ENTRY   the entry a converging finder steers on is its TARGET's own published estimate:
          the target's trace row of the cycle before the entry's timestamp (publication
          precedes the cycle's Step) or of that cycle, to 2e-4 m -- not tag 0's. Entries
          older than the first trace row are the prior and are counted apart. Non-vacuous:
          both subjects at rr2, and subject 1 at rr5, have matched rows.
  GOAL    the goal is the entry, displaced by --finderDispersal sigmas at angle
          2 pi (k div N) / group size when the group has more than one finder (1e-3 m).
  COUNTS  one row per finder per cycle (the runs dispatch at t = 0); the phases per finder
          equal the MS-1 tasking line; the sweep run has sweep rows.
  DIGEST  each tasking_ file is in run_id.txt with its sha256.
  R       an unknown value, N = 1, no finders and --finderMode=0 are refused (exit 1).
"""
import csv
import hashlib
import math
import os
import re
import sys

LINE = re.compile(r"finder (\d+) -> subject (\d+) sweep (\d+) converge (\d+) blind (\d+);")
RUNS = ("rr2", "rr5", "sweep")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def runid(d, n):
    with open(os.path.join(d, n, "run_id.txt")) as f:
        return f.read()


def arg(d, n, name, default=None):
    m = re.search(r"--" + name + r"=(\S+)", runid(d, n))
    return m.group(1) if m else default


def timing(d, n):
    m = re.search(r"duration=([0-9.]+)s\s+dt=([0-9.]+)s", log(d, n))
    return float(m.group(1)), float(m.group(2))


def tasking(d, n):
    return rows(os.path.join(d, n, "tasking_ca_tbl_full_seed7.csv"))


def subject_trace(d, n, sid, dt):
    f = "trace_ca_tbl_full_seed7.csv" if sid == 1 else f"subjtrace_ca_tbl_full_seed7_subj{sid}.csv"
    out = {}
    for r in rows(os.path.join(d, n, f)):
        out[int(round(float(r["t"]) / dt))] = (float(r["estX"]), float(r["estY"]))
    return out


def group(k, n_sub, n_find):
    kk = k % n_sub
    return (n_find - kk + n_sub - 1) // n_sub, k // n_sub


def main():
    if len(sys.argv) != 2:
        print("usage: finder_tasking_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in RUNS + ("def", "tag0") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        ft = sorted(f for f in os.listdir(os.path.join(d, "tag0")) if f != "run_id.txt")
        same = fd == ft and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "tag0", f), "rb").read() for f in fd)
        tf = [f for f in fd + ft if f.startswith("tasking_")]
        lines = log(d, "def").count("MS-1 tasking") + log(d, "tag0").count("MS-1 tasking")
        check("NULL", same and fd and not tf and lines == 0,
              f"--finderTasking=tag0 vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"tasking_ files {len(tf)}, MS-1 tasking lines {lines} (need 0, 0)")
    safe("NULL", c_null)

    def c_target():
        bad, n = [], 0
        for run in RUNS:
            ns = int(arg(d, run, "measuredSubjects"))
            for r in tasking(d, run):
                n += 1
                k = int(r["finderId"]) - 501
                if int(r["targetId"]) != (k % ns) + 1:
                    bad.append((run, r["t"], r["finderId"], r["targetId"]))
        check("TARGET", n > 0 and not bad,
              f"{n} rows; target not subject (k mod N) + 1: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("TARGET", c_target)

    def c_entry():
        bad, matched, prior, per = [], 0, 0, {}
        for run in RUNS:
            dur, dt = timing(d, run)
            ns = int(arg(d, run, "measuredSubjects"))
            traces = {}
            for r in tasking(d, run):
                if r["phase"] != "converge":
                    continue
                sid = int(r["targetId"])
                if sid not in traces:
                    traces[sid] = subject_trace(d, run, sid, dt)
                tr = traces[sid]
                ex, ey = float(r["entryX"]), float(r["entryY"])
                ke = int(round(float(r["entryT"]) / dt))
                hit = any(c in tr and abs(tr[c][0] - ex) <= 2e-4 and abs(tr[c][1] - ey) <= 2e-4
                          for c in (ke - 1, ke))
                if hit:
                    matched += 1
                    per[(run, sid)] = per.get((run, sid), 0) + 1
                elif (ke - 1) < min(tr):
                    prior += 1
                else:
                    bad.append((run, r["t"], r["finderId"], sid, ex, ey, r["entryT"]))
        # rr2: both subjects; rr5: subject 1, the one subject a lone finder reaches there.
        # Subjects 2-4 at rr5 are never heard by their finders in 600 s: a blind finder
        # holds position and learns nothing (recorded in TECH_DEBT N.4), so they are not
        # required -- their rows are all blind, and TARGET and COUNTS still read them.
        need = [("rr2", 1), ("rr2", 2), ("rr5", 1)]
        empty = [x for x in need if per.get(x, 0) == 0]
        check("ENTRY", matched > 0 and not bad and not empty,
              f"converge rows matching the target's own estimate {matched}, prior {prior}, "
              f"matching nothing {len(bad)} (need 0); tasked subjects with no matched row "
              f"{empty} (need none){': ' + str(bad[:2]) if bad else ''}")
    safe("ENTRY", c_entry)

    def c_goal():
        bad, n, spread = [], 0, 0
        for run in RUNS:
            ns = int(arg(d, run, "measuredSubjects"))
            nf = int(arg(d, run, "nFinders"))
            disp = float(arg(d, run, "finderDispersal", "0"))
            for r in tasking(d, run):
                if r["phase"] != "converge":
                    continue
                k = int(r["finderId"]) - 501
                gn, gi = group(k, ns, nf)
                gx, gy = float(r["entryX"]), float(r["entryY"])
                if disp > 0.0 and gn > 1:
                    th = 2.0 * math.pi * gi / gn
                    gx += disp * math.sqrt(max(float(r["entryVarX"]), 0.0)) * math.cos(th)
                    gy += disp * math.sqrt(max(float(r["entryVarY"]), 0.0)) * math.sin(th)
                    spread += 1
                n += 1
                if abs(gx - float(r["goalX"])) > 1e-3 or abs(gy - float(r["goalY"])) > 1e-3:
                    bad.append((run, r["t"], r["finderId"], r["goalX"], round(gx, 4)))
        check("GOAL", n > 0 and spread > 0 and not bad,
              f"{n} converge rows, {spread} of them spread within a group (need > 0); goal not "
              f"as recomputed: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("GOAL", c_goal)

    def c_counts():
        bad, sweeps = [], 0
        for run in RUNS:
            dur, dt = timing(d, run)
            steps = int(math.floor(dur / dt))
            ns = int(arg(d, run, "measuredSubjects"))
            nf = int(arg(d, run, "nFinders"))
            tk = tasking(d, run)
            got = {}
            seen = set()
            for r in tk:
                key = (r["finderId"], int(round(float(r["t"]) / dt)))
                if key in seen:
                    bad.append((run, "duplicate", key))
                seen.add(key)
                g = got.setdefault(r["finderId"], {"sweep": 0, "converge": 0, "blind": 0})
                g[r["phase"]] += 1
            if run == "sweep":
                sweeps = sum(g["sweep"] for g in got.values())
            lines = [ln for ln in log(d, run).splitlines() if "MS-1 tasking" in ln]
            parsed = LINE.findall(lines[0]) if len(lines) == 1 else []
            if len(lines) != 1 or len(parsed) != nf:
                bad.append((run, "line", len(lines), len(parsed)))
            for fid, sid, sw, cv, bl in parsed:
                g = got.get(fid, {"sweep": 0, "converge": 0, "blind": 0})
                k = int(fid) - 501
                if (int(sid), int(sw), int(cv), int(bl)) != ((k % ns) + 1, g["sweep"],
                                                             g["converge"], g["blind"]):
                    bad.append((run, fid, (sid, sw, cv, bl), g))
            for fid, g in got.items():
                if sum(g.values()) != steps:
                    bad.append((run, fid, "rows", sum(g.values()), steps))
        check("COUNTS", not bad and sweeps > 0,
              f"rows/phases/line inconsistent: {len(bad)} (need 0); sweep rows in the sweep run "
              f"{sweeps} (need > 0){': ' + str(bad[:2]) if bad else ''}")
    safe("COUNTS", c_counts)

    def c_digest():
        bad = []
        for run in RUNS:
            f = "tasking_ca_tbl_full_seed7.csv"
            with open(os.path.join(d, run, f), "rb") as fh:
                h = hashlib.sha256(fh.read()).hexdigest()
            if f"{h}  {f}" not in runid(d, run):
                bad.append(run)
        check("DIGEST", not bad, f"tasking_ files not digested as written: {bad or 'none'}")
    safe("DIGEST", c_digest)

    def c_ref():
        for n, need in (("r_bad", "FATAL: --finderTasking must be tag0 or roundrobin"),
                        ("r_one", "FATAL: --finderTasking=roundrobin needs"),
                        ("r_nofind", "FATAL: --finderTasking=roundrobin needs"),
                        ("r_mode0", "FATAL: --finderTasking=roundrobin needs")):
            got = rc(d, n)
            has = need in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G47 {tag:7s} {name:6s} {why}")
    print(f"  G47: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
