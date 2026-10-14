#!/usr/bin/env bash
# The secondary sensitivity registered in report/PREREG_IMU_PRIOR.md §8, run after the
# primary campaign: C_sparse, arms static_2p4 and ca_tbl_full, seeds 301-310,
# --imuGyroBiasInitDps in {0.05, 0.5} beside the nominal 0.15 the primary already holds.
# 0.5 dps is the data sheet's uncalibrated zero-rate offset -- the worst case.
#
# No verdict is registered for it; it is reported as a curve. Same refusals as the
# primary runner: a dirty tree, or any existing output directory.

set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${IMU_ROOT:-results/imu_prior_sens}
JOBS=${IMU_JOBS:-11}
COMMON="--duration=3600 --boxHalf=3000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true --arms=static_2p4,ca_tbl_full --imuPrior=true"

[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: $dirty tracked modification(s) under src/ scratch/"; exit 2; }

mkdir -p "$ROOT"
JOBFILE=$(mktemp)
trap 'rm -f "$JOBFILE"' EXIT
for b in 0.05 0.5; do
    for s in $(seq 301 310); do
        d="$ROOT/sparse_imu_b${b}_s${s}"
        [ -e "$d" ] && { echo "REFUSING: $d exists"; exit 2; }
        echo "$d|--seed=$s $COMMON --imuGyroBiasInitDps=$b" >> "$JOBFILE"
    done
done
echo "$(wc -l < "$JOBFILE") invocations into $ROOT"
run_one() {
    d=${1%%|*}
    args=${1#*|}
    mkdir -p "$d"
    # shellcheck disable=SC2086
    if "$BIN" $args --outdir="$d" > "$d.stdout" 2> "$d.stderr"; then echo "ok   $d"; else echo "FAIL $d"; fi
}
export -f run_one
export BIN
xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} < "$JOBFILE"
