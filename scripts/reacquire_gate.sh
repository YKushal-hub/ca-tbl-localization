#!/usr/bin/env bash
# G175 runner: --reacquireSigmaM (DD 34), re-entry into particle acquisition when a filter is lost.
# Quiet shift at the cluster site (3 gateways at the office, open terrain), seed 541, 7200 s, no searchers, ca_tbl_full.
# off  flag off: crew filters lock onto wrong roots and claim tens of metres while kilometres off
# on   --reacquireSigmaM=500
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g175.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=541 --duration=7200 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --searchLaunchOnLoss=0 --nFinders=0 --finderTasking=tag0 --finderSessions=false --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0"
# shellcheck disable=SC2086
run off $E &
# shellcheck disable=SC2086
run on $E --reacquireSigmaM=500 &
wait
python3 scripts/reacquire_gate.py "$D"
