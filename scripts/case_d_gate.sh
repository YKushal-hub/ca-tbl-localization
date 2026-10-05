#!/usr/bin/env bash
# G41 runner: Case D, a single range as a constraint on the propagated state
# (docs/SPEC_CONFORMANCE_PLAN.md DC-4, --caseD). What each run is for, and what a firing
# looks like on disk, is in scripts/case_d_gate.py, which reads them.
#
# usage: bash scripts/case_d_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Null control NC3's regime (3 gateways at boxHalf 2000), ca_tbl_full, seed 7, 1200 s, with
# --sparseInStep and --imuPredict: without Case D, with --caseD=false and with --caseD.
# Plus one refusal and one warning.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g41.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nGateways=3 --boxHalf=2000 --sparseInStep=true --imuPredict=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run base $B &
run off $B --caseD=false &
run on $B --caseD=true &
wait
run r_nosparse --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --caseD=true
run w_noimu --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --sparseInStep=true --caseD=true

python3 scripts/case_d_gate.py "$D"
