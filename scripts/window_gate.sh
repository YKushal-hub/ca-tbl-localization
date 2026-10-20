#!/usr/bin/env bash
# G152 runner: --ekf5Window, the sliding-window smoother of the 5-state filter (TECH_DEBT N.62).
# What each run is for, and what a firing looks like on disk, is in scripts/window_gate.py.
#
# on/base  5 km (the preset), seed 5, unknown start, realistic IMU and magnetometer, ekf5, 600 s, ca_tbl_full, with and
#          without --ekf5Window=60 (the realistic stack): the cell N.62 diagnosed, where the EKF ends 400 m off
# def/zero the same at 300 s with the flag absent and given 0: must be byte-identical
# r_*      refusals: without ekf5, with --ekf5Robust, and a window shorter than 2
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g152.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true --seed=5 --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true --arms=ca_tbl_full"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=600 --ekf5Window=60 &
# shellcheck disable=SC2086
run base $P --duration=600 &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --ekf5Window=0 &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5Window=60
run r_rob --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Robust=8 --ekf5Window=60
run r_neg --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Window=1
python3 scripts/window_gate.py "$D"
