#!/usr/bin/env bash
# A0 input, focused: the section 4.1 reliability cell at 20 seeds.
#
# WHY THIS EXISTS SEPARATELY FROM THE 320-CELL GRID. results/p10_grid is absent from
# this repository and is cited as the source of two headline results. The B1 grid that
# analyse_grid_b1.py reads is 5 seeds per configuration; the section 4.1 result
# -- ca_tbl_full 19/20 against static_2p4 0/20 at 1.56x energy -- is TWENTY
# deployments, so it lives in a 20-seed campaign, not in B1. Regenerating B1 alone
# cannot reproduce it.
#
# This runs that one configuration at 20 seeds. It is 40 cells against the grid's 320,
# and it is the half that carries the result A0 was written to attach a verdict to.
# Value per minute of compute is the reason it runs FIRST.
#
# THE CONFIGURATION, and the one thing being guessed. Section 4.1 names
# RURAL_FOREST, dwell=800, boxHalf=2000. It does not name nlosSource, and the grid
# carries both. Rather than guess, BOTH are run: 2 x 20 seeds. Whichever reproduces
# 19/20 vs 0/20 is the one the report used, and that is then a finding about the
# original campaign's provenance rather than an assumption in this one.
#
# COST. 6 arms is ~3x the 2-arm run measured at 91.35 s solo. 40 cells at -P 11 is
# roughly 40 minutes.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/p10_reliability
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

ARMS=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb

job() {
  ns=$1; seed=$2
  d="results/p10_reliability/c_ef1_${ns}_dw800_b2000/seed_${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 \
    --envFacility=1 --nlosSource="$ns" --dwell=800 --boxHalf=2000 \
    --arms=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb \
    --trace=true --outdir="$d" > "$d/run.log" 2>&1
  echo "ef1_${ns}_dw800_b2000 s=${seed} rc=$?"
}
export -f job

{ for ns in legacy propagation; do
    for s in $(seq 1 20); do echo "$ns $s"; done
  done; } | xargs -P 11 -n 2 bash -c 'job "$0" "$1"'

# Completeness: rows, not existence (rule 12). Six arms plus a header is 7 lines.
# analyse_grid_b1.py's census also requires 6 traces per cell; check both.
bad=0
for ns in legacy propagation; do
  n=0; t=0
  for s in $(seq 1 20); do
    d="results/p10_reliability/c_ef1_${ns}_dw800_b2000/seed_${s}"
    [ -f "$d/arm_summary.csv" ] && [ "$(grep -c '' "$d/arm_summary.csv")" -eq 7 ] && n=$((n+1))
    [ "$(ls "$d"/trace_*.csv 2>/dev/null | wc -l)" -eq 6 ] && t=$((t+1))
  done
  printf '%-14s %2d/20 cells with 7 rows   %2d/20 with 6 traces\n' "$ns" "$n" "$t"
  [ "$n" -eq 20 ] && [ "$t" -eq 20 ] || bad=1
done

d0=$(grep -rh 'dirty ' results/p10_reliability/*/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/40"
[ "$d0" -eq 40 ] || bad=1

[ $bad -eq 0 ] || { echo "P10_RELIABILITY INCOMPLETE"; exit 1; }
echo "P10_RELIABILITY DONE"
