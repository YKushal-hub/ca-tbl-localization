#!/usr/bin/env bash
# §0.5 Pareto sweep: both decision rules over the SAME grid in metres of position
# sigma, compared as fronts in (median error, joules).
#
# The run list is pareto_sweep.py's own --plan output, with one addition:
# --arms. Each point needs exactly one arm --
#     ft_*  -> fixed_threshold
#     v2_*  -> ca_tbl_no_integrity
# because that is what pareto_sweep.py collect() reads for each family. It picks
# ca_tbl_no_integrity for the v2 family deliberately, "so the integrity gate's
# escalation side effect is not confounded into the comparison" -- do not
# substitute ca_tbl_full here.
#
# --arms is sound: verified byte-identical (summary row AND full 1441-line trace)
# against baseline_v6 for ca_tbl_full (arm 6 of 6, maximum preceding RNG stream
# state) and fixed_threshold (arm 4 of 6). RunArm() resets the stream index per
# arm. Without the filter this sweep is ~7 h; with it, ~1.3 h.
#
# Grid: SIGMA_GRID_M = 1.0 1.78 3.162 5.62 10.0 17.8 31.6 56.2 100.0
#   ft  sweeps --fixedVarThreshold = sigma^2
#   v2  sweeps --pcsSigmaTargetM   = sigma
# fixedVarThresholdM2=100 (sigma=10 m) and v2's derived 3.162 m are two points on
# this grid, which is the whole reason the grid exists: comparing those two alone
# is unfair in whichever direction the constants happen to fall.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

OUT=results/pareto
if [ -e "$OUT" ]; then
  ts=$(date +%Y%m%d_%H%M%S); echo "existing $OUT -> ${OUT}_superseded_$ts"; mv "$OUT" "${OUT}_superseded_$ts" || exit 1
fi
mkdir -p "$OUT"

job() {
  fam=$1; sig=$2; val=$3; arm=$4
  d="results/pareto/${fam}_sigma${sig}"; mkdir -p "$d"
  if [ "$fam" = "ft" ]; then
    flag="--pcsForm=legacy --fixedVarThreshold=$val"
  else
    flag="--pcsForm=v2 --pcsSigmaTargetM=$val"
  fi
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --nlosSource=propagation --arbitrateDutyBudget=true $flag \
    --seedStart=1 --seedCount=20 --duration=3600 --trace=false \
    --arms="$arm" --outdir="$d" > "$d/run.log" 2>&1
  echo "${fam}_sigma${sig} arm=$arm rc=$?"
}
export -f job

# sigma label, and the parameter value each family needs for that sigma.
# ft takes a VARIANCE (sigma^2); v2 takes a sigma in metres.
{
  for s in 1 1.78 3.162 5.62 10 17.8 31.6 56.2 100; do
    v=$(python3 -c "print(f'{$s*$s:.4f}')")
    echo "ft $s $v fixed_threshold"
  done
  for s in 1 1.78 3.162 5.62 10 17.8 31.6 56.2 100; do
    echo "v2 $s $s ca_tbl_no_integrity"
  done
} | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'

n=$(find "$OUT" -name arm_summary.csv -size +0 2>/dev/null | wc -l)
printf '%s %2d/18 non-empty summaries\n' "$OUT" "$n"
[ "$n" -eq 18 ] || { echo "PARETO INCOMPLETE"; exit 1; }
echo "PARETO DONE"
