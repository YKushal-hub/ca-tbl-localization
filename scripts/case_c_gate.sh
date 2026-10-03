#!/usr/bin/env bash
# G60 runner: Case C's relative shape at the base station (docs/SPEC_CONFORMANCE_PLAN.md DC-3,
# --caseCRelative). What each run is for, and what a firing looks like on disk, is in
# scripts/case_c_gate.py, which reads them.
#
# usage: bash scripts/case_c_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G54's regime (five subjects in a 4 km box, five gateways on a 140-degree arc, finders,
# mesh links, the reverse path, the base station's batch queries every 30 s, seed 7, 900 s):
# subjects routinely lose every anchor there while still hearing each other. Case C on, and
# off (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g60.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true --reversePathHop=true --queryTierBinding=per-tier --meshLinks=all --vizEvents=true --nTags=5 --measuredSubjects=5 --queryV2=true --identityQueryManifests=1,2,3,4,5 --sessionLayer=true --sessionOverMesh=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on $B --caseCRelative=true &
run off $B &
wait
run r_nosess --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --meshLinks=all --caseCRelative=true

python3 scripts/case_c_gate.py "$D"
