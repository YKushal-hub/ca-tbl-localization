#!/usr/bin/env bash
# G50 runner: QUERY and RESPONSE carry what the spec says (docs/SPEC_CONFORMANCE_PLAN.md NW-9,
# --queryV2). What each run is for, and what a firing looks like on disk, is in
# scripts/query_v2_gate.py, which reads them.
#
# usage: bash scripts/query_v2_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G32's identity-query regime under --meshLinks=all with the event log and the reverse path
# (G48), five tags all measured, seed 7, 900 s: --queryV2 with a five-manifest batch query
# (every subject), and the same without --queryV2; and three refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g50.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true --reversePathHop=true --queryTierBinding=per-tier --meshLinks=all --vizEvents=true --nTags=5 --measuredSubjects=5"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run v2 $B --queryV2=true --identityQueryManifests=1,2,3,4,5 &
run v1 $B &
wait
S="--arms=ca_tbl_full --seed=1 --duration=30 --trace=false"
run r_noq $S --queryV2=true
run r_list $S --identityQueryPeriodS=30 --identityQueryManifests=1,2
run r_bad $S --identityQueryPeriodS=30 --queryV2=true --identityQueryManifests=1,0

python3 scripts/query_v2_gate.py "$D"
