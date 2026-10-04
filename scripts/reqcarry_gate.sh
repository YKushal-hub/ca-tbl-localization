#!/usr/bin/env bash
# G81 runner: a requester's escalation request travels as a frame (N.10, --requestOverMesh).
# What each run is for, and what a firing looks like on disk, is in
# scripts/reqcarry_gate.py, which reads them.
#
# usage: bash scripts/reqcarry_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G64's regime (G57's converging finders, five subjects, the base station's requests and the
# finders' sessions and requests): the flag on; the flag on with --halfDuplex=2 (N.6 enforced,
# so an arbiter's ACK is not destroyed by its own GRANT); the flag off (the control); and a
# refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g81.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --finderTasking=roundrobin --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --arbitrationV2=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5 --identityQueryUrgent=true --reversePath=true --reversePathHop=true --meshLinks=all --sessionLayer=true --sessionOverMesh=true --requesterEscalation=true --finderSessions=true --finderRequesterEscalation=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --requestOverMesh=true &
run on_hd $B --requestOverMesh=true --halfDuplex=2 &
run off $B &
wait
run r_noreq --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --requestOverMesh=true

python3 scripts/reqcarry_gate.py "$D"
