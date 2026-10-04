#!/usr/bin/env bash
# G45 runner: real mesh links (docs/SPEC_CONFORMANCE_PLAN.md NW-0, --meshLinks=all). What each
# run is for, and what a firing looks like on disk, is in scripts/mesh_links_gate.py.
#
# usage: bash scripts/mesh_links_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# NC9's regime (four finders converging on the walker, 1.5 km box) with two peer tags,
# handshake frames and identity queries every 30 s (QUERY is the flooded type left once
# DISCOVERY is one-hop, so it is what the node relays; bound per tier, so a frame's tier
# column is the radio it was on -- by default every query rides gateway 101's 868 MHz MAC
# labelled with another tier, NW-3), ca_tbl_full, seed 7, 600 s, with
# --vizEvents: the default, --meshLinks=star and --meshLinks=all. Plus one refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g45.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true --vizEvents=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=3 --sendHandshakeFrames=true --finderDigestPeriodS=20 --identityQueryPeriodS=30 --identityQueryTarget=1 --queryTierBinding=per-tier"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run star $B --meshLinks=star &
run all $B --meshLinks=all &
wait
run r_bad --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --meshLinks=ring

python3 scripts/mesh_links_gate.py "$D"
