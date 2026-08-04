#!/usr/bin/env bash
# 20-seed paired sweep on the v6 candidate configuration.
#
# Modelled on run_sweep20.sh, adding the two flags that define baseline_v6:
#   --nlosSource=propagation   (defect A fixed; see PCS_REDESIGN_RESULTS Part 1)
#   --arbitrateDutyBudget=true (the architecture can signal its own handoffs)
#
# Runs the optimized binary directly, as run_sweep20.sh does. Validated by the
# cache-independence test: seed 7 run after seed 6 in one process is
# byte-identical to seed 7 in a fresh process, so g_fadingComponents and
# g_foliageDynamicCache carry no state across seeds and a per-seed process split
# is the same experiment as one --seedCount=20 run. Not re-derived here.
#
# Three groups, one xargs pool -- 60 jobs, the long pole of the v6 work:
#   sweep20_v6_b1000   boxHalf 1000, gwGrid 3 (default)
#   sweep20_v6_b3000   boxHalf 3000, gwGrid 3 (default)  -- density falls 9x
#   gw9_v6             boxHalf 3000, gwGrid 9            -- CONSTANT-DENSITY control
#
# The third group is why the second is interpretable. THE GRID IS FIXED WHILE THE
# AREA IS NOT (ca_tbl_causal_sim.cc:243), so b3000 at the default gwGrid=3 varies
# gateway density as well as area -- density is gwGrid^2/(2*boxHalf/1000)^2 per
# km^2, so it falls as 1/boxHalf^2. gwGrid = 3*boxHalf/1000 holds it constant
# (:2878), i.e. gwGrid=9 at boxHalf=3000. This is the same confound that made the
# box_* directories void as an at-scale experiment; do not read b1000 -> b3000 as
# a scale result without the gw9 arm beside it.
#
# --trace=false: these feed paired_stats.py, which reads arm_summary.csv only.
# The rho analysis runs off the traced ladder cell D_prop_arb instead.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

# STANDING RULE: uniform 4-field job lines.
job() {
  box=$1; seed=$2; gw=$3; root=$4
  d="results/$root/seed_$seed"; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed="$seed" --boxHalf="$box" --gwGrid="$gw" --duration=3600 \
    --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy \
    --trace=false --outdir="$d" > "$d/run.log" 2>&1
  rc=$?
  echo "$root box=$box gw=$gw s=$seed rc=$rc"
}
export -f job

{ for s in $(seq 1 20); do echo "1000 $s 3 sweep20_v6_b1000"; done
  for s in $(seq 1 20); do echo "3000 $s 3 sweep20_v6_b3000"; done
  for s in $(seq 1 20); do echo "3000 $s 9 gw9_v6";           done; } \
  | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'

# A non-zero rc anywhere means a missing arm_summary.csv downstream, which
# paired_stats.py would silently average around. Surface it here.
bad=0
for root in sweep20_v6_b1000 sweep20_v6_b3000 gw9_v6; do
  # -size +0: the simulator creates arm_summary.csv at startup and writes it at
  # the end, so an existence count reports complete while runs are in flight.
  n=$(find results/$root -name arm_summary.csv -size +0 2>/dev/null | wc -l)
  printf '%-20s %2d/20 arm_summary.csv\n' "$root" "$n"
  [ "$n" -eq 20 ] || bad=1
done
echo "SWEEP20_V6 DONE (incomplete=$bad)"
exit $bad
