#!/usr/bin/env python3
"""G166: five items cut on 2026-10-20 and built on 2026-10-21 (TECH_DEBT N.17, 3.5, N.3, M.1(b); plan items 9, 27). Reads the run set
scripts/cut_items_gate.sh writes. What each failure looks like on disk:
  RUNS    every run but the refusal exits 0                                    (a crash under a flag)
  BUCKET  buck/subject_summary.csv has rx_/cad_/idle_ per tier, and for every  (a bucket missing, double-counted or
          subject and tier tx + rx + cad + idle + sleep = j_tier within 1e-6 J  mis-assigned to a tier)
  CASEA   ca_on/ prints Case A pre-fuse cycles > 0 for all measured nodes, and (the pre-fusion never taken, or taken
          its arm_summary.csv differs from ca_off/'s                           without effect)
  APC     apc/ prints gateway-to-gateway senses > 0, some heard, and CAD       (the sense path never reached, or never
          decisions busy only because of them > 0                              deciding a CAD)
  MOB     mob/ prints subject mobility with RECEDING and APPROACHING states    (subjects still always STATIONARY, or
          > 0, escalation handshakes > 0 and RECEDING eager pushes > 0          the push never run for a subject)
  SLICE   rs_on/ prints MAC transmissions put back > 0 over > 0 rounds, and   (the round still meeting no MAC frame,
          its arm_summary.csv differs from rs_off/'s                           or meeting them without effect)
  OFF     def/ and zero/ byte-identical, run_id.txt aside                      (a flag that is not off by default)
  REFUSE  r_ca and r_rs exit 1                                                (Case A pre-fusion without Step's sparse
                                                                               path, or the round fix without a round)
"""
import csv
import os
import re
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G166 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def main():
    d = sys.argv[1]
    names = ("buck", "ca_off", "ca_on", "apc", "mob", "rs_off", "rs_on", "def", "zero")
    rcs = {n: rc(d, n) for n in names}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    p = os.path.join(d, "buck", "subject_summary.csv")
    rows = list(csv.DictReader(open(p))) if os.path.exists(p) else []
    tiers = ("868", "2p4", "uwb")
    cols = [f"{b}_{t}" for b in ("rx", "cad", "idle") for t in tiers]
    have = bool(rows) and all(c in rows[0] for c in cols)
    worst = 0.0
    if have:
        for r in rows:
            for t in tiers:
                s = sum(float(r[f"{b}_{t}"]) for b in ("tx", "rx", "cad", "idle", "sleep"))
                worst = max(worst, abs(s - float(r[f"j_{t}"])))
    report(have and worst < 1e-6, "BUCKET", f"{len(rows)} subject rows, columns present {have}; worst |tx+rx+cad+idle+"
                                            f"sleep - j_tier| {worst:.2e} J (need < 1e-6)")
    m = re.search(r"Case A pre-fuse \(ca_tbl_full\): tag 0 (\d+), all measured nodes (\d+) cycles", log(d, "ca_on"))
    a, b = os.path.join(d, "ca_off", "arm_summary.csv"), os.path.join(d, "ca_on", "arm_summary.csv")
    moved = os.path.exists(a) and os.path.exists(b) and open(a, "rb").read() != open(b, "rb").read()
    report(bool(m) and int(m.group(2)) > 0 and moved, "CASEA",
           f"pre-fused cycles {m.group(2) if m else None} (need > 0); arm_summary moved {moved} (need True)")
    m = re.search(r"anchor-pair CAD \(ca_tbl_full\): (\d+) gateway-to-gateway frame senses, (\d+) of them heard "
                  r"\(channel busy\); CAD decisions busy only because of them (\d+)", log(d, "apc"))
    ok = bool(m) and int(m.group(1)) > 0 and int(m.group(2)) > 0 and int(m.group(3)) > 0
    report(ok, "APC", f"senses / heard / decisions {m.groups() if m else None} (need all > 0)")
    m = re.search(r"subject mobility \(ca_tbl_full\): stationary/approaching/receding (\d+)/(\d+)/(\d+); escalation "
                  r"handshakes (\d+), RECEDING eager pushes (\d+)", log(d, "mob"))
    ok = bool(m) and int(m.group(2)) > 0 and int(m.group(3)) > 0 and int(m.group(4)) > 0 and int(m.group(5)) > 0
    report(ok, "MOB", f"stationary/approaching/receding, handshakes, pushes {m.groups() if m else None} "
                      f"(need approaching, receding, handshakes and pushes > 0)")
    m = re.search(r"M\.1\(b\) relay round in slice \(ca_tbl_full\): (\d+) MAC transmissions put back over (\d+) rounds",
                  log(d, "rs_on"))
    a, b = os.path.join(d, "rs_off", "arm_summary.csv"), os.path.join(d, "rs_on", "arm_summary.csv")
    moved = os.path.exists(a) and os.path.exists(b) and open(a, "rb").read() != open(b, "rb").read()
    ok = bool(m) and int(m.group(1)) > 0 and int(m.group(2)) > 0 and moved
    report(ok, "SLICE", f"put back / rounds {m.groups() if m else None} (need > 0); arm_summary moved {moved} (need True)")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
    fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
    diff = [q for q in fa if q not in fb or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
    report(bool(fa) and not diff and fa == fb, "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    rr = {n: rc(d, n) for n in ("r_ca", "r_rs")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G166: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
