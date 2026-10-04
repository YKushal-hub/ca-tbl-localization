#!/usr/bin/env bash
# G43 runner: held entries are carried forward before use (docs/SPEC_CONFORMANCE_PLAN.md
# DC-13, --entryPrediction). What each run is for, and what a firing looks like on disk, is in
# scripts/entry_prediction_gate.py, which reads them.
#
# usage: bash scripts/entry_prediction_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# NC9's regime (four finders converging on the walker in a 1.5 km box) with the finders as
# pseudo-anchors and digests every 20 s, so the walker ranges finders and holds their
# entries: ca_tbl_full, seed 7, 1200 s, all with --entryVelocity -- without the flag, with
# --entryPrediction=false and with it on. Plus one refusal. (NC2's dispersed searchers were
# tried first: 0 substitutions in 1800 s.)
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g43.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --entryVelocity=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run base $B &
run off $B --entryPrediction=false &
run on $B --entryPrediction=true &
wait
run r_novel --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --entryPrediction=true

python3 scripts/entry_prediction_gate.py "$D"
