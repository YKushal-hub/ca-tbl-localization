#!/usr/bin/env bash
# G52 runner: anchors gossip their queue as well as their grants
# (docs/SPEC_CONFORMANCE_PLAN.md NW-6, --gossipQueue). What each run is for, and what a
# firing looks like on disk, is in scripts/queue_gossip_gate.py, which reads them.
#
# usage: bash scripts/queue_gossip_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G51's regime (five subjects, URGENT v2 queries, --arbitrationV2, --grantGatedUwb) at seed
# 1 and 1800 s, with grant gossip every 5 s over --meshLinks=all and the event log: the
# queue gossiped, and grants only (the control); and a refusal. Two arbiters holding the
# same request is rare with five scattered subjects -- in 14 probe runs (seeds, slot holds,
# finder counts, converging finders) this one alone yielded -- so the yield's reachability
# is also shown in the packet validator ([12]).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g52.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=1 --duration=1800 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --sendHandshakeFrames=true --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5 --identityQueryUrgent=true --queryTriggeredEscalation=true --arbitrationV2=true --anchorGrantGossipS=5 --meshLinks=all"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run q $B --gossipQueue=true &
run nq $B &
wait
run r_noarb --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --sendHandshakeFrames=true --anchorGrantGossipS=5 --gossipQueue=true

python3 scripts/queue_gossip_gate.py "$D"
