#!/usr/bin/env bash
# A2 stage 2 — the bias-magnitude axis.
#
# WHY THIS AXIS. Stage 1 established two things. Availability CANNOT resolve the
# treatment: the mean per-seed difference is +0.00000 on all four F1 cells with no
# non-zero seeds at all, because both arms obtain a fix on essentially every cycle
# and the metric is saturated before the treatment applies. What DID resolve was
# detection: precision 0.84-0.98 against recall 0.29-0.72. So the information is in
# the detection curve, and the registered rule needs a second magnitude before it can
# return any verdict at all.
#
# POLICY CHOICE, and it is a choice. The registration says "at the best policy from
# stage 1". Detection is a property of the gate rather than of the recovery policy,
# so the four policies differ little on it; `bounded` is taken because it carried the
# best precision/recall balance in both fault models (F1 0.980/0.389, F3 0.974/0.610)
# without `none`s 771 false positives on F3. Recorded here so the selection is not
# silently attributed to the data.
#
# 4 magnitudes x 2 fault models x 20 seeds = 160 runs, two arms, traced.
# 40 m already exists in results/c9_escalate and is NOT re-run; the curve is read
# across both directories.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
[ -x "$BIN" ] || { echo "no optimized binary -- ./ns3 build first"; exit 2; }

R=results/c9_stage2
if [ -e "$R" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"
  mv "$R" "${R}_superseded_$ts" || exit 1
fi

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  echo "TREE DIRTY -- no cell would be citable:"; git status --porcelain --untracked-files=no; exit 1
fi

POL=bounded

job() {
  bias=$1; fa=$2; seed=$3
  # fault label: --injectFaultAnchors 0 is F3 common-mode, 1 is F1 single-anchor.
  if [ "$fa" -eq 0 ]; then fl=F3; else fl=F1; fi
  d="results/c9_stage2/b${bias}_${fl}_seed${seed}"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --duration=3600 --boxHalf=2000 --gwGrid=5 \
    --escalationRequiresEvidence=true --nlosSource=propagation --pcsForm=v2 \
    --mahalUseR=true --injectRangeBias="$bias" --injectBiasFraction=0.2 \
    --injectFaultAnchors="$fa" --suspectRecovery=bounded \
    --suspectForcesEscalation=true \
    --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "b${bias}_${fl} s=${seed} rc=$?"
}
export -f job

{ for bias in 10 20 80 160; do
    for fa in 0 1; do
      for s in $(seq 1 20); do echo "$bias $fa $s"; done
    done
  done; } | xargs -P 11 -n 3 bash -c 'job "$0" "$1" "$2"'

# Completeness: rows, not existence (rule 12). Two arms plus a header is 3 lines.
bad=0
for bias in 10 20 80 160; do
  for fl in F1 F3; do
    n=0
    for s in $(seq 1 20); do
      f="results/c9_stage2/b${bias}_${fl}_seed${s}/arm_summary.csv"
      [ -f "$f" ] && [ "$(grep -c '' "$f")" -eq 3 ] && n=$((n+1))
    done
    printf 'b%-4s %-3s %2d/20 cells with 3 rows\n' "$bias" "$fl" "$n"
    [ "$n" -eq 20 ] || bad=1
  done
done

d0=$(grep -rh 'dirty ' results/c9_stage2/*/run_id.txt 2>/dev/null | grep -c 'dirty 0 ')
echo "cells recording dirty 0: $d0/160"
[ "$d0" -eq 160 ] || bad=1

[ $bad -eq 0 ] || { echo "C9_STAGE2 INCOMPLETE"; exit 1; }
echo "C9_STAGE2 DONE"
