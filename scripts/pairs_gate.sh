#!/usr/bin/env bash
# G160 runner: --finderTasking=pairs (TECH_DEBT N.62), two finders per subject.
# What each run is for, and what a firing looks like on disk, is in scripts/pairs_gate.py.
#
# on/base  the 5 km preset, seed 5, 600 s, ca_tbl_full, the realistic stack, pairs and roundrobin
# def/zero the preset at 300 s with the flag absent and given roundrobin: must be byte-identical
# r_*      refusals: an unknown tasking, and pairs with no finders
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g160.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true $PIN_DD19_22 --seed=5 --arms=ca_tbl_full --trace=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=600 --finderTasking=pairs &
# shellcheck disable=SC2086
run base $P --duration=600 --finderTasking=roundrobin &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --finderTasking=roundrobin &
wait
run r_name --specProfile=true $PIN_DD19_22 --arms=ca_tbl_full --seed=1 --duration=10 --finderTasking=triples
run r_none --specProfile=true $PIN_DD19_22 --arms=ca_tbl_full --seed=1 --duration=10 --finderTasking=pairs --nFinders=0
python3 scripts/pairs_gate.py "$D"
