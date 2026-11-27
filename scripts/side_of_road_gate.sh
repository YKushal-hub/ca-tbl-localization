#!/usr/bin/env bash
# G173 runner: --acquireSideOfRoad (DD 29) keeps the acquisition and the estimate on the work area's side of the gateway line.
# off   preset (3 gateways on the ridge at y = 2750 m), seed 501, 1800 s, no searchers, static_2p4, device-side, flag off
# on    the same with --acquireSideOfRoad=true
# nor   --acquireSideOfRoad=true with --gwRidge=false: must be refused (FATAL, rc != 0)
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD65: the preset this gate was written against (DESIGN_DECISIONS 65)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g173.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true $PIN_PRE_DD65 --seed=501 --duration=1800 --trace=true --arms=static_2p4 --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --nFinders=0 --finderTasking=tag0 --finderSessions=false"
# shellcheck disable=SC2086
run off $E &
# shellcheck disable=SC2086
run on $E --acquireSideOfRoad=true &
# shellcheck disable=SC2086
run nor $E --acquireSideOfRoad=true --gwRidge=false --duration=10 &
wait
python3 scripts/side_of_road_gate.py "$D"
