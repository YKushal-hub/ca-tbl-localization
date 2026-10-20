#!/usr/bin/env bash
# G162 runner: --traceNegInfo (N.57 / N.51 measurement). Checks in scripts/neginfo_gate.py.
# base/on: the endgame preset, seed 205 (the seed whose subjects lock onto wrong roots), 600 s, ca_tbl_full, the
# realistic stack, without and with the flag.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g162.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
P="--specProfile=true --seed=205 --duration=600 --trace=true --arms=ca_tbl_full --boxHalf=750 --finderMode=1 --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true --ekf5Window=60"
# shellcheck disable=SC2086
run base $P &
# shellcheck disable=SC2086
run on $P --traceNegInfo=true &
wait
python3 scripts/neginfo_gate.py "$D"
