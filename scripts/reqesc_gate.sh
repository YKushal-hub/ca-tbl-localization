#!/usr/bin/env bash
# G57 runner: the requester drives escalation (docs/SPEC_CONFORMANCE_PLAN.md TA-2,
# --requesterEscalation). What each run is for, and what a firing looks like on disk, is in
# scripts/reqesc_gate.py, which reads them.
#
# usage: bash scripts/reqesc_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G47's converging finders (pseudo-anchors, digests every 20 s, tasked round-robin) with five
# subjects, --grantGatedUwb and --arbitrationV2, v2 batch queries sent URGENT, mesh links, the
# reverse path and the base station's session, seed 7, 1200 s -- finders near their targets
# are what give a target an arbiter in reach to name: the requester's escalation on, and off
# (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g57.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --finderTasking=roundrobin --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --arbitrationV2=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5 --identityQueryUrgent=true --reversePath=true --reversePathHop=true --meshLinks=all --sessionLayer=true --sessionOverMesh=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run req $B --requesterEscalation=true &
run noreq $B &
wait
run r_noarb --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --identityQueryPeriodS=30 --queryV2=true --sessionLayer=true --sessionOverMesh=true --sendHandshakeFrames=true --handshakeFixes=true --grantGatedUwb=true --requesterEscalation=true

python3 scripts/reqesc_gate.py "$D"
