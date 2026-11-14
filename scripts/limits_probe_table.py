#!/usr/bin/env python3
"""LIMITS 1a: tabulate the probes (probe_range_sweep / probe_terms / run.log 'usable range') for the report.
usage: limits_probe_table.py <probe dir>   (dirs: frozen_e<E>_s361 from the frozen binary; p_e<E>_o<O>_s<S>; lever_<name>; new_e1_o<O>_s361)
Prints, per environment and offset, the 868 / 2.4 / UWB r80 per seed and the mean over seeds, and the 868 MHz total
loss L = 14 dBm + 123 dBm - mean_margin at 250 m and at r80, which is the quantity compared with the field budget.
"""
import csv
import glob
import os
import re
import sys

import numpy as np

TX, SENS = 14.0, -123.0  # tier-1 PHY, SF7/125 kHz: 14 dBm, sensitivity -123 dBm (lora-physics-parameters.cc:391)


def r80s(log):
    return [float(x) for x in re.findall(r"usable range: r80=([0-9.]+) m", open(log, errors="replace").read())]


def loss_at(d, dist):
    p = os.path.join(d, "probe_terms.csv")
    if not os.path.exists(p):
        return None
    for r in csv.DictReader(open(p)):
        if r["tier"] == "868MHz" and float(r["distance_m"]) == dist:
            return TX - SENS - float(r["mean_margin"])
    return None


def main(top):
    groups = {}
    for d in sorted(set(glob.glob(os.path.join(top, "*_s*")) + glob.glob(os.path.join(top, "lever_*")))):
        lg = os.path.join(d, "run.log")
        if not os.path.exists(lg) or not os.path.exists(os.path.join(d, "rc")) or open(os.path.join(d, "rc")).read().strip() != "0":
            continue
        n = os.path.basename(d)
        m = re.match(r"(?:frozen_e(\d)_s\d+|p_e(\d)_o(-?\d+)_s\d+|new_e1_o(\d+)_s\d+)$", n)
        if m:
            e = m.group(1) or m.group(2) or "1"
            o = m.group(3) or m.group(4) or "0"
            key = f"env {e}, {int(o):+d} dB"
        else:
            key = n
        v = r80s(lg)
        if len(v) == 3:
            groups.setdefault(key, []).append((n, v, loss_at(d, 250.0)))
    print("| setting | seeds | 868 MHz r80 (m), per seed | mean | 2.4 GHz r80 mean | UWB r80 mean | 868 total loss at 250 m (dB) |")
    print("|---|---|---|---|---|---|---|")
    for k in sorted(groups):
        g = groups[k]
        a = np.array([x[1] for x in g])
        L = [x[2] for x in g if x[2] is not None]
        print(f"| {k} | {len(g)} | {', '.join(f'{x:.0f}' for x in a[:, 0])} | {a[:, 0].mean():.0f} | {a[:, 1].mean():.0f} | {a[:, 2].mean():.1f} | "
              f"{(f'{np.mean(L):.1f}' if L else 'n/a')} |")


if __name__ == "__main__":
    main(sys.argv[1])
