#!/usr/bin/env bash
# G190 (--sigmaShadow, DD 75) runner. Collapse, 5 storeys, 20 workers, seed 731 (development), 1800 s, the DD 67 preset.
# off: default   false: the explicit default   on: --sigmaShadow=true
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g190.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
. scripts/preset_pins.sh  # PIN_PRE_DD77: the preset this gate was written against (DD 67; DESIGN_DECISIONS 77)
E="--specProfile=true $PIN_PRE_DD77 --seed=731 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true --uwbFutilityCycles=4 --collapseBracket=central --collapseNominalHeights=true --collapsePileH=2.286 --collapseDepthMax=2.286 --boxHalf=71.22 --nTags=20 --measuredSubjects=20 --identityQueryManifests=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 --finderTasking=need --nGateways=4"
# shellcheck disable=SC2086
run off $E & run false $E --sigmaShadow=false & run on $E --sigmaShadow=true &
wait
python3 scripts/sigma_shadow_gate.py "$D"
