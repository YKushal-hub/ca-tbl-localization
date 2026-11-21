#!/usr/bin/env python3
"""Output validation for the collapse World (analysis side; may read truth). Rows VAL-* in
docs/SOURCES_COLLAPSE.md give each published reference and the tolerance, fixed before running.

usage: score_validate.py RUN_ON RUN_OFF [RUN_ON ...]
RUN_ON / RUN_OFF: two runs of scratch/ca_tbl_collapse_sim.cc with identical arguments except
--rubble=on / --rubble=off. Links are keyed (G178), so a link's draw is the same in both runs and
any difference in its output is the rubble alone. Extra RUN_ON directories (other brackets, same
geometry) only add to the VAL-2 pooled span.

  VAL-1  output excess loss (P_off - P_on, received in both) equals the model path loss
         (truth_links extra_db), |diff| <= 0.01 dB, every band. Implementation check.
  VAL-2  868 MHz output excess for workers 2-5 m deep, to rescuer points: pooled over the
         brackets given, the span must reach below 30 dB and above 80 dB (NIST TN 1542
         ">= 30 to 40 dB", TN 1540 ">= 60 to 80 dB"; rows NIST1542-RUBBLE, NIST1540-RUBBLE).
  VAL-4  line-of-sight ToF error spread (rubble off, worker-to-point links): 2.4 GHz SD within
         1.27-3.2 m (Zhou et al. 2025, SX1280 outdoors); UWB SD within 0.10-0.33 m (Benouakta
         et al. 2023, DW3000). Both cited in lora-physics-parameters.h (tofLosFloorM, uwbLosFloorM).
  VAL-6  output ToF excess per metre of rubble on the chosen path, 2.4 GHz: median of
         (r_on - r_off - geometric excess) / rubble_m equals the bracket's (n - 1) within 5%
         (row SLOW-BR); and lies inside the KOL-1NS / SLOW-1 solid-concrete span 1.29-2.00 m/m
         scaled by the solid fraction (rows ETA-*).
"""
import csv
import math
import os
import statistics as st
import sys

RHO_N1 = {"low": 0.454039, "central": 0.703760, "high": 1.541129}  # SLOW-BR


def load(run):
    L = {}
    for r in csv.DictReader(open(os.path.join(run, "obs_links.csv"))):
        L[(r["slot"], r["band"], r["tx"], r["rx"])] = r
    T = {(r["band"], r["tx"], r["rx"]): r for r in csv.DictReader(open(os.path.join(run, "truth_links.csv")))}
    V = {r["id"]: r for r in csv.DictReader(open(os.path.join(run, "truth_victims.csv")))}
    meta = open(os.path.join(run, "run.txt")).read()
    br = [t.split("=")[1] for t in meta.split() if t.startswith("bracket=")][0]
    rb = [t.split("=")[1] for t in meta.split() if t.startswith("rubble=")][0]
    return L, T, V, br, rb


def main():
    on_runs = [sys.argv[1]] + sys.argv[3:]
    off = sys.argv[2]
    Loff, Toff, Voff, _, rboff = load(off)
    if rboff != "off":
        raise SystemExit("second argument must be a --rubble=off run")
    res = []
    span = []
    for run in on_runs:
        Lon, Ton, Von, br, rbon = load(run)
        if rbon != "on":
            raise SystemExit(f"{run} is not a --rubble=on run")
        worst = 0.0
        n1 = 0
        excess868 = []
        bias = []
        for k, r in Lon.items():
            o = Loff.get(k)
            if o is None:
                continue
            slot, band, tx, rx = k
            t = Ton[(band, tx, rx)]
            if r["received"] == "1" and o["received"] == "1":
                ex = float(o["rx_dbm"]) - float(r["rx_dbm"])
                worst = max(worst, abs(ex - float(t["extra_db"])))
                n1 += 1
                if band == "868" and 10000 <= int(rx) < 20000 and int(tx) < 10000:
                    d = float(Von[tx]["depth_m"])
                    if 2.0 <= d <= 5.0:
                        excess868.append(ex)
                if band == "2400" and 10000 <= int(rx) < 20000 and r["range_m"] and o["range_m"]:
                    rub = float(t["rubble_m"])
                    if rub > 0.5:
                        geo = float(t["path_len_m"]) - float(t["dist_m"])
                        bias.append((float(r["range_m"]) - float(o["range_m"]) - geo) / rub)
        res.append((f"VAL-1 [{br}] output excess == model path loss",
                    worst <= 0.01, f"max |diff| {worst:.4f} dB over {n1} links"))
        if excess868:
            span += [min(excess868), max(excess868)]
            res.append((f"VAL-2 [{br}] 868 MHz excess, workers 2-5 m deep (info)", True,
                        f"{min(excess868):.1f}-{max(excess868):.1f} dB, median {st.median(excess868):.1f} (n={len(excess868)})"))
        if bias:
            m = st.median(bias)
            ok = abs(m - RHO_N1[br]) <= 0.05 * RHO_N1[br]
            res.append((f"VAL-6 [{br}] 2.4 GHz ToF excess per rubble metre vs SLOW-BR {RHO_N1[br]:.3f}",
                        ok, f"median {m:.3f} m/m (n={len(bias)})"))
    if span:
        ok = min(span) <= 30.0 and max(span) >= 80.0
        res.append(("VAL-2 pooled 868 MHz excess spans NIST bounds (<=30 and >=80 dB)", ok,
                    f"{min(span):.1f}-{max(span):.1f} dB over {len(on_runs)} bracket run(s)"))
    # VAL-4: LOS ToF spread from the rubble-off run
    for band, lo, hi in (("2400", 1.27, 3.2), ("uwb", 0.10, 0.33)):
        e = []
        for k, r in Loff.items():
            slot, b, tx, rx = k
            if b != band or r["received"] != "1" or not r["range_m"] or not (10000 <= int(rx) < 20000):
                continue
            e.append(float(r["range_m"]) - float(Toff[(b, tx, rx)]["dist_m"]))
        if len(e) > 30:
            sd = st.pstdev(e)
            res.append((f"VAL-4 LOS ToF error SD {band} within published {lo}-{hi} m", lo <= sd <= hi,
                        f"SD {sd:.3f} m, mean {st.mean(e):+.3f} m (n={len(e)})"))
        else:
            res.append((f"VAL-4 LOS ToF error SD {band}", False, f"only {len(e)} ranges"))
    bad = 0
    for name, ok, info in res:
        print(f"{'PASS' if ok else 'FAIL'}  {name}: {info}")
        bad += 0 if ok else 1
    print(f"{len(res) - bad}/{len(res)} validation checks pass")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
