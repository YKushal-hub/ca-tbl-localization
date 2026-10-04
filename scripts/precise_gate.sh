#!/usr/bin/env bash
# G55 runner: LOCATED_PRECISE on a confirmed UWB grant, and the batch's precision spread
# (docs/SPEC_CONFORMANCE_PLAN.md TA-3 + TA-4, --sessionPreciseOnGrant). What each run is
# for, and what a firing looks like on disk, is in scripts/precise_gate.py, which reads them.
#
# usage: bash scripts/precise_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G49's five-subject regime (--grantGatedUwb with --handshakeFixes and arbitration, seed 7,
# 1200 s) with v2 batch queries for 1..5 every 30 s, the reverse path, mesh links, and the
# base station's session (TA-1a): precision on a grant, and the default rule (the
# control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g55.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --sendHandshakeFrames=true --uwbArbitration=true --handshakeFixes=true --grantGatedUwb=true --vizEvents=true --identityQueryPeriodS=30 --queryTierBinding=per-tier --queryV2=true --identityQueryManifests=1,2,3,4,5 --reversePath=true --reversePathHop=true --meshLinks=all --sessionLayer=true --sessionOverMesh=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run grant $B --sessionPreciseOnGrant=true &
run tier $B &
wait
run r_nogate --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --identityQueryPeriodS=30 --queryV2=true --sessionLayer=true --sessionOverMesh=true --sessionPreciseOnGrant=true

python3 scripts/precise_gate.py "$D"
