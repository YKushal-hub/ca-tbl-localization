#!/usr/bin/env bash
# G181 runner: DD 46 --uwbFutilityCycles, DD 47 --collapseDepthSolve, DD 48 --finderFrameSolve.
#   g_off / g_on   one buried worker (PIN_ONE_WORKER), 8 storeys, central, 600 s: the gate must cut its UWB energy
#   d              20 workers, 5 storeys, 1800 s, --collapseDepthSolve: files written, resolved depths sane
#   f_off / f_on   --finderFrameSolve: published finder positions change, every arm still runs
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g181.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true $PIN_PRE_DD65 --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --nGateways=4 --searchLaunchAtS=1 --relaySlots=false --arms=ca_tbl_full --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseBracket=central"
S8="--collapsePileH=3.658 --collapseDepthMax=3.658 --boxHalf=94.08"
S5="--collapsePileH=2.286 --collapseDepthMax=2.286 --boxHalf=71.22"
N20="--nTags=20 --measuredSubjects=20 --identityQueryManifests=$(seq -s, 1 20) --finderTasking=need"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run g_off $B $S8 $PIN_ONE_WORKER --duration=600 --seed=901 &
# shellcheck disable=SC2086
run g_on $B $S8 $PIN_ONE_WORKER --duration=600 --seed=901 --uwbFutilityCycles=4 &
# shellcheck disable=SC2086
run d $B $S5 $N20 --duration=1800 --seed=902 --collapseDepthSolve=true &
# shellcheck disable=SC2086
run f_off $B $S8 $N20 --duration=300 --seed=903 &
# shellcheck disable=SC2086
run f_on $B $S8 $N20 --duration=300 --seed=903 --finderFrameSolve=true &
wait
python3 scripts/collapse_mech_gate.py "$D"
