#!/usr/bin/env bash
# G101 runner: --ekf5RelayGate, a chi^2 gate on relayed ranges only.
# What each run is for, and what a firing looks like on disk, is in scripts/relay_gate_gate.py.
#
# on/base  the endgame, seed 3, unknown start, realistic IMU, ekf5, --finderPosSigma=2, 300 s, ca_tbl_full, with and
#          without the flag -- the cell where a relay gave z = -25
# def/zero the same at 300 s with the flag absent and given false: must be byte-identical
# r_*      refusals: without ekf5, and with --ekf5Robust
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g101.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true $PIN_DD19_22 --seed=3 --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --imuFilter=ekf5 --finderPosSigma=2 --boxHalf=750 --finderMode=1 --arms=ca_tbl_full --duration=300"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --ekf5RelayGate=true &
# shellcheck disable=SC2086
run base $P &
# shellcheck disable=SC2086
run zero $P --ekf5RelayGate=false &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5RelayGate=true
run r_rob --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Robust=8 --ekf5RelayGate=true
python3 scripts/relay_gate_gate.py "$D"
