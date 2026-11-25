#!/usr/bin/env bash
# G189 (--tdoaHeightCorrect DD 62, --tdoaRubbleDelay DD 63) runner. Collapse, 5 storeys, 20 workers, seed 731 (development), 1800 s.
# off/false without / with the explicit defaults   rd the World delay only   both the World delay and the correction
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g189.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=731 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true --uwbFutilityCycles=4 --collapseBracket=central --collapseNominalHeights=true --collapsePileH=2.286 --collapseDepthMax=2.286 --boxHalf=71.22 --nTags=20 --measuredSubjects=20 --identityQueryManifests=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 --finderTasking=need --nGateways=4 --sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true"
# shellcheck disable=SC2086
run off $E & run false $E --tdoaHeightCorrect=false --tdoaRubbleDelay=false &
# shellcheck disable=SC2086
run rd $E --tdoaRubbleDelay=true & run both $E --tdoaRubbleDelay=true --tdoaHeightCorrect=true &
# n0/n1 the same without rubble delay and with almost noiseless stamps (no PPS error, the arrival bound x 0.01), correction off / on
# shellcheck disable=SC2086
run n0 $E --tdoaPpsSigmaNs=0 --tdoaToaScale=0.01 & run n1 $E --tdoaPpsSigmaNs=0 --tdoaToaScale=0.01 --tdoaHeightCorrect=true &
wait
python3 scripts/tdoa_height_gate.py "$D"
