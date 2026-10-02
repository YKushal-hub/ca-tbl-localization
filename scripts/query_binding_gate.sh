#!/usr/bin/env bash
# G32 runner: each tier's identity-query driver sends on that tier's radio
# (docs/SPEC_CONFORMANCE_PLAN.md NW-3). What each run is for, and what a firing looks like
# on disk, is in scripts/query_binding_gate.py, which reads them.
#
# usage: bash scripts/query_binding_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Three 900 s runs in null control NC5's regime (finders, digests, identity queries every
# 30 s with the proxy and the reverse path) -- the default, --queryTierBinding=first and
# per-tier -- and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g32.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B
run first $B --queryTierBinding=first
run per $B --queryTierBinding=per-tier
run r_noq --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --queryTierBinding=per-tier
run r_bad $B --duration=30 --queryTierBinding=each

python3 scripts/query_binding_gate.py "$D"
