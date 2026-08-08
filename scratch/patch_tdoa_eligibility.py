#!/usr/bin/env python3
"""
patch_tdoa_eligibility.py
==========================
Corrects the Multi-Hop TDOA Fallacy in geometric_training_dataset.csv.

ROOT CAUSE (topology_observability_sim.cc, lines 200-211):
  WAN: reachedGwIds = nodeGwDirectIds[i]          <- direct links only (correct for TDOA)
  MESH: reachedGwIds = MeshGwReach(..., hopLimit=2) <- relay allowed (WRONG for TDOA)
  HYBRID: relay fallback if no direct links        <- WRONG for TDOA

FIX PRINCIPLE:
  TDOA timing requires direct node-to-gateway RF reception.
  Nanosecond TDOA precision cannot survive a relay hop (node -> peer -> gateway
  adds peer processing + second propagation delay, destroying the original tx timestamp).
  All three topologies share the same physical node/gateway layout per run
  (same rng seed: run * 12345 + 67890). WAN's direct-link data IS the correct
  TDOA eligibility for MESH and HYBRID.

  For each (Run, NodeId), copy WAN row's:
    TdoaEligible, GwObsCount, Gw{1..10}Id, Gw{1..10}X, Gw{1..10}Y, Gw{1..10}ErrNs
  to MESH and HYBRID rows.

  GwObsCount for MESH/HYBRID in the corrected file now means DIRECT gateway count
  (TDOA capability), not relay reach. Relay reach is implicitly captured by topology
  label and is relevant for data backhaul, not TDOA timing.

OUTPUT:
  results/geometric_training_dataset_corrected.csv
"""
import pandas as pd
import numpy as np
from pathlib import Path
import time

SRC = Path("results/geometric_training_dataset.csv")
DST = Path("results/geometric_training_dataset_corrected.csv")

GW_COLS = []
for k in range(1, 11):
    GW_COLS += [f"Gw{k}Id", f"Gw{k}X", f"Gw{k}Y", f"Gw{k}ErrNs"]

TDOA_COLS = ["GwObsCount", "TdoaEligible"] + GW_COLS

print("=" * 65)
print("Patching Multi-Hop TDOA Fallacy in geometric_training_dataset.csv")
print("=" * 65)
print(f"Source: {SRC}")
print(f"Dest:   {DST}")
print()

t0 = time.time()

print("Loading dataset...")
df = pd.read_csv(SRC)
print(f"  {len(df):,} rows loaded in {time.time()-t0:.1f}s")
print(f"  Columns: {list(df.columns[:10])} ...")
print()

# Verify structure: each (Run, NodeId) should appear exactly 3 times (WAN, MESH, HYBRID)
counts = df.groupby(["Run", "NodeId"])["Topology"].count()
assert counts.min() == 3 and counts.max() == 3, \
    f"Expected 3 rows per (Run,NodeId), got range [{counts.min()},{counts.max()}]"
print("Structure check: 3 rows per (Run,NodeId) ✓")

# Before-patch statistics
print()
print("── Before patch ──────────────────────────────────────────────")
before = df.groupby("Topology")["TdoaEligible"].agg(["sum","count","mean"])
before["eligible_%"] = (before["mean"] * 100).round(2)
print(before[["sum","eligible_%"]].to_string())

# Extract WAN reference rows
wan = df[df["Topology"] == "WAN"][["Run", "NodeId"] + TDOA_COLS].copy()
wan = wan.set_index(["Run", "NodeId"])

# Patch MESH and HYBRID: overwrite TDOA_COLS from WAN
idx_mesh   = (df["Topology"] == "MESH")
idx_hybrid = (df["Topology"] == "HYBRID")

print()
print("Applying patch (WAN direct-link data -> MESH and HYBRID)...")
t1 = time.time()

# Build lookup arrays aligned with each non-WAN row
def apply_patch(mask, label):
    sub = df[mask].copy()
    # Look up WAN values for matching (Run, NodeId)
    keys = sub.set_index(["Run", "NodeId"])
    patched = wan.loc[keys.index, TDOA_COLS]
    patched.index = sub.index  # restore original iloc index
    df.loc[mask, TDOA_COLS] = patched.values
    n = mask.sum()
    print(f"  Patched {n:,} {label} rows in {time.time()-t1:.1f}s")

apply_patch(idx_mesh,   "MESH")
apply_patch(idx_hybrid, "HYBRID")

print()
print("── After patch ──────────────────────────────────────────────")
after = df.groupby("Topology")["TdoaEligible"].agg(["sum","count","mean"])
after["eligible_%"] = (after["mean"] * 100).round(2)
print(after[["sum","eligible_%"]].to_string())

print()
print("WAN vs MESH eligibility should now be identical (same direct-link geometry):")
wan_rate  = df[df["Topology"] == "WAN"]["TdoaEligible"].mean()
mesh_rate = df[df["Topology"] == "MESH"]["TdoaEligible"].mean()
hyb_rate  = df[df["Topology"] == "HYBRID"]["TdoaEligible"].mean()
print(f"  WAN:    {wan_rate*100:.3f}%")
print(f"  MESH:   {mesh_rate*100:.3f}%  (was 99.999% -- relay inflation fixed)")
print(f"  HYBRID: {hyb_rate*100:.3f}%  (was 47.0%  -- relay fallback fixed)")

# GwObsCount distribution check
print()
print("Direct GwObsCount distribution (all topologies now identical):")
print(df[df["TdoaEligible"] == 1].groupby("Topology")["GwObsCount"].describe()[["min","50%","mean","max"]].round(2).to_string())

print()
print(f"Saving corrected dataset to {DST}...")
t2 = time.time()
df.to_csv(DST, index=False)
mb = DST.stat().st_size / 1e6
print(f"  Done: {mb:.0f}MB in {time.time()-t2:.1f}s")
print(f"Total elapsed: {time.time()-t0:.1f}s")

print()
print("Next steps:")
print("  1. Copy corrected file over original:")
print("     cp results/geometric_training_dataset_corrected.csv results/geometric_training_dataset.csv")
print("  2. Re-run: python3 process_tdoa.py")
print("  3. Re-run: python3 scratch/fix_dataset.py")
print("  4. Re-run: python3 scratch/generate_method0.py")
print("  5. Re-run: python3 scratch/multimodal_generator.py")
print("  6. Re-run: python3 scratch/generate_final_report.py")
