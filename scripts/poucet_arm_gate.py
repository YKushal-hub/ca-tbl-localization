#!/usr/bin/env python3
"""G87: the Poucet-style fusion arm (--arms=poucet_fusion; the spec's comparison table).

usage: poucet_arm_gate.py <dir written by scripts/poucet_arm_gate.sh>

Runs read: with/ (ca_tbl_full, poucet_fusion, static_uwb; the preset's endgame, seed 7,
900 s), without/ (the same without poucet_fusion), all/ (--arms=all, 30 s).

What a firing looks like on disk. poucet_fusion is the one arm that ranges all three tiers
every cycle and fuses them (Poucet, Vey et al. IPIN 2021): it never escalates, its policy
tier stays 868 MHz, every radio is awake and billed, and a cycle's fix is labelled with the
most precise tier among its ranges. The defects it guards: the arm disturbing the arms beside
it, joining --arms=all (which would change every default run), a radio left asleep, or a
label naming a tier that had no neighbour to range.

Checks (each prints "G87 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     with, without and all exited 0.
  INDEP    ca_tbl_full's and static_uwb's arm_summary rows and traces are byte-identical
           with and without poucet_fusion beside them.
  ALL      all/: exactly the six arms, poucet_fusion not among them.
  STATIC   poucet_fusion: activeTier 868 MHz on every cycle, escalations 0.
  AWAKE    poucet_fusion: energy on all three tiers > 0, and more on UWB than ca_tbl_full's.
  LABEL    poucet_fusion: a cycle labelled UWB had a UWB neighbour, one labelled 2.4 GHz a
           2.4 GHz neighbour; some cycles are labelled other than 868 MHz.
"""
import csv
import os
import sys

SIX = {"static_868", "static_2p4", "static_uwb", "fixed_threshold", "ca_tbl_no_integrity", "ca_tbl_full"}
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G87 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G87 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G87 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def raw(p):
    try:
        return open(p, "rb").read()
    except OSError:
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("with", "without", "all") if rc(d, n) != 0]
    report(not bad, "RUNS", "with, without and all exited 0" if not bad else f"non-zero exit: {bad}")
    aw = rows(os.path.join(d, "with", "arm_summary.csv"))
    ao = rows(os.path.join(d, "without", "arm_summary.csv"))
    tr = rows(os.path.join(d, "with", "trace_poucet_fusion_seed7.csv"))
    if None in (aw, ao, tr):
        missing("INDEP", "with/ or without/ lacks arm_summary.csv or the poucet_fusion trace")
        return 1
    ow = {r["arm"]: r for r in ao}
    ib = [r["arm"] for r in aw if r["arm"] != "poucet_fusion" and r != ow.get(r["arm"])]
    for a in ("ca_tbl_full", "static_uwb"):
        f = f"trace_{a}_seed7.csv"
        if raw(os.path.join(d, "with", f)) != raw(os.path.join(d, "without", f)) or raw(os.path.join(d, "with", f)) is None:
            ib.append(f)
    report(not ib, "INDEP", f"rows or traces that moved with poucet_fusion beside them: {ib} (need none)")
    al = rows(os.path.join(d, "all", "arm_summary.csv")) or []
    names = {r["arm"] for r in al}
    report(names == SIX, "ALL", f"--arms=all ran {sorted(names)} (need exactly the six, no poucet_fusion)")
    pf = next((r for r in aw if r["arm"] == "poucet_fusion"), None)
    full = next((r for r in aw if r["arm"] == "ca_tbl_full"), None)
    nact = sum(r["activeTier"] != "868MHz" for r in tr)
    report(pf is not None and nact == 0 and int(pf["escalations"]) == 0, "STATIC",
           f"poucet_fusion: cycles off 868 MHz {nact}, escalations {pf['escalations'] if pf else None} (need 0, 0)")
    j = [float(pf[k]) for k in ("j_868", "j_2p4", "j_uwb")] if pf else [0, 0, 0]
    okj = pf is not None and full is not None and all(x > 0 for x in j) and j[2] > float(full["j_uwb"])
    report(okj, "AWAKE", f"poucet_fusion J 868/2.4/UWB {j} (need each > 0), UWB against ca_tbl_full's "
           f"{full['j_uwb'] if full else None} (need more)")
    lb = [(r["t"], r["fixTier"]) for r in tr if (r["fixTier"] == "UWB" and int(r["nbrUwb"]) < 1)
          or (r["fixTier"] == "2.4GHz" and int(r["nbr2p4"]) < 1)]
    other = sum(r["fixTier"] != "868MHz" for r in tr)
    report(not lb and other > 0, "LABEL", f"cycles labelled above 868 MHz {other} (need > 0); labelled with a tier that had "
           f"no neighbour: {len(lb)} (need 0)" + (f": {lb[:2]}" if lb else ""))
    print(f"  G87: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
