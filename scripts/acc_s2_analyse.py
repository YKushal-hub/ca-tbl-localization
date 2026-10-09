#!/usr/bin/env python3
"""S2 cloud branch: paired seed-bootstrap verdicts from a committed metrics table.

usage: acc_s2_analyse.py <metrics.csv> <cellA:armA> <cellB:armB> [...more pairs]
  Prints, for each pair, cellA's arm minus cellB's arm, paired by seed: the mean difference, its
  95 % bootstrap interval (4 000 resamples, seed 5) and WIN / tie / LOSS (lower is better for
  c50, c95 and J). Pairs are given as consecutive arguments.
"""
import sys

import numpy as np
import pandas as pd

COLS = [("fix", False), ("w25", False), ("t0w25", False), ("c50", True), ("c95", True), ("J", True)]


def main():
    m = pd.read_csv(sys.argv[1])
    args = sys.argv[2:]
    rng = np.random.default_rng(5)
    for i in range(0, len(args) - 1, 2):
        (ca, aa), (cb, ab) = args[i].split(":"), args[i + 1].split(":")
        a = m[(m.cell == ca) & (m.arm == aa)].set_index("seed")
        b = m[(m.cell == cb) & (m.arm == ab)].set_index("seed")
        idx = a.index.intersection(b.index)
        out = []
        for col, low in COLS:
            d = (a.loc[idx, col] - b.loc[idx, col]).to_numpy()
            bs = d[rng.integers(0, len(d), (4000, len(d)))].mean(1)
            mu, lo, hi = d.mean(), np.quantile(bs, 0.025), np.quantile(bs, 0.975)
            vv = ("WIN" if (mu < 0) == low else "LOSS") if (lo > 0 or hi < 0) else "tie"
            out.append(f"{col} {mu:+.3f} [{lo:+.3f}, {hi:+.3f}] {vv}")
        print(f"{ca}:{aa} - {cb}:{ab} (n={len(idx)}): " + " | ".join(out))


if __name__ == "__main__":
    main()
