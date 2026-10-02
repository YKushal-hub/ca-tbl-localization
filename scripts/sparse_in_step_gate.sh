#!/usr/bin/env bash
# G39 runner: Cases A/B solved inside Step(), after the prediction
# (docs/SPEC_CONFORMANCE_PLAN.md DC-11, --sparseInStep). What each run is for, and what a
# firing looks like on disk, is in scripts/sparse_in_step_gate.py, which reads them.
#
# usage: bash scripts/sparse_in_step_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Null control NC3's regime (3 gateways at boxHalf 2000, ca_tbl_full, seed 7, 1200 s): the
# default, --sparseInStep=false, --sparseCaseA (the old path) and --sparseInStep. NC4's
# relayed regime with --sparseInStep, where relays can complete a two-range set. One refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g39.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=1200 --trace=true --nGateways=3 --boxHalf=2000"
R="--arms=static_2p4 --seed=7 --duration=600 --trace=true --boxHalf=200 --nTags=4 --relayedRanging=true --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run off $B --sparseInStep=false &
run old $B --sparseCaseA=true &
run new $B --sparseInStep=true &
run relay $R --sparseInStep=true &
wait
run r_both --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --sparseCaseA=true --sparseInStep=true

python3 scripts/sparse_in_step_gate.py "$D"
