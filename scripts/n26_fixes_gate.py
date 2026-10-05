#!/usr/bin/env python3
"""G89: the four N.26 fixes (TECH_DEBT N.26; HANDOFF_2026_10_05 sect 3).

usage: n26_fixes_gate.py <dir written by scripts/n26_fixes_gate.sh>

Runs read: e_off, e_dual, e_cross, e_chi (the preset's endgame, seed 7, 900 s, ca_tbl_full and
static_868; e_chi is --mahalUseR=true --integrityChi2=20) and k_off, k_gate (5 km, seed 1,
1 800 s, static_868; k_gate is --tdoaInnovationGate=true).

What a firing looks like on disk. Each flag prints one "N.26 fixes (<arm>): ..." line with its
counters; with every flag off there is none. --loraDualBase keeps the 2.4 GHz radio awake while
the node is on 868 MHz (nbr2p4 > 0 on such cycles) and fuses the two bands (a fix labelled
2.4 GHz with more ranges in the EKF than 2.4 GHz neighbours). --uwbCrossTierPoll addresses
more UWB anchors than the UWB table holds (polled > nbrUwb). --integrityChi2 moves the
threshold every SUSPECT is judged against. --tdoaInnovationGate refuses at least one TDoA
solve on the seed whose TDoA went wrong. The static arm is untouched by the hybrid-only flags.

Checks (each prints "G89 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run exited 0.
  OFF      e_off and k_off print no N.26 line.
  STATIC   static_868's arm_summary row and trace are byte-identical in e_off, e_dual, e_cross.
  DUAL     e_dual: ca_tbl_full's dual node-cycles = steps x measured subjects; some cycle on
           868 MHz with nbr2p4 > 0 (none in e_off); some 2.4 GHz-labelled fix with nRangesEkf
           above nbr2p4.
  CROSS    e_cross: cross-tier polls > 0, and some UWB cycle with polled > nbrUwb (none in
           e_off).
  CHI      e_chi: the line prints threshold 20; every SUSPECT tag 0 cycle has mahalanobis > 20;
           some fix cycle with 9.21 < mahalanobis <= 20 is not SUSPECT.
  GATE     k_gate: TDoA refused >= 1, refused + fused <= the TDoA lines' OK solves.
"""
import csv
import os
import re
import sys

N26 = re.compile(r"N\.26 fixes \((\w+)\): dual LoRa base node-cycles (\d+), ranges in their sets (\d+); "
                 r"UWB cross-tier polls (\d+); TDoA innovation gate refused (\d+), TDoA fused (\d+); "
                 r"integrity threshold ([\d.]+), suspects (\d+)")
OKS = re.compile(r"TDoA tier 1[^:]*: .*?solves OK (\d+)")
RUNS = ("e_off", "e_dual", "e_cross", "e_chi", "k_off", "k_gate")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G89 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G89 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G89 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def text(p):
    try:
        return open(p).read()
    except OSError:
        return None


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def line(d, n, arm):
    for m in N26.finditer(text(os.path.join(d, n + ".log")) or ""):
        if m.group(1) == arm:
            return [m.group(1)] + [float(x) for x in m.groups()[1:]]
    return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in RUNS if rc(d, n) != 0]
    report(not bad, "RUNS", "every run exited 0" if not bad else f"non-zero exit: {bad}")
    logs = {n: text(os.path.join(d, n + ".log")) for n in RUNS}
    if any(v is None for v in logs.values()):
        missing("OFF", "a run's log is missing")
        return 1
    off = [n for n in ("e_off", "k_off") if "N.26 fixes" in logs[n]]
    report(not off, "OFF", f"runs with every flag off that print an N.26 line: {off} (need none)")

    def raw(n, f):
        try:
            return open(os.path.join(d, n, f), "rb").read()
        except OSError:
            return None

    def srow(n):
        r = rows(os.path.join(d, n, "arm_summary.csv")) or []
        return next((x for x in r if x["arm"] == "static_868"), None)
    moved = [n for n in ("e_dual", "e_cross") if srow(n) is None or srow(n) != srow("e_off")
             or raw(n, "trace_static_868_seed7.csv") != raw("e_off", "trace_static_868_seed7.csv")]
    report(not moved, "STATIC", f"runs where static_868's row or trace moved: {moved} (need none)")

    toff = rows(os.path.join(d, "e_off", "trace_ca_tbl_full_seed7.csv"))
    tdual = rows(os.path.join(d, "e_dual", "trace_ca_tbl_full_seed7.csv"))
    tcross = rows(os.path.join(d, "e_cross", "trace_ca_tbl_full_seed7.csv"))
    tchi = rows(os.path.join(d, "e_chi", "trace_ca_tbl_full_seed7.csv"))
    arm = rows(os.path.join(d, "e_dual", "arm_summary.csv"))
    subj = rows(os.path.join(d, "e_dual", "subject_summary.csv"))
    if None in (toff, tdual, tcross, tchi, arm, subj):
        missing("DUAL", "a ca_tbl_full trace, or e_dual's summaries, is missing")
        return 1
    ln = line(d, "e_dual", "ca_tbl_full")
    steps = int(next(r for r in arm if r["arm"] == "ca_tbl_full")["steps"])
    nsub = sum(r["arm"] == "ca_tbl_full" for r in subj)
    on868 = lambda t: sum(r["activeTier"] == "868MHz" and int(r["nbr2p4"]) > 0 for r in t)  # noqa: E731
    fused = sum(r["hadFix"] == "1" and r["fixTier"] == "2.4GHz" and int(r["nRangesEkf"]) > int(r["nbr2p4"])
                for r in tdual)
    ok = ln is not None and ln[1] == steps * nsub and on868(tdual) > 0 and on868(toff) == 0 and fused > 0
    report(ok, "DUAL", f"node-cycles {ln[1] if ln else None} (need {steps} x {nsub}); cycles on 868 MHz with "
           f"2.4 GHz neighbours {on868(tdual)} (off: {on868(toff)}, need > 0 and 0); fused 2.4 GHz fixes {fused} (need > 0)")

    lc = line(d, "e_cross", "ca_tbl_full")
    over = lambda t: sum(r["activeTier"] == "UWB" and int(r["polled"]) > int(r["nbrUwb"]) for r in t)  # noqa: E731
    ok = lc is not None and lc[3] > 0 and over(tcross) > 0 and over(toff) == 0
    report(ok, "CROSS", f"cross-tier polls {lc[3] if lc else None} (need > 0); UWB cycles polling beyond the UWB "
           f"table {over(tcross)} (off: {over(toff)}, need > 0 and 0)")

    lq = line(d, "e_chi", "ca_tbl_full")
    fx = [r for r in tchi if r["hadFix"] == "1"]
    low = [r["t"] for r in fx if r["suspect"] == "1" and float(r["mahalanobis"]) <= 20.0]
    band = sum(9.21 < float(r["mahalanobis"]) <= 20.0 and r["suspect"] == "0" for r in fx)
    ok = lq is not None and lq[6] == 20.0 and not low and band > 0
    report(ok, "CHI", f"threshold {lq[6] if lq else None} (need 20); SUSPECT at or below it {len(low)} (need 0); "
           f"fix cycles in (9.21, 20] not SUSPECT {band} (need > 0)")

    lg = line(d, "k_gate", "static_868")
    oks = sum(int(x) for x in OKS.findall(logs["k_gate"]))
    ok = lg is not None and lg[4] >= 1 and lg[4] + lg[5] <= oks
    report(ok, "GATE", f"TDoA refused {lg[4] if lg else None} (need >= 1), fused {lg[5] if lg else None}, "
           f"OK solves {oks} (need refused + fused <= OK)")
    print(f"  G89: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
