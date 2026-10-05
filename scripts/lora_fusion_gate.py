#!/usr/bin/env python3
"""G88: the dual-band LoRa fusion arm (--arms=lora_fusion; the LoRa comparators on both bands).

usage: lora_fusion_gate.py <dir written by scripts/lora_fusion_gate.sh>

Runs read: with/ (ca_tbl_full, lora_fusion, static_868; the preset's endgame, seed 7, 900 s),
without/ (the same without lora_fusion), all/ (--arms=all, 30 s).

What a firing looks like on disk. lora_fusion is poucet_fusion without UWB: the 868 MHz and
2.4 GHz radios are awake, ranged and billed every cycle and their ranges fused into one set;
the UWB radio sleeps all run. It never escalates and its policy tier stays 868 MHz. The defects
it guards: the arm disturbing the arms beside it, joining --arms=all, a UWB exchange or
listen billed or a fix labelled UWB (the arm would be Poucet again), a LoRa radio left asleep,
or "fusion" that never puts the two bands into one set.

Checks (each prints "G88 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     with, without and all exited 0.
  INDEP    ca_tbl_full's and static_868's arm_summary rows and traces are byte-identical
           with and without lora_fusion beside them.
  ALL      all/: exactly the six arms, lora_fusion not among them.
  STATIC   lora_fusion: activeTier 868 MHz on every cycle, escalations 0.
  UWBOFF   lora_fusion: tx_uwb, rx_uwb and cad_uwb 0, j_uwb equal to static_868's (both
           asleep all run), and no cycle labelled UWB.
  AWAKE    lora_fusion: j_868 > 0, and j_2p4 and cad_2p4 above static_868's.
  FUSED    lora_fusion: some fix cycle fuses both bands -- labelled 2.4 GHz with more ranges
           in the EKF than it had 2.4 GHz neighbours -- and every cycle labelled 2.4 GHz had a
           2.4 GHz neighbour.
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
        print(f"  G88 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G88 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G88 MISSING {label:<8}{msg}")


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
    tr = rows(os.path.join(d, "with", "trace_lora_fusion_seed7.csv"))
    if None in (aw, ao, tr):
        missing("INDEP", "with/ or without/ lacks arm_summary.csv or the lora_fusion trace")
        return 1
    ow = {r["arm"]: r for r in ao}
    ib = [r["arm"] for r in aw if r["arm"] != "lora_fusion" and r != ow.get(r["arm"])]
    for a in ("ca_tbl_full", "static_868"):
        f = f"trace_{a}_seed7.csv"
        if raw(os.path.join(d, "with", f)) != raw(os.path.join(d, "without", f)) or raw(os.path.join(d, "with", f)) is None:
            ib.append(f)
    report(not ib, "INDEP", f"rows or traces that moved with lora_fusion beside them: {ib} (need none)")
    al = rows(os.path.join(d, "all", "arm_summary.csv")) or []
    names = {r["arm"] for r in al}
    report(names == SIX, "ALL", f"--arms=all ran {sorted(names)} (need exactly the six, no lora_fusion)")
    lf = next((r for r in aw if r["arm"] == "lora_fusion"), None)
    s8 = next((r for r in aw if r["arm"] == "static_868"), None)
    if lf is None or s8 is None:
        missing("STATIC", "with/arm_summary.csv lacks lora_fusion or static_868")
        return 1
    nact = sum(r["activeTier"] != "868MHz" for r in tr)
    report(nact == 0 and int(lf["escalations"]) == 0, "STATIC",
           f"lora_fusion: cycles off 868 MHz {nact}, escalations {lf['escalations']} (need 0, 0)")
    nu = sum(r["fixTier"] == "UWB" for r in tr)
    uw = [lf[k] for k in ("tx_uwb", "rx_uwb", "cad_uwb")]
    ok = all(float(x) == 0.0 for x in uw) and lf["j_uwb"] == s8["j_uwb"] and nu == 0
    report(ok, "UWBOFF", f"lora_fusion tx/rx/cad on UWB {uw} (need 0), j_uwb {lf['j_uwb']} against static_868's "
           f"{s8['j_uwb']} (need equal), cycles labelled UWB {nu} (need 0)")
    ok = (float(lf["j_868"]) > 0 and float(lf["j_2p4"]) > float(s8["j_2p4"])
          and float(lf["cad_2p4"]) > float(s8["cad_2p4"]))
    report(ok, "AWAKE", f"lora_fusion J 868 {lf['j_868']} (need > 0); J 2.4 {lf['j_2p4']} and CAD 2.4 {lf['cad_2p4']} "
           f"against static_868's {s8['j_2p4']} and {s8['cad_2p4']} (need more)")
    both = sum(r["hadFix"] == "1" and r["fixTier"] == "2.4GHz" and int(r["nRangesEkf"]) > int(r["nbr2p4"]) for r in tr)
    lb = [r["t"] for r in tr if r["fixTier"] == "2.4GHz" and int(r["nbr2p4"]) < 1]
    report(both > 0 and not lb, "FUSED", f"fix cycles fusing both bands {both} (need > 0); labelled 2.4 GHz with no "
           f"2.4 GHz neighbour {len(lb)} (need 0)")
    print(f"  G88: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
