#!/usr/bin/env bash
# Run from anywhere: resolve the repository root from this file's location.
cd "$(dirname "$0")/../.." || exit 1
export LD_LIBRARY_PATH=build/lib
# STANDING RULE: uniform 4-field job lines.
job() {
  d=results/fdgw9_$1_b3000/seed_$2; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed=$2 --boxHalf=3000 --gwGrid=9 --duration=3600 --fastDemote=$1 --trace=false \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "fdgw9 $1 s=$2 $3 $4"
}
export -f job
{ for s in $(seq 1 20); do echo "proposed $s . ."; done
  for s in $(seq 1 20); do echo "all $s . ."; done; } \
  | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'
echo "FDGW9 DONE"
