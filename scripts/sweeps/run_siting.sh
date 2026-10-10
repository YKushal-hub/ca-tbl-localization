#!/usr/bin/env bash
# Run from anywhere: resolve the repository root from this file's location.
cd "$(dirname "$0")/../.." || exit 1
export LD_LIBRARY_PATH=build/lib
for t in 50 100 150 300 600 900; do
  mkdir -p results/t2_$t
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=7 --t2Triad=$t --duration=3600 --outdir=results/t2_$t --trace=false \
     > results/t2_$t/run.log 2>&1 &
done
# 30 is EXCLUDED: nominalRangeM[UWB] is 30.0 (ca_tbl_causal_sim.cc:221) and the
# anchors sit at z=2.5, so a 30 m triad puts every anchor at a slant range strictly
# greater than the label radius -- visUwb becomes identically zero and trigF1 is
# silently zeroed (v2 section 3.10). 25 is swept in its place.
for u in 5 10 20 25; do
  mkdir -p results/uwb_$u
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=7 --uwbTriad=$u --duration=3600 --outdir=results/uwb_$u --trace=false \
     > results/uwb_$u/run.log 2>&1 &
done
wait
echo "SITING SWEEP DONE"
