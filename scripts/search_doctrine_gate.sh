#!/usr/bin/env bash
# G174 runner: the searcher fixes DD 30-32 (+ --finderStaleSearch). Cluster site (3 gateways at the office), seed 501,
# 3800 s, static_2p4, device-side estimation, overdue alarm at 3000 s.
# off  flags off: the 2.4 GHz-only party jumps 3.2 km at dispatch and never steers on the casualty's entry
# on   --finderSweepFromDispatch --finderDigestOnArmTier --finderLkpFirst --finderStaleSearch
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g174.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=501 --duration=3800 --trace=true --arms=static_2p4 --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000"
# shellcheck disable=SC2086
run off $E &
# shellcheck disable=SC2086
run on $E --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true &
wait
python3 scripts/search_doctrine_gate.py "$D"
