#!/usr/bin/env bash
# G35 runner: the uplink of a tier-2/3 range to the gateway estimator is billed
# (docs/SPEC_CONFORMANCE_PLAN.md SY-1, --edgeUplinkRanges). What each run is for, and what a
# firing looks like on disk, is in scripts/edge_uplink_gate.py, which reads them.
#
# usage: bash scripts/edge_uplink_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Three 900 s runs of all six arms at seed 7 and the default deployment -- the default,
# --edgeUplinkRanges=false and =true -- and one refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g35.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--seed=7 --duration=900 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run off $B --edgeUplinkRanges=false &
run on $B --edgeUplinkRanges=true &
wait
run r_node --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --gatewayEdgeServer=false --edgeUplinkRanges=true

python3 scripts/edge_uplink_gate.py "$D"
