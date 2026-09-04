#!/usr/bin/env bash
# A2 stage 1 -- detect-and-escalate under injected fault. THE BRANCH POINT.
#
# The pre-registration is report/PREREG_C9_ESCALATE.md, committed at 134d3e6 before
# this script existed. Nothing about the estimator, the prediction or the decision
# rule is set here. If a flag below disagrees with that document, the document wins
# and this script is wrong.
#
# WHAT IS NEW HERE, AND IT IS ONE FLAG. Every other flag matches results/abl_clean
# and results/c9_clean exactly, so the resulting table is comparable to
# CA-TBL_Technical_Report_SelfContained.md section 5.1 cell for cell. The single
# difference is --suspectForcesEscalation=true. A census of all 162 injection
# sidecars found 160 at =false and 0 at =true: the combined mechanism has never run.
#
# FAULT LABEL MAPPING -- DO NOT "SIMPLIFY" THIS.
#
#   --injectFaultAnchors=1  ->  F1   single-anchor, the canonical RAIM fault model
#   --injectFaultAnchors=0  ->  F3   common-mode across every tier-2 link
#
# The mapping is NOT fa+1. docs/WORK_PACKAGES.md A2 and docs/KICKOFF.md both give
# F$((fa+1)), which labels common-mode as F1 and emits a nonexistent F2 -- and would
# silently set F3 beside F1 when read against section 5.1. Verified against
# results/abl_clean/bounded_F1_seed1/run_id.txt (injectFaultAnchors=1) and
# results/abl_clean/bounded_F3_seed1/run_id.txt (injectFaultAnchors=0).
#
# COST. One run is 91.35 s wall at these flags, measured at 170cd20, not estimated.
# 160 runs at -P 8 is roughly 35 minutes. ~780 KB per run traced, so ~125 MB total.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

# Rule 7: verification must not mutate its subject. Never delete or overwrite a
# results directory in place; move it aside and say so.
R=results/c9_escalate
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S)
  echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

# dirty 0 or it is not citable (CLAUDE.md section 4 item 5). Checked BEFORE the runs,
# because a sidecar recording dirty>0 cannot be repaired afterwards -- the campaign
# has to be re-run.
if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- every run_id.txt would record dirty>0 and no cell would be citable:"
  git status --porcelain --untracked-files=no
  exit 1
fi

job() {
  pol=$1; fa=$2; seed=$3; fname=$4
  d="results/c9_escalate/${pol}_${fname}_seed${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 --boxHalf=2000 --gwGrid=5 \
    --escalationRequiresEvidence=true --nlosSource=propagation \
    --pcsForm=v2 --mahalUseR=true \
    --injectRangeBias=40 --injectBiasFraction=0.2 \
    --injectFaultAnchors="$fa" --suspectRecovery="$pol" \
    --suspectForcesEscalation=true \
    --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "${pol}_${fname} s=${seed} rc=$?"
}
export -f job

{ for pol in none inflate bounded reinit; do
    for s in $(seq 1 20); do
      echo "$pol 1 $s F1"
      echo "$pol 0 $s F3"
    done
  done; } | xargs -P 8 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'

# Completeness gate. Rule 12: COUNT ROWS, do not test for non-empty. arm_summary.csv
# is created at run start and written at run end, so both `ls | wc -l` and
# `find -size +0` report complete while runs are still in flight. Two arms plus a
# header is 3 lines; anything else is a cell that did not finish.
bad=0; complete=0
for pol in none inflate bounded reinit; do
  for fname in F1 F3; do
    n=0
    for s in $(seq 1 20); do
      f="results/c9_escalate/${pol}_${fname}_seed${s}/arm_summary.csv"
      [ -f "$f" ] && [ "$(grep -c '' "$f")" -eq 3 ] && n=$((n+1))
    done
    printf '%-18s %2d/20 cells with 3 rows\n' "${pol}_${fname}" "$n"
    complete=$((complete+n))
    [ "$n" -eq 20 ] || bad=1
  done
done
echo "total complete cells: $complete/160"

# Provenance gate. A cell whose sidecar records dirty>0 is not citable, and finding
# that out at analysis time means re-running the campaign.
d0=$(grep -rh 'dirty ' results/c9_escalate/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/160"
[ "$d0" -eq 160 ] || bad=1

[ $bad -eq 0 ] || { echo "C9_ESCALATE INCOMPLETE"; exit 1; }
echo "C9_ESCALATE DONE"
