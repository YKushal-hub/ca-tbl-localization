#!/usr/bin/env python3
"""Tabulate arm_summary.csv across a parameter sweep.

    python3 scripts/sweeps/summarise.py boxHalf results/box_1000 results/box_1500 ...

acc/J is recomputed here as 1/(rmseM * totalJ) rather than read from the
accuracyPerJoule column, so the reported ratio traces to two raw columns.
"""

import csv
import os
import sys

COLS = ["rmseM", "rmseFixedM", "fixCycles", "totalJ", "escalations",
        "blindCycles", "cycles_uwb", "cyclesInUwbCoverage", "trig_f1"]


def load(d):
    p = os.path.join(d, "arm_summary.csv")
    if not os.path.exists(p) or os.path.getsize(p) == 0:
        return None
    with open(p, newline="") as f:
        return {r["arm"]: r for r in csv.DictReader(f)}


def accpj(r):
    rm, j = float(r["rmseM"]), float(r["totalJ"])
    return 1.0 / (rm * j) if rm > 0 and j > 0 else float("nan")


def main():
    label = sys.argv[1]
    dirs = sys.argv[2:]

    loaded = [(d, load(d)) for d in dirs]
    loaded = [(d, a) for d, a in loaded if a]
    if not loaded:
        print("no completed runs")
        return

    arms = ["static_868", "static_2p4", "static_uwb", "fixed_threshold",
            "ca_tbl_full"]

    for col in ("rmseM", "totalJ", "acc/J", "fixCycles"):
        print(f"\n=== {col} ===")
        print(f"{label:>10} | " + " | ".join(f"{a:>19}" for a in arms))
        print("-" * (12 + 22 * len(arms)))
        for d, a in loaded:
            v = d.rsplit("_", 1)[-1]
            cells = []
            for arm in arms:
                if arm not in a:
                    cells.append(f"{'--':>19}")
                    continue
                if col == "acc/J":
                    cells.append(f"{accpj(a[arm]):>19.4e}")
                elif col == "fixCycles":
                    cells.append(f"{a[arm][col]:>19}")
                else:
                    cells.append(f"{float(a[arm][col]):>19.4f}")
            print(f"{v:>10} | " + " | ".join(cells))

    print("\n=== claim ratios (acc/J; >1 means the claim PASSES) ===")
    print(f"{label:>10} | {'c1 full/static_2p4':>20} | {'c2 full/fixed_thr':>20}"
          f" | {'2p4 fixCycles':>14} | {'full blindCycles':>17}")
    print("-" * 92)
    for d, a in loaded:
        v = d.rsplit("_", 1)[-1]
        if not all(k in a for k in ("ca_tbl_full", "static_2p4", "fixed_threshold")):
            continue
        c1 = accpj(a["ca_tbl_full"]) / accpj(a["static_2p4"])
        c2 = accpj(a["ca_tbl_full"]) / accpj(a["fixed_threshold"])
        print(f"{v:>10} | {c1:>20.4f} | {c2:>20.4f} |"
              f" {a['static_2p4']['fixCycles']:>14} |"
              f" {a['ca_tbl_full'].get('blindCycles', '--'):>17}")


if __name__ == "__main__":
    main()
