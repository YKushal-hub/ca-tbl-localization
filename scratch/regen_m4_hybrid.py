#!/usr/bin/env python3
"""
regen_m4_hybrid.py -- Targeted regeneration of M4 x HYBRID only.
Imports process() from multimodal_generator and regenerates the single
failing file with the corrected NLOS threshold (> 0.01 dB).
"""
import sys, time
import numpy as np
import pandas as pd
from pathlib import Path

# ── Inline the constants + functions from multimodal_generator ──────────────
# (Avoids having to modify the generator's main() loop)

SRC   = "./results/ml_training_dataset.csv"
DST   = "./results"
CHUNK = 500_000

METHOD = 4
TOPO   = "HYBRID"

# Import everything from the fixed generator
sys.path.insert(0, "./scratch")
from multimodal_generator import process, unit_tests

def main():
    unit_tests()   # Sanity-check anchor points first

    src = Path(SRC)
    if not src.exists():
        print("ERROR: {} not found".format(src)); sys.exit(1)

    out = Path(DST) / "multimodal_M{}_T{}.csv".format(METHOD, TOPO)
    seed = 1000 + METHOD * 100 + ["WAN","MESH","HYBRID"].index(TOPO)
    rng  = np.random.default_rng(seed)

    print("Regenerating {} ...".format(out.name))
    t0 = time.time(); first = True; rows = 0

    for ci, chunk in enumerate(pd.read_csv(src, chunksize=CHUNK)):
        oc = process(chunk, METHOD, TOPO, rng)
        oc.to_csv(out, index=False, mode="w" if first else "a", header=first)
        first = False; rows += len(oc)
        if (ci + 1) % 4 == 0:
            pct = rows / 5e6 * 100
            print("  {:>7,} rows ({:.0f}%) [{:.0f}s]".format(rows, pct, time.time()-t0))

    el = time.time() - t0
    mb = out.stat().st_size / 1e6
    print("DONE: {:,} rows  {:.0f} MB  {:.0f}s".format(rows, mb, el))
    print("Next: python3 scratch/validate_multimodal.py")

if __name__ == "__main__":
    main()
