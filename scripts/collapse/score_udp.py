#!/usr/bin/env python3
"""VAL-7 / VAL-8: the World's ranging bias when the direct path is lost, against Alsindi, Alavi & Pahlavan,
IEEE TVT 58(3) 2009 (row ALS2009-T6 / ALS2009-T3). Analysis side: reads truth_links.csv of field runs.
Normalised error psi = bias / distance (Alsindi eq. (12)); path_kind 1 = straight (direct path present),
2 = one exit and 3 = out and back in (direct path lost; DD 37).
usage: score_udp.py FIELD_DIR (holding field_low, field_central, field_high)"""
import csv
import glob
import math
import os
import statistics as st
import sys

ALS_MU_500 = (-3.13, -1.59)  # Table VI, 500 MHz, all environments: mu_{m,pd,B} span
ALS_SD_500 = (0.45, 0.88)    # Table VI, 500 MHz sigma span
ALS_SD_3G = (0.45, 1.17)     # Table VI, 3 GHz sigma span


def main():
    fd = sys.argv[1]
    ok = True
    for br in ("low", "central", "high"):
        lost, n = [], 0
        for f in glob.glob(os.path.join(fd, f"field_{br}", "seed*", "truth_links.csv")):
            for r in csv.DictReader(open(f)):
                if r["band"] != "uwb":
                    continue
                n += 1
                d, b = float(r["dist_m"]), float(r["bias_m"])
                if r["path_kind"] in ("2", "3") and d > 0 and b > 0:
                    lost.append(math.log(b / d))
        mu, sd = st.mean(lost), st.pstdev(lost)
        in_mu = ALS_MU_500[0] <= mu <= ALS_MU_500[1]
        ok &= in_mu
        print(f"VAL-7 {br:7}: UWB links {n}, direct path lost {len(lost) / n:.3f}; ln psi mean {mu:.2f} "
              f"({'inside' if in_mu else 'OUTSIDE'} Alsindi 500 MHz {ALS_MU_500}), sd {sd:.2f} "
              f"({'inside' if ALS_SD_500[0] <= sd <= ALS_SD_500[1] else 'outside'} 500 MHz {ALS_SD_500}; "
              f"{'inside' if ALS_SD_3G[0] <= sd <= ALS_SD_3G[1] else 'outside'} 3 GHz {ALS_SD_3G})")
    print("VAL-7 PASS" if ok else "VAL-7 FAIL")


if __name__ == "__main__":
    main()
