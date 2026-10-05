#!/usr/bin/env bash
# G48 runner: the reverse path really exists (docs/SPEC_CONFORMANCE_PLAN.md NW-8,
# --reversePathHop). What each run is for, and what a firing looks like on disk, is in
# scripts/reverse_path_gate.py, which reads them.
#
# usage: bash scripts/reverse_path_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G32's identity-query regime (five gateways on an arc, four finders, queries for manifest
# 1 every 30 s from every gateway radio, proxies on) with --meshLinks=all, so a query can
# travel more than one hop, and the per-frame event log: --reversePathHop on, and
# --reversePath alone (the pre-NW-8 behaviour); and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g48.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryProxy=true --reversePath=true --queryTierBinding=per-tier --meshLinks=all --vizEvents=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run hop $B --reversePathHop=true &
run legacy $B &
wait
run r_norp --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --identityQueryPeriodS=30 --reversePathHop=true
run r_noq --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --reversePath=true --reversePathHop=true

python3 scripts/reverse_path_gate.py "$D"
