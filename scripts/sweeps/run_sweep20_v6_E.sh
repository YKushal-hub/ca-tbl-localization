#!/usr/bin/env bash
# Cell E at 20 seeds: the integrity gate measured AS A GATE, not as a trigger.
#
# Part 5 §5.2 established that under a correct NLOS model the confidence model
# escalates 11 times in 1440 cycles, and that all 209 of cell D's other
# escalations are `suspect` firing through suspectForcesEscalation=true. That
# rests on ONE seed, while Claims 1 and 2 got 20 seeds x 3 geometries. This
# closes the asymmetry.
#
# Configuration is cell D's with exactly one flag changed:
#     --suspectForcesEscalation=false
# One geometry, boxHalf=1000, matching D, so the paired comparison against
# results/sweep20_v6_b1000 is at matched seeds and matched geometry.
#
# TRACED, unlike the Phase-6 sweep: the per-seed escalation and suspect
# distributions are the point here, not just the summary means.
#
# The D=E identity for fixed_threshold and ca_tbl_no_integrity is the control
# proving the flag does only what it claims -- neither arm reads `suspect`.
# Checked across all 20 seeds by the analysis, not asserted.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

ROOT=results/sweep20_v6_E_b1000

# Rule 7: never delete a results directory in place. Move it aside.
if [ -e "$ROOT" ]; then
  ts=$(date +%Y%m%d_%H%M%S)
  echo "existing $ROOT -> ${ROOT}_superseded_$ts"
  mv "$ROOT" "${ROOT}_superseded_$ts" || exit 1
fi

job() {
  seed=$1; root=$2
  d="results/$root/seed_$seed"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --boxHalf=1000 --duration=3600 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy \
    --suspectForcesEscalation=false --trace=true --outdir="$d" \
    > "$d/run.log" 2>&1
  echo "$root s=$seed rc=$?"
}
export -f job

for s in $(seq 1 20); do echo "$s sweep20_v6_E_b1000"; done \
  | xargs -P 10 -n 2 bash -c 'job "$0" "$1"'

# -size +0, NOT a bare `ls | wc -l`. The simulator creates arm_summary.csv at
# startup and writes it at the end, so an existence count reports 20/20 while
# runs are still in flight -- which is exactly how an analysis got run against
# two empty summaries during this phase.
n=$(find $ROOT -name arm_summary.csv -size +0 2>/dev/null | wc -l)
t=$(find $ROOT -name 'trace_ca_tbl_full_seed*.csv' -size +0 2>/dev/null | wc -l)
printf '%-24s %2d/20 arm_summary.csv  %2d/20 ca_tbl_full traces\n' "$ROOT" "$n" "$t"
[ "$n" -eq 20 ] && [ "$t" -eq 20 ] || { echo "SWEEP20_V6_E INCOMPLETE"; exit 1; }
echo "SWEEP20_V6_E DONE"
