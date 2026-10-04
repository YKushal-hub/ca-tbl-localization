#!/usr/bin/env bash
# G64 runner: a finder's session drives escalation (TA-1b, --finderRequesterEscalation).
# What each run is for, and what a firing looks like on disk, is in
# scripts/finderreq_gate.py, which reads them.
#
# usage: bash scripts/finderreq_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G57's converging regime (five subjects, four finders converging in a 750 m box, URGENT
# queries every 30 s, grant-gated UWB, arbitration v2, mesh links, sessions over the mesh),
# with the base station's requests and the finders' sessions on: the flag on; the flag off
# (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g64.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --finderTasking=roundrobin --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --arbitrationV2=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5 --identityQueryUrgent=true --reversePath=true --reversePathHop=true --meshLinks=all --sessionLayer=true --sessionOverMesh=true --requesterEscalation=true --finderSessions=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --finderRequesterEscalation=true &
run off $B &
wait
run r_nofs --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --identityQueryPeriodS=30 --queryV2=true --sessionLayer=true --sessionOverMesh=true --sendHandshakeFrames=true --handshakeFixes=true --grantGatedUwb=true --uwbArbitration=true --arbitrationV2=true --requesterEscalation=true --finderRequesterEscalation=true

python3 scripts/finderreq_gate.py "$D"
