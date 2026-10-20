#!/usr/bin/env bash
# G151 runner: --ekf5Iterate, the iterated (IEKF) range update of the 5-state filter (TECH_DEBT N.58).
# What each run is for, and what a firing looks like on disk, is in scripts/iterate_gate.py.
#
# on/base  5 km (the preset), seed 5, unknown start, realistic IMU, ekf5, 600 s, ca_tbl_full, with and
#          without --ekf5Iterate=10: the acquisition N.58 traced (A L0 seed 5 of EKF5-MAIN)
# def/zero the same at 300 s with the flag absent and given 0: must be byte-identical
# r_*      refusals: without ekf5, with --ekf5Robust, and a negative pass count
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g151.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true --seed=5 --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --imuFilter=ekf5 --arms=ca_tbl_full"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=600 --ekf5Iterate=10 &
# shellcheck disable=SC2086
run base $P --duration=600 &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --ekf5Iterate=0 &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5Iterate=5
run r_rob --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Robust=8 --ekf5Iterate=5
run r_neg --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Iterate=-1
python3 scripts/iterate_gate.py "$D"
