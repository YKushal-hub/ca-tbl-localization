#!/usr/bin/env python3
"""G43: held entries are carried forward before use (docs/SPEC_CONFORMANCE_PLAN.md DC-13,
--entryPrediction).

Physical 2.2 predicts table entries forward with a Q set by source type. A held finder entry
was used where it was reported, its variance inflated isotropically by (walk speed x age)^2,
because no velocity was populated. With DC-10's velocities (--entryVelocity) and the flag,
it is used at x + v age, with added variance (0.5 m/s x age)^2. entrypred_<arm>_seed<N>.csv
records every substitution.

Reads a directory written by scripts/entry_prediction_gate.sh (base, off, on: NC9's regime
with pseudo-anchor finders, digests and --entryVelocity; r_novel). Every check runs and is reported; exit 1 if any failed, 2 if
an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL       --entryPrediction=false IS the flag absent: every file byte-identical, no DC-13
             line, no entrypred_ file.
  PROPAGATE  every row's predicted position is (x0 + vx age, y0 + vy age), its added variance
             (0.5 age)^2 and its legacy variance (1.2 age)^2, recomputed (1e-4 m, 1e-6 rel).
  MOVED      an entry with a velocity and an age MOVES (> 0 of them), and one without does not.
  ENTRIES    each substituted entry is one the node held: its (id, seq, x, y, vx, vy) is a
             "held" row of entries_ for holder 1 at or before that cycle (DC-10's record).
  LINE       the DC-13 stdout counts equal the CSV's.
  R          the flag without --entryVelocity is refused (exit 1).
"""
import csv
import os
import re
import sys

LINE = re.compile(r"DC-13 entry prediction: substitutions (\d+), moved by their velocity (\d+)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(d, n, f):
    return list(csv.DictReader(open(os.path.join(d, n, f))))


def main():
    if len(sys.argv) != 2:
        print("usage: entry_prediction_gate.py <dir>", file=sys.stderr)
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
        bad = [n for n in ("base", "off", "on") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fb = sorted(f for f in os.listdir(os.path.join(d, "base")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fb == fo and all(open(os.path.join(d, "base", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fb)
        lines = log(d, "base").count("DC-13")
        ep = [f for f in fb if f.startswith("entrypred_")]
        check("NULL", same and fb and lines == 0 and not ep,
              f"--entryPrediction=false vs absent: {len(fb)} files, {'identical' if same else 'DIFFER'}; "
              f"DC-13 lines {lines}, entrypred_ files {len(ep)} (need 0, 0)")
    safe("NULL", c_null)

    def c_prop():
        ep = rows(d, "on", "entrypred_ca_tbl_full_seed7.csv")
        bad = []
        for r in ep:
            a = float(r["ageS"])
            wx = float(r["x0"]) + float(r["vx"]) * a
            wy = float(r["y0"]) + float(r["vy"]) * a
            wv, wl = (0.5 * a) ** 2, (1.2 * a) ** 2
            if abs(float(r["xPred"]) - wx) > 1e-4 or abs(float(r["yPred"]) - wy) > 1e-4 or \
                    abs(float(r["varAdd"]) - wv) > 1e-6 * (wv + 1e-9) + 1e-6 or \
                    abs(float(r["varAddLegacy"]) - wl) > 1e-6 * (wl + 1e-9) + 1e-6:
                bad.append((r["t"], r["anchorId"]))
        check("PROPAGATE", ep and not bad,
              f"on: {len(ep)} substitutions; not x0 + v age / (0.5 age)^2 / (1.2 age)^2: "
              f"{len(bad)} (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("PROPAGATE", c_prop)

    def c_moved():
        ep = rows(d, "on", "entrypred_ca_tbl_full_seed7.csv")
        moving = [r for r in ep if (float(r["vx"]) != 0 or float(r["vy"]) != 0) and float(r["ageS"]) > 0]
        moved = sum(1 for r in moving if (r["xPred"], r["yPred"]) != (r["x0"], r["y0"]))
        still = [r for r in ep if float(r["vx"]) == 0 and float(r["vy"]) == 0 and
                 (r["xPred"], r["yPred"]) != (r["x0"], r["y0"])]
        check("MOVED", moved > 0 and not still,
              f"on: entries with a velocity and an age {len(moving)}, of which moved {moved} "
              f"(need > 0); entries without one that moved {len(still)} (need 0)")
    safe("MOVED", c_moved)

    def c_entries():
        ep = rows(d, "on", "entrypred_ca_tbl_full_seed7.csv")
        held = {}
        for r in rows(d, "on", "entries_ca_tbl_full_seed7.csv"):
            if r["kind"] == "held" and r["holderId"] == "1":
                held.setdefault((r["entryId"], r["seqNo"]), []).append(r)
        bad = []
        for r in ep:
            cands = held.get((r["anchorId"], r["seqNo"]), [])
            ok = any(float(h["t"]) <= float(r["t"]) + 1e-6 and
                     abs(float(h["x"]) - float(r["x0"])) < 1e-3 and
                     abs(float(h["y"]) - float(r["y0"])) < 1e-3 and
                     abs(float(h["vx"]) - float(r["vx"])) < 1e-6 and
                     abs(float(h["vy"]) - float(r["vy"])) < 1e-6 for h in cands)
            if not ok:
                bad.append((r["t"], r["anchorId"], r["seqNo"]))
        check("ENTRIES", ep and not bad,
              f"on: substituted entries not among node 1's held entries: {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}")
    safe("ENTRIES", c_entries)

    def c_line():
        m = LINE.search(log(d, "on"))
        ep = rows(d, "on", "entrypred_ca_tbl_full_seed7.csv")
        want = (len(ep), sum(1 for r in ep if (r["xPred"], r["yPred"]) != (r["x0"], r["y0"])))
        got = (int(m.group(1)), int(m.group(2))) if m else None
        check("LINE", got == want, f"on: DC-13 line {got} vs CSV {want} (need equal)")
    safe("LINE", c_line)

    def c_ref():
        got = rc(d, "r_novel")
        has = "--entryPrediction needs --entryVelocity=true" in log(d, "r_novel")
        check("R", got == 1 and has, f"r_novel: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G43 {tag:7s} {name:9s} {why}")
    print(f"  G43: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
