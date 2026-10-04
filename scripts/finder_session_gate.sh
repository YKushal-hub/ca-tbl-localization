#!/usr/bin/env bash
# G56 runner: every finder runs its own search (docs/SPEC_CONFORMANCE_PLAN.md TA-1b,
# --finderSessions). What each run is for, and what a firing looks like on disk, is in
# scripts/finder_session_gate.py, which reads them.
#
# usage: bash scripts/finder_session_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G54's regime (G50's mesh, reverse path, five subjects, a 1..5 v2 batch, the base
# station's session, seed 7, 900 s) with finders tasked round-robin: finder sessions on,
# and off (the control); finder sessions in G47's converging regime with a 30 m cancel
# distance, so the cancel -- which the 8 m stand-off keeps out of 2 m, and which finders
# sweeping a 2 km box never approach -- is exercised; and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g56.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true --reversePathHop=true --queryTierBinding=per-tier --meshLinks=all --vizEvents=true --nTags=5 --measuredSubjects=5 --queryV2=true --identityQueryManifests=1,2,3,4,5 --sessionLayer=true --sessionOverMesh=true --finderTasking=roundrobin"
# The cancel needs finders that reach their targets: G47's converging regime (pseudo-anchors,
# digests every 20 s, mode 1, 750 m box), two subjects, finders tasked round-robin.
C="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --measuredSubjects=2 --finderTasking=roundrobin --identityQueryPeriodS=30 --queryV2=true --identityQueryManifests=1,2 --reversePath=true --reversePathHop=true --vizEvents=true --sessionLayer=true --sessionOverMesh=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run fs $B --finderSessions=true &
run nofs $B &
run fsc $C --finderSessions=true --finderCancelM=30 &
wait
run r_nobase --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nFinders=2 --finderSessions=true

python3 scripts/finder_session_gate.py "$D"
