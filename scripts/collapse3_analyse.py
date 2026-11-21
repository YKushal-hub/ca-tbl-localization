#!/usr/bin/env python3
"""COLLAPSE-3 (report/PREREG_COLLAPSE3.md): hours on one CR123A per worker, from COLLAPSE-1's energy."""
import csv
import glob
import os
import sys

CELL_J = 16740.0                     # BATT-CR123A: 1.55 Ah x 3 V
COMPUTE_JH = {"ca_tbl_full": 1.82, "static_868": 1.72, "static_2p4": 1.82}   # Phase 0 (DD 43), k_dp = 1


def main():
    out = sys.argv[1]
    print("cell                  arm           workers  hours_med  hours_p5   H7(p5>=72h)")
    allok = True
    for cell in sorted(glob.glob(os.path.join(out, "s*_*_n*"))):
        for arm in COMPUTE_JH:
            hours = []
            for sd in glob.glob(os.path.join(cell, "seed*")):
                f = os.path.join(sd, "subject_summary.csv")
                rows = list(csv.DictReader(open(f))) if os.path.exists(f) else \
                    list(csv.DictReader(open(os.path.join(sd, "arm_summary.csv")))) if os.path.exists(os.path.join(sd, "arm_summary.csv")) else []
                for r in rows:
                    if r["arm"] != arm:
                        continue
                    dur = float(r["durationS"]) if r.get("durationS") else 2.5 * float(r["steps"])  # 2.5 s cycles
                    jh = float(r["totalJ"]) * 3600.0 / dur + COMPUTE_JH[arm]
                    hours.append(CELL_J / jh)
            if not hours:
                continue
            hours.sort()
            p5 = hours[int(0.05 * (len(hours) - 1))]
            ok = p5 >= 72.0
            allok &= ok
            print(f"{os.path.basename(cell):20} {arm:13} {len(hours):7} {hours[len(hours) // 2]:10.0f} {p5:9.0f}   {'HOLDS' if ok else 'FAILS'}")
    print("H7 overall:", "HOLDS" if allok else "FAILS")


if __name__ == "__main__":
    main()
