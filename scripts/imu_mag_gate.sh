#!/usr/bin/env bash
# G99 runner: --imuMagSigmaDeg, a magnetometer heading reference fused by ekf5.
# What each run is for, and what a firing looks like on disk, is in scripts/imu_mag_gate.py.
#
# on/base  the preset at 5 km, seed 5, unknown start, 0.15 deg/s and 5 % stride noise, ekf5, 1800 s, ca_tbl_full,
#          with and without --imuMagSigmaDeg=3 -- REAL-IMU's cell whose gyro-only heading ended 1.87 rad wrong
# def/zero the preset with ekf5, 300 s, the flag absent and given as 0: must be byte-identical
# r_*      refusals: without ekf5, and a negative value
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g99.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
P="--specProfile=true --seed=5 --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --imuFilter=ekf5 --arms=ca_tbl_full"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $P --duration=1800 --imuMagSigmaDeg=3 &
run base $P --duration=1800 &
run def $P --duration=300 &
run zero $P --duration=300 --imuMagSigmaDeg=0 &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuMagSigmaDeg=3
run r_neg --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --imuMagSigmaDeg=-2

python3 scripts/imu_mag_gate.py "$D"
