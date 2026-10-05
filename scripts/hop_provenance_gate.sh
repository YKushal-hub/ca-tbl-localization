#!/usr/bin/env bash
# G53 runner: hop provenance reaches the decision and the RESULT
# (docs/SPEC_CONFORMANCE_PLAN.md NW-1, --hopProvenance). What each run is for, and what a
# firing looks like on disk, is in scripts/hop_provenance_gate.py, which reads them.
#
# usage: bash scripts/hop_provenance_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G17's relay-chain regime (static_2p4, four tags as relays, --relayChain, depth 2, seed 7,
# 600 s), plus ca_tbl_full: --hopProvenance on, and off (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g53.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=false --boxHalf=200 --nTags=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on $B --hopProvenance=true &
run off $B &
wait
run r_norelay --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --hopProvenance=true

python3 scripts/hop_provenance_gate.py "$D"
