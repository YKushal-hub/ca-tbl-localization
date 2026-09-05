#!/usr/bin/env bash
# A0 input: regenerate the 320-cell B1 grid that results/p10_grid is missing.
#
# WHY. results/p10_grid is cited by CA-TBL_Technical_Report_SelfContained.md as the
# source of two headline results and by Simulation.md M-27, and it is NOT IN THIS
# REPOSITORY. Neither is p10_plane or p10_plane_fix. No gate checks for it, and the
# README's claim that every table is sourced from a file in the repository is false
# for those rows. This regenerates the surface so A0's criterion has something to
# score.
#
# WHAT IT CANNOT DO. The regenerated sidecars carry THIS commit's provenance, not the
# original campaign's. It answers A0; it does not restore the cited evidence. That
# limitation is registered in report/PREREG_A0_JOINT_CRITERION.md section 7 and must
# travel with any number produced from this tree.
#
# THE CELL KEY is taken from scratch/python_handoff/analyse_grid_b1.py, not invented:
#
#   EFS   = [0, 1, 3, 4]            0 RURAL_OPEN  1 RURAL_FOREST
#                                   3 URBAN_MOD   4 URBAN_DENSE
#   NSS   = [legacy, propagation]
#   DWS   = [90, 800]
#   BOXES = [1500, 2000, 2500, 3000]
#   SEEDS = 1..5
#   4 x 2 x 2 x 4 x 5 = 320 cells, six arms each
#
#   dir   results/p10_grid/c_ef%d_%s_dw%d_b%d/seed_%d
#
# That script's census demands 6 traces, 6 summary rows and 7 verified digests per
# cell, so --trace=true is required and the arm list is not negotiable.
#
# UNKNOWNS, STATED. The analysis script fixes the cell key but not the rest of the
# flag set. gwGrid, pcsForm and mahalUseR are left at their defaults here because
# nothing on disk records what the original campaign set them to. If a regenerated
# cell disagrees with a published number, that is evidence about the original
# campaign's unrecorded flags, not licence to tune this script until it matches.
#
# COST. Six arms is ~3x the 2-arm run measured at 91.35 s solo. 320 cells at -P 11 is
# roughly 4-5 hours. Intended to run unattended.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/p10_grid
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

job() {
  ef=$1; ns=$2; dw=$3; b=$4; seed=$5
  d="results/p10_grid/c_ef${ef}_${ns}_dw${dw}_b${b}/seed_${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 \
    --envFacility="$ef" --nlosSource="$ns" --dwell="$dw" --boxHalf="$b" \
    --arms=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb \
    --trace=true --outdir="$d" > "$d/run.log" 2>&1
  echo "ef${ef}_${ns}_dw${dw}_b${b} s=${seed} rc=$?"
}
export -f job

{ for ef in 0 1 3 4; do
    for ns in legacy propagation; do
      for dw in 90 800; do
        for b in 1500 2000 2500 3000; do
          for s in $(seq 1 5); do echo "$ef $ns $dw $b $s"; done
        done
      done
    done
  done; } | xargs -P 11 -n 5 bash -c 'job "$0" "$1" "$2" "$3" "$4"'

# Completeness against the census analyse_grid_b1.py will run: 7 summary lines
# (6 arms + header) and 6 traces per cell. Rows, not existence (rule 12).
ok=0; missing=0
for ef in 0 1 3 4; do
  for ns in legacy propagation; do
    for dw in 90 800; do
      for b in 1500 2000 2500 3000; do
        for s in $(seq 1 5); do
          d="results/p10_grid/c_ef${ef}_${ns}_dw${dw}_b${b}/seed_${s}"
          r=0; [ -f "$d/arm_summary.csv" ] && r=$(grep -c '' "$d/arm_summary.csv")
          t=$(ls "$d"/trace_*.csv 2>/dev/null | wc -l)
          if [ "$r" -eq 7 ] && [ "$t" -eq 6 ]; then ok=$((ok+1))
          else missing=$((missing+1)); echo "  INCOMPLETE $d rows=$r traces=$t"; fi
        done
      done
    done
  done
done
echo "complete cells: $ok/320   incomplete: $missing"

d0=$(grep -rh 'dirty ' results/p10_grid/*/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/320"

[ "$ok" -eq 320 ] && [ "$d0" -eq 320 ] || { echo "P10_GRID INCOMPLETE"; exit 1; }
echo "P10_GRID DONE"
