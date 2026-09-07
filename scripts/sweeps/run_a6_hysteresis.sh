#!/usr/bin/env bash
# A6 — the hysteresis sweep. CAL-03 and CAL-04, asked for the first time.
#
# Registered in report/PREREG_A6_HYSTERESIS.md before the first cell.
#
# 5 bands x 3 dwells x 20 paired seeds = 300 runs, two arms.
#
# TWO ARMS, NOT SIX, and it is not a shortcut: the static arms never escalate, so the
# hysteresis band cannot reach them, and running them would triple the compute to
# measure a quantity that is structurally constant. fixed_threshold is kept because it
# also escalates and gives a second arm to check the direction against.
#
# THE BASELINE CELL IS band=0.2 dwell=5, which is the built-in configuration. Section 4
# prediction 1 makes it a null control INSIDE the sweep, and section 7 aborts the
# campaign if it does not reproduce a run with neither flag passed. That check runs
# first, before the other fourteen cells are spent.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/a6_hysteresis
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

ARMS=ca_tbl_full,fixed_threshold

job() {
  band=$1; dwell=$2; seed=$3
  d="results/a6_hysteresis/b${band}_n${dwell}/seed_${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 --boxHalf=2000 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy \
    --pcsHysteresisBand="$band" --nDeescalate="$dwell" \
    --arms=ca_tbl_full,fixed_threshold \
    --trace=false --outdir="$d" > "$d/run.log" 2>&1
  echo "b${band}_n${dwell} s=${seed} rc=$?"
}
export -f job

# ---- the registered abort check, first ---------------------------------------
# Prediction 1 / section 7: the baseline cell must equal a run with neither flag set.
echo "--- abort check: baseline cell (band 0.2, dwell 5) against a no-flag run ---"
NB=$(mktemp -d "${TMPDIR:-/tmp}/a6_noflag.XXXXXX")
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
  --seed=7 --duration=3600 --boxHalf=2000 \
  --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy \
  --arms=ca_tbl_full,fixed_threshold --trace=false --outdir="$NB" > "$NB/run.log" 2>&1

BC="results/a6_hysteresis/b0.2_n5/seed_7"; mkdir -p "$BC"
job 0.2 5 7 > /dev/null

if diff -q "$NB/arm_summary.csv" "$BC/arm_summary.csv" >/dev/null 2>&1; then
  echo "ABORT CHECK PASS -- the flags are inert at their defaults"
  rm -rf "$NB"
else
  echo "ABORT CHECK FAILED -- band 0.2 / dwell 5 is not the built-in configuration."
  echo "Section 7 stops the campaign here. Diff:"
  diff "$NB/arm_summary.csv" "$BC/arm_summary.csv" | cut -c1-200 | head -10
  exit 1
fi

# ---- the sweep ----------------------------------------------------------------
{ for band in 0.0 0.1 0.2 0.4 0.8; do
    for dwell in 1 5 10; do
      for s in $(seq 1 20); do
        [ "$band" = "0.2" ] && [ "$dwell" = "5" ] && [ "$s" = "7" ] && continue
        echo "$band $dwell $s"
      done
    done
  done; } | xargs -P 11 -n 3 bash -c 'job "$0" "$1" "$2"'

# ---- completeness: rows, not existence (rule 12) -------------------------------
bad=0; total=0
for band in 0.0 0.1 0.2 0.4 0.8; do
  for dwell in 1 5 10; do
    n=0
    for s in $(seq 1 20); do
      f="results/a6_hysteresis/b${band}_n${dwell}/seed_${s}/arm_summary.csv"
      [ -f "$f" ] && [ "$(grep -c '' "$f")" -eq 3 ] && n=$((n+1))
    done
    total=$((total+n))
    [ "$n" -eq 20 ] || { printf 'INCOMPLETE b%s_n%s: %d/20\n' "$band" "$dwell" "$n"; bad=1; }
  done
done
echo "complete cells: $total/300"

d0=$(grep -rh 'dirty ' results/a6_hysteresis/*/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/300"
[ "$d0" -eq 300 ] || bad=1

[ $bad -eq 0 ] || { echo "A6_HYSTERESIS INCOMPLETE"; exit 1; }
echo "A6_HYSTERESIS DONE"
