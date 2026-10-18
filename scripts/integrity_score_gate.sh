#!/usr/bin/env bash
# G30 runner: the integrity check's eligibility is read from the NLOS score
# (docs/SPEC_CONFORMANCE_PLAN.md DC-8). What each run is for, and what a firing looks like
# on disk, is in scripts/integrity_score_gate.py, which reads them.
#
# usage: bash scripts/integrity_score_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Three 900 s runs at seed 7 and the default configuration with the classifier on -- the
# control (flag off), the flag at the default threshold with the no-integrity arm beside
# it, and the flag at threshold 0.1 -- and one refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g30.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
BASE="--seed=7 --duration=900 --trace=true --nlosClassifier=true --boxHalf=1000"  # pinned: the OFF structural zero and populations were defined at the pre-DD-16 default box
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run tag --arms=ca_tbl_full $BASE
run score --arms=ca_tbl_full,ca_tbl_no_integrity $BASE --integrityNlosFromScore=true
run score_t01 --arms=ca_tbl_full $BASE --integrityNlosFromScore=true --nlosScoreThreshold=0.1
run r_noclf --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --integrityNlosFromScore=true

python3 scripts/integrity_score_gate.py "$D"
