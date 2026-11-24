#!/usr/bin/env bash
# G184 runner: --sparseRangeGeometry (DD 54), a two-range fix keeps its geometry.
# Emergency at the cluster site (3 gateways at the office), seed 701 (development), 3600 s, ca_tbl_full.
# off   the flag absent           false  the explicit default           on  the flag
# fatal the flag without --sparseInStep, must refuse
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g184.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=701 --duration=3600 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000 --incidentRandom=true --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0"
# shellcheck disable=SC2086
run off $E &
# shellcheck disable=SC2086
run false $E --sparseRangeGeometry=false &
# shellcheck disable=SC2086
run on $E --sparseRangeGeometry=true &
run fatal --seed=1 --duration=10 --sparseRangeGeometry=true &
wait
python3 scripts/sparse_geometry_gate.py "$D"
