#!/usr/bin/env bash
# v2 with the integrity gate as a GATE, not as an escalation trigger.
#
# Every other v2 run has suspectForcesEscalation=true, so ca_tbl_full's
# escalations are partly suspect-forced -- the same confound cell E was created
# to remove under the legacy form. Without this cell, v2's
# ca_tbl_full vs ca_tbl_no_integrity comparison is not an integrity ablation.
#
# Three arms, not six: ca_tbl_full is the arm under test; fixed_threshold and
# ca_tbl_no_integrity are the CONTROLS -- neither reads `suspect`, so both must
# come out byte-identical to the corresponding sweep20_v7_v2_b1000 rows on all
# 20 seeds. If that identity fails, stop and diagnose before interpreting
# anything else. --arms equivalence was verified byte-identically at per-cycle
# resolution including arm 6 of 6.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

ROOT=results/sweep20_v7_v2_E_b1000
if [ -e "$ROOT" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $ROOT -> ${ROOT}_superseded_$ts"; mv "$ROOT" "${ROOT}_superseded_$ts" || exit 1
fi

job() {
  seed=$1
  d="results/sweep20_v7_v2_E_b1000/seed_$seed"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --boxHalf=1000 --duration=3600 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=v2 \
    --suspectForcesEscalation=false --trace=true \
    --arms=fixed_threshold,ca_tbl_no_integrity,ca_tbl_full \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "v2_E s=$seed rc=$?"
}
export -f job

for s in $(seq 1 20); do echo "$s"; done | xargs -P 10 -n 1 bash -c 'job "$0"'

n=$(find "$ROOT" -name arm_summary.csv -size +0 2>/dev/null | wc -l)
t=$(find "$ROOT" -name 'trace_ca_tbl_full_seed*.csv' -size +0 2>/dev/null | wc -l)
printf '%-30s %2d/20 summaries  %2d/20 traces\n' "$ROOT" "$n" "$t"
[ "$n" -eq 20 ] && [ "$t" -eq 20 ] || { echo "V2_E INCOMPLETE"; exit 1; }
echo "V2_E DONE"
