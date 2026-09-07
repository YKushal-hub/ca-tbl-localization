#!/usr/bin/env bash
# A1 -- the --dwell sweep at b3000. No code change; --dwell has existed throughout.
#
# WHY. Gateway density is NOT the variable that explains the conjunction result.
# b1000/gwGrid=3 and b3000/gwGrid=9 carry IDENTICAL density (2.25 gw/km^2) and differ
# EIGHT-FOLD in median error, 1.53 m against 12.05 m. Precision-tier coverage fraction
# tracks it instead: 11.49% against 3.39%, and a dwell/transit model predicts both
# geometries to within 1-2 percentage points.
#
# Facility sites sit at +-0.47 x boxHalf and are hardcoded, so inter-site separation
# scales with the box while anchor rings keep a fixed absolute radius. Coverage fraction
# is therefore about dwell / (dwell + transit), and restoring b1000's coverage fraction
# at b3000 needs dwell / (dwell + 1880) ~= 0.115, i.e. dwell ~= 245 s. That value is in
# the grid below as the PREDICTED point, not as a tuned one.
#
# 5 dwells x 20 seeds = 100 runs, four arms, traced.
#
# THE CAVEAT IS REGISTERED IN ADVANCE, from WORK_PACKAGES A1: raising dwell and adding
# facility sites both raise coverage fraction but are NOT the same intervention -- more
# sites shortens transit, longer dwell increases stationary time. If the conjunction
# returns under one and not the other, coverage fraction is not the whole story and the
# claim must be stated as dwell-dependent rather than coverage-dependent. The
# facility-count sweep is the cleaner test and needs a BuildDeployment() change; this is
# the cheap one that runs first.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/dwell_b3000
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

job() {
  d=$1; seed=$2
  o="results/dwell_b3000/d${d}_seed${seed}"; mkdir -p "$o"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 --boxHalf=3000 --gwGrid=3 \
    --dwell="$d" \
    --arms=ca_tbl_full,fixed_threshold,static_2p4,static_868 \
    --trace=true --outdir="$o" > "$o/run.log" 2>&1
  echo "d${d} s=${seed} rc=$?"
}
export -f job

{ for d in 90 150 245 400 800; do
    for s in $(seq 1 20); do echo "$d $s"; done
  done; } | xargs -P 11 -n 2 bash -c 'job "$0" "$1"'

# ---- completeness: COUNT ROWS, not non-empty (rule 12) -------------------------
# With four arms a complete summary is 5 lines: header plus one row per arm. A
# non-empty test passes from the first arm written, which is how 8 of 9 Pareto points
# once sat at 14/20 rows and reached analysis.
bad=0; total=0
for d in 90 150 245 400 800; do
  n=0
  for s in $(seq 1 20); do
    f="results/dwell_b3000/d${d}_seed${s}/arm_summary.csv"
    [ -f "$f" ] && [ "$(grep -c '' "$f")" -eq 5 ] && n=$((n+1))
  done
  total=$((total+n))
  printf 'dwell %-4s %2d/20 cells with 5 rows\n' "$d" "$n"
  [ "$n" -eq 20 ] || bad=1
done
echo "complete cells: $total/100"

d0=$(grep -rh 'dirty ' results/dwell_b3000/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/100"
[ "$d0" -eq 100 ] || bad=1

[ $bad -eq 0 ] || { echo "A1_DWELL INCOMPLETE"; exit 1; }
echo "A1_DWELL DONE"
