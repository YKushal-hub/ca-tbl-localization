#!/usr/bin/env bash
# G102 runner: --ekf5StateBounds, estimate projection of the gyro bias and stride scale onto +/- k prior sigmas.
# What each run is for, and what a firing looks like on disk, is in scripts/state_bounds_gate.py.
#
# on/base  5 km, seed 12, unknown start, the realistic world (0.15 deg/s, 5 % stride, 2 m finder GNSS), ekf5,
#          magnetometer, finder consider, relay gate, 1800 s, ca_tbl_full, with and without --ekf5StateBounds=4 --
#          REAL-STACK's cell whose scale ran to -0.34
# def/zero the same at 300 s with the flag absent and given as 0: must be byte-identical
# r_*      refusals: without ekf5, and a negative value
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g102.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
# PIN (DESIGN_DECISIONS 18, 2026-10-20): this gate's thresholds were set before the preset adopted the receiver
# corrections; the two flags restore the receiver it was written against, as G63/G87/G89 pin theirs.
P="--specProfile=true $PIN_DD19_22 --seed=12 --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true --reportAirtime=true --arms=ca_tbl_full --uwbSensitivityDbm=-104 --iqOverloadOnly=false"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=1800 --ekf5StateBounds=4 &
# shellcheck disable=SC2086
run base $P --duration=1800 &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --ekf5StateBounds=0 &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5StateBounds=4
run r_neg --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5StateBounds=-1
python3 scripts/state_bounds_gate.py "$D"
