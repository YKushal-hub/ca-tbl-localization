#!/usr/bin/env bash
# G100 runner: --ekf5FinderConsider, finder GNSS offsets as Schmidt consider parameters of the 5-state filter.
# What each run is for, and what a firing looks like on disk, is in scripts/finder_consider_gate.py.
#
# on/base  the endgame, seed 1, unknown start, realistic IMU, ekf5, --finderPosSigma=2, 600 s, ca_tbl_full,
#          --traceRanges, with and without the flag
# def/zero the same at 300 s with the flag absent and given false: must be byte-identical
# r_*      refusals: without ekf5, and with --ekf5Robust
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g100.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true --seed=1 --trace=true --traceRanges=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --imuFilter=ekf5 --finderPosSigma=2 --boxHalf=750 --finderMode=1 --arms=ca_tbl_full"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=600 --ekf5FinderConsider=true &
# shellcheck disable=SC2086
run base $P --duration=600 &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --ekf5FinderConsider=false &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5FinderConsider=true
run r_rob --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Robust=8 --ekf5FinderConsider=true
python3 scripts/finder_consider_gate.py "$D"
