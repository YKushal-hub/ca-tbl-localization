#!/usr/bin/env bash
# G51 runner: query-triggered escalation and arbitration as specified
# (docs/SPEC_CONFORMANCE_PLAN.md NW-4 + NW-5, --queryTriggeredEscalation --arbitrationV2).
# What each run is for, and what a firing looks like on disk, is in
# scripts/arbitration_v2_gate.py, which reads them.
#
# usage: bash scripts/arbitration_v2_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G46/G49's five-subject regime (four finders converging, frames, --handshakeFixes,
# --grantGatedUwb, seed 7, 1200 s -- 600 s queues nothing) with v2 queries for all five every 30 s, sent URGENT, and
# the event log: both flags on; the C4 arbiter with ROUTINE queries (the control); and three
# refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g51.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --sendHandshakeFrames=true --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run v2 $B --identityQueryUrgent=true --queryTriggeredEscalation=true --arbitrationV2=true &
run c4 $B &
wait
S="--arms=ca_tbl_full --seed=1 --duration=30 --trace=false"
run r_qte $S --identityQueryPeriodS=30 --queryTriggeredEscalation=true
run r_urg $S --identityQueryPeriodS=30 --identityQueryUrgent=true
run r_arb $S --sendHandshakeFrames=true --uwbArbitration=true --arbitrationV2=true

python3 scripts/arbitration_v2_gate.py "$D"
