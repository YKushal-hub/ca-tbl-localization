#!/usr/bin/env bash
# PCS v2, 20 seeds, two geometries, TRACED.
#
# The pre-registration is report/PCS_REDESIGN_RESULTS.md Part 0. Nothing about the
# form, the thresholds or the estimator is set here -- --pcsForm=v2 makes
# RunArm() call CaTblPolicyConfig::ApplyV2Thresholds() (ca_tbl_causal_sim.cc:1864),
# which is where the pre-registered 0.0 / log10(2) live. Do not set thresholds at a
# call site; that is how a pre-registration drifts.
#
# TRACED because analyse_pcs.py needs per-cycle data and Part 0 §0.6 requires the
# 20-seed pooled population with a seed-level bootstrap. Phase 6 correctly declined
# to trace the legacy sweep; this is the run that was deferred to.
#
# ~2.2 MB per traced seed, so 40 seeds is ~90 MB. Disk is not a constraint.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

# Rule 7: never delete a results directory in place.
for b in 1000 3000; do
  R=results/sweep20_v7_v2_b$b
  if [ -e "$R" ]; then
    ts=$(date +%Y%m%d_%H%M%S); echo "existing $R -> ${R}_superseded_$ts"; mv "$R" "${R}_superseded_$ts" || exit 1
  fi
done

job() {
  box=$1; seed=$2; root=$3
  d="results/$root/seed_$seed"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --boxHalf="$box" --duration=3600 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=v2 \
    --trace=true --outdir="$d" > "$d/run.log" 2>&1
  echo "$root box=$box s=$seed rc=$?"
}
export -f job

{ for s in $(seq 1 20); do echo "1000 $s sweep20_v7_v2_b1000"; done
  for s in $(seq 1 20); do echo "3000 $s sweep20_v7_v2_b3000"; done; } \
  | xargs -P 10 -n 3 bash -c 'job "$0" "$1" "$2"'

# Completeness gate: content, not existence. arm_summary.csv is created at run
# start and written at run end, so `ls | wc -l` reports complete while runs are
# still in flight. Analysis must chain AFTER this exits zero.
bad=0
for b in 1000 3000; do
  R=results/sweep20_v7_v2_b$b
  n=$(find "$R" -name arm_summary.csv -size +0 2>/dev/null | wc -l)
  t=$(find "$R" -name 'trace_ca_tbl_full_seed*.csv' -size +0 2>/dev/null | wc -l)
  printf '%-28s %2d/20 summaries  %2d/20 ca_tbl_full traces\n' "$R" "$n" "$t"
  [ "$n" -eq 20 ] && [ "$t" -eq 20 ] || bad=1
done
[ $bad -eq 0 ] || { echo "SWEEP20_V7_V2 INCOMPLETE"; exit 1; }
echo "SWEEP20_V7_V2 DONE"
