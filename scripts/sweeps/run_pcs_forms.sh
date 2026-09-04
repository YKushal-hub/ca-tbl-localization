#!/usr/bin/env bash
# D-1 repair campaign. Registered in report/PREREG_PCS_FORMS.md, committed 4fc556c
# BEFORE this script was run.
#
# 4 forms x 20 paired seeds x 6 arms = 80 runs, traced.
#
# WHY IT RUNS FROM THE DEV WORKTREE. --pcsForm=weighted and =gated exist only on
# wip/lane-b. The main tree is mid-campaign on p10_grid and must not be committed to
# while that runs, because moving HEAD under a running campaign is what produced the
# mixed-SHA problem in c9_escalate. So this runs here, its sidecars cite this branch,
# and the trees are merged once both campaigns are finished.
#
# THE LEGACY CELLS ARE THE ABORT CHECK, not filler. Section 7 of the registration
# stops the campaign if legacy at seed 7 does not reproduce results/baseline_v7
# byte-for-byte. Run them first so the check fires before compute is spent on the
# other three forms.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/pcs_forms
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

job() {
  form=$1; seed=$2
  d="results/pcs_forms/${form}/seed_${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 --boxHalf=2000 \
    --nlosSource=propagation --arbitrateDutyBudget=true \
    --pcsForm="$form" \
    --arms=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb \
    --trace=true --outdir="$d" > "$d/run.log" 2>&1
  echo "${form} s=${seed} rc=$?"
}
export -f job

# ---- legacy first, so the abort check fires early ------------------------------
{ for s in $(seq 1 20); do echo "legacy $s"; done; } \
  | xargs -P 11 -n 2 bash -c 'job "$0" "$1"'

# Registered abort: legacy at seed 7 must reproduce baseline_v7 byte-for-byte.
# baseline_v7 lives in the OTHER worktree; both are the same commit history and the
# file is tracked, so compare against it directly.
B7="$HOME/projects/ca-tbl-localization/results/baseline_v7/arm_summary.csv"
if [ -f "$B7" ]; then
  if diff -q "$B7" results/pcs_forms/legacy/seed_7/arm_summary.csv >/dev/null 2>&1; then
    echo "ABORT CHECK PASS -- legacy seed 7 reproduces baseline_v7 byte-for-byte"
  else
    echo "ABORT CHECK FAILED -- legacy seed 7 does not reproduce baseline_v7."
    echo "Section 7 of the registration stops the campaign here. Diff:"
    diff "$B7" results/pcs_forms/legacy/seed_7/arm_summary.csv | head -20
    exit 1
  fi
else
  echo "WARNING: baseline_v7 not found, abort check could not run"
fi

# ---- the three remaining forms -------------------------------------------------
{ for form in v2 weighted gated; do
    for s in $(seq 1 20); do echo "$form $s"; done
  done; } | xargs -P 11 -n 2 bash -c 'job "$0" "$1"'

# ---- completeness: rows, not existence (rule 12) -------------------------------
bad=0
for form in legacy v2 weighted gated; do
  n=0; t=0
  for s in $(seq 1 20); do
    d="results/pcs_forms/${form}/seed_${s}"
    [ -f "$d/arm_summary.csv" ] && [ "$(grep -c '' "$d/arm_summary.csv")" -eq 7 ] && n=$((n+1))
    [ "$(ls "$d"/trace_*.csv 2>/dev/null | wc -l)" -eq 6 ] && t=$((t+1))
  done
  printf '%-10s %2d/20 cells with 7 rows   %2d/20 with 6 traces\n' "$form" "$n" "$t"
  [ "$n" -eq 20 ] && [ "$t" -eq 20 ] || bad=1
done

d0=$(grep -rh 'dirty ' results/pcs_forms/*/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/80"
[ "$d0" -eq 80 ] || bad=1

[ $bad -eq 0 ] || { echo "PCS_FORMS INCOMPLETE"; exit 1; }
echo "PCS_FORMS DONE"
