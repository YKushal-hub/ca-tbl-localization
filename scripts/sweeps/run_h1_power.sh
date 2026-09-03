#!/usr/bin/env bash
# h = 1 at the registered seed count of 40. results/h1_power/PREDICTION.md, commit 9805817,
# was committed before the first cell.
#
#   bash scripts/sweeps/run_h1_power.sh <seed_lo> <seed_hi>
#
# Cells are (policy, seed); each invocation runs BOTH arms, so the ablation sees the same
# fault set and the comparison is not measuring fault placement.
set -u
cd "$(dirname "$0")/../.." || exit 2
LO=${1:?seed_lo}; HI=${2:?seed_hi}
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
OUT=results/h1_power
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
for seed in $(seq "$LO" "$HI"); do
  for pol in none exclude; do
    d="$OUT/h1_${pol}_seed${seed}"
    [ -s "$d/arm_summary.csv" ] && { echo "skip  $d"; continue; }
    mkdir -p "$d"
    t0=$(date +%s)
    "$BIN" --seed="$seed" --duration=3600 --boxHalf=2000 --gwGrid=5 \
      --escalationRequiresEvidence=true --nlosSource=propagation --pcsForm=v2 \
      --suspectForcesEscalation=false --mahalUseR=true \
      --injectRangeBias=40 --injectBiasFraction=0.2 --injectFaultAnchors=1 \
      --faultMultiplicity=1 --suspectRecovery="$pol" \
      --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true --outdir="$d" \
      > "$d/stdout.txt" 2> "$d/stderr.txt"
    rc=$?
    t1=$(date +%s)
    echo "cell $d rc=$rc $((t1 - t0))s"
  done
done
