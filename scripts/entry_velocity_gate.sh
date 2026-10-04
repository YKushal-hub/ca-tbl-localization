#!/usr/bin/env bash
# G31 runner: table entries carry the producer's velocity (docs/SPEC_CONFORMANCE_PLAN.md
# DC-10). What each run is for, and what a firing looks like on disk, is in
# scripts/entry_velocity_gate.py, which reads them.
#
# usage: bash scripts/entry_velocity_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Two pairs, flag off and on: the searcher regime of null control NC2 at 900 s (finders
# and digests, so entries travel), and the relay regime of NC4 (peer tags publish too).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g31.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
S="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180"
R="--arms=static_2p4 --seed=7 --duration=600 --trace=true --boxHalf=200 --nTags=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run s_off $S
run s_on $S --entryVelocity=true
run r_off $R
run r_on $R --entryVelocity=true

python3 scripts/entry_velocity_gate.py "$D"
