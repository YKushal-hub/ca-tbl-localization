#!/usr/bin/env bash
# G97 runner: --imuStrideSigmaPct, per-step stride noise in DC-1 (realistic pedestrian dead reckoning).
# What each run is for, and what a firing looks like on disk, is in scripts/imu_stride_gate.py.
#
# usage: bash scripts/imu_stride_gate.sh [<workdir>]
#
# on       the preset at 5 km, seed 7, 600 s, ca_tbl_full, --imuStrideSigmaPct=5 (its imu_ trace is read)
# def/zero the same at 300 s with the flag absent and given as 0: must be byte-identical
# r_*      refusals: the flag without --imuPredict, and a negative value
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g97.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
P="--specProfile=true $PIN_PRE_DD19 --seed=7 --trace=true --arms=ca_tbl_full"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $P --duration=600 --imuStrideSigmaPct=5 &
run def $P --duration=300 &
run zero $P --duration=300 --imuStrideSigmaPct=0 &
wait
run r_noimu --arms=ca_tbl_full --seed=1 --duration=10 --imuStrideSigmaPct=5
run r_neg --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuStrideSigmaPct=-1

python3 scripts/imu_stride_gate.py "$D"
