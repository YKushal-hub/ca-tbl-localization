#!/usr/bin/env python3
"""patch_generator.py — Apply all three fixes to multimodal_generator.py"""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)


PATH = ROOT / "scratch/multimodal_generator.py"
with open(PATH) as f:
    src = f.read()

original = src  # keep for diff

# ── Patch 1: add json import ───────────────────────────────────────────────
src = src.replace(
    "import math, sys, time",
    "import math, sys, time, json",
    1
)

# ── Patch 2: replace hardcoded TDOA_RATES / FIX_INTERVAL ─────────────────
OLD_RATES = 'TDOA_RATES   = {"WAN": 0.72, "MESH": 0.31, "HYBRID": 0.54}\nFIX_INTERVAL = {"WAN": 15.0, "MESH": 62.0, "HYBRID": 28.0}  # seconds'
NEW_RATES = '''def _load_real_tdoa_rates(json_path="./results/topology_rates_summary.json"):
    """Load measured TdoaEligible rates from topology_observability_sim output.
    Falls back to literature constants with a loud warning if file not found."""
    try:
        with open(json_path) as f:
            d = json.load(f)
        print("[generator] Loaded REAL TDOA rates from", json_path)
        print(f"  WAN={d[\'WAN\']:.4f}  MESH={d[\'MESH\']:.4f}  HYBRID={d[\'HYBRID\']:.4f}")
        print(f"  (n_nodes={d[\'n_nodes\']}, n_gw={d[\'n_gw\']}, runs={d[\'runs\']})")
        return d
    except FileNotFoundError:
        print("[generator] WARNING: topology_rates_summary.json not found.")
        print("  Run topology_observability_sim first to replace literature constants.")
        print("  Falling back to literature values -- results will carry topology caveat.")
        return {"WAN": 0.72, "MESH": 0.31, "HYBRID": 0.54,
                "source": "literature_fallback"}

_REAL_RATES  = _load_real_tdoa_rates()
TDOA_RATES   = {t: _REAL_RATES[t] for t in ["WAN", "MESH", "HYBRID"]}
# FIX_INTERVAL: mean seconds between TDOA fixes, inversely proportional to rate.
# Reference: WAN at 15 s baseline. Capped at 120 s for very low-rate topologies.
BASE_INTERVAL_S = 15.0
FIX_INTERVAL = {t: min(BASE_INTERVAL_S / max(TDOA_RATES[t], 0.01), 120.0)
                for t in ["WAN", "MESH", "HYBRID"]}'''

if OLD_RATES not in src:
    print("ERROR: TDOA_RATES pattern not found — check generator version"); sys.exit(1)
src = src.replace(OLD_RATES, NEW_RATES, 1)

# ── Patch 3: fix EKF contamination + add IMU_ONLY mode ───────────────────
OLD_EKF = '''    if method in (2, 4):
        # EKF fusion only on rows where handoff fired (hok); fallback to pos_e
        fpe_ekf = ekf_fuse(pos_e, rerr, tsf)
        fpe = np.where(hok, fpe_ekf, pos_e)
    else:
        lv   = np.maximum(pos_e, 1.0)**2
        sv   = np.where(~np.isnan(rerr), (np.abs(rerr)+0.3)**2, 1e12)
        fpe  = np.where(act, np.sqrt((lv*sv)/(lv+sv+1e-9)), pos_e)

    mode = np.where(~hok, "LORA_ONLY", np.where(act, "FUSED", "LORA_ONLY"))'''

NEW_EKF = '''    if method in (2, 4):
        # EKF fusion — three distinct cases:
        #   act (hok & s2ok): full fusion — LoRa + IMU + Stage-2 range  => FUSED
        #   hok & ~act:       IMU-only    — handoff fired, Stage-2 failed => IMU_ONLY
        #   ~hok:             pure LoRa   — handoff never fired            => LORA_ONLY
        #
        # BUG FIX (confirmed 2026-07-11): the previous np.where(hok, fpe_ekf, pos_e)
        # applied EKF to ALL hok rows including hok & ~act, then labelled those rows
        # "LORA_ONLY". This contaminated LORA_MAE by ~32 m in IMU variants (M2/M4)
        # vs non-IMU variants (M1/M3). 23.45% of LORA_ONLY rows in M2 were affected.
        nan_rerr = np.full_like(rerr, np.nan)    # force IMU-only path
        fpe_full = ekf_fuse(pos_e, rerr,     tsf)  # full: LoRa + IMU + Stage-2
        fpe_imu  = ekf_fuse(pos_e, nan_rerr, tsf)  # IMU-only: LoRa + IMU
        fpe = np.where(act,
                       fpe_full,         # full fusion
              np.where(iact & ~act,
                       fpe_imu,          # IMU-only (hok but Stage-2 ranging failed)
                       pos_e))           # pure LoRa (handoff never fired)
    else:
        lv   = np.maximum(pos_e, 1.0)**2
        sv   = np.where(~np.isnan(rerr), (np.abs(rerr)+0.3)**2, 1e12)
        fpe  = np.where(act, np.sqrt((lv*sv)/(lv+sv+1e-9)), pos_e)

    # Mode labels cleanly partition the three cases:
    #   FUSED    : handoff fired AND Stage-2 ranging succeeded
    #   IMU_ONLY : handoff fired, Stage-2 ranging failed (IMU dead-reckoning only)
    #   LORA_ONLY: handoff never fired (FinalPositionErrorM == PositionErrorM exactly)
    mode = np.where(act,           "FUSED",
           np.where(iact & ~act,   "IMU_ONLY",
                                   "LORA_ONLY"))'''

if OLD_EKF not in src:
    print("ERROR: EKF pattern not found — check generator version"); sys.exit(1)
src = src.replace(OLD_EKF, NEW_EKF, 1)

# ── Write patched file ────────────────────────────────────────────────────
with open(PATH, "w") as f:
    f.write(src)

# Verify patches applied
assert "import math, sys, time, json" in src,        "Patch 1 FAILED"
assert "_load_real_tdoa_rates" in src,                "Patch 2 FAILED"
assert "IMU_ONLY" in src,                             "Patch 3 FAILED"
assert "BUG FIX (confirmed" in src,                   "Patch 3 comment FAILED"
assert "np.where(hok, fpe_ekf, pos_e)" not in src,   "Old EKF line still present"
assert '"WAN": 0.72' not in src,                      "Old TDOA_RATES still present"

print("All 3 patches applied and verified.")
print()

# Show diff summary
changed = [(i+1, a, b) for i,(a,b) in enumerate(zip(original.splitlines(), src.splitlines())) if a!=b]
print(f"Lines changed: {len(changed)}")
for lineno, old, new in changed[:20]:
    print(f"  L{lineno}: - {old.strip()}")
    print(f"       + {new.strip()}")
