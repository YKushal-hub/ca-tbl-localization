#!/usr/bin/env bash
# G54 runner: the base station's session is fed by delivered RESPONSEs
# (docs/SPEC_CONFORMANCE_PLAN.md TA-1a, --sessionOverMesh). What each run is for, and what
# a firing looks like on disk, is in scripts/session_gate.py, which reads them.
#
# usage: bash scripts/session_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G50's regime (G32's identity queries under mesh links and the reverse path, five subjects,
# a five-manifest v2 batch, seed 7, 900 s) with --sessionLayer: the session over the mesh,
# and the legacy policy feed (the control); and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g54.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true --reversePathHop=true --queryTierBinding=per-tier --meshLinks=all --vizEvents=true --nTags=5 --measuredSubjects=5 --queryV2=true --identityQueryManifests=1,2,3,4,5 --sessionLayer=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run mesh $B --sessionOverMesh=true &
run legacy $B &
wait
S="--arms=ca_tbl_full --seed=1 --duration=30 --trace=false"
run r_nosess $S --identityQueryPeriodS=30 --queryV2=true --sessionOverMesh=true
run r_stale $S --sessionStalenessS=60

python3 scripts/session_gate.py "$D"
