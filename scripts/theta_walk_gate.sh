#!/usr/bin/env bash
# G187 (--ekf5WindowThetaWalk, DD 59) runner: 8 h quiet shift, cluster site, seed 732 (development).
# off/false  without / with the explicit default (3600 s)   base/walk  the 8 h shift without / with the switch
# r_cv  refused without the smoother
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g187.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
Q="--specProfile=true --seed=732 --trace=true --arms=ca_tbl_full --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --searchLaunchOnLoss=0 --nFinders=0 --finderTasking=tag0 --finderSessions=false --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0 --sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true"
# shellcheck disable=SC2086
run off $Q --duration=3600 & run false $Q --duration=3600 --ekf5WindowThetaWalk=false &
# shellcheck disable=SC2086
run base $Q --duration=28800 & run walk $Q --duration=28800 --ekf5WindowThetaWalk=true &
run r_cv --specProfile=true --ekf5Window=0 --ekf5WindowThetaWalk=true --duration=10 &
wait
python3 scripts/theta_walk_gate.py "$D"
