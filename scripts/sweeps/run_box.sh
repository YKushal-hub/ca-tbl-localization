#!/usr/bin/env bash
# Run from anywhere: resolve the repository root from this file's location.
cd "$(dirname "$0")/../.." || exit 1
export LD_LIBRARY_PATH=build/lib
for b in 1000 1500 2000 3000 4000 6000; do
  mkdir -p results/box_$b
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=7 --boxHalf=$b --duration=3600 --outdir=results/box_$b --trace=false \
     > results/box_$b/run.log 2>&1 &
done
wait
echo "BOX SWEEP DONE"
