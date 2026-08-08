#!/usr/bin/env python3
"""sanity_check_regen.py -- Verify regenerated CSVs have all fixes applied."""
import json, sys
import numpy as np
import pandas as pd
from pathlib import Path

DST = Path("./results")
CHUNK = 500_000

def check(label, cond, detail=""):
    status = "PASS" if cond else "FAIL"
    print(f"  [{status}] {label}" + (f": {detail}" if detail else ""))
    return cond

print("=" * 60)
print("  Sanity check: regenerated CSVs")
print("=" * 60)

# Load real rates
rates = json.load(open(DST / "topology_rates_summary.json"))
print(f"Real TDOA rates: WAN={rates['WAN']:.4f} MESH={rates['MESH']:.4f} HYBRID={rates['HYBRID']:.4f}")
print()

all_ok = True
for meth in [1, 2, 3, 4]:
    for topo in ["WAN", "MESH", "HYBRID"]:
        csv = DST / f"multimodal_M{meth}_T{topo}.csv"
        if not csv.exists():
            print(f"M{meth}x{topo}: MISSING")
            all_ok = False
            continue

        chunk = next(pd.read_csv(csv, chunksize=CHUNK))
        n = len(chunk)
        print(f"M{meth}×{topo}  ({n:,} rows in first chunk):")

        # Check 1: LORA_ONLY rows have FinalPositionErrorM == PositionErrorM
        lo = chunk[chunk["FinalLocalizationMode"] == "LORA_ONLY"]
        if len(lo) > 0:
            diff = (lo["FinalPositionErrorM"] - lo["PositionErrorM"]).abs()
            contaminated = (diff > 1e-6).mean()
            ok = check("LORA_ONLY clean", contaminated < 0.001,
                       f"{contaminated*100:.3f}% contaminated, n={len(lo):,}")
            all_ok = all_ok and ok

        # Check 2: IMU variants have IMU_ONLY mode present
        modes = chunk["FinalLocalizationMode"].value_counts()
        if meth in (2, 4):
            has_imu_only = "IMU_ONLY" in modes.index
            ok = check("IMU_ONLY mode exists", has_imu_only,
                       f"modes: {dict(modes)}")
            all_ok = all_ok and ok
        else:
            no_imu = "IMU_ONLY" not in modes.index
            ok = check("No IMU_ONLY in M1/M3", no_imu,
                       f"modes: {dict(modes)}")
            all_ok = all_ok and ok

        # Check 3: FUSED rate roughly matches real TDOA rate
        fused_rate = (chunk["FinalLocalizationMode"] == "FUSED").mean()
        expected = rates[topo]
        # FUSED <= TDOA_RATE (gated by pos_err<=50 AND s2ok)
        ok = check("FUSED rate <= TDOA rate",
                   fused_rate <= expected * 1.05,
                   f"FUSED={fused_rate:.4f} TDOA_RATE={expected:.4f}")
        all_ok = all_ok and ok
        print()

print("=" * 60)
print(f"Overall: {'ALL PASS' if all_ok else 'SOME FAILURES — see above'}")
