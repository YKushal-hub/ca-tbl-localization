#!/usr/bin/env bash
# G188 (--sharedDuty868Finders, --dutyReport868Finders, DD 60) runner. ROAD3, four searchers, seed 731 (development), 7200 s.
# off/false without / with the explicit defaults   report counts only   duty enforces
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g188.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=731 --duration=7200 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000 --incidentRandom=true --acquireSideOfRoad=true --nFinders=4 --sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true"
# shellcheck disable=SC2086
run off $E & run false $E --sharedDuty868Finders=false --dutyReport868Finders=false &
# shellcheck disable=SC2086
run report $E --dutyReport868Finders=true & run duty $E --sharedDuty868Finders=true &
wait
python3 scripts/finder_duty_gate.py "$D"
