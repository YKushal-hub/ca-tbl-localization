#!/usr/bin/env python3
"""G31: table entries carry the producer's velocity (docs/SPEC_CONFORMANCE_PLAN.md DC-10).

The spec's entry is {pos, Sigma, timestamp, seq_no, hop_count, source_type,
velocity_estimate} (Physical 2.1). The wire format billed the two velocity fields and
every producer published 0, 0. Under --entryVelocity a field node publishes its EKF
velocity, a finder its GNSS velocity (displacement over the time since its last
publication), and a gateway 0.

Reads a directory written by scripts/entry_velocity_gate.sh: two pairs of runs, each with
the flag off and on -- the searcher regime of null control NC2 (finders, digests: entries
travel), and the relay regime of NC4 (peer tags publish too) -- with <name>.rc beside them.
Every check runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL   nothing reads the velocity yet, so turning it on moves NO other output: every file
         of the flag-off run is byte-identical in the flag-on run, which adds exactly
         entries_*.csv. (A velocity that leaked into a decision would show here.)
  OFF    the flag-off runs write no entries_* file.
  PUB    each field node's published velocity, READ BACK from its own MAC table slot,
         equals the EKF velocity it was handed (refVx/refVy), to the 9 dp printed, and is
         non-zero on some rows (a moving subject).
  FINDER INDEPENDENT RECOUNT (rule 3): each finder's published velocity equals its own
         published displacement over the time between consecutive publications (the
         GNSS offset is constant and cancels); the first publication is 0.
  CARRY  every entry a node HOLDS of a moving producer carries the velocity that producer
         published under the same sequence number -- so the fields crossed the air
         intact -- and some held entry of the subject has a non-zero velocity.
  GW     every gateway entry, anywhere, has velocity 0.
"""
import csv
import os
import sys
from collections import defaultdict


def rows(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def files(d, n):
    return sorted(f for f in os.listdir(os.path.join(d, n)) if f != "run_id.txt")


def read(p):
    with open(p, "rb") as f:
        return f.read()


# (off, on, entries travel): the searcher regime digests, so held entries must exist; the
# relay regime has no finders and no digest exchange, and is there for the peers' PUB.
PAIRS = [("s_off", "s_on", True), ("r_off", "r_on", False)]


def main():
    if len(sys.argv) != 2:
        print("usage: entry_velocity_gate.py <dir>", file=sys.stderr)
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

    def c_runs():
        bad = [n for p in PAIRS for n in p[:2] if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    for off, on, travels in PAIRS:
        def c_null(off=off, on=on):
            fo, fn = files(d, off), files(d, on)
            extra = [f for f in fn if f not in fo]
            missing = [f for f in fo if f not in fn]
            diff = [f for f in fo if f in fn and read(os.path.join(d, off, f)) != read(os.path.join(d, on, f))]
            ok = not missing and not diff and len(extra) >= 1 and all(f.startswith("entries_") for f in extra)
            check("NULL", ok, f"{on} vs {off}: {len(fo)} files compared, {len(diff)} differ {diff[:3]}, "
                              f"{len(missing)} missing; extra {extra} (need only entries_*)")
        safe("NULL", c_null)

        def c_off(off=off):
            e = [f for f in files(d, off) if f.startswith("entries_")]
            check("OFF", not e, f"{off}: {len(e)} entries_* files (need 0)")
        safe("OFF", c_off)

        def c_entries(on=on, travels=travels):
            E = []
            for f in files(d, on):
                if f.startswith("entries_"):
                    E += rows(os.path.join(d, on, f))
            if not E:
                check("PUB", False, f"{on}: no entries rows")
                return
            pub = [r for r in E if r["kind"] == "pub"]
            held = [r for r in E if r["kind"] == "held"]
            field = [r for r in pub if int(r["entryId"]) < 101]
            finder = [r for r in pub if int(r["entryId"]) >= 501]
            badp = sum(1 for r in field if (r["vx"], r["vy"]) != (r["refVx"], r["refVy"]))
            movp = sum(1 for r in field if abs(float(r["vx"])) + abs(float(r["vy"])) > 1e-6)
            ids = sorted({r["entryId"] for r in field})
            check("PUB", field and badp == 0 and movp > 0,
                  f"{on}: {len(field)} field-node publications (nodes {ids}); {badp} whose read-back "
                  f"velocity is not the EKF's (need 0); {movp} non-zero (need > 0)")

            if finder:
                byf = defaultdict(list)
                for r in finder:
                    byf[r["entryId"]].append(r)
                worst, nz, bad0, n = 0.0, 0, 0, 0
                for fid, rs in byf.items():
                    rs.sort(key=lambda r: float(r["t"]))
                    bad0 += abs(float(rs[0]["vx"])) + abs(float(rs[0]["vy"])) > 0.0
                    for a, b in zip(rs, rs[1:]):
                        dt = float(b["t"]) - float(a["t"])
                        if dt <= 0:
                            continue
                        n += 1
                        ex = (float(b["x"]) - float(a["x"])) / dt
                        ey = (float(b["y"]) - float(a["y"])) / dt
                        worst = max(worst, abs(ex - float(b["vx"])), abs(ey - float(b["vy"])))
                        nz += abs(float(b["vx"])) + abs(float(b["vy"])) > 1e-6
                        if (b["vx"], b["vy"]) != (b["refVx"], b["refVy"]):
                            worst = max(worst, 1.0)
                check("FINDER", n > 0 and nz > 0 and worst <= 1e-5 and bad0 == 0,
                      f"{on}: {len(byf)} finders, {n} consecutive publications; published velocity vs "
                      f"recounted displacement/time: worst {worst:.1e} (need <= 1e-5); {nz} non-zero; "
                      f"{bad0} first publications non-zero (need 0)")

            P = {(r["entryId"], r["seqNo"]): (r["vx"], r["vy"]) for r in pub}
            prod = {r["entryId"] for r in pub}
            hp = [r for r in held if r["entryId"] in prod]
            nomatch = sum(1 for r in hp if (r["entryId"], r["seqNo"]) not in P)
            wrong = sum(1 for r in hp if (r["entryId"], r["seqNo"]) in P and
                        P[(r["entryId"], r["seqNo"])] != (r["vx"], r["vy"]))
            subj = sum(1 for r in hp if int(r["entryId"]) < 101 and
                       abs(float(r["vx"])) + abs(float(r["vy"])) > 1e-6)
            check("CARRY", nomatch == 0 and wrong == 0 and (not travels or (hp and subj > 0)),
                  f"{on}: {len(hp)} held entries of producers; {nomatch} with a sequence never "
                  f"published, {wrong} whose velocity differs from the publication (need 0); "
                  f"{subj} held entries of a field node with non-zero velocity"
                  + (" (need > 0)" if travels else " (no digest exchange in this regime)"))
            gw = [r for r in E if 101 <= int(r["entryId"]) <= 199]
            gbad = sum(1 for r in gw if float(r["vx"]) != 0.0 or float(r["vy"]) != 0.0)
            check("GW", gbad == 0, f"{on}: {len(gw)} gateway entries, {gbad} with a non-zero velocity (need 0)")
        safe("ENTRIES", c_entries)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G31 {tag:7s} {name:6s} {why}")
    print(f"  G31: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
