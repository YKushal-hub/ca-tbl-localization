#!/usr/bin/env bash
# G185 (--sharedDuty868, DD 56) and G186 (--peerSplitCi, DD 57) runner. Cluster site, seed 722 (development), 7200 s.
# base  --sparseRangeGeometry --dutyReport868 (counts true airtime, enforces nothing)   off/false  without / with the explicit defaults
# duty  + --sharedDuty868     sci  + --peerSplitCi     fatal  --peerSplitCi without the ekf5 filter
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD65: the preset this gate was written against (DESIGN_DECISIONS 65)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g185.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true $PIN_PRE_DD65 --seed=722 --duration=7200 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000 --incidentRandom=true --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0 --sparseRangeGeometry=true"
# shellcheck disable=SC2086
run off $E & run false $E --sharedDuty868=false --peerSplitCi=false --dutyReport868=false --traceSubjectRanges=false &
# shellcheck disable=SC2086
run base $E --dutyReport868=true & run duty $E --sharedDuty868=true & run sci $E --peerSplitCi=true &
run fatal --specProfile=true $PIN_PRE_DD65 --imuFilter=cv --imuMagSigmaDeg=0 --ekf5FinderConsider=false --ekf5RelayGate=false --ekf5Window=0 --peerSplitCi=true --duration=10 &
wait
python3 scripts/duty_sci_gate.py "$D"
