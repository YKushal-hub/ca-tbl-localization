#!/usr/bin/env bash
# G58 runner: per-hop variance and NLOS enter a relayed range's sigma (docs/SPEC_CONFORMANCE_PLAN.md
# NW-1 step 3, --hopVarianceTerms). What each run is for, and what a firing looks like on
# disk, is in scripts/hop_variance_gate.py, which reads them.
#
# usage: bash scripts/hop_variance_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G53's relayed regime (four tags in a 400 m box, relayed ranging chained to depth 2, A2's
# budget at 156 m, both a static 2.4 GHz arm and the full arm, seed 7, 600 s) with the
# anchor-link NLOS classifier on: the terms on, and off (the control); and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g58.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=false --boxHalf=200 --nTags=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336 --nlosClassifier=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on $B --hopVarianceTerms=true &
run off $B &
wait
run r_nochain --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=4 --relayedRanging=true --nlosClassifier=true --hopVarianceTerms=true
run r_noclass --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --hopVarianceTerms=true

python3 scripts/hop_variance_gate.py "$D"
