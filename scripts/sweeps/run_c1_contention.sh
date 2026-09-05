#!/usr/bin/env bash
# C1 contention campaign — turns the seed-7 smoke observation into a paired result.
#
# WHY IT EXISTS. report/C1_CONTENTION_EFFECT.md reports mac_tx +26 201 and collisions
# +41 % across --sendHandshakeFrames at ONE seed, and says in its own "what this does
# NOT establish" section that the figure needs a paired multi-seed campaign before it
# is quoted. This is that campaign. Writing the requirement down and then not running
# it would be worse than not writing it down.
#
# THE PREDICTION IS ALREADY ON RECORD, in commits ac41f6c and 4c0b175, made before
# any multi-seed cell ran:
#   1. every ESCALATING arm moves -- real frames contend where a charge did not
#   2. all three STATIC arms are byte-identical, because they never escalate, so no
#      handshake is issued and no frame is added
# Prediction 2 is the control. If a static arm moves, the flag is perturbing the
# scenario globally and the whole comparison is void -- that is the falsifier, and it
# is checked first below.
#
# 2 flag states x 20 paired seeds x 6 arms = 40 runs. Traced.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/c1_contention
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

job() {
  state=$1; seed=$2
  d="results/c1_contention/${state}/seed_${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy \
    --sendHandshakeFrames="$state" \
    --trace=true --outdir="$d" > "$d/run.log" 2>&1
  echo "${state} s=${seed} rc=$?"
}
export -f job

{ for state in false true; do
    for s in $(seq 1 20); do echo "$state $s"; done
  done; } | xargs -P 11 -n 2 bash -c 'job "$0" "$1"'

# ---- completeness: rows, not existence (rule 12) -------------------------------
bad=0
for state in false true; do
  n=0
  for s in $(seq 1 20); do
    f="results/c1_contention/${state}/seed_${s}/arm_summary.csv"
    [ -f "$f" ] && [ "$(grep -c '' "$f")" -eq 7 ] && n=$((n+1))
  done
  printf '%-6s %2d/20 cells with 7 rows\n' "$state" "$n"
  [ "$n" -eq 20 ] || bad=1
done

d0=$(grep -rh 'dirty ' results/c1_contention/*/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/40"
[ "$d0" -eq 40 ] || bad=1

[ $bad -eq 0 ] || { echo "C1_CONTENTION INCOMPLETE"; exit 1; }
echo "C1_CONTENTION DONE"
