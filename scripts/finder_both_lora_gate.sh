#!/usr/bin/env bash
# G192 (--finderDigestBothLora, DD 78) runner. Open site, three gateways at the office (CLUS3), two searchers launched at
# 600 s, 2400 s, seed 779 (development), ours, the flat threshold and both single LoRa radios.
# off: default   false: the explicit default   on: --finderDigestBothLora=true
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g192.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=779 --trace=true --arms=ca_tbl_full,static_2p4,static_868,fixed_threshold --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --duration=2400 --trailDeviateAtS=200 --trailStopAfterS=200 --searchLaunchOnLoss=0 --searchLaunchAtS=600 --incidentRandom=true --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0 --sharedDuty868Finders=true"
# shellcheck disable=SC2086
run off $E & run false $E --finderDigestBothLora=false & run on $E --finderDigestBothLora=true &
wait
python3 scripts/finder_both_lora_gate.py "$D"
