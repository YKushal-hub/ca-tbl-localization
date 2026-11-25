#!/usr/bin/env bash
# G172 runner: --rangeHeightCorrect (DD 28) projects mast ranges onto the ground before the 2-D filter reads them.
# off  preset, seed 7, 600 s, static_868, device-side estimation, flag off
# on   the same with --rangeHeightCorrect=true
# Off-byte-identity against the build before the flag was checked by hand when it was added (DESIGN_DECISIONS 28).
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD65: the preset this gate was written against (DESIGN_DECISIONS 65)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g172.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true $PIN_PRE_DD65 --seed=7 --duration=600 --trace=true --arms=static_868 --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false"
# shellcheck disable=SC2086
run off $E &
# shellcheck disable=SC2086
run on $E --rangeHeightCorrect=true &
wait
python3 scripts/height_gate.py "$D"
