#!/usr/bin/env python3
"""
diag_tdoa.py -- Bug 2: Measure real TdoaEligible rates from Stage 1 data.

The generator uses TDOA_RATES = {WAN:0.72, MESH:0.31, HYBRID:0.54}
as literature-guessed constants. This script derives the REAL rates
by examining what fraction of rows ACTUALLY qualify for Stage-1 handoff
(PositionErrorM <= 50m) in the base LoRa dataset.

Note: TdoaEligible has two components:
  1. Physical eligibility: PositionErrorM <= 50m (gateway geometry sufficient)
  2. Topology probability: TDOA_RATES[topo] (gateway observability rate)

Component 1 is measurable from the data. Component 2 is the ASSUMED part.
We measure component 1 and flag component 2 as still needing real simulation.
"""
import pandas as pd
import numpy as np

SRC = "./results/ml_training_dataset.csv"

print("=== Bug 2 Diagnostic: Real TdoaEligible rate from Stage 1 data ===\n")
print("Loading 2% sample of ml_training_dataset.csv...")
chunks = []
for chunk in pd.read_csv(SRC, chunksize=500_000):
    chunks.append(chunk.sample(frac=0.02, random_state=42))
df = pd.concat(chunks, ignore_index=True)
print(f"  {len(df):,} rows loaded\n")

# Physical eligibility (component 1 — measurable)
elig = df["PositionErrorM"] <= 50.0
print(f"Physical eligibility (PositionErrorM <= 50m): {elig.mean()*100:.1f}% of all rows")
print(f"  Mean PositionErrorM: {df['PositionErrorM'].mean():.1f}m")
print(f"  Median:              {df['PositionErrorM'].median():.1f}m")
print(f"  P90:                 {df['PositionErrorM'].quantile(0.90):.1f}m")
print(f"  P99:                 {df['PositionErrorM'].quantile(0.99):.1f}m")
print()

# This is what actually feeds into handoff
assumed = {"WAN": 0.72, "MESH": 0.31, "HYBRID": 0.54}
elig_rate = elig.mean()
print("Resulting TdoaEligible (physical * topology probability):")
for topo, rate in assumed.items():
    combined = elig_rate * rate
    print(f"  {topo:6s}: physical={elig_rate*100:.1f}% × assumed_topology={rate*100:.0f}% = {combined*100:.1f}% effective handoff rate")

print()
print("Current FUSED rates in multimodal CSVs (from actual generated data):")
import json, os
for m in [1,2,3,4]:
    for t in ["WAN","MESH","HYBRID"]:
        p = f"./results/stage2_result_M{m}_T{t}.json"
        if os.path.exists(p):
            with open(p) as f:
                row = json.load(f)
            print(f"  M{m}x{t:6s}: FusedRate={row.get('FusedRate_pct','?')}%")

print()
print("VERDICT on Bug 2:")
print("  TDOA_RATES = {WAN:0.72, MESH:0.31, HYBRID:0.54} is NOT from")
print("  the topology simulator — it is from a literature guess on line 34.")
print("  The topology ordering WAN > HYBRID > MESH in results directly mirrors")
print("  this assumed rate ordering 0.72 > 0.54 > 0.31, confirming the results")
print("  are replaying the assumption, not measuring topology physics.")
print()
print("  Path A: Run hybrid_lorawan_mesh.cc if it exists and produces per-node")
print("          GatewayObservabilityCount >= 3 fractions per topology.")
print("  Path B: If simulator does not exist, label all topology comparisons as")
print("          'illustrative — TDOA rates not yet from simulator output'.")
