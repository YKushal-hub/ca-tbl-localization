#!/usr/bin/env python3
"""Diagnostic 2: check stage2 model feature list for leakage."""
import pickle

LEAKAGE_BLACKLIST = [
    "PositionErrorM",        # IS the target-equivalent for LORA_ONLY rows
    "RangeErrorM",           # ground-truth range error
    "Stage2RangeErrorM",     # ground-truth fine-range error
    "FinalPositionErrorM",   # the actual target
    "EstimatedRangeM",
    "PropagationLossBiasDb",
    "Stage1HandoffTimeS",    # derived from ground truth timing
]

OBSERVABLE_OK = [
    "MeasuredRssiDbm", "FinalSnrDb", "RawSnrDb", "NoiseFloorDbm",
    "SpreadingFactor", "TxPowerDbm", "GatewayHeightM", "NodeHeightM",
    "Environment", "DistanceM",
    "Stage2RangeEstimateM",   # the MEASURED fine-range (OK)
    "Stage2IsNLOS",           # NLOS detection flag (OK)
    "ImuActive", "TimeSinceLastLoRaFixS", "ImuDriftEstimateM",
]

model_path = "results/stage2_M1_TWAN.pkl"
with open(model_path, "rb") as f:
    saved = pickle.load(f)

model = saved["model"]
feat_cols = saved.get("features", [])

try:
    actual_features = list(model.booster_.feature_name())
except Exception:
    actual_features = feat_cols

print("=== DIAGNOSTIC 2: Feature Leakage Check ===\n")
print("Features used to train stage2_M1_TWAN model:")
for i, f in enumerate(actual_features):
    tag = ""
    if f in LEAKAGE_BLACKLIST:
        tag = "  <<< LEAKAGE — MUST REMOVE"
    elif f in OBSERVABLE_OK:
        tag = "  (ok — observable)"
    print(f"  [{i:2d}] {f}{tag}")

leaked = [f for f in actual_features if f in LEAKAGE_BLACKLIST]
print()
if leaked:
    print(f"RESULT: {len(leaked)} LEAKED FEATURES FOUND: {leaked}")
    print("This is the root cause of Bug 1 — the model learns to copy")
    print("its target via these ground-truth columns.")
else:
    print("RESULT: No obvious leakage in feature list.")
    print("Bug 1 root cause must be in the CSV generator or evaluation logic.")

print()
print(f"Total features: {len(actual_features)}")
print(f"Leaked: {leaked}")
