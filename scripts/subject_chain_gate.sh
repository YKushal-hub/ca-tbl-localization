#!/usr/bin/env bash
# G62 runner: measured subjects in the relay chain (TECH_DEBT N.3 and N.15,
# --subjectRelayChain). What each run is for, and what a firing looks like on disk, is in
# scripts/subject_chain_gate.py, which reads them.
#
# usage: bash scripts/subject_chain_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G17's chain regime (static_2p4, 200 m box, four tags, depth 2, budget 156^2) with every
# field node a measured subject, hop provenance on, subject traces on: the flag on; the
# full arm with the flag on (a hybrid arm: the floor, escalation, awake tiers); the flag
# off (the control); and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g62.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--seed=7 --duration=600 --trace=true --boxHalf=200 --nTags=4 --measuredSubjects=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336 --hopProvenance=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on --arms=static_2p4 $B --subjectRelayChain=true &
run full --arms=ca_tbl_full $B --subjectRelayChain=true &
run off --arms=static_2p4 $B &
wait
run r_nochain --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --relayedRanging=true --nTags=4 --measuredSubjects=4 --subjectRelayChain=true
run r_nosubj --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --relayedRanging=true --relayChain=true --nTags=4 --subjectRelayChain=true

python3 scripts/subject_chain_gate.py "$D"
