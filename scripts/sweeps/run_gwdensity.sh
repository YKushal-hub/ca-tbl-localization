#!/usr/bin/env bash
# Run from anywhere: resolve the repository root from this file's location.
cd "$(dirname "$0")/../.." || exit 1
export LD_LIBRARY_PATH=build/lib
# STANDING RULE: uniform 4-field job lines.
# gwGrid=9 at boxHalf 3000 gives 81 gw / 36 km^2 = 2.25/km^2, matching boxHalf 1000
# at gwGrid=3. Isolates "tier-2 coverage became spotty" from "tier-1 redundancy
# collapsed", which the original boxHalf sweep varied together.
job() {
  d=results/gw$1_b$2/seed_$3; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed=$3 --boxHalf=$2 --gwGrid=$1 --duration=3600 --trace=false \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "gw$1 b$2 s=$3 $4"
}
export -f job
{ for s in $(seq 1 20); do echo "9 3000 $s ."; done; } \
  | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'
echo "GWDENSITY DONE"
