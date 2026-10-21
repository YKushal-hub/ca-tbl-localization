#!/usr/bin/env python3
"""G165: finder dissemination and tasking (plan items 23, 23b, 24). Reads the run set scripts/finder_dissem_gate.sh
writes. What each failure looks like on disk:
  RUNS    every run but the refusal exits 0                                    (a crash under a flag)
  BCE     bce/: DISCOVERY frames carry an entry (payload "entry="), off/'s     (the beacon carrying nothing, or
          carry none; finders merged > 0 and other radios merged none but      subjects merging neighbours' beacon
          heard > 0; the median age of the entry a finder steers on is under   entries, or no freshness gained)
          half of off/'s
  DOF     dof/: digest answers > 0, own-entry answers > 0                      (the own-first rule never taken)
  NEED    need/: the need tasking line, and at dispatch every subject but      (tag 0 still holding a pair while a
          tag 0 (nodes 2-5) has a finder                                       subject has none)
  SS      ss/: stale-entry search cycles > 0                                   (the search never entered)
  OFF     def/ and zero/ byte-identical, run_id.txt aside                      (a flag that is not off by default)
  REFUSE  r_task exits 1                                                       (an unknown tasking accepted)
"""
import csv
import glob
import os
import re
import sys

import numpy as np

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G165 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def one(d, n, pat):
    f = glob.glob(os.path.join(d, n, pat))
    return f[0] if f else None


def entry_beacons(d, n):
    f = one(d, n, "events_ca_tbl_full_seed*.csv")
    if not f:
        return None
    return sum(1 for r in csv.DictReader(open(f)) if r["msgType"] == "DISCOVERY" and "entry=" in r["payload"])


def ages(d, n):
    f = one(d, n, "tasking_ca_tbl_full_seed*.csv")
    if not f:
        return None
    a = [float(r["t"]) - float(r["entryT"]) for r in csv.DictReader(open(f)) if r["entryT"] not in ("", "nan")]
    return float(np.median(a)) if a else None


def main():
    d = sys.argv[1]
    names = ("off", "bce", "dof", "need", "ss", "def", "zero")
    rcs = {n: rc(d, n) for n in names}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    eb, eo = entry_beacons(d, "bce"), entry_beacons(d, "off")
    m = re.search(r"beacon entries \(ca_tbl_full\): (\d+) merged by finders from DISCOVERY beacons; (\d+) heard by other "
                  r"radios, not merged", log(d, "bce"))
    merged, ignored = (int(m.group(1)), int(m.group(2))) if m else (None, None)
    ab, ao = ages(d, "bce"), ages(d, "off")
    ok = (eb or 0) > 0 and eo == 0 and (merged or 0) > 0 and (ignored or 0) > 0 and ab is not None and ao is not None \
        and ab < 0.5 * ao
    report(ok, "BCE", f"beacons carrying an entry bce {eb} off {eo} (need > 0, 0); merged by finders {merged}, heard "
                      f"by others not merged {ignored} (need > 0, > 0); steering entry age median bce {ab} s, off {ao} s "
                      f"(need bce < off / 2)")
    m = re.search(r"digest own-first \(ca_tbl_full\): (\d+) digest answers, (\d+) of them", log(d, "dof"))
    ok = bool(m) and int(m.group(1)) > 0 and int(m.group(2)) > 0
    report(ok, "DOF", f"dof: {m.group(0) if m else 'no own-first line'} (need answers > 0 and own > 0)")
    f = one(d, "need", "tasking_ca_tbl_full_seed*.csv")
    first = {}
    if f:
        for r in csv.DictReader(open(f)):
            first.setdefault(r["finderId"], int(r["targetId"]))
    cov = sorted(set(first.values()))
    ok = "MS-1 tasking (need)" in log(d, "need") and {2, 3, 4, 5} <= set(cov)
    report(ok, "NEED", f"need: tasking line {'MS-1 tasking (need)' in log(d, 'need')}; nodes tasked at dispatch {cov} "
                       f"(need 2, 3, 4 and 5 among them)")
    m = re.search(r"stale-entry search \(ca_tbl_full\): (\d+) finder-cycles", log(d, "ss"))
    report(bool(m) and int(m.group(1)) > 0, "SS", f"ss: stale-search cycles {m.group(1) if m else None} (need > 0)")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
    fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
    diff = [q for q in fa if q not in fb or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
    report(bool(fa) and not diff and fa == fb, "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    report(rc(d, "r_task") == 1, "REFUSE", f"r_task exit {rc(d, 'r_task')} (need 1)")
    print(f"  G165: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
