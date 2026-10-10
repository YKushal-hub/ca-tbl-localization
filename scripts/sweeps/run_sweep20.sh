#!/usr/bin/env bash
# Run from anywhere: resolve the repository root from this file's location.
cd "$(dirname "$0")/../.." || exit 1
export LD_LIBRARY_PATH=build/lib
# Validated by the cache-independence test: seed 7 run after seed 6 in one process
# is byte-identical to seed 7 in a fresh process, so g_fadingComponents and
# g_foliageDynamicCache do not carry state across seeds and a per-seed process
# split is the same experiment as one --seedCount=20 run.
run_one() {
  b=$1; s=$2
  d=results/sweep20_b${b}/seed_${s}
  mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=$s --boxHalf=$b --duration=3600 --outdir="$d" --trace=false \
     > "$d/run.log" 2>&1
  echo "done b=$b s=$s"
}
export -f run_one
for b in 1000 3000; do for s in $(seq 1 20); do echo "$b $s"; done; done \
  | xargs -P 10 -n 2 bash -c 'run_one "$0" "$1"'
echo "SWEEP20 DONE"
